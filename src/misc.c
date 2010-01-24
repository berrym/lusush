/*
 * misc.c - various functions that didn't fit anywhere else.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "ldefs.h"
#include "env.h"
#include "misc.h"

/*
 * print_debug:
 *
 *  Ifq MACRO PRINT_DEBUG is defined at compile time then this function
 *  will behave just like printf.  It is used to print degugging
 *  statements without having to go back through all of the code and
 *  comment out unwanted printf statements.
 */
void print_debug(const char *fmt, ...)
{
#if defined( PRINT_DEBUG )
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}

/*
 * build_prompt:
 *      Builds the user's prompt displaying the current working directory.
 */
void build_prompt(void)
{
    char *cwd = (char *)NULL;
    char ENV_PROMPT[MAXLINE];

    if ((cwd = getcwd((char *)NULL, 0)) == (char *)NULL) {
        perror("lusush: build_prompt");
        strcpy(ENV_PROMPT, "% ");
    }
    else {
        strncpy(ENV_PROMPT, cwd, MAXLINE);
        strcat(ENV_PROMPT, "% ");
    }
    setenv("PROMPT", ENV_PROMPT, 1);

    if (cwd) free(cwd);
    cwd = (char *)NULL;
}

void global_cleanup(void)
{
    ENV_LOGNAME = NULL;

    ENV_HOME = NULL;

    ENV_PATH = NULL;
    
    ENV_PROMPT = NULL;

    ENV_SHELL = NULL;
}

