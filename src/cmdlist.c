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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "cmdlist.h"

static struct command *head = NULL; // head node in command list

/**
 * create_command_list:
 *      Initialize a struct command list.
 */
struct command *create_command_list(void)
{
    if (head)
        return head;

    if ((head = alloc_command()) == NULL) {
        error_return("lusush: alloc_command error");
        return NULL;
    }

    vputs("create_command_list: success\n");

    return head;
}

/**
 * alloc_command:
 *      Allocate memory for a struct command.
 */
struct command *alloc_command(void)
{
    struct command *cmd = NULL; // pointer to new struct command

    // Allocate struct command
    if ((cmd = calloc(1, sizeof(struct command))) == NULL) {
        error_return("lusush: alloc_command error");
        return NULL;
    }

    // Allocate pointer to pointer char
    if ((cmd->argv = calloc(128, sizeof(char *))) == NULL) {
        free(cmd);
        cmd = NULL;
        error_return("lusush: alloc_command cmd->argv error");
        return NULL;
    }

    // Allocate room for the first string on the heap
    if ((*cmd->argv = calloc(MAXLINE + 1, sizeof(char))) == NULL) {
        free(cmd->argv);
        cmd->argv = NULL;
        free(cmd);
        cmd = NULL;
        error_return("lusush: alloc_command error on *cmd->argv");
        return NULL;
    }

    // Make sure everything else is zero/null
    cmd->argc = cmd->pfd[0] = cmd->pfd[1] = -1;
    cmd->pipe = cmd->pipe_head = false;
    cmd->iredir = cmd->oredir = cmd->oredir_append = cmd->background = false;
    *cmd->ifname = *cmd->ofname = '\0';
    cmd->ifd = cmd->ofd = -1;
    cmd->prev = cmd->next = NULL;

    vputs("successful alloc_command call\n");

    return cmd;
}

/**
 * free_command:
 *      Frees the memory pointed to by cmd, including recursive
 *      freeing of the strings in cmd->argv.
 */
static void free_command(struct command *cmd)
{
    char **s = NULL;

    if (cmd == NULL || cmd->argv == NULL)
        return;

    // Free each argument string
    for (s = cmd->argv; *s; s++)
        free(*s);

    // Free argument vector
    free(cmd->argv);
    cmd->argv = NULL;

    // Close open pipes
    if (cmd->pfd[0] >= 0)
        if (close(cmd->pfd[0]) < 0)
            error_return("close");

    if (cmd->pfd[1] >= 0)
        if (close(cmd->pfd[1]) < 0)
            error_return("close");

    // Fix links
    if (cmd->next)
        cmd->next->prev = cmd->prev;

    if (cmd->prev)
        cmd->prev->next = cmd->next;

    // Free command
    free(cmd);
    cmd = NULL;
}

/**
 * free_command_list:
 *      Free nodes in a doubly linked list of struct commands.
 */
void free_command_list(void)
{
    struct command *curr = NULL;

    // Free each node in the list
    while ((curr = head) != NULL) {
        head = head->next;
        free_command(curr);
    }
}

/**
 * display_command:
 *      Display details of a struct command.
 */
void display_command(struct command *cmd)
{
    size_t i;

    if (cmd == NULL) {
        error_message("lusush: "
                      "no access to struct command, cannot display.\n");
        return;
    }

    printf("Processed Command:\n");
    printf("\targc->%d\n", cmd->argc);
    for (i = 0; i < cmd->argc; i++)
        printf("\targv->[%4zu]->%s\n", i, cmd->argv[i]);
    printf("\tpipe->%s\n", cmd->pipe ? "true" : "false");
    printf("\tpipe_head->%s\n", cmd->pipe_head ? "true" : "false");
    printf("\tbackground->%s\n", cmd->background ? "true" : "false");
    printf("\tiredir->%s\n", cmd->iredir ? "true" : "false");
    printf("\toredir->%s\n", cmd->oredir ? "true" : "false");
    printf("\toredir_append->%s\n", cmd->oredir_append ? "true" : "false");
    if (strnlen(cmd->ifname, MAXLINE) > 0)
        printf("\tifname->%s\n", cmd->ifname);
    else
        printf("\tifname->empty\n");
    if (strnlen(cmd->ofname, MAXLINE) > 0)
        printf("\tofname->%s\n", cmd->ofname);
    else
        printf("\tofname->empty\n");
}
