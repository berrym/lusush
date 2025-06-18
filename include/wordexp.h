#ifndef WORDEXP_H
#define WORDEXP_H

#include "expand.h"
#include "lusush.h"
#include <stddef.h>

// Create a new word token from a string
word_t *make_word(char *str);

// Free a list of words
void free_all_words(word_t *first);

// Convert a list of words to a space-separated string
char *wordlist_to_str(word_t *word);

// Check if a string is a valid variable/alias name
bool is_name(const char *str);

// Main word expansion function
word_t *word_expand(const char *word);

// Shortcut to expand a word to a string
char *word_expand_to_str(const char *word);

// Alias-specific expansion functions
char *expand_alias(const char *alias_name);
bool is_special_alias_char(char c);

#endif /* WORDEXP_H */
