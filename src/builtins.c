#include <unistd.h>
#include <stdio.h>
#include "ldefs.h"
#include "ltypes.h"
#include "builtins.h"
#include "misc.h"

#if defined( USING_READLINE )
#include <readline/history.h>
static HIST_ENTRY **hist_list;
#endif

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

///////////////////////// BUILTIN COMMANDS ///////////////////////////////////

void help(const char *cmdname)
{
    int i;
    if (cmdname == (char *)NULL) {
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

void cd(const char *path)
{
    if (chdir(path) < 0)
        perror("lusush: chdir");
    build_prompt();
}

void pwd(void)
{
    char cwd[MAXLINE];

    if (getcwd(cwd, MAXLINE) == (char *)NULL)
        perror("lusush: getcwd");
    else
        printf("%s\n", cwd);
}

#if defined( USING_READLINE )
void history(void)
{
    int i;
    if ((hist_list = history_list())) {
        printf("Command history.\n");
        for (i = 0; hist_list[i]; i++) {
            printf("%4d:\t%s\n", i+history_base, hist_list[i]->line);
        }
    }
}
#else
void history(CMD *cmd)
{
    int i = 0;

    printf("Command history.\n");

    while (cmd->prev) {
        cmd = cmd->prev;
    }

    while (cmd->next) {
        printf("%4d:\t%s\n", i, cmd->buf);
        cmd = cmd->next;
        i++;
    }  
}
#endif
