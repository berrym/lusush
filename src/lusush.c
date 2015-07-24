/**
 * lusush.c - LUSUs' SHell
 *
 * Copyright (c) 2009-2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_LIBREADLINE
#include <readline/history.h>
#endif
#include "lusush.h"
#include "init.h"
#include "history.h"
#include "input.h"
#include "parse.h"
#include "cmdlist.h"
#include "exec.h"
#include "misc.h"
#include "opts.h"

/**
 * main:
 *      Program entry point, let the magic begin!
 */
int main(int argc, char **argv)
{
    char *ENV_PROMPT = NULL;
    char prompt[MAXLINE] = { '\0' };
    bool bActive = true;
    int ret = 0;
    char *line = NULL;
    FILE *in = NULL;
    struct command *cmd = NULL;

    // Perform startup tasks
    init(argc, argv);

    // Open input stream
    if (SHELL_TYPE == NORMAL_SHELL) {
        // Open the file stream with in pointing to it
        if ((in = fopen(argv[1], "r")) == NULL) {
            perror("lusush: lusush.c: main: fopen");
            exit(EXIT_FAILURE);
        }
    }
    else {
        in = stdin;
    }

    // Read input one line at a time until user exits
    // or EOF is read from either stdin or input file
    while (bActive) {
        // Allocate memory for doubly linked list of commands
        if ((cmd = calloc(1, sizeof(struct command))) == NULL) {
            perror("lusush: lusush.c: main: calloc");
            exit(EXIT_FAILURE);
        }

        // Build our prompt string
        ENV_PROMPT = getenv("PROMPT");
        strncpy(prompt, ENV_PROMPT ? ENV_PROMPT : "% ", MAXLINE);

        // Read a line of input from the opened stream
        line = get_input(in, prompt);

        // Handle the results of get_input
        switch (ret = parse_command(line, cmd)) {
        case -1:                    // Error
            bActive = false;        // Exit program
            break;
        case 0:                     // Empty input, ignore
            break;
        default:                    // Processed input
            if (opt_is_set(VERBOSE_PRINT))
                display_cmd(cmd);

            vprint("ret (at) main --> %d\n", ret);

            // Execute the command(s)
            exec_cmd(cmd, ret);

            // Free command(s)
            free_cmdlist(cmd);
            break;
        }
    }

    // Save command history
    write_history(histfilename());

    if (SHELL_TYPE != NORMAL_SHELL)
        printf("\n");

    exit(EXIT_SUCCESS);
}
