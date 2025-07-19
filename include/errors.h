#ifndef ERRORS_H
#define ERRORS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum error_code_e {
    EXPECTED_TOKEN,
    UNEXPECTED_TOKEN,
    SYNTAX_ERROR,
    MISSING_QUOTE,
    MISSING_BRACE,
    UNMATCHED_CONTROL,
    INVALID_REDIRECTION,
    UNEXPECTED_EOF,
} error_code;

typedef enum error_severity_e {
    ERROR_WARNING,     // Continue parsing
    ERROR_RECOVERABLE, // Skip to recovery point
    ERROR_FATAL,       // Abort parsing
} error_severity;

// Standard error functions
void error_return(const char *, ...);
void error_syscall(const char *, ...);
void error_message(const char *, ...);
void error_quit(const char *, ...);
void error_abort(const char *, ...);

// Enhanced colored message functions
void warning_message(const char *, ...);
void info_message(const char *, ...);
void success_message(const char *, ...);

// Enhanced parser error functions

void sigsegv_handler(int);

#endif
