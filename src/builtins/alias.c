#include "../../include/alias.h"

#include "../../include/builtins.h"
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
    set_alias("l", "ls --color=auto");
    set_alias("la", "ls -a --color=force");
    set_alias("ll", "ls -alF --color=force");
    set_alias("ls", "ls --color=force");
}

/**
 * free_aliases:
 *      Delete the entire alias hash table.
 */
void free_aliases(void) {
    if (aliases) {
        ht_strstr_destroy(aliases);
        aliases = NULL;
    }
}

/**
 * lookup_alias:
 *      Find the alias value associated with a given key name.
 */
char *lookup_alias(const char *key) {
    if (!aliases || !key) {
        return NULL;
    }
    const char *val = ht_strstr_get(aliases, key);
    return (char *)val;
}

/**
 * print_aliases:
 *      Print out the entire hash table of aliases in POSIX format.
 */
void print_aliases(void) {
    const char *k = NULL, *v = NULL;
    aliases_e = ht_strstr_enum_create(aliases);

    while (ht_strstr_enum_next(aliases_e, &k, &v)) {
        // POSIX format: alias name='value'
        printf("alias %s='%s'\n", k, v);
    }
    ht_strstr_enum_destroy(aliases_e);
}

/**
 * set_alias:
 *      Insert a new key-value pair into the hash table.
 */
bool set_alias(const char *key, const char *val) {
    if (!aliases || !key || !val) {
        return false;
    }

    ht_strstr_insert(aliases, key, val);
    char *alias = lookup_alias(key);
    return (alias != NULL);
}

/**
 * unset_alias:
 *      Remove an entry from the hash table.
 */
void unset_alias(const char *key) {
    if (aliases && key) {
        ht_strstr_remove(aliases, key);
    }
}

/**
 * valid_alias_name_char:
 *      Check that a character is valid for an alias name per POSIX.
 *      POSIX allows alphanumeric characters and underscore.
 *      Some shells extend this to include additional characters.
 */
bool valid_alias_name_char(char c) {
    // POSIX base: alphanumeric and underscore
    if (isalnum((unsigned char)c) || c == '_') {
        return true;
    }

    // Common extensions in practice
    switch (c) {
    case '.': // Often used for navigation aliases
    case '-': // Common in command names
    case '+': // Sometimes used
        return true;
    default:
        return false;
    }
}

/**
 * skip_whitespace:
 *      Skip whitespace characters in a string.
 */
