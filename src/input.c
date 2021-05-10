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
#include <ctype.h>
#include "lusush.h"
#include "errors.h"
#include "input.h"
#include "init.h"
#include "history.h"
#include "prompt.h"

static char *line_read = NULL;  // storage for readline and fgets
static char *buf = NULL;

/**
 * strip_trailing_whspc:
 *      Remove whitespace at the end of a string.
 */
static inline size_t strip_trailing_whspc(char *s)
{
    size_t i = 0;
    while (strnlen(s, MAXLINE) && isspace((int)s[strnlen(s, MAXLINE) - 1])) {
        s[strnlen(s, MAXLINE) - 1] = '\0';
        i++;
    }
    return i;
}

/**
 * free_line_read:
 *      Free memory allocated to line_read.
 */
void free_line_read(void)
{
    if (line_read)
        free(line_read);

    if (buf)
        free(buf);

    line_read = buf = NULL;
}

/**
 * get_input:
 *      Read a line of input, store the line in history.
 *      Return a pointer to the line read.
 */
char *get_input(FILE *in)
{
    size_t buflen = 0;
    size_t linecap = 0;
    ssize_t linelen;

    // If the buffer has been previously allocated free it
    free_line_read();

    // Allocate memory for a line of input
    if ((line_read = calloc(MAXLINE + 1, sizeof(char))) == NULL)
        error_syscall("get_input: calloc");

    // Allocate memory for extended line of input
    if ((buf = calloc(MAXLINE + 1, sizeof(char))) == NULL)
        error_syscall("get_input: calloc");

    // If the shell is interactive print a prompt string
    if (shell_type() != NORMAL_SHELL) {
        build_prompt();
        printf("%s", getenv("PROMPT"));
    }

    // Read a line of input
    while ((linelen = getline(&line_read, &linecap, in))) {
        if (feof(in) || ferror(in))
            return NULL;
        strncat(buf, line_read, linelen);
        buflen += linelen;
        buflen -= strip_trailing_whspc(buf);
        if (buf[buflen - 1] == '\\') {
            buf[buflen - 1] = '\0';
            buflen -= 1;
            if (shell_type() != NORMAL_SHELL)
                fprintf(stderr, "> ");
        } else {
            break;
        }
    }

    // Add line to command history
    if (in == stdin && *buf)
        add_history(buf);

    // Return full line read
    return buf;
}
