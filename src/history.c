/**
 * history.c - routines to work with command input history
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
#include "history.h"
#include "opts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_LIBREADLINE
#include <readline/history.h>
#endif

static size_t MAXHIST = 50;     // maximum length of history
#ifdef HAVE_LIBREADLINE
static HIST_ENTRY **hist_list = NULL;
#else
static size_t hist_size = 0;
static char **hist_list = NULL;
#endif

/**
 * init_history:
 *      Create or read the history file.
 */
void init_history(void)
{
    // Check if the history list is already initialized
    if (hist_list) {
        error_message("lusush: init_history: already initialized.\n");
        return;
    }

#ifdef HAVE_LIBREADLINE
    using_history();
    stifle_history(MAXHIST);
#else
    if ((hist_list = calloc(MAXHIST, sizeof(char *))) == NULL)
        error_syscall("lusush: history.c: init_history: calloc");    
#endif

    // Read the history file
    if (read_history() != 0)
        return;
}

#ifndef HAVE_LIBREADLINE
/**
 * read_history:
 *      Read stored commands from the history file.
 */
int read_history(void)
{
    size_t i;                        // loop counter
    FILE *fp = NULL;                 // file stream pointer
    const char *fn = histfilename(); // file stream name

    vputs("Reading history %s\n", fn);

    // Open the history file for reading
    if ((fp = fopen(fn, "r")) == NULL) {
        if (opt_is_set(VERBOSE_PRINT))
            error_return("lusush: history.c: read_histfile: fopen");
        return 1;
    }

    vputs("File %s is open with descriptor %u\n", fn, fp);

    // Read the history file one line at a time
    for (i = 0; i < MAXHIST; i++) {
        if ((hist_list[i] = calloc(MAXLINE + 1, sizeof(char))) == NULL)
            error_syscall("lusush: history.c: read_history: calloc");

        if (fgets(hist_list[i], MAXLINE + 1, fp) == NULL)
            break;
        else
            if (hist_list[i][strnlen(hist_list[i], MAXLINE) - 1] == '\n')
                hist_list[i][strnlen(hist_list[i], MAXLINE) - 1] = '\0';

        // Check the stream for errors
        if (ferror(fp))
            error_return("lusush: history.c: read_history");
    }

    hist_size = i;              // set the history count
    fclose(fp);                 // close the file stream

    return 0;
}

/**
 * add_history:
 *      Add a line of history to hist_list.
 */
void add_history(const char *line)
{
    if (!line || !*line)
        return;

    // Max history limit has been reached, grow the array
    if (hist_size == MAXHIST) {
        MAXHIST *= 2;
        if ((hist_list = realloc(hist_list, MAXHIST*sizeof(char *))) == NULL)
            error_syscall("lusush: history.c: add_history: realloc");
        vputs("*** GREW HISTORY TO %u\n", MAXHIST);
    }

    // Allocate next input history
    if ((hist_list[hist_size] = calloc(MAXLINE + 1, sizeof(char))) == NULL)
        error_syscall("lusush: history.c: add_history: calloc");

    // Save the line to the history list
    strncpy(hist_list[hist_size], line, MAXLINE);
    hist_size++;
}


/**
 * write_history:
 *      Write command history to a file.
 */
void write_history(void)
{
    size_t i;                        // loop counter
    FILE *fp = NULL;                 // file stream pointer
    const char *fn = histfilename(); // filestream name

    // Open the history file for writing
    if ((fp = fopen(fn, "w")) == NULL) {
        if (opt_is_set(VERBOSE_PRINT))
            error_return("lusush: history.c: write_history: fopen");
        return;
    }

    // Write each history item as a new line
    for (i = 0; hist_list[i] && *hist_list[i]; i++)
        fprintf(fp, "%s\n", hist_list[i]);

    // Close the file stream
    fclose(fp);
}


/**
 * free_history_list:
 *      Free the input history.
 */
void free_history_list(void)
{
    if (!hist_list || !*hist_list)
        return;

    // Free individual input histories
    for (; hist_size; hist_size--) {
        free(hist_list[hist_size]);
        hist_list[hist_size] == NULL;
    }

    // Free history array
    free(hist_list);
    hist_list = NULL;
}
#endif

/**
 * histfilename:
 *      Return the name of the history file.
 */
const char *histfilename(void)
{
    static char fn[MAXLINE + 1] = { '\0' };

    if (!*fn)
        snprintf(fn, MAXLINE, "%s/.lusushist", getenv("HOME"));

    return fn;
}

/**
 * print_history:
 *      Display a the list of lines stored in history.
 */
void print_history(void)
{
    size_t i;

#ifdef HAVE_LIBREADLINE
    if (!(hist_list = history_list()))
        return;

    for (i = 0; hist_list[i]; i++)
        printf("%4zu:\t%s\n", i + history_base, hist_list[i]->line);
#else
    if (!hist_list) {
        error_message("no hist list");
        return;
    }
    for (i = 0; i < hist_size && *hist_list[i]; i++)
        printf("%zu:\t%s\n", i + 1, hist_list[i]);
#endif
}
