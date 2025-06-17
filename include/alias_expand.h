#ifndef ALIAS_EXPAND_H
#define ALIAS_EXPAND_H

/**
 * expand_aliases_recursive:
 *     Expand an alias name recursively, with cycle detection.
 *     Returns a newly allocated string with the expanded alias value,
 *     or NULL if the alias doesn't exist or can't be expanded.
 *     max_depth prevents infinite recursion.
 */
char *expand_aliases_recursive(const char *name, int max_depth);

/**
 * expand_first_word_alias:
 *     Expand only the first word of a command line as an alias.
 *     This matches the POSIX behavior where only the first word
 *     of a simple command can be an alias.
 */
char *expand_first_word_alias(const char *command);

#endif /* ALIAS_EXPAND_H */
