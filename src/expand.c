#include "../include/expand.h"
#include "../include/alias.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/strings.h"
#include "../include/symtable.h"
#include "../include/lusush.h"

#include <ctype.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// Initialize expansion context with default values
void expand_ctx_init(expand_ctx_t *ctx, int mode) {
    if (ctx) {
        ctx->mode = mode;
        ctx->in_quotes = false;
        ctx->in_backticks = false;
    }
}

// Check if a specific expansion mode is enabled
bool expand_ctx_check(expand_ctx_t *ctx, int mode_flag) {
    if (!ctx) {
        return false;
    }
    return (ctx->mode & mode_flag) != 0;
}

// Create a new word from a string
word_t *word_create(const char *str) {
    if (!str) {
        return NULL;
    }
    
    // Allocate word structure
    word_t *word = calloc(1, sizeof(word_t));
    if (!word) {
        return NULL;
    }
    
    // Allocate and copy the word text
    size_t len = strlen(str);
    word->data = calloc(len + 1, sizeof(char));
    if (!word->data) {
        free(word);
        return NULL;
    }
    
    strcpy(word->data, str);
    word->len = len;
    word->next = NULL;
    
    return word;
}

// Free a word list
void word_free_list(word_t *head) {
    while (head) {
        word_t *next = head->next;
        free(head->data);
        free(head);
        head = next;
    }
}

// Convert a word list to a string
char *word_list_to_string(word_t *head) {
    if (!head) {
        return NULL;
    }
    
    // Calculate total length
    size_t total_len = 0;
    for (word_t *w = head; w; w = w->next) {
        total_len += w->len + 1; // +1 for space or null
    }
    
    // Allocate and build string
    char *result = calloc(total_len + 1, sizeof(char));
    if (!result) {
        return NULL;
    }
    
    char *p = result;
    for (word_t *w = head; w; w = w->next) {
        strcpy(p, w->data);
        p += w->len;
        if (w->next) {
            *p++ = ' ';
        }
    }
    
    return result;
}



// Expand an alias recursively
char *expand_alias_recursive(const char *alias_name) {
    if (!alias_name) {
        return NULL;
    }
    
    // Look up the initial alias
    char *alias_value = lookup_alias(alias_name);
    if (!alias_value) {
        return NULL;
    }
    
    // Make a copy we can modify
    char *result = strdup(alias_value);
    if (!result) {
        return NULL;
    }
    
    // Recursively expand any further aliases
    // This is a simplified version - real implementation would need
    // to handle word tokenization and prevent infinite recursion
    
    return result;
}
