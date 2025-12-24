#include "errors.h"

#include "lusush.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * do_error:
 *      Print an error message and return to caller.
 */
static void do_error(bool errnoflag, int err, const char *fmt, va_list args) {
    char buf[MAXLINE + 1] = {'\0'};
    char tmp[MAXLINE + 1] = {'\0'};

    strcpy(tmp, buf);

    vsprintf(buf, fmt, args);
    if (errnoflag) {
        sprintf(buf, "%s: %s", tmp, strerror(err));
    }

    strncat(buf, "\n", 2);
    fflush(stdout); // in case stdout and stdin are the same
    fputs(buf, stderr);
    fflush(NULL); // flush all stdio output streams
}

/*
 * error_return:
 *      Nonfatal error related to a system call.
 *      Print an error message and return.
 */
void error_return(const char *fmt, ...) {
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
void error_syscall(const char *fmt, ...) {
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
void error_message(const char *fmt, ...) {
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
void error_quit(const char *fmt, ...) {
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
void error_abort(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
    abort();            // dump core and terminate
    exit(EXIT_FAILURE); // should never happen
}

/*
 * Enhanced colored message functions for better user experience
 */
void warning_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void info_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Info: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void success_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Success: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/*
 * NOTE: NECESSARALY FATAL
 * sigsegv_handler:
 *      Segmentation fault handler, insult programmer then abort.
 */
void sigsegv_handler(int signo) {
    error_abort(
        "lusush: caught signal %d, terminating.\n\tAnd fix your damn code.\n",
        signo);
}
