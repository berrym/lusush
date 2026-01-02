/**
 * @file errors.h
 * @brief Error handling and reporting utilities
 *
 * Provides error codes, severity levels, and standardized error
 * reporting functions for parser and shell operations.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Parser error codes
 */
typedef enum error_code_e {
    EXPECTED_TOKEN,      /**< Expected a specific token */
    UNEXPECTED_TOKEN,    /**< Unexpected token encountered */
    SYNTAX_ERROR,        /**< General syntax error */
    MISSING_QUOTE,       /**< Unclosed quote */
    MISSING_BRACE,       /**< Unclosed brace or bracket */
    UNMATCHED_CONTROL,   /**< Unmatched control structure (if/fi, etc.) */
    INVALID_REDIRECTION, /**< Invalid redirection syntax */
    UNEXPECTED_EOF,      /**< Unexpected end of input */
} error_code;

/**
 * @brief Error severity levels
 */
typedef enum error_severity_e {
    ERROR_WARNING,     /**< Warning - continue parsing */
    ERROR_RECOVERABLE, /**< Error - skip to recovery point */
    ERROR_FATAL,       /**< Fatal error - abort parsing */
} error_severity;

/**
 * @brief Report an error and return from function
 *
 * Prints an error message to stderr and returns from the calling function.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void error_return(const char *fmt, ...);

/**
 * @brief Report a system call error
 *
 * Prints an error message including the system error (errno) description.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void error_syscall(const char *fmt, ...);

/**
 * @brief Print an error message
 *
 * Prints an error message to stderr without terminating.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void error_message(const char *fmt, ...);

/**
 * @brief Report an error and exit the shell
 *
 * Prints an error message and terminates the shell process.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void error_quit(const char *fmt, ...);

/**
 * @brief Report an error and abort
 *
 * Prints an error message and calls abort() for debugging.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void error_abort(const char *fmt, ...);

/**
 * @brief Print a warning message with color
 *
 * Prints a yellow-colored warning message to stderr.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void warning_message(const char *fmt, ...);

/**
 * @brief Print an informational message with color
 *
 * Prints a blue-colored info message to stderr.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void info_message(const char *fmt, ...);

/**
 * @brief Print a success message with color
 *
 * Prints a green-colored success message to stderr.
 * Uses printf-style formatting.
 *
 * @param fmt Format string
 * @param ... Format arguments
 */
void success_message(const char *fmt, ...);

/**
 * @brief Signal handler for SIGSEGV
 *
 * Handles segmentation fault signals, printing diagnostic
 * information before terminating.
 *
 * @param sig Signal number (should be SIGSEGV)
 */
void sigsegv_handler(int sig);

#endif
