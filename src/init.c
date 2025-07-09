#include "../include/init.h"

#include "../include/alias.h"
#include "../include/autocorrect.h"
#include "../include/builtins.h"
#include "../include/completion.h"
#include "../include/config.h"
#include "../include/errors.h"
#include "../include/history.h"
#include "../include/input.h"
#include "../include/linenoise/encodings/utf8.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/network.h"
#include "../include/prompt.h"
#include "../include/signals.h"
#include "../include/symtable.h"
#include "../include/termcap.h"
#include "../include/themes.h"
#include "../include/version.h"

#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern char **environ;

bool exit_flag = false;

// POSIX-compliant shell type tracking
static int SHELL_TYPE;
static bool IS_LOGIN_SHELL = false;
static bool IS_INTERACTIVE_SHELL = false;

static int parse_opts(int argc, char **argv);
static void usage(int err);

// Function to handle shebang processing
static void process_shebang(FILE *file) {
    if (!file) {
        return;
    }

    // Check if the first line starts with #!
    long pos = ftell(file);
    char line[256];

    if (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "#!", 2) == 0) {
            // This is a shebang line - we just consume it and continue
            // The shebang has already done its job by launching our shell
            return;
        }
    }

    // Not a shebang line, rewind to beginning
    fseek(file, pos, SEEK_SET);
}

int shell_type(void) { return SHELL_TYPE; }

// POSIX-compliant shell type functions
bool is_interactive_shell(void) { return IS_INTERACTIVE_SHELL; }
bool is_login_shell(void) { return IS_LOGIN_SHELL; }

