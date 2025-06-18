#include "../include/token_pushback.h"
#include "../include/errors.h"
#include <stdlib.h>
#include <string.h>

token_pushback_manager_t *create_pushback_manager(size_t capacity) {
    if (capacity == 0) {
        capacity = DEFAULT_PUSHBACK_CAPACITY;
    }
    
    token_pushback_manager_t *mgr = calloc(1, sizeof(token_pushback_manager_t));
    if (!mgr) {
        error_syscall("create_pushback_manager: failed to allocate manager");
        return NULL;
    }
    
    mgr->tokens = calloc(capacity, sizeof(token_t *));
    if (!mgr->tokens) {
        error_syscall("create_pushback_manager: failed to allocate token buffer");
        free(mgr);
        return NULL;
    }
    
    mgr->capacity = capacity;
    mgr->count = 0;
    mgr->head = 0;
    mgr->tail = 0;
    
    return mgr;
}

void destroy_pushback_manager(token_pushback_manager_t *mgr) {
    if (!mgr) {
        return;
    }
    
    // Free all remaining tokens
    clear_pushback(mgr);
    
    // Free the token array
    if (mgr->tokens) {
        free(mgr->tokens);
    }
    
    // Free the manager itself
    free(mgr);
}

int pushback_token(token_pushback_manager_t *mgr, token_t *tok) {
    if (!mgr || !tok) {
        return -1;
    }
    
    if (is_pushback_full(mgr)) {
        error_message("warning: token pushback buffer full, dropping oldest token");
        // Remove the oldest token to make room
        token_t *old_tok = mgr->tokens[mgr->head];
        if (old_tok && old_tok != &eof_token) {
            free_token(old_tok);
        }
        mgr->head = (mgr->head + 1) % mgr->capacity;
        mgr->count--;
    }
    
    // Add the new token at the tail
    mgr->tokens[mgr->tail] = tok;
    mgr->tail = (mgr->tail + 1) % mgr->capacity;
    mgr->count++;
    
    return 0;
}

token_t *pop_token(token_pushback_manager_t *mgr) {
    if (!mgr || is_pushback_empty(mgr)) {
        return NULL;
    }
    
    // Get the most recently pushed token (LIFO)
    mgr->tail = (mgr->tail + mgr->capacity - 1) % mgr->capacity;
    token_t *tok = mgr->tokens[mgr->tail];
    mgr->tokens[mgr->tail] = NULL;
    mgr->count--;
    
    return tok;
}

token_t *peek_token(token_pushback_manager_t *mgr, size_t offset) {
    if (!mgr || offset >= mgr->count) {
        return NULL;
    }
    
    // Calculate index for the token at the given offset from the most recent
    size_t index = (mgr->tail + mgr->capacity - 1 - offset) % mgr->capacity;
    return mgr->tokens[index];
}

size_t pushback_count(token_pushback_manager_t *mgr) {
    return mgr ? mgr->count : 0;
}

void clear_pushback(token_pushback_manager_t *mgr) {
    if (!mgr) {
        return;
    }
    
    // Free all tokens in the buffer
    while (!is_pushback_empty(mgr)) {
        token_t *tok = pop_token(mgr);
        if (tok && tok != &eof_token) {
            free_token(tok);
        }
    }
    
    // Reset indices
    mgr->head = 0;
    mgr->tail = 0;
    mgr->count = 0;
}

int is_pushback_empty(token_pushback_manager_t *mgr) {
    return !mgr || mgr->count == 0;
}

int is_pushback_full(token_pushback_manager_t *mgr) {
    return mgr && mgr->count >= mgr->capacity;
}
