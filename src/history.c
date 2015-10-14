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

#define MAXHIST 10000           // maximum length of history

static char histfile[MAXLINE] = { '\0' };
static bool HIST_INITIALIZED = false;
#ifdef HAVE_LIBREADLINE
static HIST_ENTRY **hist_list = NULL;
#else
static FILE *histfp = NULL;
static long hist_size = 0;
static char hist_list[MAXHIST][MAXLINE] = { "\0" };
#endif

/**
 * init_history:
 *      Create or read the history file.
 */
void init_history(void)
{
    char *ENV_HOME = NULL;

    if (HIST_INITIALIZED) {
        error_message("lusush: init_history: already initialized.\n");
        return;
    }

#ifdef HAVE_LIBREADLINE
    using_history();
    stifle_history(MAXHIST);
#endif

    ENV_HOME = getenv("HOME");

    if (!*histfile)
        snprintf(histfile, MAXLINE, "%s/.lusushist", ENV_HOME);

    if (read_history(histfile) != 0)
        return;

    HIST_INITIALIZED = true;
}

#ifndef HAVE_LIBREADLINE
/**
 * read_history:
 *      Read stored commands from the history file.
 */
int read_history(const char *histfile)
{
    size_t i;

    if ((histfp = fopen(histfile, "r")) == NULL) {
        if (opt_is_set(VERBOSE_PRINT))
            error_return("lusush: history.c: read_histfile: fopen");
        return;
    }

    for (i = 0; i < MAXHIST && hist_list[i]; i++) {
        if (fgets(hist_list[i], MAXLINE, histfp) == NULL)
            break;

        if (hist_list[i][strlen(hist_list[i]) - 1] == '\n')
            hist_list[i][strlen(hist_list[i]) - 1] = '\0';
    }

    fclose(histfp);

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

    if (hist_size < MAXHIST) {
        strncpy(hist_list[hist_size], line, MAXLINE);
        hist_size++;
    }
}

/**
 * write_history:
 *      Write command history to a file.
 */
void write_history(const char *fn)
{
    size_t i;

    if (fn == NULL)
        fn = histfilename();

    if (!HIST_INITIALIZED)
        return;

    if ((histfp = fopen(fn, "w")) == NULL) {
        if (opt_is_set(VERBOSE_PRINT))
            error_return("lusush: history.c: write_history: fopen");
        return;
    }

    for (i = 0; hist_list[i] && *hist_list[i]; i++)
        fprintf(histfp, "%s\n", hist_list[i]);

    fclose(histfp);
}
#endif

/**
 * histfilename:
 *      Return the name of the history file.
 */
const char *histfilename(void)
{
    return histfile;
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
    for (i = 0; i < MAXHIST && *hist_list[i]; i++)
        printf("%zu:\t%s\n", i + 1, hist_list[i]);
#endif
}
