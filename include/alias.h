/**
 * @file alias.h
 * @brief Shell alias management declarations
 *
 * Provides alias creation, lookup, expansion, and validation functions
 * for shell command aliasing.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef ALIAS_H
#define ALIAS_H

#include <stdbool.h>
#include <stddef.h>

#include "libhashtable/ht.h"

extern ht_strstr_t *aliases;

void init_aliases(void);
void free_aliases(void);
char *lookup_alias(const char *);
void print_aliases(void);
bool set_alias(const char *, const char *);
void unset_alias(const char *);
bool valid_alias_name(const char *);
void alias_usage(void);
void unalias_usage(void);

// Modern alias expansion functions using tokenizer
char *expand_aliases_recursive(const char *name, int max_depth);
char *expand_first_word_alias(const char *command);
char *expand_alias_with_shell_operators(const char *command);
bool contains_shell_operators(const char *value);
bool is_special_alias_char(char c);

#endif
