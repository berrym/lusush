#ifndef ERRORS_H
#define ERRORS_H

#include "scanner.h"

#include <stdarg.h>

typedef enum error_code_e {
    EXPECTED_TOKEN,
    UNEXPECTED_TOKEN,
} error_code;

typedef struct error_s {
    error_code errcode;
    size_t lineno, charno, linestart;
    source_s *src;
    char *desc;
} error;

void error_return(const char *, ...);
void error_syscall(const char *, ...);
void error_message(const char *, ...);
void error_quit(const char *, ...);
void error_abort(const char *, ...);

void sigsegv_handler(int);

#endif
