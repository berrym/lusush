/**
 * lusush.c - LUSUs' SHell
 *
 * Copyright (c) 2015 Michael Berry <trismegustis@gmail.com>
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

#include "init.h"
#include "tty.h"
#include "cmdlist.h"
#include "input.h"
#include "parse.h"
#include "exec.h"
#include <stdlib.h>
#include <string.h>

/**
 * main:
 *      Program entry point, read-parse-execute loop.
 */
int main(int argc, char **argv)
{
    FILE *in = stdin;                // input file stream pointer
    bool looping = true;             // boolean flag to keep looping
    struct command *cmd = NULL;      // storage for command details
    char *line = NULL;               // pointer to a line of input read
    int ret = 0;                     // return status for parse_command

    // Perform startup tasks
    init(argc, argv, &in);

    // Read input one line at a time until user exits
    // or EOF is read from either stdin or input file
    while (looping) {
        tty_init(true);

        // Allocate memory for doubly linked list of commands
        cmd = create_command_list();

        // Read a line of input from the opened stream
        line = get_input(in);

        // Parse command(s) from line
        switch (ret = parse_command(line, cmd)) {
        case -1:                    // error
            looping = false;        // exit program
            break;
        case 0:                     // empty input, ignore
            break;
        default:                    // command(s) parsed
            vputs("ret @ main --> %d\n", ret);

            // Execute the command(s)
            exec_cmd(cmd);

            // Free command(s)
            free_command_list();
            break;
        }
        tty_close();
    }

    if (shell_type() != NORMAL_SHELL)
        printf("\n");

    exit(EXIT_SUCCESS);
}
