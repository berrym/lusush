#include "../../include/alias.h"

#include "../../include/errors.h"
#include "../../include/libhashtable/ht.h"
#include "../../include/lusush.h"
#include "../../include/strings.h"
#include "../../include/tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ht_strstr_t *aliases = NULL; // alias hash table
ht_enum_t *aliases_e = NULL; // alias enumeration object

/**
 * init_aliases:
 *      Initialization code for aliases hash table, set some aliases.
 */
void init_aliases(void) {
    if (aliases == NULL) {
        aliases = ht_strstr_create(HT_STR_CASECMP | HT_SEED_RANDOM);
    }

    // set some example aliases
    set_alias("..", "cd ../");
    set_alias("...", "cd ../../");
    set_alias("day", "echo $(date +'%A')");
    set_alias("hour", "echo $(date +'%l%p')");
    set_alias("l", "ls --color=auto");
    set_alias("la", "ls -a --color=force");
    set_alias("ll", "ls -alF --color=force");
    set_alias("ls", "ls --color=force");
    set_alias("month", "echo $(date +'%B')");
    set_alias("time", "echo $(date +'%T')");
    set_alias("year", "echo $(date +'%Y')");
}

/**
 * free_aliases:
 *      Delete the entire alias hash table.
 */
void free_aliases(void) { ht_strstr_destroy(aliases); }

/**
 * lookup_alias:
 *      Find the alias value associated with a given key name,
 */
char *lookup_alias(const char *key) {
    const char *val = ht_strstr_get(aliases, key);
    return (char *)val;
}

/**
 * print_aliases:
 *      Print out the entire hash table of aliases.
 */
void print_aliases(void) {
    const char *k = NULL, *v = NULL;
    aliases_e = ht_strstr_enum_create(aliases);
    printf("aliases:\n");
    while (ht_strstr_enum_next(aliases_e, &k, &v)) {
        printf("%s='%s'\n", k, v);
    }
    ht_strstr_enum_destroy(aliases_e);
}

/**
 * set_alias:
 *      Insert a new key-value pair into the hash table.
 */
bool set_alias(const char *key, const char *val) {
    ht_strstr_insert(aliases, key, val);
    char *alias = lookup_alias(key);
    if (alias == NULL) {
        return false;
    }
    return true;
}

/**
 * unset_alias:
 *      Remove an entry from the hash table.
 */
void unset_alias(const char *key) { ht_strstr_remove(aliases, key); };

/**
 * valid_name_char:
 *      Check that a character is valid for an alias name.
 */
bool valid_alias_name_char(char c) {
    switch (c) {
    case '.':
    case '_':
    case '!':
    case '%':
    case ',':
    case '@':
        return true;
    default:
        if (isalnum((int)c)) {
            return true;
        } else {
            return false;
        }
    }
}

/**
 * valid_alias_name:
 *      Check that an alias key name consists of valid characters.
 */
bool valid_alias_name(const char *key) {
    const char *p = key;

    if (!*p) {
        return false;
    }

    while (*p) {
        if (!valid_alias_name_char(*p)) {
            return false;
        }
        p++;
    }

    return true;
}

/**
 * alias_usage:
 *      Print how to use the builtin alias command.
 */
void alias_usage(void) {
    fprintf(stderr, "usage:\talias (print a list of all aliases)\n"
                    "\talias name (print the value of an alias)\n"
                    "\talias name='replacement text' (set an alias)\n");
}

/**
 * unalias_usage:
 *      Print how to use the builtin unalias command.
 */
void unalias_usage(void) {
    fprintf(stderr, "usage:\tunalias name (unset alias with key name)\n");
}

/**
 * src_str_from_argv:
 *       Convert elements of an argument vector into a single string
 *       seperating arguments with the string value given in the sep argument.
 */
char *src_str_from_argv(size_t argc, char **argv, const char *sep) {
    char *src = NULL;

    src = alloc_str(MAXLINE + 1, false);
    if (src == NULL) {
        error_message("alias: unable to allocate source string");
        return NULL;
    }

    // recreate a single source string
    for (size_t i = 0; i < argc; i++) {
        strcat(src, argv[i]);
        strcat(src, sep);
    }
    null_terminate_str(src);

    return src;
}

