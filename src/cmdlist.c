#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ldefs.h"
#include "ltypes.h"
#include "cmdlist.h"

///////////////////////// DOUBLY LINKED LIST ////////////////////////////////

/**
 * cmdalloc:
 *      alloc's room on the heap for a struct _CMD pointed to by cmd, and
 *      alloc's room for the pointer to pointer char argv in a struct _CMD.
 */
int cmdalloc(CMD *cmd)
{
    // Allocate the next node
    if ((cmd->next = (CMD *)calloc(1, sizeof(CMD))) == (CMD *)NULL) {
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
    cmd->hist_offset = cmd->argc = 0;
    cmd->fd[0] = cmd->fd[1] = 0;
    cmd->pipe = cmd->background = cmd->in_redirect = cmd->out_redirect = false;
    *cmd->in_filename = *cmd->out_filename = '\0';

    return 0;
}

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
                cmd->argv[i] = (char *)NULL;
            }
        }

        strcpy(cmd->buf, "\0");
        cmd->argc = 0;

        if (cmd->next)
            cmd->next->prev = cmd->prev;
        if (cmd->prev)
            cmd->prev->next = cmd->next;

        free(cmd);
        cmd = (CMD *)NULL;
    }
}

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

void display_cmdlist(CMDLIST *cmdl)
{
    CMD *cmd = &cmdl->head;

    printf("cmdl->size->[%4d]\n", cmdl->size);
    while (cmd && cmd->argc) {
        display_cmd(cmd);
        cmd = cmd->next;
    }
}

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
    printf("\tbackground->%s\n", cmd->background
            ? "true" : "false");
    printf("\tin_redirect->%s\n", cmd->in_redirect
            ? "true" : "false");
    printf("\tout_redirect->%s\n", cmd->out_redirect
            ? "true" : "false");
    printf("\tin_filename->%s\n", cmd->in_filename
            ? cmd->in_filename : "empty");
    printf("\tout_filename->%s\n", cmd->out_filename
            ? cmd->out_filename : "empty");
}

void timestamp_cmd(CMD *cmd)
{
    time_t rawtime;
    struct tm *timeinfo;
    char *thetime;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    thetime = asctime(timeinfo);
    strcpy(cmd->timestamp, thetime);
}