int init(int argc, char **argv, FILE **in) {
    struct stat st; // stat buffer

    if (argv == NULL) {
        exit(EXIT_FAILURE);
    }

    // Set all locales according to environment
    setlocale(LC_ALL, "");

    // Setup signal handlers
    init_signal_handlers();

    // Initialize symbol table
    init_symtable();

    // Import environment variables into symtable
    char **env_ptr = environ;
    while (*env_ptr) {
        char *eq = strchr(*env_ptr, '=');
        if (eq) {
            // Split name=value
            size_t name_len = eq - *env_ptr;
            char *name = malloc(name_len + 1);
            if (!name) {
                error_abort(
                    "Failed to allocate memory for environment variable name");
            }

            strncpy(name, *env_ptr, name_len);
            name[name_len] = '\0';

            // Set variable and mark as exported
            symtable_set_global(name, eq + 1);
            symtable_export_global(name);

            free(name);
        } else {
            // Environment variable without value (set but empty)
            symtable_set_global(*env_ptr, "");
            symtable_export_global(*env_ptr);
        }
        env_ptr++;
    }

    init_shell_opts();

    // Initialize POSIX shell options with defaults
    init_posix_options();

    // Initialize configuration system
    config_init();

    // Execute login scripts for login shells
    if (IS_LOGIN_SHELL) {
        config_execute_login_scripts();
    }

    // Execute startup scripts for interactive shells
    if (IS_INTERACTIVE_SHELL) {
        config_execute_startup_scripts();
    }

    // Initialize auto-correction system
    autocorrect_init();

    // Initialize network system (Phase 3 Target 3)
    if (network_init() != 0) {
        if (IS_INTERACTIVE_SHELL) {
            fprintf(stderr, "Warning: Failed to initialize network system\n");
        }
    }

    // Initialize terminal capabilities (Phase 3 Target 4)
    if (termcap_init() != TERMCAP_OK) {
        if (IS_INTERACTIVE_SHELL) {
            fprintf(stderr, "Warning: Failed to initialize terminal capabilities\n");
        }
    } else {
        // Terminal capabilities successfully initialized
        const terminal_info_t *term_info = termcap_get_info();
        if (IS_INTERACTIVE_SHELL && term_info->is_tty) {
            // Create safe bottom margin for interactive shells
            termcap_ensure_bottom_margin();
        }
    }

    // Initialize theme system (Phase 3 Target 2)
    if (!theme_init()) {
        if (IS_INTERACTIVE_SHELL) {
            fprintf(stderr, "Warning: Failed to initialize theme system\n");
        }
    } else {
        // Apply theme configuration from config file
        if (config.theme_name && strlen(config.theme_name) > 0) {
            if (!theme_set_active(config.theme_name)) {
                // Try fallback themes silently
                if (!theme_set_active("minimal") &&
                    !theme_set_active("classic")) {
                    // Only warn in interactive mode
                    if (IS_INTERACTIVE_SHELL) {
                        fprintf(stderr,
                                "Warning: Failed to set theme '%s', using "
                                "default\n",
                                config.theme_name);
                    }
                }
            }
        } else {
            // Set default theme silently
            if (!theme_set_active("corporate")) {
                theme_set_active("minimal"); // Final fallback
            }
        }

        // Set up corporate branding if configured
        if (config.theme_corporate_company ||
            config.theme_corporate_department) {
            branding_config_t branding = {0};
            if (config.theme_corporate_company) {
                strncpy(branding.company_name, config.theme_corporate_company,
                        BRAND_TEXT_MAX - 1);
            }
            if (config.theme_corporate_department) {
                strncpy(branding.department, config.theme_corporate_department,
                        BRAND_TEXT_MAX - 1);
            }
            if (config.theme_corporate_project) {
                strncpy(branding.project, config.theme_corporate_project,
                        BRAND_TEXT_MAX - 1);
            }
            if (config.theme_corporate_environment) {
                strncpy(branding.environment,
                        config.theme_corporate_environment, BRAND_TEXT_MAX - 1);
            }
            branding.show_company_in_prompt = config.theme_show_company;
            branding.show_department_in_prompt = config.theme_show_department;
            theme_set_branding(&branding);
        }
    }

    // Set up auto-correction configuration from config system
    autocorrect_config_t autocorrect_cfg;
    autocorrect_get_default_config(&autocorrect_cfg);
    autocorrect_cfg.enabled = config.spell_correction;
    autocorrect_cfg.max_suggestions = config.autocorrect_max_suggestions;
    autocorrect_cfg.similarity_threshold = config.autocorrect_threshold;
    autocorrect_cfg.interactive_prompts = config.autocorrect_interactive;
    autocorrect_cfg.learn_from_history = config.autocorrect_learn_history;
    autocorrect_cfg.correct_builtins = config.autocorrect_builtins;
    autocorrect_cfg.correct_external = config.autocorrect_external;
    autocorrect_cfg.case_sensitive = config.autocorrect_case_sensitive;
    autocorrect_load_config(&autocorrect_cfg);

    // Parse command line options
    size_t optind = parse_opts(argc, argv);

    // POSIX-compliant shell type determination

    // 1. Determine if this is a login shell
    IS_LOGIN_SHELL = (**argv == '-') || shell_opts.login_shell;

    // 2. Determine interactive vs non-interactive
    bool has_script_file = (optind && argv[optind] && *argv[optind]);
    bool forced_interactive = shell_opts.interactive;
    bool stdin_is_terminal = isatty(STDIN_FILENO);

    if (shell_opts.command_mode) {
        // -c command mode: always non-interactive
        IS_INTERACTIVE_SHELL = false;
        SHELL_TYPE = SHELL_NON_INTERACTIVE;
        *in = stdin; // Not used in -c mode
    } else if (has_script_file) {
        // Script file execution: always non-interactive
        IS_INTERACTIVE_SHELL = false;
        SHELL_TYPE = SHELL_NON_INTERACTIVE;

        // Check that the script file is valid
        stat(argv[optind], &st);
        if (!(S_ISREG(st.st_mode))) {
            error_message("error: `init`: %s is not a regular file",
                          argv[optind]);
            // Fall back to interactive mode
            IS_INTERACTIVE_SHELL = true;
            SHELL_TYPE = SHELL_INTERACTIVE;
            optind = 0;
            *in = stdin;
        } else {
            if ((*in = fopen(argv[optind], "r")) == NULL) {
                error_syscall("error: `init`: fopen");
            } else {
                process_shebang(*in);
            }
        }
    } else if (forced_interactive ||
               (stdin_is_terminal && !shell_opts.stdin_mode)) {
        // Interactive shell: stdin is terminal OR forced with -i
        IS_INTERACTIVE_SHELL = true;
        SHELL_TYPE = SHELL_INTERACTIVE;
        *in = stdin;
    } else {
        // Non-interactive: piped input, -s mode, or stdin not a terminal
        IS_INTERACTIVE_SHELL = false;
        SHELL_TYPE = SHELL_NON_INTERACTIVE;
        *in = stdin;
    }

    // Set up interactive shell features if needed
    if (IS_INTERACTIVE_SHELL) {
        linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                      linenoiseUtf8NextCharLen,
                                      linenoiseUtf8ReadCode);
        // Multiline mode disabled for better reliability and performance
        linenoiseSetMultiLine(0);
        build_prompt();
    }

    // For login shells, set the appropriate type
    if (IS_LOGIN_SHELL) {
        SHELL_TYPE = SHELL_LOGIN;
    }

    // Get and set shell's ppid in environment
    pid_t ppid = getppid();
    char ppid_str[32];
    snprintf(ppid_str, sizeof(ppid_str), "%d", ppid);
    setenv("PPID", ppid_str, 1);
    symtable_set_global("PPID", ppid_str);
    symtable_export_global("PPID");

    // Set initial exit status
    set_exit_status(0);

    // Set shell PID ($$ special variable)
    pid_t shell_pid = getpid();
    char shell_pid_str[32];
    snprintf(shell_pid_str, sizeof(shell_pid_str), "%d", (int)shell_pid);
    symtable_set_global("$", shell_pid_str);

    // Set shell name/script name and positional parameters
    if (shell_type() == NORMAL_SHELL && optind > 0 && argv[optind]) {
        // Running a script - set up script arguments
        symtable_set_global("0", argv[optind]); // Script name

        // Update global shell_argc and shell_argv for script arguments
        extern int shell_argc;
        extern char **shell_argv;
        shell_argc =
            argc - optind; // Number of script args (including script name)
        shell_argv = &argv[optind]; // Pointer to script args
    } else {
        // Interactive or command mode - use shell arguments
        symtable_set_global("0", argv[0]);
    }

    // Initialize history for interactive shells
    if (IS_INTERACTIVE_SHELL) {
        init_history();
    }

    // Initialize aliases
    init_aliases();

    // Initialize command hash table
    init_command_hash();

    // Set line completion function
    linenoiseSetCompletionCallback(lusush_completion_callback);

    // Set hints system callbacks (only if enabled in config)
    if (config.hints_enabled) {
        linenoiseSetHintsCallback(lusush_hints_callback);
        linenoiseSetFreeHintsCallback(lusush_free_hints_callback);
    }

    // Set memory cleanup procedures on termination
    atexit(free_global_symtable);
    atexit(free_aliases);
    atexit(free_command_hash);
    atexit(autocorrect_cleanup);
    atexit(theme_cleanup);
    atexit(network_cleanup);
    // atexit(config_cleanup);  // Temporarily disabled
    if (!IS_INTERACTIVE_SHELL) {
        atexit(free_input_buffers);
    }

    // Process shebang if the shell is invoked with a script
    if (!IS_INTERACTIVE_SHELL && *in && has_script_file) {
        process_shebang(*in);
    }

    return 0;
}

