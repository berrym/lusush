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
 *      exit    exit program
 *      cd      change directory
 *      pwd     print working directory
 *      history print command history
 */
const char *builtin_cmds[BUILTIN_CMD_CNT] =
{
    "exit",
    "cd",
    "pwd",
    "history"
};

///////////////////////// BUILTIN COMMANDS ///////////////////////////////////

void cd(const char *path)
{
    if (chdir(path) < 0)
        perror("lusush: chdir");
    build_prompt();
}

void pwd(void)
{
    char cwd[MAXLINE];

    if (getcwd(cwd, MAXLINE) == NULL)
        perror("lusush: getcwd");
    else
        printf("%s\n", cwd);
}

#if defined( USING_READLINE )
void history(void)
{
    int i;
    if ((hist_list = history_list())) {
        puts("Command history.\n");
        for (i = 0; hist_list[i]; i++) {
            if (hist_list[i])
                printf("%4d:\t%s\n", i+history_base, hist_list[i]->line);
        }
    }
}
#else
void history(CMD *cmd)
{
    int i = 0;

    puts("Command history.\n");

    while (cmd->prev != NULL) {
        cmd = cmd->prev;
    }

    while (cmd->next != NULL) {
        printf("%4d:\t%s\n", i, cmd->buf);
        cmd = cmd->next;
        i++;
    }  
}
#endif
