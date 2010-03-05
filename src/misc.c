/*
 * misc.c - various functions that didn't fit anywhere else.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "misc.h"
#include "opts.h"

/*
 * print_debug:
 *      If MACRO PRINT_DEBUG is defined at compile time then this function
 *      will behave just like printf.  It is used to print degugging
 *      statements without having to go back through all of the code and
 *      comment out unwanted printf statements.
 */
void print_debug(const char *fmt, ...)
{
    //#ifdef PRINT_DEBUG
    if (opt_is_set(VERBOSE_PRINT)) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
    //#endif
}


