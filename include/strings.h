#ifndef STRINGS_H
#define STRINGS_H

#include <stdbool.h>
#include <sys/types.h>

void init_str_symtable(void);
char *get_alloced_str_direct(char *);
char *get_alloced_str(char *);
void free_alloced_str(char *s);
char *alloc_str(size_t, bool);
void free_str(char *);
bool strupper(char *);
bool strlower(char *);
char *str_strip_whitespace(char *s);
size_t str_skip_whitespace(char *);
size_t str_strip_leading_whitespace(char *);
ssize_t str_strip_trailing_whitespace(char *);
void null_replace_newline(char *);
void null_terminate_str(char *);

#endif
