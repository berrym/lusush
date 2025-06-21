#ifndef PARSER_NEW_SIMPLE_H
#define PARSER_NEW_SIMPLE_H

#include "node.h"
#include "scanner_old.h"

/**
 * LUSUSH PARSER REWRITE - SIMPLIFIED HEADER
 * 
 * Simplified interface for the new POSIX-compliant parser.
 */

// Forward declarations
typedef struct parser parser_t;

/**
 * Parser state structure - manages token stream and parsing context
 */
typedef struct parser {
    source_t *source;               // Input source
    token_t *current_token;         // Current token being processed
    token_t *lookahead_token;       // One token lookahead
    void *errors;                   // Error reporting context (simplified)
    int recursion_depth;            // Stack overflow protection
    int max_recursion;              // Maximum allowed recursion
    bool in_function;               // Function definition context
    bool in_case;                   // Case statement context
    bool interactive;               // Interactive mode flag
} parser_t;

// ============================================================================
// PARSER INITIALIZATION AND CLEANUP
// ============================================================================

/**
 * Create and initialize a new parser
 */
parser_t *parser_create(source_t *source, void *errors);

/**
 * Clean up parser resources
 */
void parser_destroy(parser_t *parser);

/**
 * Reset parser state for new input
 */
void parser_reset(parser_t *parser, source_t *source);

// ============================================================================
// PARSER INTERFACE
// ============================================================================

/**
 * Main parsing interface - parses input and returns AST
 */
node_t *parser_parse(parser_t *parser);

#endif /* PARSER_NEW_SIMPLE_H */
