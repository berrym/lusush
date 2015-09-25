/**
 * misc.c - various functions that didn't fit anywhere else.
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

#include "opts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

/**
 * do_error:
 *      Print an error message and return to caller.
 */
static void do_error(int errnoflag, int err, const char *fmt, va_list args)
{
    char buf[MAXLINE] = { '\0' };

    vsnprintf(buf, MAXLINE - 1, fmt, args);
    if (errnoflag)
        snprintf(buf + strnlen(buf, MAXLINE),
                 MAXLINE - strnlen(buf, MAXLINE) - 1, ": %s", strerror(err));

    strncat(buf, "\n", 2);
    fflush(stdout);             // in case stdout and stdin are the same
    fputs(buf, stderr);
    fflush(NULL);               // flush all stdio output streams
}

/**
 * error_return:
 *      Nonfatal error related to a system call.
 *      Print and error message and return.
 */
void error_return(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(1, errno, fmt, args);
    va_end(args);
}

/**
 * error_syscall:
 *      Fatal error message related to a system call.
 *      Print and error message and terminate.
 */
void error_syscall(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(1, errno, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

/**
 * error_message:
 *      Nonfatal error unrelated to a system call.
 *      Print an error message and return.
 */
void error_message(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(0, 0, fmt, args);
    va_end(args);
}

/**
 * error_quit:
 *      Fatal error unrelated to a system call.
 *      Print an error message and return.
 */
void error_quit(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(0, 0, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

/**
 * error_coredump:
 *      Fatal error related to a system call.
 *      Print an error message, dump core, and terminate.
 */
void error_coredump(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(0, 0, fmt, args);
    va_end(args);
    abort();                    // dump core and terminate
    exit(EXIT_FAILURE);         // should never happen
}

#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t maxlen)
{
    size_t len;

    for (len = 0; len < maxlen; len++, s++)
        if (!*s)
            break;

    return len;
}
#endif

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n)
{
    char *result = NULL;
    size_t len = strnlen(s, MAXLINE);

    if (n < len)
        len = n;

    if ((result = calloc(len + 1, sizeof(char))) == NULL)
        error_syscall("lusush: misc.c: strndup: calloc");

    result[len] = '\0';

    return memcpy(result, s, len);
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
