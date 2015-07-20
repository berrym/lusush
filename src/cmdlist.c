/**
 * cmdlist.c - routines to work with a doubly linked list of struct command.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lusush.h"
#include "cmdlist.h"

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
 * cmdalloc:
 *      Allocates memory a struct command pointed to by cmd, and then
 *      allocates the argument vector and it's first string.
 */
int cmdalloc(struct command *cmd)
{
    // Allocate the next node
    if ((cmd->next = calloc(1, sizeof(struct command))) == NULL) {
        perror("lusush: cmdlist.c: cmdalloc: calloc");
        return -1;
    }

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
    **cmd->argv = '\0';  // initialize with null character

    // Make sure everything is zero/null
    cmd->argc = 0;
    cmd->fd[0] = cmd->fd[1] = 0;
    cmd->pipe = cmd->pipe_head = false;
    cmd->background = false;
    cmd->iredir = cmd->oredir = cmd->oredir_append = false;
    *cmd->ifname = *cmd->ofname = '\0';

    return 0;
}

/**
 * free_cmdlist
 *      recursively free nodes in doubly linked list
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
 *      display details of a struct command
 */
void display_cmd(struct command *cmd)
{
    unsigned int i;

    printf("Processed Command:\n");
    printf("\targc->%d\n", cmd->argc);
    for (i = 0; i < cmd->argc; i++)
        printf("\targv->[%4d]->%s\n", i, cmd->argv[i]);
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
