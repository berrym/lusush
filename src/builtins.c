/**
 * builtins.c - builtin commands
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
#include "builtins.h"
#include "misc.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * Builtin commands:
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

/* typedef struct { */
/*     char *name; */
/*     char *doc; */
/* } BUILTIN; */

/* const BUILTIN builtins[] = { */
/*     { "exit",         "exit program"                }, */
/*     { "help",         "display basic help"          }, */
/*     { "cd",           "change directory"            }, */
/*     { "pwd",          "print working directory"     }, */
/*     { "history",      "print command history"       }, */
/*     { "setenv",       "set environment variable"    }, */
/*     { "unsetenv",     "delete environment variable" }, */
/*     { "alias",        "set an alias"                }, */
/*     { "unalias",      "unset an alias"              }, */
/*     { "setopt",       "turn on an option"           }, */
/*     { "unsetopt",     "turn off an option"          }, */
/*     { "setprompt",    "set prompt colors"           } */
/* }; */

static const char *builtins[BUILTIN_CMD_CNT] =
{
    "exit",         "exit program",
    "help",         "display basic help",
    "cd",           "change directory",
    "pwd",          "print working directory",
    "history",      "print command history",
    "setenv",       "set environment variable",
    "unsetenv",     "delete environment variable",
    "alias",        "set an alias",
    "unalias",      "unset an alias",
    "setopt",       "turn on an option",
    "unsetopt",     "turn off an option",
    "setprompt",    "set prompt colors"
};

/**
 * is_builtin_cmd:
 *      Compare (cmdname) to elements in array of strings
 *      builtins, if it matches return the index of the element.
 */
int is_builtin_cmd(const char *cmdname)
{
    unsigned i;

    if (!cmdname || !*cmdname)
        return -1;

    for (i = 0; i < BUILTIN_CMD_CNT; i += 2)
        if (strncmp(cmdname, builtins[i], strnlen(cmdname, MAXLINE)) == 0)
            return i;

    return -1;
}

/**
 * help:
 *      Display the list of builtin commands, each  with a brief description.
 */
void help(const char *cmdname)
{
    int i;

    if (cmdname == NULL) {
        printf("Builtin commands:\n");
        for (i = 0; i < BUILTIN_CMD_CNT; i += 2)
            printf("\t%-10s%-40s\n", builtins[i], builtins[i+1]);
    }
    else {
        if ((i = is_builtin_cmd(cmdname)) != -1)
            printf("\t%-10s%-40s\n", builtins[i], builtins[i+1]);
    }
}

/**
 * cd:
 *      Change working directory.
 */
void cd(const char *path)
{
    if (chdir(path) < 0)
        perror("lusush: builtins.c: cd: chdir");
}

/**
 * pwd:
 *      Print working directory.
 */
void pwd(void)
{
    char cwd[MAXLINE] = { '\0' };

    if (getcwd(cwd, MAXLINE) == NULL)
        perror("lusush: builtins.c: pwd: getcwd");
    else
        printf("%s\n", cwd);
}
