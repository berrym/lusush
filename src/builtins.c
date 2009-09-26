/**
 * builtins.c - bultin commands
 */

// include statements {{{

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "ldefs.h"
#include "ltypes.h"
#include "builtins.h"
#include "misc.h"
#include "history.h"

// end of include statements }}}

// macros/globals {{{

/**
 * Builtin commands
 *      exit        exit program
 *      help        print help
 *      cd          change directory
 *      pwd         print working directory
 *      history     print command history
 *      setenv      set environment variable
 *      unsetenv    unset environment variable
 */

const char *builtins[BUILTIN_CMD_CNT] =
{
    "exit",         "exit program",
    "help",         "display basic help",
    "cd",           "change directory",
    "pwd",          "print working directory",
    "history",      "print command history",
    "setenv",       "set environment variable",
    "unsetenv",     "delete environment variable"
};

// end of macros/globals }}}

// function help {{{

void help(const char *cmdname)
{
    int i;
    if (cmdname == (char *)0) {
        printf("Builtin commands:\n");
        for (i = 0; i < BUILTIN_CMD_CNT; i += 2) {
            printf("\t%-10s%-40s\n", builtins[i], builtins[i+1]);
        }
    }
    else {
        if ((i = is_builtin_cmd(cmdname)) != -1) {
            printf("\t%-10s%-40s\n", builtins[i], builtins[i+1]);
        }
    }
}

// end of help }}}

// function is_builtin_cmd {{{

/**
 * is_builtin_cmd:
 *      compare (cmdname) to elements is array of strings
 *      builtins, if it matches return the index of the element.
 */
int is_builtin_cmd(const char *cmdname)
{
    register int i;

    for (i = 0; i < BUILTIN_CMD_CNT; i += 2) {
        if (strcmp(cmdname, builtins[i]) == 0)
            return i;
    }

    return -1;
}

// end of is_builtin_cmd }}}

// function cd {{{

void cd(const char *path)
{
    if (chdir(path) < 0)
        perror("lusush: chdir");
    build_prompt();
}

// end of cd }}}

// function pwd {{{

void pwd(void)
{
    char cwd[MAXLINE];

    if (getcwd(cwd, MAXLINE) == (char *)0)
        perror("lusush: getcwd");
    else
        printf("%s\n", cwd);
}

// end of pwd }}}

// function history {{{

void history(void)
{
    register int i;
#if defined( USING_READLINE )
    if ((hist_list = history_list())) {
        printf("Command history.\n");
        for (i = 0; hist_list[i]; i++) {
            printf("%4d:\t%s\n", i+history_base, hist_list[i]->line);
        }
    }
#else
    for (i = 0; i < MAXHIST && *hist_list[i]; i++) {
        printf("%4d:\t%s\n", 1+i, hist_list[i]);
    }
#endif
}

// end of history }}}

// vim:filetype=c foldmethod=marker autoindent expandtab shiftwidth=4
