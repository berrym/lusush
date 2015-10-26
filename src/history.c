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

#if defined(HAVE_EDITLINE_READLINE_H)
char *hist_list = NULL;
#elif defined(HAVE_LIBREADLINE)
char *hist_list = NULL;
#else
static size_t HIST_LIST_SIZE = 50;     // current size of the table
static size_t hist_size = 0;           // current number of table entries
static char **hist_list = NULL;
static size_t grow_hist_list(size_t);
static int read_history(const char *);
static void write_history(const char *);
#endif

/**
 * histfilename:
 *      Return the name of the history file.
 */
static const char *histfilename(void)
{
    static char fn[MAXLINE + 1] = { '\0' };

    if (!*fn)
        snprintf(fn, MAXLINE, "%s/.lusushist", getenv("HOME"));

    return fn;
}

/**
 * init_history:
 *      Create or read the history file.
 */
void init_history(void)
{
#if !defined(HAVE_EDITLINE_READLINE_H) && !defined(HAVE_LIBREADLINE)
    // Check if the history list is already initialized
    if (hist_list) {
        error_message("lusush: history.c: init_history: "
                      "already initialized.\n");
        return;
    }

    if ((hist_list = calloc(HIST_LIST_SIZE, sizeof(char *))) == NULL) {
        error_return("lusush: history.c: init_history: calloc");
        return;
    }
#else
    using_history();
#endif
    // Read the history file
    if (read_history(histfilename()) != 0)
        return;
}

/**
 * print_history:
 *      Display a list of the input history.
 */
void print_history(void)
{
#if defined(HAVE_EDITLINE_READLINE_H)
    HIST_ENTRY *h = NULL;
    size_t i = 0;

    for (i = 0; h = history_get(i + history_base); i++)
        printf("%s\n", h->line);
#elif defined(HAVE_LIBREADLINE)
    HIST_ENTRY **hl = NULL;
    size_t i = 0;

    if (!(hl = history_list()))
        return;

    for (i = 0; hl[i]; i++)
        printf("%s\n", hl[i]->line);
#else
    char **s = NULL;

    for (s = hist_list; *s; s++)
        printf("%s\n", *s);
#endif
}

/**
 * save_history:
 *      Function wrapper around write_history.
 */
void save_history(void)
{
    write_history(histfilename());
}

/**
 * free_history_list:
 *      Free the input history.
 */
void free_history_list(void)
{
#if !defined(HAVE_EDITLINE_READLINE_H) && !defined(HAVE_LIBREADLINE)
    char **i = NULL;

    if (!hist_list || !*hist_list)
        return;

    for (i = hist_list; *i; i++)
        free(*i);

    free(hist_list);
    hist_list = NULL;
#endif
}

// Readline functionality reimplementations
#if !defined(HAVE_EDITLINE_READLINE_H) && !defined(HAVE_LIBREADLINE)
/**
 * grow_hist_list:
 *      Grow size of the history table by N elements.
 */
static size_t grow_hist_list(size_t N)
{
    HIST_LIST_SIZE += N;

    if ((hist_list = realloc(hist_list,
                             HIST_LIST_SIZE * sizeof(char *))) == NULL) {
        error_return("lusush: history.c: grow_hist_list: realloc");
        return 0;
    }

    vputs("*** GREW HISTORY TO %u\n", HIST_LIST_SIZE);
    return HIST_LIST_SIZE;
}

/**
 * read_history:
 *      Read stored commands from the history file.
 */
static int read_history(const char *fn)
{
    size_t i;                        // loop counter
    FILE *fp = NULL;                 // file stream pointer

    vputs("Reading history %s\n", fn);

    // Open the history file for reading
    if ((fp = fopen(fn, "r")) == NULL) {
        if (opt_is_set(VERBOSE_PRINT))
            error_return("lusush: history.c: read_histfile: fopen");
        return 1;
    }

    vputs("File %s is open with descriptor %u\n", fn, fp);

    // Read the history file one line at a time
    for (i = 0; i < HIST_LIST_SIZE; i++) {
        // Make sure the table is not full, if it is then enlarge it
        if (i == HIST_LIST_SIZE - 1)
            if (!grow_hist_list(50))
                return 1;

        if ((hist_list[i] = calloc(MAXLINE + 1, sizeof(char))) == NULL) {
            error_return("lusush: history.c: read_history: calloc");
            return 1;
        }

        if (fgets(hist_list[i], MAXLINE, fp) == NULL)
            break;
        else
            if (hist_list[i][strnlen(hist_list[i], MAXLINE) - 1] == '\n')
                hist_list[i][strnlen(hist_list[i], MAXLINE) - 1] = '\0';

        // Check the stream for errors
        if (ferror(fp)) {
            error_return("lusush: history.c: read_history");
            return 1;
        }
    }

    hist_size = i + 1;          // set the history count
    fclose(fp);                 // close the file stream

    return 0;
}

/**
 * add_history:
 *      Add a line of history to hist_list.
 */
void add_history(const char *line)
{
    if (!hist_list || !line || !*line)
        return;

    // Table limit has been reached, attempt to grow the array
    if (hist_size == HIST_LIST_SIZE)
        if (!grow_hist_list(50))
            return;

    // Allocate next input history
    if ((hist_list[hist_size] = calloc(MAXLINE + 1, sizeof(char))) == NULL) {
        error_return("lusush: history.c: add_history: calloc");
        return;
    }

    // Save the line to the history list
    strncpy(hist_list[hist_size], line, MAXLINE);
    hist_size++;
}

/**
 * write_history:
 *      Write command history to a file.
 */
static void write_history(const char *fn)
{
    char **s;                   // iterator
    FILE *fp = NULL;            // file stream pointer

    if (!hist_list || !*hist_list)
        return;

    // Open the history file for writing
    if ((fp = fopen(fn, "w")) == NULL) {
        if (opt_is_set(VERBOSE_PRINT))
            error_return("lusush: history.c: write_history: fopen");
        return;
    }

    // Write each history item as a new line
    for (s = hist_list; *s; s++)
        fprintf(fp, "%s\n", *s);

    // Close the file stream
    fclose(fp);
}
#endif
