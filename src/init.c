#include "../include/init.h"

#include "../include/alias.h"
#include "../include/completion.h"
#include "../include/errors.h"
#include "../include/history.h"
#include "../include/input.h"
#include "../include/linenoise/encodings/utf8.h"
#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/scanner_old.h"
#include "../include/signals.h"
#include "../include/symtable.h"
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

int shell_type(void) { return SHELL_TYPE; }

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

    symtable_entry_t *entry = NULL;
    char **p = environ;
    char *eq = NULL;
    size_t len = 0;
    while (*p) {
        eq = strchr(*p, '=');
        if (eq) {
            len = eq - *p;
            char name[len + 1];

            strncpy(name, *p, len);
            name[len] = '\0';

            entry = add_to_symtable(name);

            if (entry) {
                symtable_entry_setval(entry, eq + 1);
                entry->flags |= FLAG_EXPORT;
            }
        } else {
            entry = add_to_symtable(*p);
        }
        p++;
    }

    init_shell_opts();
    
    // Initialize POSIX shell options with defaults
    init_posix_options();

    // Parse command line options
    size_t optind = parse_opts(argc, argv);

    // Determine the shell type
    if (**argv == '-') {
        SHELL_TYPE = LOGIN_SHELL;
    } else if (optind && argv[optind] && *argv[optind]) {
        // Check that argv[optind] is a regular file
        stat(argv[optind], &st);
        if (!(S_ISREG(st.st_mode))) {
            error_message("error: `init`: %s is not a regular file",
                          argv[optind]);
            optind = 0;
            SHELL_TYPE = INTERACTIVE_SHELL;
            // Set input to stdin for interactive mode (including piped input)
            *in = stdin;

            linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                          linenoiseUtf8NextCharLen,
                                          linenoiseUtf8ReadCode);
            linenoiseSetMultiLine(get_shell_vari("MULTILINE_EDIT", true));
            build_prompt();
        } else {
            SHELL_TYPE = NORMAL_SHELL;
            if ((*in = fopen(argv[optind], "r")) == NULL) {
                error_syscall("error: `init`: fopen");
            } else {
                // Process shebang line if present
                process_shebang(*in);
            }
        }
    } else {
        optind = 0;
        SHELL_TYPE = INTERACTIVE_SHELL;
        // Set input to stdin for interactive mode (including piped input)
        *in = stdin;
    }

    // Get and set shell's ppid in environment
    pid_t ppid = getppid();
    char ppid_str[10];
    sprintf(ppid_str, "%u", ppid);
    setenv("PPID", ppid_str, 1);
    symtable_entry_t *ppid_entry = add_to_symtable("PPID");
    symtable_entry_setval(ppid_entry, ppid_str);
    entry->flags |= FLAG_READONLY;

    // Initialize history
    if (shell_type() != NORMAL_SHELL) {
        init_history();
    }

    // Initialize aliases
    init_aliases();

    // Set line completion function
    linenoiseSetCompletionCallback(lusush_completion_callback);

    // Set memory cleanup procedures on termination
    atexit(free_tok_buf);
    atexit(free_global_symtable);
    atexit(free_aliases);
    if (shell_type() == NORMAL_SHELL) {
        atexit(free_input_buffers);
    }

    // Process shebang if the shell is invoked with a script
    if (shell_type() == NORMAL_SHELL && *in) {
        process_shebang(*in);
    }

    return 0;
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
