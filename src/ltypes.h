#include "ldefs.h"
#include "config.h"

#ifdef HAVE__BOOL
#include <stdbool.h>
#endif

#ifndef __LTYPES_H__
#define __LTYPES_H__

/**
 * enum bool
 */
#ifndef HAVE__BOOL
typedef enum { false = 0, true = 1 } bool;
#endif

/**
 * Doubly linked list of commands to pass to exec
 */
typedef struct _CMD
{
    char buf[MAXLINE];          // null terminated user input
    char timestamp[60];         // timestamp
    int argc;                   // argument count
    char **argv;                // arguments to pass to exec
    bool background;            // background process flag
    bool pipe;                  // pipe flag
    bool pchain_master;         // parent command in pipe chain
    int fd[2];                  // pipe filedescriptors
    bool iredir;           // input redirection flag
    bool oredir;          // output redirection flag
    bool oredir_append;         // output redirection opens file to append
    char ifname[MAXLINE];  // input filename - if redirect set
    char ofname[MAXLINE]; // output filename - if redirect set
    struct _CMD *next;          // next command
    struct _CMD *prev;          // previous command
} CMD;

#endif
