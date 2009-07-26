#include "ldefs.h"

#ifndef __LTYPES_H__
#define __LTYPES_H__

///////////////////////// TYPES /////////////////////////////////////////////

/**
 * enum bool
 */
#ifndef bool
typedef enum { false = 0, true = 1 } bool;
#endif

/**
 * struct _CMD:
 *      a doubly linked list of commands to pass to exec
 */
struct _CMD
{
    char buf[MAXLINE];          // null terminated user input
    int argc;                   // argument count
    char timestamp[60];         // timestamp
    bool background;            // background process flag
    bool in_redirect;           // input redirection flag
    bool out_redirect;          // output redirection flag
    char in_filename[MAXLINE];  // input filename - if redirect set
    char out_filename[MAXLINE]; // output filename - if redirect set
    struct _CMD *next;          // next command
    struct _CMD *prev;          // previous command
    char *argv[1024];           // argument vector to pass to exec
};
typedef struct _CMD CMD;

/**
 * struct _CMDLIST:
 *      container object for struct _CMD
 */
struct _CMDLIST
{
    int size;
    CMD *head;
};
typedef struct _CMDLIST CMDLIST;

#endif
