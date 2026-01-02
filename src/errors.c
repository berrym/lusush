/**
 * @file errors.c
 * @brief Error reporting and handling utilities
 *
 * Provides standardized error reporting functions for:
 * - System call errors (with errno)
 * - Non-system errors
 * - Fatal errors (with exit or abort)
 * - Informational messages (warning, info, success)
 * - Signal handlers (SIGSEGV)
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#include "errors.h"

#include "lusush.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Internal error printing function
 *
 * Formats and prints error messages, optionally including errno info.
 *
 * @param errnoflag If true, append strerror(err) to message
 * @param err Error number to use with strerror
 * @param fmt Format string
 * @param args Variable argument list
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

/**
 * @brief Nonfatal error related to a system call
 *
 * Prints an error message with errno information and returns.
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void error_return(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_error(true, errno, fmt, args);
    va_end(args);
}

/**
 * @brief Fatal error related to a system call
 *
 * Prints an error message with errno information and exits with
 * EXIT_FAILURE.
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void error_syscall(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_error(true, errno, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

/**
 * @brief Nonfatal error unrelated to a system call
 *
 * Prints an error message without errno information and returns.
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void error_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
}

/**
 * @brief Fatal error unrelated to a system call
 *
 * Prints an error message and exits with EXIT_FAILURE.
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void error_quit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

/**
 * @brief Fatal error with core dump
 *
 * Prints an error message, calls abort() to generate a core dump,
 * and terminates the process.
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void error_abort(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_error(false, 0, fmt, args);
    va_end(args);
    abort();            // dump core and terminate
    exit(EXIT_FAILURE); // should never happen
}

/**
 * @brief Print a warning message
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void warning_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * @brief Print an informational message
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void info_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Info: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * @brief Print a success message
 *
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 */
void success_message(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Success: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * @brief Segmentation fault signal handler
 *
 * Prints an error message with the signal number and aborts.
 * This handler is necessarily fatal.
 *
 * @param signo Signal number (SIGSEGV)
 */
void sigsegv_handler(int signo) {
    error_abort(
        "lusush: caught signal %d, terminating.\n\tAnd fix your damn code.\n",
        signo);
}