/**
 * parse_alias_var_name:
 *      Parse the word before an equal sign in a whitespace seperated
 *      token source string that represents the alias name.
 */
char *parse_alias_var_name(char *src) {
    char *sp = NULL, *ep = NULL, *var = NULL, **argv = NULL;
    size_t argc = 0, targc = 0, cpos = 0;

    if (check_buffer_bounds(&argc, &targc, &argv)) {
        argv[argc] = alloc_str(MAXLINE + 1, false);
        if (argv[argc] == NULL) {
            error_message(
                "error: `alias`: insufficient memory to complete operation");
            return NULL;
        }
    } else {
        error_message(
            "error: `alias`: insufficient memory to complete operation");
        return NULL;
    }

    for (char *p = src; *p; p++) {
        str_skip_whitespace(p);
        sp = p;

        // Fill tokens seperated by whitespace, grow buffer if necessary
        while (!isspace((int)*p) && *p != '=') {
            argv[argc][cpos] = *p, p++, cpos++;
        }

        null_terminate_str(argv[argc]);
        argc++, cpos = 0;

        if (argc == targc) {
            if (!check_buffer_bounds(&argc, &targc, &argv)) {
                error_message("error: `alias`: insufficient memory to complete "
                              "operation");
                return NULL;
            }
        }
        argv[argc] = alloc_str(MAXLINE + 1, false);
        if (argv[argc] == NULL) {
            error_message(
                "error: `alias`: insufficient memory to complete operation");
            return NULL;
        }

        // '=' found, parse the alias variable name token that precedes it
        if (*p == '=') {
            ep = p;
            p = sp;

            // Fill tokens until we reach ep, the address of the '='
            while (p < ep) {
                if (!isspace((int)*p)) {
                    argv[argc][cpos] = *p;
                    cpos++, p++;
                } else {
                    null_terminate_str(argv[argc]);
                    argc++, cpos = 0;
                    if (argc == targc) {
                        if (!check_buffer_bounds(&argc, &targc, &argv)) {
                            error_message("error: `alias`: insufficient memory "
                                          "to complete operation");
                            return NULL;
                        }
                    }
                    argv[argc] = alloc_str(MAXLINE + 1, false);
                    if (argv[argc] == NULL) {
                        error_message("error: `alias`: insufficient memory to "
                                      "complete operation");
                        return NULL;
                    }
                }
            }

            // Ignore parsed empty string tokens
            while (!*argv[argc]) {
                argc--;
            }

            var = strdup(argv[argc]);
            if (var == NULL) {
                error_message("error: `alias`: insufficient memory to complete "
                              "operation");
                break;
            }

            free_argv(argc, argv);

            return var;
        }
    }

    return NULL;
}

/**
 * parse_alias_var_value:
 *      Parse a substring between quotes that represents the alias substitution
 *      value.
 */
