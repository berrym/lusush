#include <stdarg.h>
#include "misc.h"

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
