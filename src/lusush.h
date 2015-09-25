/**
 * lusush.h
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

#ifndef LUSUSH_H
#define LUSUSH_H

#include "config.h"

#ifdef MAXLINE
#undef MAXLINE
#endif
#define MAXLINE 4096                // maximum string length

#define MAXHIST 10000               // maximum length of history

#ifdef HAVE__BOOL
#include <stdbool.h>
#else
typedef enum { false = 0, true = 1 } bool;
#endif

#ifndef size_t
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#elif defined(HAVE_STDDEF_H)
#include <stddef.h>
#else
typedef unsigned size_t;
#endif
#endif

/**
 * Doubly linked list of commands to pass to exec
 */
struct command
{
    int argc;                   // argument count
    char **argv;                // arguments to pass to exec
    bool background;            // background process flag
    bool pipe;                  // pipe flag
    bool pipe_head;             // head node in pipe chain flag
    int fd[2];                  // pipe filedescriptors
    bool iredir;                // input redirection flag
    bool oredir;                // output redirection flag
    bool oredir_append;         // output redirection opens file to append
    char ifname[MAXLINE];       // input filename - if redirect set
    char ofname[MAXLINE];       // output filename - if redirect set
    struct command *next;       // next command
    struct command *prev;       // previous command
};

extern void vputs(const char *, ...);
extern void error_return(const char *, ...);
extern void error_syscall(const char *, ...);
extern void error_message(const char *, ...);
extern void error_quit(const char *, ...);
extern void error_coredump(const char *, ...);

#ifndef HAVE_STRNLEN
extern size_t strnlen(const char *, size_t);
#endif

#ifndef HAVE_STRNDUP
extern char *strndup(const char *, size_t);
#endif

#endif
