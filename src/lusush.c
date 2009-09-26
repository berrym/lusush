/**
 * lusush.c - LUSUs' SHell
 *
 * Copyright (c) 2009 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Include statements {{{

#include <sys/stat.h>
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
    char ps1[MAXLINE];
    CMD *cmd = NULL;
    bool bActive = true;
    register int i = 1;
    int ret = 0;
    char *line = NULL;
    FILE *in = NULL;
    CMDLIST cmdhist;
    struct stat st;

    // Perform startup tasks
    init(argc, argv);

    // Initialize doubly linked list of commands
    cmdhist.head.next = NULL;
    cmdhist.head.prev = NULL;
    cmdhist.size = 0;

    // Point cmd to list head
    cmd = &cmdhist.head;

    // Open input stream
    if (SHELL_TYPE == NORMAL_SHELL) {
        // check that argv[1] is a regular file
        if (stat(argv[1], &st)) {
            if (!S_ISREG(st.st_mode)) {
                fprintf(stderr, "Lusush: %s is not a regular file.\n", argv[1]);
            }
        }

        // open the file stream with in pointing to it
        if ((in = fopen(argv[1], "r")) == NULL) {
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

    // Read input one line at a time unitll user exits
    // or EOF is read from either stdin or input file
    while (bActive) {
        // Build our prompt string
        ENV_PROMPT = getenv("PROMPT");
        strncpy(ps1, ENV_PROMPT ? ENV_PROMPT : "%", MAXLINE);
        line = get_input(in, ps1);

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