static const char *skip_whitespace(const char *str) {
    if (!str) {
        return NULL;
    }
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

/**
 * valid_alias_name:
 *      Check that an alias key name consists of valid characters.
 */
bool valid_alias_name(const char *key) {
    if (!key || !*key) {
        return false;
    }

    // Check for empty string after trimming whitespace
    const char *trimmed = skip_whitespace(key);
    if (!trimmed || !*trimmed) {
        return false;
    }

    // First character cannot be a digit (POSIX requirement)
    if (isdigit((unsigned char)*trimmed)) {
        return false;
    }

    const char *p = trimmed;
    while (*p && !isspace((unsigned char)*p)) {
        if (!valid_alias_name_char(*p)) {
            return false;
        }
        p++;
    }

    // Make sure we processed at least one character
    return (p > trimmed);
}

/**
 * alias_usage:
 *      Print how to use the builtin alias command.
 */
void alias_usage(void) {
    fprintf(stderr, "usage: alias [name[=value] ...]\n"
                    "       alias [name ...]\n"
                    "\n"
                    "Define or display aliases.\n"
                    "\n"
                    "Without arguments, print all aliases.\n"
                    "With name arguments, print the aliases for those names.\n"
                    "With name=value arguments, define aliases.\n");
}

/**
 * unalias_usage:
 *      Print how to use the builtin unalias command.
 */
void unalias_usage(void) {
    fprintf(stderr, "usage: unalias [-a] name [name ...]\n"
                    "\n"
                    "Remove alias definitions.\n"
                    "\n"
                    "  -a    remove all alias definitions\n");
}

/**
 * find_equals:
 *      Find the first unquoted equals sign in a string.
 */
static const char *find_equals(const char *str) {
    if (!str) {
        return NULL;
    }

    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;

    for (const char *p = str; *p; p++) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (*p == '\\' && !in_single_quote) {
            escaped = true;
            continue;
        }

        if (*p == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (*p == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }

        if (*p == '=' && !in_single_quote && !in_double_quote) {
            return p;
        }
    }

    return NULL;
}

/**
 * parse_alias_assignment:
 *      Parse an alias assignment in the form name=value.
 *      Returns true on success, false on failure.
 *      On success, *name and *value are set to newly allocated strings.
 */
static bool parse_alias_assignment(const char *assignment, char **name,
                                   char **value) {
    if (!assignment || !name || !value) {
        return false;
    }

    *name = NULL;
    *value = NULL;

    const char *equals = find_equals(assignment);
    if (!equals) {
        return false;
    }

    // Extract name part
    size_t name_len = equals - assignment;
    const char *name_start = skip_whitespace(assignment);
    const char *name_end = equals;

    // Trim trailing whitespace from name
    while (name_end > name_start && isspace((unsigned char)*(name_end - 1))) {
        name_end--;
    }

    name_len = name_end - name_start;
    if (name_len == 0) {
        return false;
    }

    *name = malloc(name_len + 1);
    if (!*name) {
        return false;
    }
    strncpy(*name, name_start, name_len);
    (*name)[name_len] = '\0';

    // Extract value part
    const char *value_start = skip_whitespace(equals + 1);
    const char *value_end = value_start + strlen(value_start);

    // Trim trailing whitespace from value
    while (value_end > value_start &&
           isspace((unsigned char)*(value_end - 1))) {
        value_end--;
    }

    // Handle quoted values
    if (value_end > value_start) {
        char quote_char = 0;
        if ((*value_start == '\'' || *value_start == '"') &&
            *(value_end - 1) == *value_start) {
            quote_char = *value_start;
            value_start++;
            value_end--;
        }

        size_t value_len = value_end - value_start;
        *value = malloc(value_len + 1);
        if (!*value) {
            free(*name);
            *name = NULL;
            return false;
        }

        // Copy value, handling escape sequences if in double quotes
        size_t j = 0;
        for (const char *p = value_start; p < value_end; p++) {
            if (quote_char == '"' && *p == '\\' && p + 1 < value_end) {
                // Handle common escape sequences in double quotes
                switch (*(p + 1)) {
                case 'n':
                    (*value)[j++] = '\n';
                    p++;
                    break;
                case 't':
                    (*value)[j++] = '\t';
                    p++;
                    break;
                case 'r':
                    (*value)[j++] = '\r';
                    p++;
                    break;
                case '\\':
                    (*value)[j++] = '\\';
                    p++;
                    break;
                case '"':
                    (*value)[j++] = '"';
                    p++;
                    break;
                case '$':
                    (*value)[j++] = '$';
                    p++;
                    break;
                default:
                    (*value)[j++] = *p;
                    break;
                }
            } else {
                (*value)[j++] = *p;
            }
        }
        (*value)[j] = '\0';
    } else {
        // Empty value
        *value = malloc(1);
        if (!*value) {
            free(*name);
            *name = NULL;
            return false;
        }
        (*value)[0] = '\0';
    }

    return true;
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
bool is_special_alias_char(char c) { return valid_alias_name_char(c); }

/**
 * contains_shell_operators:
 *     Check if an alias value contains shell operators that require re-parsing.
 *     This detects pipes, redirections, logical operators, etc.
 */
bool contains_shell_operators(const char *value) {
    if (!value) {
        return false;
    }

    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;

    for (const char *p = value; *p; p++) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (*p == '\\' && !in_single_quote) {
            escaped = true;
            continue;
        }

        if (*p == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (*p == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }

        // Check for shell operators outside of quotes
        if (!in_single_quote && !in_double_quote) {
            switch (*p) {
            case '|': // Pipe
                if (*(p + 1) == '|') {
                    return true; // Logical OR ||
                }
                return true; // Pipe |
            case '&':        // Background or logical AND
                if (*(p + 1) == '&') {
                    return true; // Logical AND &&
                }
                return true; // Background &
            case '>':        // Redirect out
            case '<':        // Redirect in
                return true;
            case ';': // Command separator
                return true;
            case '(': // Subshell
            case ')':
                return true;
            case '{': // Command group
            case '}':
                return true;
            case '`': // Command substitution
                return true;
            case '$': // Variable expansion or command substitution
                if (*(p + 1) == '(') {
                    return true; // Command substitution $(...)
                }
                break;
            }
        }
    }

    return false;
}

/**
 * expand_alias_with_shell_operators:
 *     Enhanced alias expansion that can handle shell operators by re-parsing.
 *     Returns a newly allocated string with the full command line to execute,
 *     or NULL if no alias expansion is needed.
 */
char *expand_alias_with_shell_operators(const char *command) {
    if (!command) {
        return NULL;
    }

    // Use tokenizer to get the first word
    tokenizer_t *tokenizer = tokenizer_new(command);
    if (!tokenizer) {
        return NULL;
    }

    token_t *first_token = tokenizer_current(tokenizer);
    if (!first_token || first_token->type != TOK_WORD) {
        tokenizer_free(tokenizer);
        return NULL;
    }

    // Check if first word is an alias
    char *alias_value = lookup_alias(first_token->text);
    if (!alias_value) {
        tokenizer_free(tokenizer);
        return NULL;
    }

    // Get the rest of the command after the first word
    const char *rest_of_command = command + strlen(first_token->text);

    // Skip whitespace after first word
    while (*rest_of_command && isspace((unsigned char)*rest_of_command)) {
        rest_of_command++;
    }

    // Build the expanded command
    size_t alias_len = strlen(alias_value);
    size_t rest_len = strlen(rest_of_command);
    size_t total_len = alias_len + (rest_len > 0 ? rest_len + 1 : 0) + 1;

    char *expanded = malloc(total_len);
    if (!expanded) {
        tokenizer_free(tokenizer);
        return NULL;
    }

    strcpy(expanded, alias_value);
    if (rest_len > 0) {
        strcat(expanded, " ");
        strcat(expanded, rest_of_command);
    }

    tokenizer_free(tokenizer);
    return expanded;
}

/**
 * bin_alias:
 *      Create aliased commands, or print alias values.
 *      Improved POSIX-compliant implementation.
 */
int bin_alias(int argc, char **argv) {
    // No arguments: print all aliases
    if (argc == 1) {
        print_aliases();
        return 0;
    }

    int exit_status = 0;

    // Process each argument
    for (int i = 1; i < argc; i++) {
        char *name = NULL;
        char *value = NULL;

        // Try to parse as assignment (name=value)
        if (parse_alias_assignment(argv[i], &name, &value)) {
            // Check if name is valid
            if (!valid_alias_name(name)) {
                error_message("alias: invalid alias name: %s", name);
                free(name);
                free(value);
                exit_status = 1;
                continue;
            }

            // Can't alias builtin commands or keywords
            if (is_builtin(name)) {
                error_message("alias: cannot alias shell keyword: %s", name);
                free(name);
                free(value);
                exit_status = 1;
                continue;
            }

            // Set the alias
            if (!set_alias(name, value)) {
                error_message("alias: failed to create alias: %s", name);
                exit_status = 1;
            }

            free(name);
            free(value);
        } else {
            // Not an assignment, treat as name lookup
            char *alias_value = lookup_alias(argv[i]);
            if (alias_value) {
                printf("alias %s='%s'\n", argv[i], alias_value);
            } else {
                error_message("alias: %s: not found", argv[i]);
                exit_status = 1;
            }
        }
    }

    return exit_status;
}

/**
 * bin_unalias:
 *      Remove aliased commands.
 *      Improved POSIX-compliant implementation.
 */
int bin_unalias(int argc, char **argv) {
    if (argc < 2) {
        unalias_usage();
        return 1;
    }

    int exit_status = 0;

    // Handle -a option (remove all aliases)
    if (argc == 2 && strcmp(argv[1], "-a") == 0) {
        if (aliases) {
            ht_strstr_destroy(aliases);
            aliases = ht_strstr_create(HT_STR_CASECMP | HT_SEED_RANDOM);
        }
        return 0;
    }

    // Process each name argument
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            error_message("unalias: -a must be used alone");
            exit_status = 1;
            continue;
        }

        if (lookup_alias(argv[i])) {
            unset_alias(argv[i]);
        } else {
            error_message("unalias: %s: not found", argv[i]);
            exit_status = 1;
        }
    }

    return exit_status;
}
