#ifndef ERRORS_H
#define ERRORS_H

#include "scanner_old.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

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
    ERROR_WARNING,    // Continue parsing
    ERROR_RECOVERABLE, // Skip to recovery point
    ERROR_FATAL,      // Abort parsing
} error_severity;

typedef struct error_s {
    error_code errcode;
    error_severity severity;
    size_t lineno, charno, linestart;
    source_t *src;
    char *desc;
    char *suggestion;
} error;

// Parser error recovery context
typedef struct parser_error_context {
    bool in_recovery_mode;
    token_type_t recovery_tokens[8];  // Tokens to synchronize on
    size_t recovery_count;
    size_t error_count;
    size_t max_errors;
} parser_error_context_t;

// Standard error functions
void error_return(const char *, ...);
void error_syscall(const char *, ...);
void error_message(const char *, ...);
void error_quit(const char *, ...);
void error_abort(const char *, ...);

// Enhanced parser error functions
void parser_error(parser_error_context_t *ctx, source_t *src, error_code code, 
                  error_severity severity, const char *fmt, ...);
void parser_error_with_suggestion(parser_error_context_t *ctx, source_t *src, 
                                  error_code code, const char *suggestion, 
                                  const char *fmt, ...);
bool parser_should_continue(parser_error_context_t *ctx);
void parser_enter_recovery(parser_error_context_t *ctx, token_type_t *recovery_tokens, size_t count);
bool parser_try_recover(parser_error_context_t *ctx, token_t *tok);
void parser_reset_errors(parser_error_context_t *ctx);

void sigsegv_handler(int);

#endif
