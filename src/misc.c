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
 * print_v
 *      print messages if VERBOSE_PRINT option is set
 */
void print_v(const char *fmt, ...)
{
    if (opt_is_set(VERBOSE_PRINT)) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}
