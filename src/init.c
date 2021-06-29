#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "alias.h"
#include "errors.h"
#include "history.h"
#include "init.h"
#include "input.h"
#include "lusush.h"
#include "scanner.h"
#include "signals.h"
#include "symtable.h"

extern char **environ;

// The type of shell instance
static int SHELL_TYPE;

int parse_opts(int argc, char **argv);
void usage(int err);

int shell_type(void)
{
    return SHELL_TYPE;
}

int init(int argc, char **argv, FILE **in)
{
    struct stat st;             // stat buffer

    if (!argv)
        exit(EXIT_FAILURE);

    // Set all locales according to environment
    setlocale(LC_ALL, "");

    // Setup signal handlers
    init_signal_handlers();

    // Initialize symbol table
    init_symtable();

    struct symtable_entry *entry;
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
                symtable_entry_setval(entry, eq+1);
                entry->flags |= FLAG_EXPORT;
            }
        } else {
            entry = add_to_symtable(*p);
        }
        p++;
    }

    entry = add_to_symtable("PS1");
    symtable_entry_setval(entry, "% ");
    entry = add_to_symtable("PS2");
    symtable_entry_setval(entry, "> ");

    // Parse command line options
    size_t optind = parse_opts(argc, argv);

    // Determine the shell type
    if (**argv == '-') {
        SHELL_TYPE = LOGIN_SHELL;
    } else if (optind && argv[optind] && *argv[optind]) {
        // Check that argv[optind] is a regular file
        stat(argv[optind], &st);
        if (!S_ISREG(st.st_mode)) {
            error_message("lusush: %s is not a regular file.\n",
                          argv[optind]);
            optind = 0;
            SHELL_TYPE = INTERACTIVE_SHELL;
        } else {
            SHELL_TYPE = NORMAL_SHELL;
            if ((*in = fopen(argv[optind], "r")) == NULL)
                error_syscall("fopen");
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
    struct symtable_entry *ppid_entry = add_to_symtable("PPID");
    symtable_entry_setval(ppid_entry, ppid_str);
    entry->flags |= FLAG_READONLY;

    // Initialize history
    init_history();
    // Read the history file
    if (read_history(histfn) != 0)
        error_message("init: unable to read history");

    // Initialize aliases
    init_aliases();

    // Set memory cleanup procedures on termination
    atexit(free_tok_buf);
    atexit(free_global_symtable);
    atexit(free_alias_list);
    #ifndef USING_READLINE
    atexit(free_input_buffers);
    #endif
    atexit(free_history_list);
    atexit(save_history);

    return 0;
}

int parse_opts(int argc, char **argv)
{
    // next option
    int nopt = 0;
    // string of valid short options
    const char *sopts = "h";
    // array describing valid long options
    const struct option lopts[] = {
        { "help", 0, NULL, 'h' },
        { NULL, 0, NULL, 0 }
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
            error_abort("unknown error terminating\n"); // should never happen
        }
    } while (nopt != -1);

    return optind;
}

void usage(int err)
{
    error_message("Usage:\n\t-h This Help\n");
    exit(err);
}
