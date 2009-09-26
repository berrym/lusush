/**
 * lusush.c - main function
 */

// Include statements {{{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ltypes.h"
#include "init.h"
#include "env.h"
#include "history.h"
#include "input.h"
#include "cmdlist.h"
#include "exec.h"
#include "builtins.h"
#include "misc.h"

// End of includes }}}

// function main {{{

int main(int argc, char **argv)
{
    CMD *cmd = (CMD *)0;
    bool bActive = true;
    register int i = 1;
    int ret = 0;
    char *line = (char *)0;
    FILE *in = (FILE *)0;
    CMDLIST cmdhist;

    // Perform startup tasks
    init(argc, argv);

    // Initialize doubly linked list of commands
    cmdhist.head.next = (CMD *)0;
    cmdhist.head.prev = (CMD *)0;
    cmdhist.size = 0;

    // Point cmd to list head
    cmd = &cmdhist.head;

    // Open input stream
    if (SHELL_TYPE == NORMAL_SHELL) {
        if ((in = fopen(argv[1], "r")) == (FILE *)0) {
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
        line = get_input(in, (ENV_PROMPT = getenv("PROMPT"))
                ? ENV_PROMPT : "% ");

        // Handle the results of get_input
        switch (ret = do_line(line, &cmdhist, cmd)) {
            case -1:                    // Error
                bActive = false;        // Exit program
                break;
            case 0:                     // Empty input, ignore
                break;
            default:                    // Processed input
                // Mark place of first command parsed by get_input 
                cmd->hist_offset = i;

                // Very verbose printing of command history, only useful
                // if debugging the program, macro defined in ldefs.h
#if defined( PRINT_DEBUG_CRAZY )
                display_cmdlist(&cmdhist);
#elif defined ( PRINT_DEBUG )
                display_cmd(cmd);
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

                free_cmdlist(&cmdhist);
                cmd = &cmdhist.head;
                break;
        }
        i++;
    }

    //////////////////////////////////////////////////
    // Cleanup/Pre-Exit Tasks 
    //////////////////////////////////////////////////

    write_histfile(histfile);

    for (i = 0; i < cmdhist.size; i++) {
        print_debug("#%d# ", i+1);
    }

    if (SHELL_TYPE != NORMAL_SHELL) {
        printf("\n");
    }

    global_cleanup();                   // Free globals, set to zero/0
    if (cmdhist.size) {
        free_cmdlist(&cmdhist);         // Free command history
    }

    return 0;                           // Optimist!
}

// End of main }}}


// vim:filetype=c foldmethod=marker autoindent expandtab shiftwidth=4
