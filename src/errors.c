#include "../include/errors.h"

#include "../include/lusush.h"
#include "../include/scanner_old.h"

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
 * parser_error:
 *      Enhanced parser error with context and recovery
 */
void parser_error(parser_error_context_t *ctx, source_t *src, error_code code, 
                  error_severity severity, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char buf[MAXLINE + 1];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    // Print error with location info
    if (src) {
        fprintf(stderr, "%s:%zu:%zu: error: %s\n", 
                src->buf ? "<input>" : "<stdin>", 
                src->curline, src->curchar, buf);
    } else {
        fprintf(stderr, "error: %s\n", buf);
    }
    
    if (ctx) {
        ctx->error_count++;
        
        // Enter recovery mode for non-fatal errors
        if (severity != ERROR_FATAL) {
            ctx->in_recovery_mode = true;
        }
    }
}

/*
 * parser_error_with_suggestion:
 *      Parser error with helpful suggestion
 */
void parser_error_with_suggestion(parser_error_context_t *ctx, source_t *src, 
                                  error_code code, const char *suggestion, 
                                  const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char buf[MAXLINE + 1];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    // Print error with location and suggestion
    if (src) {
        fprintf(stderr, "%s:%zu:%zu: error: %s\n", 
                src->buf ? "<input>" : "<stdin>", 
                src->curline, src->curchar, buf);
        if (suggestion) {
            fprintf(stderr, "%s:%zu:%zu: suggestion: %s\n",
                    src->buf ? "<input>" : "<stdin>", 
                    src->curline, src->curchar, suggestion);
        }
    } else {
        fprintf(stderr, "error: %s\n", buf);
        if (suggestion) {
            fprintf(stderr, "suggestion: %s\n", suggestion);
        }
    }
    
    if (ctx) {
        ctx->error_count++;
        ctx->in_recovery_mode = true;
    }
}

/*
 * parser_should_continue:
 *      Check if parser should continue after errors
 */
bool parser_should_continue(parser_error_context_t *ctx) {
    if (!ctx) return true;
    return ctx->error_count < ctx->max_errors;
}

/*
 * parser_enter_recovery:
 *      Set up recovery synchronization tokens
 */
void parser_enter_recovery(parser_error_context_t *ctx, token_type_t *recovery_tokens, size_t count) {
    if (!ctx || count > 8) return;
    
    ctx->in_recovery_mode = true;
    ctx->recovery_count = count;
    for (size_t i = 0; i < count; i++) {
        ctx->recovery_tokens[i] = recovery_tokens[i];
    }
}

/*
 * parser_try_recover:
 *      Try to recover by finding synchronization token
 */
bool parser_try_recover(parser_error_context_t *ctx, token_t *tok) {
    if (!ctx || !ctx->in_recovery_mode || !tok) return false;
    
    // Check if current token is a recovery token
    for (size_t i = 0; i < ctx->recovery_count; i++) {
        if (tok->type == ctx->recovery_tokens[i]) {
            ctx->in_recovery_mode = false;
            return true;
        }
    }
    
    return false;
}

/*
 * parser_reset_errors:
 *      Reset error context for new parsing session
 */
void parser_reset_errors(parser_error_context_t *ctx) {
    if (!ctx) return;
    
    ctx->in_recovery_mode = false;
    ctx->error_count = 0;
    ctx->recovery_count = 0;
    ctx->max_errors = 10;  // Default max errors
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
