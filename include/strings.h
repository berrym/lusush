#ifndef STRINGS_H
#define STRINGS_H

#include <stdbool.h>
#include <sys/types.h>

void init_str_symtable(void);
char *alloc_str(size_t len, bool exitflag);
void free_str(char *s);
char *get_alloced_str_direct(char *s);
char *get_alloced_str(char *s);
void free_alloced_str(char *s);
bool strupper(char *s);
bool strlower(char *s);
char *str_strip_whitespace(char *s);
size_t str_skip_whitespace(char *s);
size_t str_strip_leading_whitespace(char *s);
ssize_t str_strip_trailing_whitespace(char *s);
void null_replace_newline(char *s);
void null_terminate_str(char *s);
void delete_char_at(char *s, size_t index);
char *strchr_any(char *s, char *chars);
char find_opening_quote_type(char *s);
size_t find_last_quote(char *s);
size_t find_closing_quote(char *s);
size_t find_closing_brace(char *s);
char *quote_val(char *val, bool add_quotes);
char *substitute_str(char *s1, char *s2, size_t start, size_t end);
char *process_token_escapes(const char *str);
bool check_buffer_bounds(const size_t *count, size_t *len, char ***buf);
void free_argv(size_t argc, char **argv);

#endif
