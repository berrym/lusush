#ifndef TOKEN_PUSHBACK_H
#define TOKEN_PUSHBACK_H

#include "scanner.h"
#include <stddef.h>

// Default capacity for token pushback buffer
#define DEFAULT_PUSHBACK_CAPACITY 16

// Token pushback manager structure
typedef struct {
    token_t **tokens;     // Circular buffer of tokens
    size_t capacity;      // Maximum number of tokens
    size_t count;         // Current number of tokens
    size_t head;          // Index of first token (for popping)
    size_t tail;          // Index for next push
} token_pushback_manager_t;

// Create a new token pushback manager
token_pushback_manager_t *create_pushback_manager(size_t capacity);

// Destroy a token pushback manager and free all tokens
void destroy_pushback_manager(token_pushback_manager_t *mgr);

// Push a token back onto the stack (LIFO order)
int pushback_token(token_pushback_manager_t *mgr, token_t *tok);

// Pop the most recently pushed token (LIFO order)
token_t *pop_token(token_pushback_manager_t *mgr);

// Peek at a token without removing it (offset 0 = most recent)
token_t *peek_token(token_pushback_manager_t *mgr, size_t offset);

// Get the number of tokens currently in pushback
size_t pushback_count(token_pushback_manager_t *mgr);

// Clear all pushed back tokens
void clear_pushback(token_pushback_manager_t *mgr);

// Check if pushback buffer is empty
int is_pushback_empty(token_pushback_manager_t *mgr);

// Check if pushback buffer is full
int is_pushback_full(token_pushback_manager_t *mgr);

#endif
