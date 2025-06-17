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
#include "../include/scanner.h"
#include "../include/signals.h"
#include "../include/symtable.h"

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

            linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                          linenoiseUtf8NextCharLen,
                                          linenoiseUtf8ReadCode);
            linenoiseSetMultiLine(get_shell_vari("MULTILINE_EDIT", true));
            build_prompt();
        } else {
            SHELL_TYPE = NORMAL_SHELL;
            if ((*in = fopen(argv[optind], "r")) == NULL) {
                error_syscall("error: `init`: fopen");
            }
        }
    } else {
        optind = 0;
        SHELL_TYPE = INTERACTIVE_SHELL;
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

    return 0;
}

static int parse_opts(int argc, char **argv) {
    // next option
    int nopt = 0;
    // string of valid short options
    const char *sopts = "h";
    // array describing valid long options
    const struct option lopts[] = {
        {"help", 0, NULL, 'h'},
        {  NULL, 0, NULL,   0}
    };

    do {
        nopt = getopt_long(argc, argv, sopts, lopts, NULL);

        switch (nopt) {
        case 'h':
            usage(EXIT_SUCCESS);
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
    error_message("Usage:\n\t-h This Help\n");
    exit(err);
}
