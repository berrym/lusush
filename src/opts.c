#include "../include/linenoise/linenoise.h"
#include "../include/lusush.h"
#include "../include/symtable.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Key->value struct for ANSI codes
typedef struct {
    const char *opt;
    int val;
} bool_opts;

// Key-value table for bool shell opts
static const bool_opts bool_shell_opts[] = {
    { "MULTILINE_EDIT",  MULTILINE_EDIT},
    {"HISTORY_NO_DUPS", HISTORY_NO_DUPS},
    { "NO_WORD_EXPAND",  NO_WORD_EXPAND}
};
static const int NUM_BOOL_OPTS = sizeof(bool_shell_opts) / sizeof(bool_opts);

// boolean option flags
static bool history_no_dups = false;
static bool multiline_edit = true;
static bool no_word_expand = false;

static void setopt_usage(void) {
    printf("usage:\n\t-h\t\t\tThis help\n\t");
    printf("-b BOOLEAN OPTION\ttoggle a boolean shell option on or off\n\t");
    printf("-g BOOLEAN OPTION\tget the value of a boolean shell option\n\t");
    printf("-v\t\t\tshow valid options\n");
}

void init_shell_opts(void) {
    symtable_set_global_int("MULTILINE_EDIT", multiline_edit);
    symtable_set_global_int("HISTORY_NO_DUPS", history_no_dups);
    symtable_set_global_int("NO_WORD_EXPAND", no_word_expand);
}

void setopt(int argc, char **argv) {
    int i = 0;
    int nopt = 0;                 // next option
    const char *sopts = "hb:g:v"; // string of valid short options
    // array describing valid long options
    const struct option lopts[] = {
        {      "help", 0, NULL, 'h'},
        {      "bool", 1, NULL, 'b'},
        {"get-option", 1, NULL, 'g'},
        {"valid-opts", 0, NULL, 'v'},
        {        NULL, 0, NULL,   0}
    };

    optind = 1;

    if (argc < 2) {
        setopt_usage();
        return;
    }

    do {
        nopt = getopt_long(argc, argv, sopts, lopts, NULL);

        switch (nopt) {
        case 'h': // show usage help
            setopt_usage();
            break;
        case 'b': // toggle a shell bool option on or off
            for (i = 0; i < NUM_BOOL_OPTS; i++) {
                if (strcmp(optarg, bool_shell_opts[i].opt) == 0) {
                    switch (bool_shell_opts[i].val) {
                    case MULTILINE_EDIT:
                        multiline_edit = !multiline_edit;
                        linenoiseSetMultiLine(multiline_edit);
                        symtable_set_global_int("MULTILINE_EDIT",
                                                multiline_edit);
                        break;
                    case HISTORY_NO_DUPS:
                        history_no_dups = !history_no_dups;
                        linenoiseHistoryNoDups(history_no_dups);
                        symtable_set_global_int("HISTORY_NO_DUPS",
                                                history_no_dups);
                        break;
                    case NO_WORD_EXPAND:
                        no_word_expand = !no_word_expand;
                        symtable_set_global_int("NO_WORD_EXPAND",
                                                no_word_expand);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case 'g': // print an option and it's value
            for (i = 0; i < NUM_BOOL_OPTS; i++) {
                if (strcmp(optarg, bool_shell_opts[i].opt) == 0) {
                    switch (bool_shell_opts[i].val) {
                    case MULTILINE_EDIT:
                        printf("%s: %d\n", bool_shell_opts[i].opt,
                               multiline_edit);
                        break;
                    case HISTORY_NO_DUPS:
                        printf("%s: %d\n", bool_shell_opts[i].opt,
                               history_no_dups);
                        break;
                    case NO_WORD_EXPAND:
                        printf("%s: %d\n", bool_shell_opts[i].opt,
                               no_word_expand);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case 'v': // print valid setopt options
            printf("VALID BOOL OPTIONS :\n");
            for (i = 0; i < NUM_BOOL_OPTS; i++) {
                printf("\t%s\n", bool_shell_opts[i].opt);
            }
            break;
        case -1:
            break;
        default:
            setopt_usage();
            break;
        }
    } while (nopt != -1);
}
