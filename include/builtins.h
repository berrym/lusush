#ifndef BUILTINS_H
#define BUILTINS_H

struct builtin {
    char *name;
    char *doc;
    int (*func)(int argc, char **argv);
};

extern struct builtin builtins[];
extern size_t builtins_count;

int bin_exit(int, char **);
int bin_help(int, char **);
int bin_cd(int, char **);
int bin_pwd(int, char **);
int bin_history(int, char **);
int bin_alias(int, char **);
int bin_unalias(int, char **);
int bin_dump(int, char **);

#endif
