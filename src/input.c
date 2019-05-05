/**
 * input.c - input routines
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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include "lusush.h"
#include "errors.h"
#include "input.h"
#include "init.h"
#include "history.h"
#include "prompt.h"

static char *line_read = NULL;  // storage for readline and fgets

/**
 * null_terminate_line:
 *      Terminate a string with null character instead of a newline.
 */
static inline void null_terminate_line(char *s)
{
    if (s[strnlen(s, MAXLINE) - 1] == '\n')
        s[strnlen(s, MAXLINE) - 1] = '\0';
}

/**
 * free_line_read:
 *      Free memory allocated to line_read.
 */
void free_line_read(void)
{
    if (line_read)
        free(line_read);

    line_read = NULL;
}

/**
 * get_input:
 *      Read a line of input, store the line in history.
 *      Return a pointer to the line read.
 */
char *get_input(FILE *in)
{
    // If the buffer has been previously allocated free it
    free_line_read();

    // Allocate memory for a line of input
    if ((line_read = calloc(MAXLINE + 1, sizeof(char))) == NULL) {
        error_syscall("get_input: calloc");

    // If the shell is interactive print a prompt string
    if (shell_type() != NORMAL_SHELL) {
        build_prompt();
        printf("%s", getenv("PROMPT"));
    }

    // Read a line of input
    if (fgets(line_read, MAXLINE, in) == NULL)
        return NULL;

    null_terminate_line(line_read);

    if (in == stdin && line_read && *line_read)
        add_history(line_read);

    return line_read;
}
