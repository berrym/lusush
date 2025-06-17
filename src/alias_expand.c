#include "../include/alias.h"
#include "../include/alias_expand.h"
#include "../include/expand.h"
#include "../include/errors.h"
#include "../include/strings.h"
#include "../include/lusush.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * expand_aliases_recursive:
 *     Expand an alias name recursively, with cycle detection.
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
    
    // Make a copy we can modify
    char *result = strdup(value);
    if (!result) {
        return NULL;
    }
    
    // Parse the first word of the alias value
    // This is a simplified approach - a real implementation would use
    // the shell's tokenizer to properly handle word splitting
    char *first_word = result;
    char *space = strchr(result, ' ');
    if (space) {
        *space = '\0';  // Temporarily split string
        
        // Check if the first word is also an alias
        char *recursive = expand_aliases_recursive(first_word, max_depth - 1);
        if (recursive) {
            // Rebuild the string with the expanded alias
            char *new_result = calloc(strlen(recursive) + strlen(space + 1) + 2, 1);
            if (new_result) {
                strcpy(new_result, recursive);
                strcat(new_result, " ");
                strcat(new_result, space + 1);
                free(result);
                free(recursive);
                return new_result;
            }
            free(recursive);
        }
        
        // Restore the string
        *space = ' ';
    } else {
        // Single word alias - check for recursive expansion
        char *recursive = expand_aliases_recursive(first_word, max_depth - 1);
        if (recursive) {
            free(result);
            return recursive;
        }
    }
    
    return result;
}

/**
 * expand_first_word_alias:
 *     Expand only the first word of a command line as an alias.
 *     This matches the POSIX behavior where only the first word
 *     of a simple command can be an alias.
 */
char *expand_first_word_alias(const char *command) {
    if (!command) {
        return NULL;
    }
    
    // Find the first word
    char *copy = strdup(command);
    if (!copy) {
        return NULL;
    }
    
    // Split at first space
    char *space = strchr(copy, ' ');
    if (space) {
        *space = '\0';
    }
    
    // Try to expand the alias
    char *alias_value = lookup_alias(copy);
    if (!alias_value) {
        free(copy);
        return strdup(command);  // No alias found, return original
    }
    
    // Construct the new command with the alias expansion
    char *result;
    if (space) {
        // Command had arguments - append them to the alias value
        result = calloc(strlen(alias_value) + strlen(space + 1) + 2, 1);
        if (result) {
            strcpy(result, alias_value);
            strcat(result, " ");
            strcat(result, space + 1);
        }
    } else {
        // Command was just the alias
        result = strdup(alias_value);
    }
    
    free(copy);
    return result;
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
