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
#include "builtins.h"
#include "misc.h"

///////////////////////// MAIN FUNCTION //////////////////////////////////////

int main(int argc, char **argv)
{
    CMD *cmd;
    bool bActive = true;
    int i;
    int ret;
    char *cmdpath;
    CMDLIST cmdhist;

    // Perform startup tasks
    init(argv);

    // Initialize doubly linked list of commands
    cmdhist.head.next = (CMD *) NULL;
    cmdhist.head.prev = (CMD *) NULL;
    cmdhist.size = 0;

    // Point cmd to list head
    cmd = &cmdhist.head;

    while (bActive) {
        if ((cmdpath = (char *)calloc(MAXLINE, sizeof(char))) == NULL) {
            perror("lusush: calloc");
            bActive = false;
        }
#if !defined( USING_READLINE )
        printf("%s", (ENV_PROMPT = getenv("PROMPT")) ? ENV_PROMPT : "% ");
#endif
        switch(ret = get_input(stdin, &cmdhist, cmd)) {
            case -1:
                bActive = false;
                break;
            case 0:
                break;
            default:
#if defined( PRINT_DEBUG )
                display_cmdlist(&cmdhist);
#endif
                if ((ret = is_builtin_cmd(cmd->argv[0])) != -1) {
                    exec_builtin_cmd(ret, cmd);
                }
                else {
                    cmdpath = path_to_cmd(cmd->argv[0]);
                    if (cmdpath != NULL && strcmp(cmdpath, "S_ISDIR") == 0) {
                        print_debug("lusush: %s is a directory.\n",
                                cmd->argv[0]);
                        cd(cmd->argv[0]);
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
                break;
        }
    }

    for (i = 0; i < cmdhist.size; i++) {
        print_debug("#%d# ", i+1);
    }
    printf("\n");

    global_cleanup();
    if (cmdhist.size) {
        free_cmdlist(&cmdhist);
    }

    return 0;
}
