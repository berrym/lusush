/**
 * cmdlist.c - routines to work with a doubly linked list of struct command.
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

#include "cmdlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * cmdfree:
 *      Frees the memory pointed to by cmd, including recursive
 *      freeing of the strings in cmd->argv.
 */
static void cmdfree(struct command *cmd)
{
    if (!cmd || !cmd->argv)
        return;

    for (; cmd->argc >= 0; cmd->argc--) {
        free(cmd->argv[cmd->argc]);
        cmd->argv[cmd->argc] = NULL;
    }

    free(cmd->argv);
    cmd->argv = NULL;

    if (cmd->next)
        cmd->next->prev = cmd->prev;

    if (cmd->prev)
        cmd->prev->next = cmd->next;

    free(cmd);
    cmd = NULL;
}

/**
 * TODO:
 *      This is a misleading name, and the the way command lists
 *      work is not a very intuitive approach, change the name
 *      or even better choose a different methodology for implementation.
 *
 * cmdalloc:
 *      The parameter cmd should be an already allocated struct,
 *      it will allocate the argument vector and it's first string,
 *      additional strings must be allocated as needed.  Allocates
 *      the next struct command in the list as well.
 */
int cmdalloc(struct command *cmd)
{
    // Allocate pointer to pointer char
    if ((cmd->argv = calloc(1024, sizeof(char *))) == NULL) {
        perror("lusush: cmdlist.c: cmdalloc: calloc");
        return -1;
    }
    *cmd->argv = NULL;

    // Allocate room for the first string on the heap
    if ((*cmd->argv = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: cmlist.c: cmdalloc: calloc");
        return -1;
    }
    **cmd->argv = '\0';         // initialize with null character

    // Make sure everything else is zero/null
    cmd->argc = 0;
    cmd->fd[0] = cmd->fd[1] = 0;
    cmd->pipe = cmd->pipe_head = false;
    cmd->iredir = false;
    cmd->oredir = cmd->oredir_append = false;
    cmd->background = false;
    *cmd->ifname = *cmd->ofname = '\0';

    // Allocate the next node
    if ((cmd->next = calloc(1, sizeof(struct command))) == NULL) {
        perror("lusush: cmdlist.c: cmdalloc: calloc");
        return -1;
    }

    return 0;
}

/**
 * free_cmdlist:
 *      Recursively free nodes in a doubly linked list of struct commands.
 */
void free_cmdlist(struct command *cmd)
{
    struct command *next = NULL;

    while (cmd) {
        next = cmd->next;
        cmdfree(cmd);
        if (next)
            cmd = next;
        else
            cmd = NULL;
    }
}

/**
 * display_cmd:
 *      Display details of a struct command.
 */
void display_cmd(struct command *cmd)
{
    size_t i;

    if (!cmd) {
        fprintf(stderr, "lusush: cmdlist.c: display_cmd: "
                "no access to cmd, cannot display.\n");
        return;
    }

    printf("Processed Command:\n");
    printf("\targc->%d\n", cmd->argc);
    for (i = 0; i < cmd->argc; i++)
        printf("\targv->[%4zu]->%s\n", i, cmd->argv[i]);
    printf("\tpipe->%s\n", cmd->pipe ? "true" : "false");
    printf("\tpipe_head->%s\n", cmd->pipe_head
            ? "true" : "false");
    printf("\tbackground->%s\n", cmd->background
            ? "true" : "false");
    printf("\tiredir->%s\n", cmd->iredir
            ? "true" : "false");
    printf("\toredir->%s\n", cmd->oredir
            ? "true" : "false");
    printf("\toredir_append->%s\n", cmd->oredir_append
            ? "true" : "false");
    printf("\tifname->%s\n", cmd->ifname
            ? cmd->ifname : "empty");
    printf("\tofname->%s\n", cmd->ofname
            ? cmd->ofname : "empty");
}
