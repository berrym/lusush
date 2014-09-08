/**
 * cmdlist.c - routines to work with doubly linked list
 *
 * Copyright (c) 2009-2014 Michael Berry <trismegustis@gmail.com>
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
 * cmdalloc:
 *      allocs room on the heap for a struct _CMD pointed to by cmd, and
 *      allocs room for the pointer to pointer char argv in a struct _CMD.
 */
int cmdalloc(CMD *cmd)
{
    // Allocate the next node
    if ((cmd->next = calloc(1, sizeof(CMD))) == NULL) {
        perror("lusush: calloc");
        return -1;
    }

    // Allocate pointer to pointer char
    if ((cmd->argv = calloc(1024, sizeof(char *))) == NULL) {
        perror("lusush: calloc");
        return -1;
    }

    *cmd->argv = NULL;

    // Allocate room for the first string on the heap
    if ((*cmd->argv = calloc(BUFSIZE, sizeof(char))) == NULL) {
        perror("lusush: calloc");
        return -1;
    }
    else {
       **cmd->argv = '\0';  // initialize with null character
    }

    // Make sure everything is zero/null
    cmd->argc = 0;
    cmd->fd[0] = cmd->fd[1] = 0;
    cmd->pipe = cmd->pipe_head = false;
    cmd->background = false;
    cmd->iredir = cmd->oredir = cmd->oredir_append = false;
    *cmd->buf = '\0';
    *cmd->ifname = *cmd->ofname = '\0';

    return 0;
}

/**
 * cmdfree:
 *      frees the memory pointed to by cmd, including recursive 
 *      freeing of the strings in cmd->argv.
 */
void cmdfree(CMD *cmd)
{
    int i;

    if (cmd) {
        if (cmd->argv) {
            for (i = 0; cmd->argv[i]; i++) {
                free(cmd->argv[i]);
                cmd->argv[i] = NULL;
            }
            free(cmd->argv);
        }

        strncpy(cmd->buf, "\0", 1);
        cmd->argc = 0;

        if (cmd->next)
            cmd->next->prev = cmd->prev;

        if (cmd->prev)
            cmd->prev->next = cmd->next;

        free(cmd);
        cmd = NULL;
    }
}

/**
 * free_cmdlist
 *      recursively free nodes in doubly linked list
 */
void free_cmdlist(CMD *cmd)
{
    CMD *tmp = NULL;
    
    while (cmd->next) {
        cmd = cmd->next;
    }

    while (cmd) {
        if (cmd->prev)
            tmp = cmd->prev;
        else
            tmp = NULL;

        cmdfree(cmd);

        if (tmp)
            cmd = tmp;
        else
            cmd = NULL;
    }

}

/**
 * display_cmd:
 *      display details of a CMD
 */
void display_cmd(CMD *cmd)
{
    int i = 0;

    printf("Processed Command:\n");
    printf("\targc->%d\n", cmd->argc);
    for (i=0; i < cmd->argc; i++) {
        printf("\targv->[%4d]->%s\n", i, cmd->argv[i]);
    }
    printf("\tbuf->%s\n", cmd->buf);
    printf("\ttimestamp->%s", cmd->timestamp);
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
