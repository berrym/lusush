/**
 * builtins.c - builtin commands
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "lusush.h"
#include "builtins.h"
#include "misc.h"
#include "history.h"
#include "prompt.h"

/**
 * Builtin commands:
 *      exit        exit program
 *      help        print help
 *      cd          change directory
 *      pwd         print working directory
 *      history     print command history
 *      setenv      set environment variable
 *      unsetenv    unset environment variable
 *      alias       set an alias
 *      unalias     unset an alias
 *      setopt      turn on an option
 *      unsetopt    turn off an option
 *      setprompt   set prompt colors
 */

const char *builtins[BUILTIN_CMD_CNT] =
{
    "exit",         "exit program",
    "help",         "display basic help",
    "cd",           "change directory",
    "pwd",          "print working directory",
    "history",      "print command history",
    "setenv",       "set environment variable",
    "unsetenv",     "delete environment variable",
    "alias",        "set an alias",
    "unalias",      "unset an alias",
    "setopt",       "turn on an option",
    "unsetopt",     "turn off an option",
    "setprompt",    "set prompt colors"
};

/**
 * is_builtin_cmd:
 *      compare (cmdname) to elements in array of strings
 *      builtins, if it matches return the index of the element.
 */
int is_builtin_cmd(const char *cmdname)
{
    int i;

    for (i = 0; i < BUILTIN_CMD_CNT; i += 2) {
        if (strcmp(cmdname, builtins[i]) == 0)
            return i;
    }

    return -1;
}

/**
 * help:
 *    display list of builtin commands with a brief description
 *    of it's function
 */
void help(const char *cmdname)
{
    int i;
    if (cmdname == NULL) {
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

/**
 * cd:
 *    change working directory
 */
void cd(const char *path)
{
    if (chdir(path) < 0)
        perror("lusush: chdir");
    build_prompt();
}

/**
 * pwd:
 *    print working directory
 */
void pwd(void)
{
    char cwd[BUFSIZ];

    if (getcwd(cwd, BUFSIZ) == NULL)
        perror("lusush: getcwd");
    else
        printf("%s\n", cwd);
}

/**
 * history:
 *    display a list of stored user inputed commands
 */
void history(void)
{
    int i;

#ifdef HAVE_LIBREADLINE
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
