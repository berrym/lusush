/**
 * builtins.h
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

#include "lusush.h"

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

/**
 * Builtin commands
 *      exit        exit program
 *      help        print help
 *      cd          change directory
 *      pwd         print working directory
 *      history     print command history
 *      setenv      set environment variable
 *      unsetenv    unset environment variable
 *      alias       set an alias
 *      unalias     unset an alias
 *      setopt      turn on an option
 *      unsetopt    turn off an option
 *      setprompt   set prompt colors
 */

enum {
        BUILTIN_CMD_EXIT=0,
        BUILTIN_CMD_HELP=2,
        BUILTIN_CMD_CD=4,
        BUILTIN_CMD_PWD=6,
        BUILTIN_CMD_HISTORY=8,
        BUILTIN_CMD_SETENV=10,
        BUILTIN_CMD_UNSETENV=12,
        BUILTIN_CMD_ALIAS=14,
        BUILTIN_CMD_UNALIAS=16,
        BUILTIN_CMD_SETOPT=18,
        BUILTIN_CMD_UNSETOPT=20,
        BUILTIN_CMD_SETPROMPT=22,
        BUILTIN_CMD_CNT=24
};

extern const char *builtins[BUILTIN_CMD_CNT];

extern int is_builtin_cmd(const char *);
extern void help(const char *);
extern void cd(const char *);
extern void pwd(void);
extern void history(void);

#endif
