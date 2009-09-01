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
    CMD *cmd = (CMD *)NULL;
    bool bActive = true;
    register int i = 0;
    int ret = 0;
    FILE *in = (FILE *)NULL;
    CMDLIST cmdhist;

    // Perform startup tasks
    init(argc, argv);

    // Initialize doubly linked list of commands
    cmdhist.head.next = (CMD *)NULL;
    cmdhist.head.prev = (CMD *)NULL;
    cmdhist.size = 0;

    // Point cmd to list head
    cmd = &cmdhist.head;

    // Open input stream
    if (SHELL_TYPE == NORMAL_SHELL) {
        if ((in = fopen(argv[1], "r")) == (FILE *)NULL) {
            perror("lusush: fopen");
            exit(EXIT_FAILURE);
        }
    }
    else {
        in = stdin;
    }

    //////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////

    while (bActive) {

        // Read input one line at a time unitll user exits
        // or EOF is read from either stdin or input file

        // If not using readline print the prompt here
#if !defined( USING_READLINE )
        if (SHELL_TYPE != NORMAL_SHELL) {
            printf("%s", (ENV_PROMPT = getenv("PROMPT")) ? ENV_PROMPT : "% ");
        }
#endif

        // Handle the results of get_input
        switch (ret = do_line(in, &cmdhist, cmd)) {
            case -1:                    // Error
                bActive = false;        // Exit program
                break;
            case 0:                     // Empty input, ignore
                break;
            default:                    // Processed input

                // Mark place of first command parsed by get_input 
                cmd->hist_offset = cmdhist.size;

                // Very verbose printing of command history, only useful
                // if debugging the program, macro defined in ldefs.h
#if defined( PRINT_DEBUG )
                display_cmdlist(&cmdhist);
#endif
                print_debug("ret (at) main --> %d\n", ret);

                // Execute the number of commands parsed by get_input (ret)
                if (exec_cmd(cmd, ret) < ret) {
                    while (cmd->next) {         // If an error occured find the
                        cmd = cmd->next;        // end of the command history
                    }
                }
                else {
                    cmd = cmd->next;            // Point cmd to next (empty)
                }
                break;
        }
    }

    //////////////////////////////////////////////////
    // Cleanup/Pre-Exit Tasks 
    //////////////////////////////////////////////////

    for (i = 0; i < cmdhist.size; i++) {
        print_debug("#%d# ", i+1);
    }

    if (SHELL_TYPE != NORMAL_SHELL) {
        printf("\n");
    }

    global_cleanup();                   // Free globals, set to zero/NULL
    if (cmdhist.size) {
        free_cmdlist(&cmdhist);         // Free command history
    }

    return 0;                           // Optimist!
}
