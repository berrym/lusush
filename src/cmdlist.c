/**
 * cmdlist.c - routines to work with doubly linked list
 */

// include statements {{{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ldefs.h"
#include "ltypes.h"
#include "cmdlist.h"

// end of inclkude statements }}}

// DOUBLY LINKED LIST {{{

// function cmdalloc {{{

/**
 * cmdalloc:
 *      alloc's room on the heap for a struct _CMD pointed to by cmd, and
 *      alloc's room for the pointer to pointer char argv in a struct _CMD.
 */
int cmdalloc(CMD *cmd)
{
    // Allocate the next node
    if ((cmd->next = (CMD *)calloc(1, sizeof(CMD))) == (CMD *)0) {
        perror("lusush: calloc");
        return -1;
    }

    // Allocate room for the first string on the heap.
    if ((cmd->argv[0] = (char *)calloc(MAXLINE, sizeof(char))) == (char *)0) {
        perror("lusush: calloc");
        return -1;
    }
    else {
       cmd->argv[0][0] = '\0';  // initialize with null character
    }

    // Make sure everything is zero/null
    cmd->hist_offset = 0;
    cmd->argc = 0;
    cmd->fd[0] = cmd->fd[1] = 0;
    cmd->pipe = cmd->pchain_master = false;
    cmd->background = 0;
    cmd->in_redirect = cmd->out_redirect = cmd->oredir_append = false;
    *cmd->in_filename = *cmd->out_filename = '\0';

    return 0;
}

// end of cmdalloc }}}

// function cmdfree {{{

/**
 * cmdfree:
 *      free's the memory pointed to by cmd, including recursive 
 *      free'ing of the strings in cmd->argv.
 */
void cmdfree(CMD *cmd)
{
    int i;

    if (cmd) {
        if (cmd->argv) {
            for (i = 0; cmd->argv[i]; i--) {
                free(cmd->argv[i]);
                cmd->argv[i] = (char *)0;
            }
        }

        strncpy(cmd->buf, "\0", 1);
        cmd->argc = 0;

        if (cmd->next)
            cmd->next->prev = cmd->prev;
        if (cmd->prev)
            cmd->prev->next = cmd->next;

        free(cmd);
        cmd = (CMD *)0;
    }
}

// end of cmdfree }}}

// function free_cmdlist {{{

/**
 * free_cmdlist
 *      recursively free nodes in doublt linked list
 */
void free_cmdlist(CMDLIST *cmdl)
{
    CMD *cmd, *tmp;
    cmd = &cmdl->head;

    while (cmd) {
        cmd = cmd->next;
    }

    while (cmd) {
        tmp = cmd->prev;
        cmdfree(cmd);
        cmd = tmp;
    }
}

// end of free_cmdlist }}}

// function display_cmdlist {{{

/**
 * display_cmdlist
 *      display the details of each node in the list by calling display_cmd
 */
void display_cmdlist(CMDLIST *cmdl)
{
    CMD *cmd = &cmdl->head;

    printf("cmdl->size->[%4d]\n", cmdl->size);
    while (cmd && cmd->argc) {
        display_cmd(cmd);
        cmd = cmd->next;
    }
}

// end of display_cmdlist }}}

// function display_cmd {{{

/**
 * display_cmd: display details of a CMD
 */
void display_cmd(CMD *cmd)
{
    register int i;

    printf("Processed Command:\n");
    printf("\thist_offset->%d\n", cmd->hist_offset);
    printf("\targc->%d\n", cmd->argc);
    for (i=0; i < cmd->argc; i++) {
        printf("\targv->[%4d]->%s\n", i, cmd->argv[i]);
    }
    printf("\tbuf->%s\n", cmd->buf);
    printf("\ttimestamp->%s", cmd->timestamp);
    printf("\tpipe->%s\n", cmd->pipe ? "true" : "false");
    printf("\tpchain_master->%s\n", cmd->pchain_master
            ? "true" : "false");
    printf("\tbackground->%s\n", cmd->background
            ? "true" : "false");
    printf("\tin_redirect->%s\n", cmd->in_redirect
            ? "true" : "false");
    printf("\tout_redirect->%s\n", cmd->out_redirect
            ? "true" : "false");
    printf("\toredir_append->%s\n", cmd->oredir_append
            ? "true" : "false");
    printf("\tin_filename->%s\n", cmd->in_filename
            ? cmd->in_filename : "empty");
    printf("\tout_filename->%s\n", cmd->out_filename
            ? cmd->out_filename : "empty");
}

// end of display_cmd }}}

// function timestamp_cmd {{{

/**
 * timestamp_cmd
 *      create a timestamp and put it in a command
 */
void timestamp_cmd(CMD *cmd)
{
    time_t rawtime;
    struct tm *timeinfo;
    char *thetime;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    thetime = asctime(timeinfo);
    strncpy(cmd->timestamp, thetime, strlen(thetime));
}

// end of timestamp_cmd }}}

// end of DOUBLY LINKE LIST }}}

// vim:filetype=c foldmethod=marker autoindent expandtab shiftwidth=4
