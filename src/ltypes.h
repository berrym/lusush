#include "ldefs.h"
#include "../config.h"
#ifdef HAVE__BOOL
#include <stdbool.h>
#endif

#ifndef __LTYPES_H__
#define __LTYPES_H__

///////////////////////// TYPES /////////////////////////////////////////////

/**
 * enum bool
 */
#ifndef HAVE__BOOL
typedef enum { false = 0, true = 1 } bool;
#endif

/**
 * struct _CMD:
 *      a doubly linked list of commands to pass to exec
 */
struct _CMD
{
    char buf[MAXLINE];          // null terminated user input
    char timestamp[60];         // timestamp
    int argc;                   // argument count
    char *argv[1024];           // argument vector to pass to exec
    bool background;            // background process flag
    bool pipe;                  // pipe flag
    bool pchain_master;         // parent command in pipe chain
    int fd[2];                  // pipe filedescriptors
    bool in_redirect;           // input redirection flag
    bool out_redirect;          // output redirection flag
    bool oredir_append;         // output redirection opens file to append
    char in_filename[MAXLINE];  // input filename - if redirect set
    char out_filename[MAXLINE]; // output filename - if redirect set
    struct _CMD *next;          // next command
    struct _CMD *prev;          // previous command
};
typedef struct _CMD CMD;

/**
 * struct _CMDLIST:
 *      container object for struct _CMD
 */
struct _CMDLIST
{
    int size;
    CMD head;
};
typedef struct _CMDLIST CMDLIST;

#endif
