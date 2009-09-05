/**
 * misc.c - various functions that didn't fit anywhere else.
 */

// Includes {{{

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "ldefs.h"
#include "env.h"
#include "misc.h"

// End of includes }}}

// print_debug {{{

/**
 * print_debug:
 *
 *  If MACRO PRINT_DEBUG is defined at compile time then this function
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

//}}}

// build_prompt {{{

/**
 * build_prompt:
 *      Builds the user's prompt displaying the current working directory.
 */
void build_prompt(void)
{
    char cwd[MAXLINE];
    char ENV_PROMPT[MAXLINE];

    if (getcwd(cwd, MAXLINE) == (char *)NULL) {
        perror("lusush: build_prompt");
        strcpy(ENV_PROMPT, "% ");
    }
    else
    {
        strcpy(ENV_PROMPT, cwd);
        strcat(ENV_PROMPT, "% ");
    }
    setenv("PROMPT", ENV_PROMPT, 1);
}

// End of build_prompt }}} 

// global_cleanup {{{

void global_cleanup(void)
{
    ENV_LOGNAME = (char *)NULL;

    ENV_HOME = (char *)NULL;

    ENV_PATH = (char *)NULL;
    
    ENV_PROMPT = (char *)NULL;

    ENV_SHELL = (char *)NULL;
}

// End of global cleanup }}}

// vim:filetype=c foldmethod=marker autoindent expandtab shiftwidth=4