static int parse_opts(int argc, char **argv) {
    // POSIX-compliant argument parsing: shell [options] script [script-args]
    // Only parse options that come before the script name
    // Everything after the script name should be treated as script arguments

    int arg_index = 1; // Start from first argument (skip program name)

    // Manual option parsing to properly handle POSIX argument separation
    while (arg_index < argc) {
        char *arg = argv[arg_index];

        // If argument doesn't start with '-', it's the script name
        // Stop parsing options here - everything after is script arguments
        if (arg[0] != '-' || strlen(arg) == 1) {
            break;
        }

        // Handle special case: "--" ends option parsing
        if (strcmp(arg, "--") == 0) {
            arg_index++; // Skip the "--"
            break;
        }

        // Handle long options
        if (arg[0] == '-' && arg[1] == '-') {
            if (strcmp(arg, "--help") == 0) {
                usage(EXIT_SUCCESS);
            } else if (strcmp(arg, "--version") == 0) {
                printf("%s %s\n", LUSUSH_NAME, LUSUSH_VERSION_STRING);
                printf("%s\n", LUSUSH_DESCRIPTION);
                printf("Copyright (c) 2025. Licensed under MIT License.\n");
                exit(EXIT_SUCCESS);
            } else {
                fprintf(stderr, "%s: invalid option -- '%s'\n", argv[0], arg);
                usage(EXIT_FAILURE);
            }
            arg_index++;
            continue;
        }

        // Handle short options (can be combined like -abc)
        for (int i = 1; arg[i] != '\0'; i++) {
            char opt = arg[i];

            switch (opt) {
            case 'h':
                usage(EXIT_SUCCESS);
                break;
            case 'V':
                printf("%s %s\n", LUSUSH_NAME, LUSUSH_VERSION_STRING);
                printf("%s\n", LUSUSH_DESCRIPTION);
                printf("Copyright (c) 2025. Licensed under MIT License.\n");
                exit(EXIT_SUCCESS);
                break;
            case 'c':
                // Execute command string - requires argument
                if (arg[i + 1] != '\0') {
                    // Argument attached to option: -ccommand
                    shell_opts.command_mode = true;
                    shell_opts.command_string = strdup(&arg[i + 1]);
                    if (!shell_opts.command_string) {
                        error_abort(
                            "failed to allocate memory for command string");
                    }
                    goto next_arg; // Skip rest of this argument
                } else if (arg_index + 1 < argc) {
                    // Argument in next parameter: -c command
                    arg_index++;
                    shell_opts.command_mode = true;
                    shell_opts.command_string = strdup(argv[arg_index]);
                    if (!shell_opts.command_string) {
                        error_abort(
                            "failed to allocate memory for command string");
                    }
                    goto next_arg;
                } else {
                    fprintf(stderr, "%s: option requires an argument -- '%c'\n",
                            argv[0], opt);
                    usage(EXIT_FAILURE);
                }
                break;
            case 's':
                shell_opts.stdin_mode = true;
                break;
            case 'i':
                shell_opts.interactive = true;
                break;
            case 'l':
                shell_opts.login_shell = true;
                break;
            case 'e':
                shell_opts.exit_on_error = true;
                break;
            case 'x':
                shell_opts.trace_execution = true;
                break;
            case 'n':
                shell_opts.syntax_check = true;
                break;
            case 'u':
                shell_opts.unset_error = true;
                break;
            case 'v':
                shell_opts.verbose = true;
                break;
            case 'f':
                shell_opts.no_globbing = true;
                break;
            case 'm':
                shell_opts.job_control = true;
                break;
            default:
                fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], opt);
                usage(EXIT_FAILURE);
            }
        }

    next_arg:
        arg_index++;
    }

    // Return the index of the first non-option argument (script name)
    return arg_index;
}

