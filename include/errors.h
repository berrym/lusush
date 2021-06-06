#ifndef ERRORS_H
#define ERRORS_H

#include <stdarg.h>
#include "scanner.h"

enum error_code {
    EXPECTED_TOKEN,
    UNEXPECTED_TOKEN,
};

struct error {
    enum error_code errcode;
    size_t lineno, charno, linestart;
    struct source *src;
    char *desc;
};

void print_parser_error(struct error *, char *errstr);
void raise_parser_error(struct error err);

void error_return(const char *, ...);
void error_syscall(const char *, ...);
void error_message(const char *, ...);
void error_quit(const char *, ...);
void error_abort(const char *, ...);

void sigsegv_handler(int);

#endif
