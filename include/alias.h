#ifndef ALIAS_H
#define ALIAS_H

#include <stdbool.h>

void init_aliases(void);
void free_aliases(void);
char *lookup_alias(const char *);
void print_aliases(void);
bool set_alias(const char *, const char *);
void unset_alias(const char *);
bool valid_alias_name(const char *);
void alias_usage(void);
void unalias_usage(void);
char *src_str_from_argv(size_t, char **, const char *);
const char find_opening_quote_type(char *);
char *parse_alias_var_name(char *);
char *parse_alias_var_value(char *, const char);

#endif