static void usage(int err) {
    printf("Usage: %s [OPTIONS] [SCRIPT]\n", LUSUSH_NAME);
    printf("A POSIX-compliant shell with modern features\n\n");
    printf("Options:\n");
    printf("  -h, --help       Show this help message and exit\n");
    printf("  -V, --version    Show version information and exit\n");
    printf("  -c command       Execute command string and exit\n");
    printf("  -s               Read commands from standard input\n");
    printf("  -i               Force interactive mode\n");
    printf("  -l               Act as login shell\n");
    printf("  -e               Exit immediately on command failure (set -e)\n");
    printf("  -x               Trace command execution (set -x)\n");
    printf("  -n               Syntax check mode - read but don't execute (set "
           "-n)\n");
    printf("  -u               Treat unset variables as error (set -u)\n");
    printf("  -v               Verbose mode - print input lines (set -v)\n");
    printf("  -f               Disable pathname expansion (set -f)\n");
    printf("  -m               Enable job control (set -m)\n");
    printf("\nArguments:\n");
    printf("  SCRIPT           Execute commands from script file\n");
    printf("\nShell Options:\n");
    printf(
        "  Use 'set -o option' or 'set +o option' to control shell behavior\n");
    printf("  Available options: errexit, xtrace, noexec, nounset, verbose,\n");
    printf("                     noglob, hashall, monitor\n");
    printf("\nFor more information, see the manual or documentation.\n");
    exit(err);
}