char *parse_alias_var_value(char *src,
                            const char delim __attribute__((unused))) {
    // Find the first '='
    char *eq = strchr(src, '=');
    if (!eq) {
        error_message("error: `alias`: '=' not found in alias definition");
        return NULL;
    }
    // Skip whitespace after '='
    char *p = eq + 1;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    // Take the rest of the string as the value, trim trailing whitespace
    char *end = p + strlen(p);
    while (end > p && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    size_t len = end - p;
    char *val = alloc_str(len + 1, false);
    if (!val) {
        error_message(
            "error: `alias`: insufficient memory to complete operation");
        return NULL;
    }
    strncpy(val, p, len);
    val[len] = '\0';
    return val;
}

/**
 * expand_aliases_recursive:
 *     Expand an alias name recursively, with cycle detection.
 *     Uses the modern tokenizer for proper word splitting.
 *     Returns a newly allocated string with the expanded alias value,
 *     or NULL if the alias doesn't exist or can't be expanded.
 */
char *expand_aliases_recursive(const char *name, int max_depth) {
    if (!name || max_depth <= 0) {
        return NULL;
    }

    // Look up the initial alias
    char *value = lookup_alias(name);
    if (!value) {
        return NULL;
    }

    // Make a copy we can work with
    char *result = strdup(value);
    if (!result) {
        return NULL;
    }

    // Use tokenizer to properly parse the alias value
    tokenizer_t *tokenizer = tokenizer_new(result);
    if (!tokenizer) {
        free(result);
        return NULL;
    }

    // Get the first token to check for recursive expansion
    token_t *first_token = tokenizer_current(tokenizer);
    if (!first_token || first_token->type != TOK_WORD) {
        tokenizer_free(tokenizer);
        return result; // Return as-is if no valid word token
    }

    // Check if the first word is also an alias
    char *recursive =
        expand_aliases_recursive(first_token->text, max_depth - 1);
    if (recursive) {
        // Build new command with expanded first word
        size_t recursive_len = strlen(recursive);
        size_t remaining_len = strlen(result) - strlen(first_token->text);
        char *new_result = malloc(recursive_len + remaining_len + 2);

        if (new_result) {
            strcpy(new_result, recursive);

            // Add the rest of the original command after the first word
            const char *rest = result + strlen(first_token->text);
            if (*rest) {
                strcat(new_result, rest);
            }

            free(result);
            free(recursive);
            tokenizer_free(tokenizer);
            return new_result;
        }
        free(recursive);
    }

    tokenizer_free(tokenizer);
    return result;
}

/**
 * expand_first_word_alias:
 *     Expand only the first word of a command line as an alias.
 *     Uses the modern tokenizer for proper word boundaries.
 *     This matches the POSIX behavior where only the first word
 *     of a simple command can be an alias.
 */
char *expand_first_word_alias(const char *command) {
    if (!command) {
        return NULL;
    }

    // Use tokenizer to properly identify the first word
    tokenizer_t *tokenizer = tokenizer_new(command);
    if (!tokenizer) {
        return strdup(command); // Return original on tokenizer failure
    }

    token_t *first_token = tokenizer_current(tokenizer);
    if (!first_token || first_token->type != TOK_WORD) {
        tokenizer_free(tokenizer);
        return strdup(command); // Return original if no word token
    }

    // Try to expand the first word as an alias
    char *alias_value = lookup_alias(first_token->text);
    if (!alias_value) {
        tokenizer_free(tokenizer);
        return strdup(command); // No alias found, return original
    }

    // Construct the new command with the alias expansion
    const char *rest_of_command = command + strlen(first_token->text);
    size_t alias_len = strlen(alias_value);
    size_t rest_len = strlen(rest_of_command);
    char *result = malloc(alias_len + rest_len + 1);

    if (result) {
        strcpy(result, alias_value);
        strcat(result, rest_of_command);
    }

    tokenizer_free(tokenizer);
    return result ? result : strdup(command);
}

/**
 * is_special_alias_char:
 *     Check if a character is special in alias names.
 *     POSIX allows some special characters in alias names
 *     that aren't allowed in variable names.
 */
bool is_special_alias_char(char c) {
    switch (c) {
    case '.':
    case '_':
    case '!':
    case '%':
    case ',':
    case '@':
        return true;
    default:
        return isalnum(c);
    }
}

// /**
//  * parse_alias_var_value:
//  *      Parse a substring between quotes that represents the alias
//  substitution
//  * value.
//  */
// char *parse_alias_var_value(char *src, const char delim) {
//     char *val = NULL, *sp = NULL, *ep = NULL;

//     if (!delim) {
//         error_message("error: `alias`: value not properly quoted");
//         return NULL;
//     }

//     for (char *p = src; *p; p++) {              // for each char in line
//         if (sp == NULL && *p == delim) {        // find first delimeter
//             sp = p, sp++;                       // set start ptr
//         } else if (ep == NULL && *p == delim) { // find second delimeter
//             ep = p;                             // set end ptr
//         }

//         if (sp && ep) {               // if both set
//             char substr[ep - sp + 1]; // declare substr
//             p = sp;

//             for (size_t i = 0; p < ep; i++, p++) { // copy to substr
//                 substr[i] = *p;
//             }

//             substr[ep - sp] = '\0'; // nul-terminate

//             val = strdup(substr);
//             if (val == NULL) {
//                 error_message("error: `alias`: insufficient memory to
//                 complete "
//                               "operation");
//                 return NULL;
//             }
//             break;
//         }
//     }

//     if (ep == NULL) {
//         error_message("error: `alias`: missing closing quote");
//         return NULL;
//     }

//     return val;
// }
