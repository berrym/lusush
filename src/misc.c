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

#include "misc.h"
#include "opts.h"
#include <stdio.h>
#include <stdarg.h>

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
#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n)
{
    char *result = NULL;
    size_t len = strnlen(s, MAXLINE);

    if (n < len)
        len = n;

    if ((result = calloc(len + 1, sizeof(char))) == NULL) {
        perror("lusush: misc.c: strndup: calloc");
        return NULL;
    }

    result[len] = '\0';

    return (char *)memcpy(result, s, len);
}
#endif
