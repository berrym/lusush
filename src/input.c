/**
 * input.c - input routines
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

#ifdef HAVE_LIBREADLINE
#include <stdio.h>                  // Needed for readline history to compile
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lusush.h"
#include "input.h"
#include "expand.h"
#include "cmdlist.h"
#include "init.h"
#include "parse.h"
#include "history.h"
#include "misc.h"

#define DBGSTR "DEBUG: input.c: "

static char *line_read = NULL;      // storage for readline and fgets

/**
 * strip_trailing_whspc:
 *      Remove whitespace at the end of a string.
 */
static void strip_trailing_whspc(char *s)
{
    while (strlen(s) && isspace((int)s[strlen(s) - 1]))
        s[strlen(s) - 1] = '\0';
}

#ifdef HAVE_LIBREADLINE
/**
 * rl_gets:
 *      Read a string, and return a pointer to it.  Returns 0 on EOF.
 */
char *rl_gets(const char *prompt)
{
    // If the buffer has already been allocated,
    // return the memory to the free pool
    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    // Get a line from the user
    line_read = readline(prompt);

    // If the line has any text in it, save it on the history
    if (line_read && *line_read)
        add_history(line_read);

    return line_read;
}
#endif

/**
 * get_input:
 *      return a pointer to a line of user input, store line in history
 */
char *get_input(FILE *const restrict in, const char *const restrict prompt)
{
    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

#ifdef HAVE_LIBREADLINE
    char *tmp = NULL;

    if ((tmp = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: input.c: get_input: calloc");
        return NULL;
    }
    
    if (SHELL_TYPE != NORMAL_SHELL) {
        if ((line_read = rl_gets(prompt)) == NULL) {
            if (tmp) {
                free(tmp);
                tmp = NULL;
            }
            return NULL;
        }
    }
    else {
        if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL) {
            perror("lusush: input.c: get_input: calloc");
            if (tmp) {
                free(tmp);
                tmp = NULL;
            }
            return NULL;
        }

        if (fgets(line_read, MAXLINE, in) == NULL) {            
            if (tmp) {
                free(tmp);
                tmp = NULL;
            }
            return NULL;
        }
        
        if (line_read[strlen(line_read) - 1] == '\n')
            line_read[strlen(line_read) - 1] = '\0';
    }

    strncpy(tmp, line_read, MAXLINE);
    expand(tmp);
    vprint("%sexpanded_line=%s\n", DBGSTR, tmp);
    if (strcmp(tmp, line_read) != 0) {
        free(line_read);
        if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL) {
            perror("lusush: input.c: get_input: calloc");
            return NULL;
        }
        strncpy(line_read, tmp, MAXLINE);
    }

    if (tmp) {
        free(tmp);
        tmp = NULL;
    }
#else
    if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: input.c: get_input: calloc");
        return NULL;
    }

    if (SHELL_TYPE != NORMAL_SHELL)
        printf("%s", prompt);

    if (fgets(line_read, MAXLINE, in) == NULL)
        return NULL;

    if (line_read[strlen(line_read) - 1] == '\n')
        line_read[strlen(line_read) - 1] = '\0';

    strncpy(hist_list[hist_size], line_read, MAXLINE);
    hist_size++;

    expand(line_read);
    vprint("%sexpanded_line=%s\n", DBGSTR, line_read);
#endif

    return line_read;
}

/**
 * do_line:
 *      parse a line and fill struct command with data
 */
int do_line(const char *const restrict line, struct command *restrict cmd)
{
    size_t cnt = 0;                     // Number of commands parsed
    int err = 0;                        // error code
    int i = 0, j = 0;                   // loop variables
    bool pipe = false;                  // pipe chain flag

    // Storage for first tier of tokens (";")
    char *tok = NULL, *ptr1 = NULL, *savep1 = NULL;
    // Storage for secondary tier of tokens ("|")
    char *subtok = NULL, *ptr2 = NULL, *savep2 = NULL;
    // buffer for a copy of line to mangle with strtok_r
    char *tmp = NULL;

    if ((tmp = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: input.c: do_line: calloc");
        return -1;
    }

    if (!line) {
        err = -1;
        goto cleanup;
    }

    if (!*line) {
        err = 0;
        goto cleanup;
    }

    strncpy(tmp, line, MAXLINE); // make a copy of line to mangle

    for (i = 0, ptr1 = tmp; ; i++, ptr1 = NULL) {
        if (!(tok = strtok_r(ptr1, ";", &savep1)))
            break;

        strip_trailing_whspc(tok);

        for (j = 0, ptr2 = tok; ; j++, ptr2 = NULL) {
            if (!(subtok = strtok_r(ptr2, "|", &savep2))) {
                pipe = false;
                break;
            }

            strip_trailing_whspc(subtok);

            if (cmdalloc(cmd) < 0) {
                err = -1;
                goto cleanup;
            }

            strncpy(cmd->buf, subtok, strlen(cmd->buf)); // Copy the string

            if (j == 1) {
                vprint("****do pipe %s\n", subtok);
                cmd->prev->pipe = true;
                cmd->prev->pipe_head = true;
                pipe = true;
            }

            if (pipe)
                cmd->pipe = true;

            switch (err = parse_cmd(cmd, subtok)) {
            case -1:
            case 0:
                goto cleanup;
            default:
                cmd->next->prev = cmd;
                cmd = cmd->next;
                cmd->next = NULL;
                cnt++;
            }
        }
    }

cleanup:
    if (tmp) {
        free(tmp);
        tmp = NULL;
    }

    switch (err) {
    case -1:
    case 0:
        return err;
    default:
        return cnt;
    }
}
