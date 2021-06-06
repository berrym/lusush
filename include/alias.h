#include <stdbool.h>

#ifndef ALIAS_H
#define ALIAS_H

#define MAX_ALIAS_LEN 128

struct alias_entry {
    char *key;
    char *val;
    struct alias_entry *next;
};

struct alias_list {
    size_t len;
    struct alias_entry *head;
    struct alias_entry *tail;
};

void init_aliases(void);
void free_alias_list(void);
void print_alias_list(void);
bool set_alias(const char *, const char *);
void unset_alias(const char *);
char *expand_alias(char *);
bool valid_alias_name(const char *);

#endif
