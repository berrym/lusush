#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include "lusush.h"
#include "errors.h"

/*
 * do_error:
 *      Print an error message and return to caller.
 */
static void do_error(bool errnoflag, int err, const char *fmt, va_list args)
{
    char buf[MAXLINE] = { '\0' };

    vsnprintf(buf, MAXLINE - 1, fmt, args);
    if (errnoflag)
        snprintf(buf + strlen(buf),
                 MAXLINE - strlen(buf) - 1,
                 ": %s",
                 strerror(err));

    strncat(buf, "\n", 2);
    fflush(stdout);             // in case stdout and stdin are the same
    fputs(buf, stderr);
    fflush(NULL);               // flush all stdio output streams
}

/*
 * error_return:
 *      Nonfatal error related to a system call.
 *      Print an error message and return.
 */
void error_return(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(true, errno, fmt, args);
    va_end(args);
}

/*
 * error_syscall:
 *      Fatal error related to a system call.
 *      Print an error message and terminate.
 */
void error_syscall(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(true, errno, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

/*
 * error_message:
 *      Nonfatal error unrelated to a system call.
 *      Print an error message and return.
 */
void error_message(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
}

/*
 * error_quit:
 *      Fatal error unrelated to a system call.
 *      Print an error message and return.
 */
void error_quit(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

/*
 * error_abort:
 *      Fatal error related to a system call.
 *      Print an error message, dump core, and terminate.
 */
void error_abort(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
    abort();                    // dump core and terminate
    exit(EXIT_FAILURE);         // should never happen
}

/*
 * NOTE: NECESSARALY FATAL
 * sigsegv_handler:
 *      Segmentation fault handler, insult programmer then abort.
 */
void sigsegv_handler(int signo)
{
    error_abort("lusush: caught signal %d, terminating.\n"
                "\tAnd fix your damn code.\n", signo);
}
