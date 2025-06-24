#ifndef EXPAND_H
#define EXPAND_H

#include <stdbool.h>

// Expansion mode flags
#define EXPAND_NORMAL 0x00  // Normal word expansion
#define EXPAND_ALIAS 0x01   // Expanding an alias
#define EXPAND_NOQUOTE 0x02 // Don't remove quotes
#define EXPAND_NOVAR 0x04   // Don't expand variables
#define EXPAND_NOCMD 0x08   // Don't expand command substitutions
#define EXPAND_NOGLOB 0x10  // Don't expand globs/wildcards

// Expansion context to replace global state
typedef struct {
    int mode;          // Expansion mode flags
    bool in_quotes;    // Whether we're inside quotes
    bool in_backticks; // Whether we're inside backticks
} expand_ctx_t;

// Initialize expansion context with default values
void expand_ctx_init(expand_ctx_t *ctx, int mode);

// Check if a specific expansion mode is enabled
bool expand_ctx_check(expand_ctx_t *ctx, int mode_flag);

#endif /* EXPAND_H */
