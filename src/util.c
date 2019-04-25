/**
 * util.c - various utility functions that didn't fit anywhere else.
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "errors.h"
#include "opts.h"

#ifndef HAVE_STRNLEN
/**
 * strnlen:
 *      Count the number of characters in a string up to maxlen.
 */
size_t strnlen(const char *s, size_t maxlen)
{
    size_t len;

    for (len = 0; len < maxlen; len++, s++)
        if (!*s)
            break;

    return len;
}
#endif

/**
 * vputs:
 *      Print formatted string if VERBOSE_PRINT option is set.
 */
void vputs(const char *fmt, ...)
{
    if (opt_is_set(VERBOSE_PRINT)) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

/**
 * close_std_ttys:
 *      Close the standard input, output, and error.
 */
void close_std_ttys(void)
{
    if (isatty(STDIN_FILENO))
        if (close(STDIN_FILENO) < 0)
            error_return("close_std_ttys: close(STDIN_FILENO)");

    if (isatty(STDERR_FILENO))
        if (close(STDERR_FILENO) < 0)
            error_return("close_std_ttys: close(STDERR_FILENO)");

    if (isatty(STDOUT_FILENO))
        if (close(STDOUT_FILENO) < 0)
            error_return("close_std_ttys: close(STDOUT_FILENO)");
}