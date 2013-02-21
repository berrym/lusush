#include "config.h"

#ifndef __LUSUSH_H__
#define __LUSUSH_H__

#ifdef MAXLINE
#undef MAXLINE
#endif
#define MAXLINE 4096        // maximum string length
#define MAXHIST 10000       // maximum length of history

#ifdef HAVE__BOOL
#include <stdbool.h>
#else
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
    bool pipe_head;             // head node in pipe chain
    int fd[2];                  // pipe filedescriptors
    bool iredir;                // input redirection flag
    bool oredir;                // output redirection flag
    bool oredir_append;         // output redirection opens file to append
    char ifname[MAXLINE];       // input filename - if redirect set
    char ofname[MAXLINE];       // output filename - if redirect set
    struct _CMD *next;          // next command
    struct _CMD *prev;          // previous command
} CMD;

#endif
