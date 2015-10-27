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

#include "builtins.h"
#include "alias.h"
#include "history.h"
#include "opts.h"
#include "prompt.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Built in command functions
static int bin_exit(struct command *);
static int bin_help(struct command *);
static int bin_cd(struct command *);
static int bin_pwd(struct command *);
static int bin_history(struct command *);
static int bin_setenv(struct command *);
static int bin_unsetenv(struct command *);
static int bin_alias(struct command *);
static int bin_unalias(struct command *);
static int bin_setopt(struct command *);
static int bin_unsetopt(struct command *);
static int bin_setprompt(struct command *);

// Built in commands struct table
static struct builtin builtins[] = {
    { "exit",      "exit program",               bin_exit      },
    { "help",      "display help",               bin_help      },
    { "cd",        "change directory",           bin_cd        },
    { "pwd",       "print working directory",    bin_pwd       },
    { "history",   "print command history",      bin_history   },
    { "setenv",    "set environment variable",   bin_setenv    },
    { "unsetenv",  "unset environment variable", bin_unsetenv  },
    { "alias",     "set an alias",               bin_alias     },
    { "unalias",   "unset an alias",             bin_unalias   },
    { "setopt",    "turn on a shell option",     bin_setopt    },
    { "unsetopt",  "turn off a shell option",    bin_unsetopt  },
    { "setprompt", "set prompt attributes",      bin_setprompt },
    { NULL,        NULL,                         NULL          }
};

/**
 * bin_exit:
 *      Exit the current shell instance.
 */
static int bin_exit(struct command *cmd)
{
    printf("Goodbye!\n");
    free((void *)cmd);
    cmd = NULL;
    exit(EXIT_SUCCESS);
    return 377;                   // should never happen
}

/**
 * bin_help:
 *      Display the list of builtin commands, each with a brief description.
 */
static int bin_help(struct command *cmd)
{
    size_t i;
    struct builtin *bin = NULL;

    if (!(cmd->argc == 2) || !(bin = find_builtin(cmd->argv[1])))
        for (i = 0; builtins[i].name; i++)
            printf("\t%-10s%-40s\n", builtins[i].name, builtins[i].doc);
    else
        printf("\t%-10s%-40s\n", bin->name, bin->doc);

    if (bin)
        free(bin);

    bin = NULL;

    return 0;
}

/**
 * bin_cd:
 *      Change working directory.
 */
static int bin_cd(struct command *cmd)
{
    if (chdir(cmd->argv[1]) < 0) {
        error_return("lusush: builtins.c: cd: chdir");
        return 1;
    }

    return 0;
}

/**
 * bin_pwd:
 *      Print working directory.
 */
static int bin_pwd(struct command *ignore)
{
    char cwd[MAXLINE] = { '\0' };

    if (getcwd(cwd, MAXLINE - 1) == NULL) {
        error_return("lusush: builtins.c: pwd: getcwd");
        return 1;
    }

    printf("%s\n", cwd);

    return 0;
}

/**
 * bin_history:
 *      Print the local command history.
 */
static int bin_history(struct command *ignore)
{
    print_history();
    return 0;
}

/**
 * bin_setenv:
 *      Set an environment variable.
 */
static int bin_setenv(struct command *cmd)
{
    if (cmd->argc != 3) {
        printf("usage: setenv variable value\n");
        return 1;
    }

    if (setenv(cmd->argv[1], cmd->argv[2], 1) < 0) {
        error_return("lusush: exec.c: exec_builtin_cmd: setenv");
        return 1;
    }

    return 0;
}

/**
 * bin_unsetenv:
 *    Unset an environment variable.
 */
static int bin_unsetenv(struct command *cmd)
{
    if (cmd->argc != 2) {
        printf("usage: unsetenv variable\n");
        return 1;
    }

    if (unsetenv(cmd->argv[1]) < 0) {
        error_return("lusush: exec.c: exec_builtin_cmd: unsetenv");
        return 1;
    }

    return 0;
}

/**
 * bin_alias:
 *      Set an alias.
 */
static int bin_alias(struct command *cmd)
{
    size_t i;
    char tmp[MAXLINE] = { '\0' };

    switch (cmd->argc) {
    case 1:
        print_alias_list();
        break;
    case 2:
        printf("usage: alias word replacement text\n");
        return 1;
        break;
    default:
        for (i = 2; cmd->argv[i]; i++)
            snprintf(tmp + strlen(tmp),
                     MAXLINE - strlen(tmp) - 1,
                     "%s ",
                     cmd->argv[i]);

        tmp[strlen(tmp) - 1] = '\0'; // replace trailing space with null
        set_alias(cmd->argv[1], tmp);
        break;
    }

    return 0;
}

/**
 * bin_unalias:
 *      Unset an alias.
 */
static int bin_unalias(struct command *cmd)
{
    if (cmd->argc != 2) {
        printf("usage: unalias alias\n");
        return 1;
    }

    unset_alias(cmd->argv[1]);

    return 0;
}

/**
 * bin_setopt:
 *      Turn on a shell option.
 */
static int bin_setopt(struct command *cmd)
{
    if (cmd->argc != 2) {
        printf("usage: setopt option\n");
        return 1;
    }

    if (strcmp(cmd->argv[1], "VERBOSE_PRINT") == 0)
        set_bool_opt(VERBOSE_PRINT, true);
    else if (strcmp(cmd->argv[1], "FANCY_PROMPT") == 0)
        set_bool_opt(FANCY_PROMPT, true);
    else
        error_message("setopt: Unknown option %s\n", cmd->argv[1]);

    return 0;
}

/**
 * bin_unsetopt:
 *      Turn off a shell option.
 */
static int bin_unsetopt(struct command *cmd)
{
    if (cmd->argc != 2) {
        printf("usage: unsetopt option\n");
        return 1;
    }

    if (strcmp(cmd->argv[1], "VERBOSE_PRINT") == 0)
        set_bool_opt(VERBOSE_PRINT, false);
    else if (strcmp(cmd->argv[1], "FANCY_PROMPT") == 0)
        set_bool_opt(FANCY_PROMPT, false);
    else
        error_message("setopt: Unknown option %s\n", cmd->argv[1]);

    return 0;
}

/**
 * bin_setprompt:
 *      Set prompt attributes.
 */
static int bin_setprompt(struct command *cmd)
{
    set_prompt(cmd->argc, cmd->argv);
    return 0;
}

/**
 * find_builtin:
 *      Return a pointer to a struct builtin.
 */
struct builtin *find_builtin(const char *name)
{
    size_t i;
    struct builtin *bin = NULL;

    for (i = 0; builtins[i].name; i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            if ((bin = calloc(1, sizeof(builtins[i]))) == NULL) {
                error_return("lusush: builtins.c: find_builtin: calloc");
                return NULL;
            }
            return memcpy(bin, &builtins[i], sizeof(builtins[i]));
        }
    }

    return NULL;
}
