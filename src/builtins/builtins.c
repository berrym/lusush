#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "builtins.h"
#include "alias.h"
#include "history.h"
#include "lusush.h"
#include "prompt.h"
#include "symtable.h"

struct builtin builtins[] = {
    { "exit",        "exit shell",                  bin_exit      },
    { "help",        "builtin help",                bin_help      },
    { "cd",          "change directory",            bin_cd        },
    { "pwd",         "print working directory",     bin_pwd       },
    { "history",     "print command history",       bin_history   },
    { "alias",       "set an alias",                bin_alias     },
    { "unalias",     "unset an alias",              bin_unalias   },
    { "dump",        "dump symbol table",           bin_dump      },
};

size_t builtins_count = sizeof(builtins) / sizeof(struct builtin);

int bin_exit(int argc, char **argv)
{
    exit(EXIT_SUCCESS);
    return 0;
}

int bin_help(int argc, char **argv)
{
    for (size_t i = 0; i < builtins_count; i++)
        fprintf(stderr, "\t%-10s%-40s\n",
                builtins[i].name, builtins[i].doc);

    return 0;
}

int bin_cd(int argc, char **argv)
{
    if (chdir(argv[1]) < 0) {
        error_return("cd: chdir");
        return 1;
    }

    return 0;
}

int bin_pwd(int argc, char **argv)
{
    char cwd[_PC_PATH_MAX + 1] = { '\0' };

    if (getcwd(cwd, _PC_PATH_MAX) == NULL) {
        error_return("pwd: getcwd");
        return 1;
    }

    fprintf(stderr, "%s\n", cwd);

    return 0;
}

int bin_history(int argc, char **argv)
{
    print_history();
    return 0;
}

int bin_alias(int argc, char **argv)
{
    size_t v = 1, i;
    char *str = NULL, *eq = NULL;
    int res = 0, res3;

    switch (argc) {
    case 1:
        print_alias_list();
        break;
    default:
        for (; v < argc; v++) {
            str = argv[v];
            eq = strchr(str, '=');
            if (eq) {
                i = eq-str;
                char tmp[i+1];
                strncpy(tmp, str, i);
                tmp[i] = '\0';

                if(strncmp(tmp, "alias", i) == 0 || strncmp(tmp, "unalias", i) == 0) {
                    error_message("alias: cannot alias shell keyword: %s\n", tmp);
                    res = 2;
                } else {
                    // Set the alias value, which is the part after the =
                    res3 = set_alias(tmp, eq+1);

                    // If error, return an error result
                    if (res3)
                        res = res3;
                }
            }
            else {
                error_message("usage: alias word replacement text\n");
                break;
            }
        }
    }

    return res;
}

int bin_unalias(int argc, char **argv)
{
    if (argc < 2) {
        error_message("alias: not enough arguments");
        return 1;
    }

    switch (argc) {
    case 2:
        unset_alias(argv[1]);
        break;
    default:
        error_message("usage: unalias word");
        break;
    }

    return 0;
}

int bin_dump(int argc, char **argv)
{
    dump_local_symtable();
    return 0;
}
