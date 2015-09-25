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

#include "lusush.h"
#include "input.h"
#include "init.h"
#include "history.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define DBGSTR "DEBUG: input.c: "

static char *line_read = NULL;  // storage for readline and fgets

/**
 * null_terminate:
 *      Terminate a string with null character instead of a newline.
 */
static inline void null_terminate(char *s)
{
    if (s[strnlen(s, MAXLINE) - 1] == '\n')
        s[strnlen(s, MAXLINE) - 1] = '\0';
}

#ifdef HAVE_LIBREADLINE
/**
 * rl_gets:
 *      Read a string, and return a pointer to it.  Returns 0 on EOF.
 */
static char *rl_gets(const char *prompt)
{
    // A line of input
    char *s = NULL;

    // Get a line from the user
    s = readline(prompt);

    // If the line has any text in it, save it in history
    if (s && *s)
        add_history(s);

    // Return a copy of s
    return s;
}
#endif

/**
 * get_input:
 *      Read a line of input, store the line in history.
 *      Return a pointer to the line read.
 */
char *get_input(FILE *in, const char *prompt)
{
    // If the buffer has been previously allocated free it
    if (line_read)
        free(line_read);

    line_read = NULL;

#ifdef HAVE_LIBREADLINE
    // Read a line from either a file or standard input
    if (shell_type() != NORMAL_SHELL) {
        if ((line_read = rl_gets(prompt)) == NULL)
            return NULL;
    }
    else {
        if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL)
            error_syscall("lusush: input.c: get_input: calloc");

        if (fgets(line_read, MAXLINE, in) == NULL)
            return NULL;

        null_terminate(line_read);
    }
#else
    if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL)
        error_syscall("lusush: input.c: get_input: calloc");

    if (shell_type() != NORMAL_SHELL)
        printf("%s", prompt);

    if (fgets(line_read, MAXLINE, in) == NULL)
        return NULL;

    null_terminate(line_read);

    if (in == stdin && line_read && *line_read)
        add_history(line_read);
#endif

    return line_read;
}
