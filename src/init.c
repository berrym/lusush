#include "init.h"

#include "alias.h"
#include "autocorrect.h"
#include "builtins.h"
#include "config.h"
#include "errors.h"
#include "history.h"
#include "input.h"
#include "posix_history.h"

#include "lle/completion/ssh_hosts.h"
#include "lusush.h"
#include "signals.h"
#include "symtable.h"

#include "display_integration.h"
#include "lle/adaptive_terminal_integration.h"
#include "lle/lle_shell_integration.h"
#include "lusush_memory_pool.h"
#include "version.h"

#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/sysmacros.h>
#else
// macOS doesn't have sys/sysmacros.h, major/minor are in sys/types.h
// but we'll avoid using them for portability
#endif

extern char **environ;
extern posix_history_manager_t *global_posix_history;

// POSIX-compliant shell type tracking
static int SHELL_TYPE;
static bool IS_LOGIN_SHELL = false;
static bool IS_INTERACTIVE_SHELL = false;

static int parse_opts(int argc, char **argv);
static void usage(int err);

/**
 * Ensure safe bottom margin for interactive shell display.
 * Creates scroll space at the bottom of the terminal.
 */
static void ensure_bottom_margin(void) {
    static bool margin_created = false;
    if (margin_created) {
        return;
    }

    int rows = 0;
    lle_get_terminal_size(NULL, &rows);
    if (rows <= 0) {
        rows = 24;
    }

    /* Save cursor, move to bottom, scroll, restore */
    char cmd[64];
    int len = snprintf(cmd, sizeof(cmd), "\x1b[s\x1b[%d;1H\n\x1b[u", rows);
    if (len > 0 && (size_t)len < sizeof(cmd)) {
        (void)write(STDOUT_FILENO, cmd, (size_t)len);
    }

    margin_created = true;
}

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

    // Initialize SSH host cache for completion
    if (ssh_hosts_init() != 0) {
        if (IS_INTERACTIVE_SHELL) {
            fprintf(stderr, "Warning: Failed to initialize SSH host cache\n");
        }
    }

    // Initialize terminal capabilities via LLE adaptive detection
    lle_terminal_detection_result_t *detection = NULL;
    if (lle_detect_terminal_capabilities_optimized(&detection) != LLE_SUCCESS) {
        if (IS_INTERACTIVE_SHELL) {
            fprintf(stderr,
                    "Warning: Failed to detect terminal capabilities\n");
        }
    } else {
        // Terminal capabilities successfully detected
        if (IS_INTERACTIVE_SHELL && detection->stdout_is_tty) {
            // Create safe bottom margin for interactive shells
            ensure_bottom_margin();
        }
        // Note: detection result is cached, no need to destroy here
    }

    /* Theme system removed - LLE prompt composer handles themes now */

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

    // Terminal detection using standard methods
    // Enhanced terminal integration has been replaced with readline

    // POSIX-compliant shell type determination

    // 1. Determine if this is a login shell
    IS_LOGIN_SHELL = (**argv == '-') || shell_opts.login_shell;

    // 2. Determine interactive vs non-interactive
    bool has_script_file = (optind && argv[optind] && *argv[optind]);
    bool forced_interactive = shell_opts.interactive;
    bool stdin_is_terminal = isatty(STDIN_FILENO);

    // Debug: Show TTY detection details
    const char *debug_env = getenv("LUSUSH_DEBUG");
    if (debug_env &&
        (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) {
        fprintf(stderr, "[INIT] TTY Detection: STDIN_FILENO=%d, isatty()=%s\n",
                STDIN_FILENO, stdin_is_terminal ? "true" : "false");

        // Show what stdin is connected to
        struct stat st;
        if (fstat(STDIN_FILENO, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                fprintf(stderr, "[INIT] stdin is a regular file\n");
            } else if (S_ISDIR(st.st_mode)) {
                fprintf(stderr, "[INIT] stdin is a directory\n");
            } else if (S_ISCHR(st.st_mode)) {
#ifdef __linux__
                fprintf(stderr,
                        "[INIT] stdin is a character device (dev %lu:%lu)\n",
                        (unsigned long)major(st.st_rdev),
                        (unsigned long)minor(st.st_rdev));
#else
                fprintf(stderr, "[INIT] stdin is a character device\n");
#endif
            } else if (S_ISBLK(st.st_mode)) {
                fprintf(stderr, "[INIT] stdin is a block device\n");
            } else if (S_ISFIFO(st.st_mode)) {
                fprintf(stderr, "[INIT] stdin is a FIFO/pipe\n");
            } else if (S_ISLNK(st.st_mode)) {
                fprintf(stderr, "[INIT] stdin is a symbolic link\n");
            } else if (S_ISSOCK(st.st_mode)) {
                fprintf(stderr, "[INIT] stdin is a socket\n");
            } else {
                fprintf(stderr, "[INIT] stdin is unknown type (mode 0x%x)\n",
                        st.st_mode);
            }
        } else {
            fprintf(stderr, "[INIT] fstat() failed: %s\n", strerror(errno));
        }
    }

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
    } else if (lle_adaptive_should_shell_be_interactive(
                   forced_interactive, has_script_file,
                   shell_opts.stdin_mode)) {
        /*
         * Interactive shell detection using LLE adaptive terminal system.
         * This handles:
         * - Traditional TTY detection (stdin/stdout are terminals)
         * - Editor terminals (Zed, VS Code, Cursor) with non-TTY stdin
         * - Terminal multiplexers (tmux, screen)
         * - Force interactive flag (-i)
         */
        IS_INTERACTIVE_SHELL = true;
        SHELL_TYPE = SHELL_INTERACTIVE;
        *in = stdin;

        // Debug: Show interactive detection
        const char *debug_env = getenv("LUSUSH_DEBUG");
        if (debug_env &&
            (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) {
            fprintf(
                stderr,
                "[INIT] Interactive shell detected via adaptive detection\n");
            fprintf(stderr,
                    "[INIT]   forced=%s, stdin_is_terminal=%s, stdin_mode=%s\n",
                    forced_interactive ? "true" : "false",
                    stdin_is_terminal ? "true" : "false",
                    shell_opts.stdin_mode ? "true" : "false");
        }
    } else {
        // Non-interactive: piped input, -s mode, or no capable terminal
        IS_INTERACTIVE_SHELL = false;
        SHELL_TYPE = SHELL_NON_INTERACTIVE;
        *in = stdin;

        // Debug: Show non-interactive detection
        const char *debug_env = getenv("LUSUSH_DEBUG");
        if (debug_env &&
            (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) {
            fprintf(stderr,
                    "[INIT] Non-interactive shell (adaptive detection)\n");
            fprintf(stderr,
                    "[INIT]   forced=%s, stdin_is_terminal=%s, stdin_mode=%s\n",
                    forced_interactive ? "true" : "false",
                    stdin_is_terminal ? "true" : "false",
                    shell_opts.stdin_mode ? "true" : "false");
        }
    }

    // Set up interactive shell features if needed
    if (IS_INTERACTIVE_SHELL) {
        // Enable job control by default for interactive shells (POSIX behavior)
        // This allows background jobs (cmd &) to be tracked and managed
        shell_opts.job_control = true;
        
        // Initialize display integration system config first (needed for debug flags)
        display_integration_config_t display_config;
        display_integration_create_default_config(&display_config);

        // Configure based on environment and user preferences
        const char *layered_display_env = getenv("LUSUSH_LAYERED_DISPLAY");
        if (layered_display_env) {
            // v1.3.0: Layered display is now exclusive - environment variable
            // ignored Layered display is always enabled
        }

        // Enable debug mode if requested
        const char *display_debug_env = getenv("LUSUSH_DISPLAY_DEBUG");
        if (display_debug_env && (strcmp(display_debug_env, "1") == 0 ||
                                  strcmp(display_debug_env, "true") == 0)) {
            display_config.debug_mode = true;
        }

        // Set optimization level based on environment
        const char *optimization_env = getenv("LUSUSH_DISPLAY_OPTIMIZATION");
        if (optimization_env) {
            int opt_level = atoi(optimization_env);
            if (opt_level >= DISPLAY_OPTIMIZATION_DISABLED &&
                opt_level <= DISPLAY_OPTIMIZATION_MAXIMUM) {
                display_config.optimization_level =
                    (display_optimization_level_t)opt_level;
            }
        }

        // Initialize memory pool system FIRST - required by LLE and display
        lusush_pool_config_t pool_config =
            lusush_pool_get_display_optimized_config();
        pool_config.enable_debugging = (getenv("LUSUSH_MEMORY_DEBUG") != NULL);

        lusush_pool_error_t pool_result = lusush_pool_init(&pool_config);
        if (pool_result != LUSUSH_POOL_SUCCESS) {
            if (display_config.debug_mode || getenv("LUSUSH_MEMORY_DEBUG")) {
                fprintf(
                    stderr,
                    "Warning: Failed to initialize memory pool system: %s\n",
                    lusush_pool_error_string(pool_result));
                fprintf(stderr,
                        "Continuing with standard malloc/free operations\n");
            }
        } else {
            /* Register memory pool cleanup FIRST so it runs LAST in atexit.
             * atexit handlers run in LIFO order (last registered = first run).
             * By registering pool shutdown BEFORE LLE init, the LLE atexit
             * handler (registered later) will run BEFORE pool shutdown,
             * ensuring LLE can safely save history using pool memory. */
            atexit(lusush_pool_shutdown);

            if (display_config.debug_mode || getenv("LUSUSH_MEMORY_DEBUG")) {
                fprintf(stderr, "Memory pool system initialized successfully\n");
            }
        }

        /* Initialize LLE shell integration (Spec 26)
         * LLE is the sole line editor - no GNU readline fallback.
         * Requires: global_memory_pool (initialized above)
         */
        lle_result_t lle_result = lle_shell_integration_init();
        if (lle_result != LLE_SUCCESS) {
            fprintf(stderr, "Warning: Failed to initialize LLE: %d\n", lle_result);
        }

        // Initialize display integration ONLY in interactive mode
        if (IS_INTERACTIVE_SHELL) {
            // Configure display options based on environment and command line
            // v1.3.0: Layered display is now exclusive - always enabled
            // No configuration needed

            // Always initialize display integration to support runtime display
            // enable
            if (!display_integration_init(&display_config)) {
                if (display_config.debug_mode ||
                    getenv("LUSUSH_DISPLAY_DEBUG")) {
                    fprintf(stderr, "Warning: Failed to initialize display "
                                    "integration, using standard display\n");
                }
                // Continue with standard display - no fatal error
            } else {
                // Announce activation with visual impact (only when debug mode
                // enabled)
                if (display_config.debug_mode) {
                    printf("Display integration initialized "
                           "(layered_display=exclusive)\n");
                }
            }
        } else {
            // Non-interactive mode: no display integration needed
            if (getenv("LUSUSH_DISPLAY_DEBUG")) {
                fprintf(stderr,
                        "Display integration skipped (non-interactive mode)\n");
            }
        }

        /* Generate initial prompt */
        lle_shell_update_prompt();
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
        // LLE history is initialized via lle_shell_integration_init()
        // Initialize enhanced POSIX history system
        if (!global_posix_history) {
            global_posix_history = posix_history_create(0);
            if (global_posix_history) {
                // Set default filename and load existing history
                char *home = symtable_get_global_default("HOME", "");
                if (home && *home) {
                    char histfile[1024];
                    snprintf(histfile, sizeof(histfile), "%s/.lusush_history",
                             home);
                    posix_history_set_filename(global_posix_history, histfile);
                    posix_history_load(global_posix_history, histfile, false);
                }

                // Enable duplicate detection by default
                posix_history_set_no_duplicates(global_posix_history, true);
            }
        }
    }

    // Initialize aliases
    init_aliases();

    // Initialize command hash table
    init_command_hash();

    // Completion is handled automatically by readline integration
    // No need to set callbacks - they're integrated in lusush_readline_init()

    // Hints system is not implemented in readline integration yet
    // TODO: Implement hints for readline if needed

    // Set memory cleanup procedures on termination
    atexit(free_global_symtable);
    atexit(free_aliases);
    atexit(free_command_hash);
    atexit(autocorrect_cleanup);
    atexit(ssh_hosts_cleanup);

    // LLE handles all history management now
    // POSIX history manager cleanup for non-interactive shells
    (void)global_posix_history; // May still be used by scripts
    // atexit(config_cleanup);  // Temporarily disabled
    if (!IS_INTERACTIVE_SHELL) {
        atexit(free_input_buffers);
    }

    // Process shebang if the shell is invoked with a script
    if (!IS_INTERACTIVE_SHELL && *in && has_script_file) {
        process_shebang(*in);
    }

    // Register cleanup for display integration
    // Note: atexit handlers run in REVERSE order of registration
    // LLE cleanup is handled by lle_shell_integration_shutdown (registered in init)
    if (IS_INTERACTIVE_SHELL) {
        atexit(display_integration_cleanup);
    }

    /* Memory pool cleanup is now registered immediately after pool init
     * (before LLE init) so it runs LAST in atexit order (LIFO).
     * This ensures LLE can safely use pool memory during shutdown. */

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
                printf("Copyright (C) 2021-2025 Michael Berry. Licensed under "
                       "GPL-3.0+.\n");
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
                shell_opts.hash_commands = true;
                break;
            case 'V':
                printf("%s %s\n", LUSUSH_NAME, LUSUSH_VERSION_STRING);
                printf("%s\n", LUSUSH_DESCRIPTION);
                printf("Copyright (C) 2021-2025 Michael Berry. Licensed under "
                       "GPL-3.0+.\n");
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
            case 'b':
                shell_opts.notify = true;
                break;
            case 't':
                shell_opts.onecmd = true;
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
    printf("      --help       Show this help message and exit\n");
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
    printf("  -h               Enable command path hashing (set -o hashall)\n");
    printf("  -m               Enable job control (set -m)\n");
    printf("  -b               Notify asynchronously of background job "
           "completion (set -o notify)\n");
    printf("  -t               Exit after executing one command (set -o "
           "onecmd)\n");
    printf("\nArguments:\n");
    printf("  SCRIPT           Execute commands from script file\n");
    printf("\nShell Options:\n");
    printf(
        "  Use 'set -o option' or 'set +o option' to control shell behavior\n");
    printf("  Available options: errexit, xtrace, noexec, nounset, verbose,\n");
    printf("                     noglob, hashall, monitor, notify, onecmd\n");
    printf("\nFor more information, see the manual or documentation.\n");
    exit(err);
}
