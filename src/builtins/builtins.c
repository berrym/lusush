#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "builtins.h"
#include "alias.h"
#include "exec.h"
#include "history.h"
#include "lusush.h"
#include "scanner.h"
#include "strings.h"
#include "symtable.h"

// Table of builtin commands
builtin builtins[] = {
    { "exit",        "exit shell",                  bin_exit      },
    { "help",        "builtin help",                bin_help      },
    { "cd",          "change directory",            bin_cd        },
    { "pwd",         "print working directory",     bin_pwd       },
    { "history",     "print command history",       bin_history   },
    { "alias",       "set an alias",                bin_alias     },
    { "unalias",     "unset an alias",              bin_unalias   },
    { "dump",        "dump symbol table",           bin_dump      },
};

const size_t builtins_count = sizeof(builtins) / sizeof(builtin);

/**
 * bin_exit:
 *      Exit the shell.
 */
int bin_exit(int argc, char **argv)
{
    exit(EXIT_SUCCESS);
    return 0;
}

/**
 * bin_help:
 *      Print a list of builtins and their description.
 */
int bin_help(int argc, char **argv)
{
    for (size_t i = 0; i < builtins_count; i++)
        fprintf(stderr, "\t%-10s%-40s\n",
                builtins[i].name, builtins[i].doc);

    return 0;
}

/**
 * bin_cd:
 *      Change working directory.
 */
int bin_cd(int argc, char **argv)
{
    if (argc == 1) {
        if (chdir(getenv("HOME")) != 0) {
            error_return("cd");
            return 1;
        }

        return 0;
    }

    if (argc != 2) {
        error_message("usage: cd pathname");
        return 1;
    }

    if (chdir(argv[1]) < 0) {
        error_return("cd");
        return 1;
    }

    return 0;
}

/**
 * bin_pwd:
 *      Print working directory.
 */
int bin_pwd(int argc, char **argv)
{
    char cwd[MAXLINE] = { '\0' };

    if (getcwd(cwd, MAXLINE) == NULL) {
        error_return("pwd");
        return 1;
    }

    printf("%s\n", cwd);

    return 0;
}

/**
 * bin_history:
 *      Implementation of a history command.
 */
int bin_history(int argc, char **argv)
{
    switch (argc) {
    case 1:
        print_history();
        break;
    case 2:
        // Lookup a history entry
        char *s = lookup_history(argv[1]);
        if (!s) {
            history_usage();
            break;
        }

        // Create a source struct from history entry
        source_s src;
        src.buf = s;
        src.bufsize = strlen(s);
        src.pos = INIT_SRC_POS;

        parse_and_execute(&src);
        break;
    default:
        history_usage();
        break;
    }

    return 0;
}

/**
 * bin_alias:
 *      Create aliased commands, or print alias values.
 */
int bin_alias(int argc, char **argv)
{
    char *src = NULL, *name = NULL, *val = NULL;

    switch (argc) {
    case 1:
        print_aliases();
        break;
    case 2:
        const char *s = lookup_alias(argv[1]);
        if (!*s) {
            alias_usage();
            return 1;
        } else {
            printf("%s=\"%s\"\n", argv[1], s);
        }
        break;
    default:
        src = src_str_from_argv(argc, argv, " ");
        if (!src)
            return 1;

        name = parse_alias_var_name(src);
        if (!name) {
            alias_usage();
            return 1;
        }

        val = parse_alias_val_dquotes(src);
        if (!val) {
            alias_usage();
            return 1;
        }

        if (is_builtin(name)) {
            error_message("alias: cannot alias shell keyword: %s", name);
            return 1;
        } else {
            if (!set_alias(name, val)) {
                error_message("alias: failed to create alias");
                return 1;
            }
        }
    }

    free_str(src);
    free_str(name);
    free_str(val);

    return 0;
}

/**
 * bin_unalias:
 *      Remove an aliased command.
 */
int bin_unalias(int argc, char **argv)
{
    if (argc < 2) {
        error_message("unalias: not enough arguments");
        return 1;
    }

    switch (argc) {
    case 2:
        unset_alias(argv[1]);
        break;
    default:
        error_message("usage: unalias name");
        break;
    }

    return 0;
}

/**
 * bin_dump:
 *      Print a local symbol table.
 */
int bin_dump(int argc, char **argv)
{
    dump_local_symtable();
    return 0;
}

/**
 * is_builtin:
 *      Check if a command name is a builtin command.
 */
bool is_builtin(const char *name)
{
    for (size_t i = 0; i < builtins_count; i++)
        if (strcmp(name, builtins[i].name) == 0)
            return true;

    return false;
}
