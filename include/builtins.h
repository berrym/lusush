#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct builtin_s {
    const char *name;
    const char *doc;
    int (*func)(int argc, char **argv);
} builtin;

extern builtin builtins[];
extern const size_t builtins_count;

int bin_exit(int, char **);
int bin_help(int, char **);
int bin_cd(int, char **);
int bin_pwd(int, char **);
int bin_history(int, char **);
int bin_alias(int, char **);
int bin_unalias(int, char **);
int bin_setopt(int, char **);
int bin_setprompt(int, char **);
int bin_clear(int, char **);
int bin_type(int, char **);
int bin_unset(int, char **);
int bin_dump(int, char **);
int bin_echo(int, char **);
int bin_printf(int, char **);
int bin_export(int, char **);
int bin_source(int, char **);
int bin_test(int, char **);
int bin_read(int, char **);
int bin_eval(int, char **);
int bin_true(int, char **);
int bin_false(int, char **);
int bin_set(int, char **);
int bin_shift(int, char **);
int bin_break(int, char **);
int bin_continue(int, char **);
int bin_return(int, char **);
int bin_trap(int, char **);
int bin_exec(int, char **);
int bin_wait(int, char **);
int bin_umask(int, char **);
int bin_ulimit(int, char **);
int bin_times(int, char **);
int bin_getopts(int, char **);
int bin_local(int, char **);
bool is_builtin(const char *);

#endif
