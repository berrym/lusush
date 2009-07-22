/**
 * lusush.c - main function
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ltypes.h"
#include "init.h"
#include "env.h"
#include "input.h"
#include "cmdlist.h"
#include "exec.h"
#include "misc.h"

///////////////////////// MAIN FUNCTION //////////////////////////////////////

int main(int argc, char **argv, char **env)
{
    CMD *cmd;
    bool bActive = true;
    int i;
    char *cmdpath;
    CMDLIST cmdhist;

    if ((cmdhist.head = (CMD *)calloc(1, sizeof(CMD *))) == NULL) {
        perror("lusush: calloc");
        exit(EXIT_FAILURE);
    }

    // Perform startup tasks
    init(argv);

    // Initialize doubly linked list of commands
    cmdhist.head->next = NULL;
    cmdhist.head->prev = NULL;
    cmdhist.size = 0;

    // Point cmd to list head
    cmd = cmdhist.head;

    while (bActive) {
        if ((cmdpath = (char *)calloc(MAXLINE, sizeof(char))) == NULL) {
            perror("lusush: calloc");
            bActive = false;
        }
        printf("%s ", ENV_PROMPT);

        if (get_input(stdin, &cmdhist, cmd) < 0) {
            bActive = false;
        }
        else {
#if defined( PRINT_DEBUG )
            display_cmdlist(&cmdhist);
#endif
            if (is_builtin_cmd(cmd->argv[0]) == true) {
                exec_builtin_cmd(cmd);
            }
            else {
                cmdpath = path_to_cmd(cmd->argv[0]);
                if (cmdpath != NULL && strcmp(cmdpath, "S_ISDIR") == 0) {
                    printf("lusush: %s is a directory.\n", cmd->argv[0]);
                }
                else if (cmdpath != NULL) {
                    strcpy(cmd->argv[0], cmdpath);
                    exec_external_cmd(cmd, NULL);
                }
                else {
                    printf("lusush: command not found.\n");
                }

                if (cmdpath != NULL)
                    free(cmdpath);
                cmdpath = NULL;
            }
            cmd = cmd->next;
        }
    }

    for (i = 0; i < cmdhist.size; i++) {
        printf("#%d# ", i+1);
    }
    printf("\n");

    global_cleanup();
    //free_cmdlist(&cmdhist);

    return 0;
}
