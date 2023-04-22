#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "errors.h"
#include "lusush.h"
#include "strings.h"
#include "alias.h"
#include "dict.h"

dict_s *aliases = NULL;           // Dictionary for storing aliases

/**
 * init_aliases:
 *      Initialization code for aliases dictionary, set some aliases.
 */
void init_aliases(void)
{
    if (!aliases)
        aliases = dict_create();

    set_alias("ll", "ls -alF");
    set_alias("..", "cd ..");
    set_alias("...", "cd ../../");
}

/**
 * free_aliases:
 *      Delete the entire alias dictionary.
 */
void free_aliases(void)
{
    dict_destroy(aliases);
}

/**
 * lookup_alias:
 *      Find the alias value associated with a given key name,
 */
char *lookup_alias(const char *key)
{
    char *val = dict_search(aliases, key);
    return val;
}

/**
 * print_aliases:
 *      Print out the entire dictionary table of aliases.
 */
void print_aliases(void) {
    printf("aliases:\n");
    print_dict(aliases);
}

/**
 * set_alias:
 *      Insert a new key-value pair into the dictionary table.
 */
bool set_alias(const char *key, const char *val)
{
    if (!dict_insert(aliases, key, val))
        return false;

    return true;
}

/**
 * unset_alias:
 *      Remove a record from the dictionary table.
 */
void unset_alias(const char *key)
{
    dict_delete(aliases, key);
}

/**
 * valid_alias_name:
 *      Check that an alias key name consists of valid characters.
 */
bool valid_alias_name(const char *key)
{
    const char *p = key;

    if (!*p)
        return false;

    while (*p) {
        if (isalnum((int)*p)
            || *p == '.'
            || *p == '_'
            || *p == '!'
            || *p == '%'
            || *p == ','
            || *p == '@') {
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
void alias_usage(void)
{
    fprintf(stderr, "usage:\talias (print a list of all aliases)\n"
            "\talias name (print the value of an alias)\n"
            "\talias name='replacement text' (set an alias)\n");
}

/**
 * unalias_usage:
 *      Print how to use the builtin unalias command.
 */
void unalias_usage(void)
{
    fprintf(stderr, "usage:\tunalias name (unset alias with key name)\n");
}

/**
 * src_str_from_argv:
 *      Convert elements of an argument vector into a single string
 *      seperating arguments with the string value given in the sep argument.
 */
char *src_str_from_argv(size_t argc, char **argv, const char *sep)
{
    char *src = NULL;

    src = alloc_str(MAXLINE + 1, false);
    if (!src) {
        error_message("error: unable to allocate source string");
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
const char find_opening_quote_type(char *src)
{
    for (char *p = src; *p; p++)
        if (*p == '\'' || *p == '\"')
            return (const char)*p;

    return '\0';
}

/**
 * parse_alias_var_name:
 *      Parse the word before an equal sign in a source string that represents the alias key.
 */
char *parse_alias_var_name(char *src)
{
    char *sp = NULL, *ep = NULL, *var = NULL;
    const char delim = '=';
    char argv[1024][MAXLINE] = { '\0' };
    size_t tok_count = 0, char_count = 0;

    for (char *p = src; p; p++) {
        str_skip_whitespace(p);
        sp = p;

        while (!isspace((int)*p) && *p != delim)
            argv[tok_count][char_count] = *p, p++, char_count++;

        null_terminate_str(argv[tok_count]);
        tok_count++, char_count = 0;

        if (*p == delim) {
            ep = p;
            p = sp;

            while (p < ep) {
                if (!isspace((int)*p)) {
                    argv[tok_count][char_count] = *p;
                    char_count++, p++;
                } else {
                    null_terminate_str(argv[tok_count]);
                    tok_count++, char_count = 0;
                }
            }

            while (!*argv[tok_count])
                tok_count--;

            var = strdup(argv[tok_count]);
            if (!var) {
                error_message("error: unable to copy alias key name");
                break;
            }

            return var;
        }
    }

    return NULL;
}

/**
 * parse_alias_var_value:
 *      Parse a substring between quotes that represents the alias value.
 */
char *parse_alias_var_value(char *src, const char delim)
{
    char *val = NULL, *sp = NULL, *ep = NULL;

    if (!delim) {
        error_message("alias: not properly quoted");
        return NULL;
    }

    for (char *p = src; *p; p++) {               // for each char in line
        if (!sp && *p == delim)                  // find 1st delim
            sp = p, sp++;                        // set start ptr
        else if (!ep && *p == delim)             // find 2nd delim
            ep = p;                              // set end ptr

        if (sp && ep) {                          // if both set
            char substr[ep - sp + 1];            // declare substr
            p = sp;

            for (size_t i = 0; p < ep; i++, p++) // copy to substr
                substr[i] = *p;
            substr[ep - sp] = '\0';              // nul-terminate

            val = strdup(substr);
            if (!val) {
                error_message("alias: unable to duplicate substring");
                return NULL;
            }
            break;
        }
    }

    if (!ep) {
        error_message("alias: unbalanced quotes");
        return NULL;
    }
    sp = ep = NULL;

    return val;
}
