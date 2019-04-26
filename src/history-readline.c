/**
 * history-readline.c - input history usnig GNU readline
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "lusush.h"
#include "errors.h"
#include "history.h"
#include "opts.h"

char *hist_list = NULL;

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
    using_history();

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
    size_t i = 0;
    HIST_ENTRY *h = NULL;

    for (i = 0; (h = history_get(i + history_base)); i++)
        printf("%5zu\t%s\n", i + 1, h->line);
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
    // place holder
}
