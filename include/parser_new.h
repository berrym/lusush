#ifndef PARSER_NEW_H
#define PARSER_NEW_H

#include "node.h"
#include "scanner.h"
#include "errors.h"

/**
 * LUSUSH PARSER REWRITE - POSIX-COMPLIANT ARCHITECTURE
 * 
 * This is a complete rewrite of the lusush parser following strict POSIX.1-2017
 * shell grammar. It replaces the inconsistent multi-path parser with a clean
 * recursive descent implementation.
 */

// Forward declarations
typedef struct parser parser_t;
typedef struct source_location source_location_t;

// ============================================================================
// PARSER STATE MANAGEMENT
// ============================================================================

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

/**
 * Source location for error reporting
 */
typedef struct source_location {
    int line;                       // Line number (1-based)
    int column;                     // Column number (1-based)
    int position;                   // Absolute position in source
    const char *filename;           // Source filename (if any)
} source_location_t;

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
// TOKEN STREAM INTERFACE (NO PUSHBACK COMPLEXITY)
// ============================================================================

/**
 * Get current token without advancing
 */
token_t *parser_current_token(parser_t *parser);

/**
 * Get lookahead token without advancing
 */
token_t *parser_peek_token(parser_t *parser);

/**
 * Advance to next token
 */
void parser_advance(parser_t *parser);

/**
 * Check if current token matches type without consuming
 */
bool parser_check(parser_t *parser, token_type_t type);

/**
 * Consume token if it matches type, return success
 */
bool parser_match(parser_t *parser, token_type_t type);

/**
 * Expect specific token type, error if not found
 */
bool parser_expect(parser_t *parser, token_type_t type, const char *context);

/**
 * Check if at end of input
 */
bool parser_at_end(parser_t *parser);

// ============================================================================
// POSIX GRAMMAR PARSING FUNCTIONS
// ============================================================================

/**
 * Top-level parsing functions following POSIX grammar
 */

// complete_command : list separator_op | list
node_t *parse_complete_command(parser_t *parser);

// list : list separator_op and_or | and_or
node_t *parse_list(parser_t *parser);

// and_or : and_or AND_IF pipeline | and_or OR_IF pipeline | pipeline
node_t *parse_and_or(parser_t *parser);

// pipeline : pipe_sequence
node_t *parse_pipeline(parser_t *parser);

// pipe_sequence : pipe_sequence '|' command | command
node_t *parse_pipe_sequence(parser_t *parser);

// command : simple_command | compound_command
node_t *parse_command(parser_t *parser);

// simple_command : cmd_prefix cmd_word cmd_suffix | cmd_prefix cmd_word | ...
node_t *parse_simple_command(parser_t *parser);

// compound_command : if_clause | while_clause | for_clause | case_clause | until_clause
node_t *parse_compound_command(parser_t *parser);

// ============================================================================
// CONTROL STRUCTURE PARSING
// ============================================================================

// if_clause : IF compound_list THEN compound_list else_part FI | IF compound_list THEN compound_list FI
node_t *parse_if_clause(parser_t *parser);

// while_clause : WHILE compound_list DO compound_list DONE
node_t *parse_while_clause(parser_t *parser);

// for_clause : FOR name linebreak DO compound_list DONE | FOR name linebreak IN wordlist linebreak DO compound_list DONE
node_t *parse_for_clause(parser_t *parser);

// case_clause : CASE WORD linebreak IN case_list ESAC | CASE WORD linebreak IN ESAC
node_t *parse_case_clause(parser_t *parser);

// until_clause : UNTIL compound_list DO compound_list DONE
node_t *parse_until_clause(parser_t *parser);

// ============================================================================
// COMPOUND LIST PARSING
// ============================================================================

// compound_list : linebreak term | linebreak term separator_op
node_t *parse_compound_list(parser_t *parser);

// term : term separator and_or | and_or
node_t *parse_term(parser_t *parser);

// ============================================================================
// UTILITY PARSING FUNCTIONS
// ============================================================================

/**
 * Parse linebreak (newlines or empty)
 */
bool parse_linebreak(parser_t *parser);

/**
 * Parse word list for for loops and case statements
 */
node_t *parse_wordlist(parser_t *parser);

/**
 * Parse case pattern list
 */
node_t *parse_case_list(parser_t *parser);

/**
 * Parse case item (pattern) commands ;;)
 */
node_t *parse_case_item(parser_t *parser);

/**
 * Parse redirection
 */
node_t *parse_redirection(parser_t *parser);

/**
 * Parse assignment
 */
node_t *parse_assignment(parser_t *parser);

// ============================================================================
// ERROR HANDLING
// ============================================================================

/**
 * Report parser error with context
 */
void parser_error(parser_t *parser, const char *format, ...);

/**
 * Report parser error with suggestion
 */
void parser_error_with_suggestion(parser_t *parser, const char *suggestion, const char *format, ...);

/**
 * Attempt error recovery by skipping to synchronization point
 */
bool parser_synchronize(parser_t *parser);

/**
 * Get current source location for error reporting
 */
source_location_t parser_get_location(parser_t *parser);

// ============================================================================
// MAIN ENTRY POINTS
// ============================================================================

/**
 * Parse and execute a complete command from source (main entry point)
 */
int parse_and_execute_new(source_t *src);

/**
 * Parse a complete command and return AST (for testing)
 */
node_t *parse_command_new(source_t *src);

#endif /* PARSER_NEW_H */
