#include "../../include/alias.h"
#include "../../include/errors.h"
#include "../../include/ht.h"
#include "../../include/lusush.h"
#include "../../include/strings.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ht_s *aliases = NULL; // Hash table for storing aliases

/**
 * init_aliases:
 *      Initialization code for aliases hash table, set some aliases.
 */
void init_aliases(void) {
    if (aliases == NULL)
        aliases = ht_create();

    set_alias("ll", "ls -alF");
    set_alias("..", "cd ..");
    set_alias("...", "cd ../../");
}

/**
 * free_aliases:
 *      Delete the entire alias hash table.
 */
void free_aliases(void) { ht_destroy(aliases); }

/**
 * lookup_alias:
 *      Find the alias value associated with a given key name,
 */
char *lookup_alias(const char *key) {
    char *val = ht_search(aliases, key);
    return val;
}

/**
 * print_aliases:
 *      Print out the entire hash table of aliases.
 */
void print_aliases(void) {
    printf("aliases:\n");
    ht_print(aliases);
}

/**
 * set_alias:
 *      Insert a new key-value pair into the hash table.
 */
bool set_alias(const char *key, const char *val) {
    if (!ht_insert(aliases, key, val))
        return false;

    return true;
}

/**
 * unset_alias:
 *      Remove an entry from the hash table.
 */
void unset_alias(const char *key) { ht_delete(aliases, key); }

/**
 * valid_alias_name:
 *      Check that an alias key name consists of valid characters.
 */
bool valid_alias_name(const char *key) {
    const char *p = key;

    if (!*p)
        return false;

    while (*p) {
        if (isalnum((int)*p) || *p == '.' || *p == '_' || *p == '!' ||
            *p == '%' || *p == ',' || *p == '@') {
            p++;
        } else {
            return false;
        }
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
 * find_opening_quote_type:
 *      Determine wether a quoted value starts with a single or double quote,
 *      return then char value found, or NUL byte.
 */
char find_opening_quote_type(char *src) {
    for (char *p = src; *p; p++)
        if (*p == '\'' || *p == '\"')
            return *p;

    return '\0';
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

    for (char *p = src; p; p++) {
        str_skip_whitespace(p);
        sp = p;

        // Fill tokens seperated by whitespace, grow buffer if necessary
        while (!isspace((int)*p) && *p != '=')
            argv[argc][cpos] = *p, p++, cpos++;

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
            while (!*argv[argc])
                argc--;

            var = strdup(argv[argc]);
            if (var == NULL) {
                error_message("error: `alias`: insufficient memory to complete "
                              "operation");
                break;
            }

            free(argv);

            return var;
        }
    }

    return NULL;
}

/**
 * parse_alias_var_value:
 *      Parse a substring between quotes that represents the alias substitution
 * value.
 */
char *parse_alias_var_value(char *src, const char delim) {
    char *val = NULL, *sp = NULL, *ep = NULL;

    if (!delim) {
        error_message("error: `alias`: value not properly quoted");
        return NULL;
    }

    for (char *p = src; *p; p++) {          // for each char in line
        if (sp == NULL && *p == delim)      // find first delimeter
            sp = p, sp++;                   // set start ptr
        else if (ep == NULL && *p == delim) // find second delimeter
            ep = p;                         // set end ptr

        if (sp && ep) {               // if both set
            char substr[ep - sp + 1]; // declare substr
            p = sp;

            for (size_t i = 0; p < ep; i++, p++) // copy to substr
                substr[i] = *p;

            substr[ep - sp] = '\0'; // nul-terminate

            val = strdup(substr);
            if (val == NULL) {
                error_message("error: `alias`: insufficient memory to complete "
                              "operation");
                return NULL;
            }
            break;
        }
    }

    if (ep == NULL) {
        error_message("error: `alias`: missing closing quote");
        return NULL;
    }

    return val;
}
