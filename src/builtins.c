#include <unistd.h>
#include <stdio.h>
#include "ldefs.h"
#include "builtins.h"
#include "misc.h"

/**
 * Builtin commands
 *      exit    exit program
 *      cd      change directory
 *      pwd     print working directory
 */
const char *builtin_cmds[BUILTIN_CMD_CNT] =
{
    "exit",
    "cd",
    "pwd"
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
