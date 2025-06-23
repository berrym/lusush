/*
 * Modern initialization system for Lusush shell
 * Uses modern symtable and clean component interfaces
 */

#include "../include/init.h"
#include "../include/symtable_modern.h"
#include "../include/alias.h"
#include "../include/completion.h"
#include "../include/errors.h"
#include "../include/history.h"
#include "../include/linenoise/encodings/utf8.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/signals.h"
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

// The type of shell instance
static int SHELL_TYPE;

static int parse_opts(int argc, char **argv);
static void usage(int err);

// Function to handle shebang processing
static void process_shebang(FILE *file) {
    if (!file) return;
    
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

int shell_type(void) { 
    return SHELL_TYPE; 
}

// Modern initialization function that returns a configured symtable manager
symtable_manager_t *init_modern(int argc, char **argv, FILE **in) {
    struct stat st; // stat buffer

    if (argv == NULL) {
        exit(EXIT_FAILURE);
    }

    // Set all locales according to environment
    setlocale(LC_ALL, "");

    // Setup signal handlers
    init_signal_handlers();

    // Create modern symbol table manager
    symtable_manager_t *symtable = symtable_manager_new();
    if (!symtable) {
        error_abort("Failed to create symbol table manager");
    }

    // Import environment variables into modern symtable
    char **env_ptr = environ;
    while (*env_ptr) {
        char *eq = strchr(*env_ptr, '=');
        if (eq) {
            // Split name=value
            size_t name_len = eq - *env_ptr;
            char *name = malloc(name_len + 1);
            if (!name) {
                error_abort("Failed to allocate memory for environment variable name");
            }
            
            strncpy(name, *env_ptr, name_len);
            name[name_len] = '\0';
            
            // Set variable and mark as exported
            symtable_set_var(symtable, name, eq + 1, SYMVAR_EXPORTED);
            
            free(name);
        } else {
            // Environment variable without value (set but empty)
            symtable_set_var(symtable, *env_ptr, "", SYMVAR_EXPORTED);
        }
        env_ptr++;
    }

    // Initialize shell options
    init_shell_opts();
    
    // Initialize POSIX shell options with defaults
    init_posix_options();

    // Parse command line options
    int optind = parse_opts(argc, argv);

    // Determine shell type according to POSIX specification
    bool is_login = false;
    bool is_interactive = false;
    
    // POSIX: Login shell determination
    // 1. If argv[0] starts with '-', it's a login shell
    // 2. If invoked with -l option, it's a login shell
    if (argv[0][0] == '-' || shell_opts.login_shell) {
        is_login = true;
    }
    
    // POSIX: Interactive shell determination
    // 1. If -i option is specified, force interactive
    // 2. If stdin is a terminal and no script file, interactive
    // 3. If -c option is used, non-interactive unless -i is also specified
    if (shell_opts.interactive) {
        is_interactive = true;
    } else if (shell_opts.command_mode) {
        is_interactive = false;  // -c implies non-interactive unless -i
    } else if (optind < argc && argv[optind]) {
        is_interactive = false;  // Script file implies non-interactive
    } else if (isatty(STDIN_FILENO) && isatty(STDERR_FILENO)) {
        is_interactive = true;   // Terminal input/output implies interactive
    } else {
        is_interactive = false;  // Piped input implies non-interactive
    }
    
    // Set shell type based on POSIX classification
    if (is_login) {
        SHELL_TYPE = LOGIN_SHELL;
    } else if (is_interactive) {
        SHELL_TYPE = INTERACTIVE_SHELL;
    } else {
        SHELL_TYPE = NORMAL_SHELL;
    }
    
    // Handle input source based on shell type and arguments
    if (optind < argc && argv[optind]) {
        // Script file specified
        if (stat(argv[optind], &st) != 0) {
            error_syscall("error: `init_modern`: cannot access script file");
            symtable_manager_free(symtable);
            return NULL;
        }
        
        if (!S_ISREG(st.st_mode)) {
            error_message("error: `init_modern`: %s is not a regular file", argv[optind]);
            symtable_manager_free(symtable);
            return NULL;
        }
        
        if ((*in = fopen(argv[optind], "r")) == NULL) {
            error_syscall("error: `init_modern`: cannot open script file");
            symtable_manager_free(symtable);
            return NULL;
        }
        
        // Process shebang line if present
        process_shebang(*in);
        
        // Override shell type to NORMAL_SHELL when executing script
        if (!shell_opts.interactive) {
            SHELL_TYPE = NORMAL_SHELL;
        }
    } else {
        // No script file - use stdin
        *in = stdin;
    }

    // Set up interactive features if needed
    if (is_interactive) {
        linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                      linenoiseUtf8NextCharLen,
                                      linenoiseUtf8ReadCode);
        linenoiseSetMultiLine(get_shell_vari("MULTILINE_EDIT", true));
        build_prompt();
        
        // POSIX: Interactive shells should handle job control
        if (shell_opts.job_control) {
            // Initialize job control (placeholder for future implementation)
        }
    }
    
    // POSIX: Set positional parameters
    // $0 is the shell name or script name
    if (optind < argc && argv[optind]) {
        symtable_set_var(symtable, "0", argv[optind], SYMVAR_READONLY);
        // Set $1, $2, etc. for script arguments
        for (int i = optind + 1; i < argc; i++) {
            char param_name[16];
            snprintf(param_name, sizeof(param_name), "%d", i - optind);
            symtable_set_var(symtable, param_name, argv[i], SYMVAR_NONE);
        }
        // Set $# (argument count)
        char argc_str[16];
        snprintf(argc_str, sizeof(argc_str), "%d", argc - optind - 1);
        symtable_set_var(symtable, "#", argc_str, SYMVAR_READONLY);
    } else {
        symtable_set_var(symtable, "0", argv[0], SYMVAR_READONLY);
        symtable_set_var(symtable, "#", "0", SYMVAR_READONLY);
    }

    // Set shell process information in modern symtable
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", getpid());
    symtable_set_var(symtable, "SHELL_PID", pid_str, SYMVAR_READONLY);
    
    char ppid_str[32];
    snprintf(ppid_str, sizeof(ppid_str), "%d", getppid());
    symtable_set_var(symtable, "PPID", ppid_str, SYMVAR_READONLY | SYMVAR_EXPORTED);
    
    // Set initial exit status
    symtable_set_var(symtable, "?", "0", SYMVAR_NONE);

    // Initialize history for interactive and login shells
    if (shell_type() != NORMAL_SHELL) {
        init_history();
    }

    // Initialize aliases
    init_aliases();

    // Set line completion function
    linenoiseSetCompletionCallback(lusush_completion_callback);

    // Set memory cleanup procedures on termination
    atexit(free_aliases);

    // Process shebang if the shell is invoked with a script
    if (shell_type() == NORMAL_SHELL && *in) {
        process_shebang(*in);
    }

    return symtable;
}

