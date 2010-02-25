/*
 * cmdlist.c - routines to work with doubly linked list
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ldefs.h"
#include "ltypes.h"
#include "cmdlist.h"

/*
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
    if ((*cmd->argv = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: calloc");
        return -1;
    }
    else {
       **cmd->argv = '\0';  // initialize with null character
    }

    // Make sure everything is zero/null
    cmd->argc = 0;
    cmd->fd[0] = cmd->fd[1] = 0;
    cmd->pipe = cmd->pchain_master = false;
    cmd->background = false;
    cmd->in_redirect = cmd->out_redirect = cmd->oredir_append = false;
    *cmd->buf = '\0';
    *cmd->in_filename = *cmd->out_filename = '\0';

    return 0;
}

/*
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

/*
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

/*
 * display_cmd: display details of a CMD
 */
void display_cmd(CMD *cmd)
{
    int i;

    printf("Processed Command:\n");
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

/*
 * timestamp_cmd
 *      create a timestamp and put it in a command
 */
void timestamp_cmd(CMD *cmd)
{
    time_t rawtime;
    struct tm *timeinfo = NULL;
    char *thetime = NULL;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    thetime = asctime(timeinfo);
    strncpy(cmd->timestamp, thetime, strlen(thetime));
}
