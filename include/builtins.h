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
int bin_dump(int, char **);
bool is_builtin(const char *);

#endif