static int parse_opts(int argc, char **argv) {
    // next option
    int nopt = 0;
    // string of valid short options - include all POSIX options
    const char *sopts = "hVc:silexnuvfm";
    // array describing valid long options
    const struct option lopts[] = {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'V'},
        {  NULL,    0, NULL,   0}
    };

    do {
        nopt = getopt_long(argc, argv, sopts, lopts, NULL);

        switch (nopt) {
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
            // Execute command string
            shell_opts.command_mode = true;
            shell_opts.command_string = strdup(optarg);
            if (!shell_opts.command_string) {
                error_abort("failed to allocate memory for command string");
            }
            break;
        case 's':
            // Read commands from standard input
            shell_opts.stdin_mode = true;
            break;
        case 'i':
            // Force interactive mode
            shell_opts.interactive = true;
            break;
        case 'l':
            // Login shell behavior
            shell_opts.login_shell = true;
            break;
        case 'e':
            // Exit immediately on command failure
            shell_opts.exit_on_error = true;
            break;
        case 'x':
            // Trace command execution
            shell_opts.trace_execution = true;
            break;
        case 'n':
            // Syntax check mode - read but don't execute
            shell_opts.syntax_check = true;
            break;
        case 'u':
            // Treat unset variables as error
            shell_opts.unset_error = true;
            break;
        case 'v':
            // Verbose mode - print input lines as read
            shell_opts.verbose = true;
            break;
        case 'f':
            // Disable pathname expansion (globbing)
            shell_opts.no_globbing = true;
            break;
        case 'm':
            // Enable job control
            shell_opts.job_control = true;
            break;
        case '?':
            usage(EXIT_FAILURE);
        case -1:
            break;
        default:
            error_abort("unknown error terminating"); // should never happen
        }
    } while (nopt != -1);

    return optind;
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
    printf("  -n               Syntax check mode - read but don't execute (set -n)\n");
    printf("  -u               Treat unset variables as error (set -u)\n");
    printf("  -v               Verbose mode - print input lines (set -v)\n");
    printf("  -f               Disable pathname expansion (set -f)\n");
    printf("  -m               Enable job control (set -m)\n");
    printf("\nArguments:\n");
    printf("  SCRIPT           Execute commands from script file\n");
    printf("\nShell Options:\n");
    printf("  Use 'set -o option' or 'set +o option' to control shell behavior\n");
    printf("  Available options: errexit, xtrace, noexec, nounset, verbose,\n");
    printf("                     noglob, hashall, monitor\n");
    printf("\nFor more information, see the manual or documentation.\n");
    exit(err);
}