/**
 * LUSUSH PARSER REWRITE - POSIX GRAMMAR IMPLEMENTATION
 * 
 * This file implements the POSIX shell grammar parsing functions using
 * the existing node_t structure for compatibility.
 */

#include "../include/parser_new_simple.h"
#include "../include/node.h"
#include "../include/scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

// ============================================================================
// PARSER ERROR HANDLING
// ============================================================================

static void parser_error(parser_t *parser, const char *format, ...) {
    if (!parser || !format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // For now, just print to stderr
    // In a full implementation, this would use the error context
    fprintf(stderr, "Parser error: %s\n", buffer);
    
    va_end(args);
}

// ============================================================================
// TOKEN STREAM MANAGEMENT
// ============================================================================

static void parser_advance(parser_t *parser) {
    if (!parser || !parser->source) {
        return;
    }
    
    // Move current to previous, lookahead to current
    if (parser->current_token && parser->current_token != &eof_token) {
        free_token(parser->current_token);
    }
    
    parser->current_token = parser->lookahead_token;
    parser->lookahead_token = tokenize(parser->source);
    
    if (!parser->lookahead_token) {
        parser->lookahead_token = &eof_token;
    }
}

static bool parser_match(parser_t *parser, token_type_t expected) {
    if (!parser || !parser->current_token) {
        return false;
    }
    
    if (parser->current_token->type == expected) {
        parser_advance(parser);
        return true;
    }
    
    return false;
}

static bool parser_expect(parser_t *parser, token_type_t expected, const char *context) {
    if (!parser || !parser->current_token) {
        parser_error(parser, "Unexpected end of input in %s", context ? context : "unknown");
        return false;
    }
    
    if (parser->current_token->type != expected) {
        parser_error(parser, "Expected token type %d in %s, got %d", 
                    expected, context ? context : "unknown", parser->current_token->type);
        return false;
    }
    
    parser_advance(parser);
    return true;
}

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static node_t *parse_complete_command(parser_t *parser);
static node_t *parse_list(parser_t *parser);
static node_t *parse_and_or(parser_t *parser);
static node_t *parse_pipeline(parser_t *parser);
static node_t *parse_pipe_sequence(parser_t *parser);
static node_t *parse_command(parser_t *parser);
static node_t *parse_simple_command(parser_t *parser);
static node_t *parse_compound_command(parser_t *parser);
static node_t *parse_compound_list(parser_t *parser);
static node_t *parse_brace_group(parser_t *parser);
static node_t *parse_subshell(parser_t *parser);
static node_t *parse_if_clause(parser_t *parser);
static node_t *parse_while_clause(parser_t *parser);
static node_t *parse_for_clause(parser_t *parser);
static node_t *parse_case_clause(parser_t *parser);
static node_t *parse_until_clause(parser_t *parser);
static node_t *parse_function_def(parser_t *parser);

// Helper functions
static bool is_function_definition(parser_t *parser);
static bool is_assignment(const token_t *token);
static bool is_redirection(const token_t *token);

// ============================================================================
// MAIN PARSING INTERFACE
// ============================================================================

/**
 * Parse input from the given source
 */
node_t *parser_parse(parser_t *parser) {
    if (!parser) {
        return NULL;
    }
    
    // Check recursion limit
    if (parser->recursion_depth >= parser->max_recursion) {
        parser_error(parser, "Maximum recursion depth exceeded");
        return NULL;
    }
    
    parser->recursion_depth++;
    
    node_t *result = parse_complete_command(parser);
    
    parser->recursion_depth--;
    
    return result;
}

// ============================================================================
// POSIX GRAMMAR PARSING FUNCTIONS
// ============================================================================

/**
 * Parse a complete command (top-level production)
 * complete_command : list separator_op
 *                  | list
 */
static node_t *parse_complete_command(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    node_t *list_node = parse_list(parser);
    if (!list_node) {
        return NULL;
    }

    // Check for optional separator_op
    if (parser->current_token && 
        (parser->current_token->type == TOKEN_SEMI ||
         parser->current_token->type == TOKEN_AND ||
         parser->current_token->type == TOKEN_NEWLINE)) {
        parser_advance(parser);
    }

    return list_node;
}

/**
 * Parse a list (sequence of and_or commands)
 * list : list separator_op and_or
 *      | and_or
 */
static node_t *parse_list(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    node_t *left = parse_and_or(parser);
    if (!left) {
        return NULL;
    }

    // Handle list continuation
    while (parser->current_token && 
           (parser->current_token->type == TOKEN_SEMI ||
            parser->current_token->type == TOKEN_AND ||
            parser->current_token->type == TOKEN_NEWLINE)) {
        
        token_type_t separator = parser->current_token->type;
        parser_advance(parser);

        // Skip any additional newlines
        while (parser->current_token && 
               parser->current_token->type == TOKEN_NEWLINE) {
            parser_advance(parser);
        }

        // Parse next and_or
        node_t *right = parse_and_or(parser);
        if (!right) {
            break; // End of list
        }

        // Create a compound node to hold the sequence
        node_t *sequence_node = new_node(NODE_COMMAND);
        if (!sequence_node) {
            free_node_tree(left);
            free_node_tree(right);
            parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        // Set up the sequence structure
        // For now, use a simple approach - add both as children
        add_child_node(sequence_node, left);
        add_child_node(sequence_node, right);
        
        // Store separator info in node value
        if (separator == TOKEN_AND) {
            set_node_val_str(sequence_node, "&");
        } else {
            set_node_val_str(sequence_node, ";");
        }
        
        left = sequence_node;
    }

    return left;
}

/**
 * Parse and_or (pipeline with && or ||)
 * and_or : and_or AND_IF pipeline
 *        | and_or OR_IF pipeline  
 *        | pipeline
 */
static node_t *parse_and_or(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    node_t *left = parse_pipeline(parser);
    if (!left) {
        return NULL;
    }

    // Handle && and || operators
    while (parser->current_token && 
           (parser->current_token->type == TOKEN_AND_IF ||
            parser->current_token->type == TOKEN_OR_IF)) {
        
        token_type_t op = parser->current_token->type;
        parser_advance(parser);

        // Skip newlines after operator
        while (parser->current_token && 
               parser->current_token->type == TOKEN_NEWLINE) {
            parser_advance(parser);
        }

        node_t *right = parse_pipeline(parser);
        if (!right) {
            parser_error(parser, "Expected pipeline after logical operator");
            free_node_tree(left);
            return NULL;
        }

        // Create logical operation node
        node_t *logical_node = new_node(NODE_COMMAND);
        if (!logical_node) {
            free_node_tree(left);
            free_node_tree(right);
            parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        add_child_node(logical_node, left);
        add_child_node(logical_node, right);
        
        // Store operator info
        if (op == TOKEN_AND_IF) {
            set_node_val_str(logical_node, "&&");
        } else {
            set_node_val_str(logical_node, "||");
        }
        
        left = logical_node;
    }

    return left;
}

/**
 * Parse pipeline
 * pipeline : bang_opt pipe_sequence
 * bang_opt : '!' | empty
 */
static node_t *parse_pipeline(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    bool negated = false;
    
    // Check for bang (!)
    if (parser->current_token->type == TOKEN_KEYWORD_BANG) {
        negated = true;
        parser_advance(parser);
    }

    node_t *pipe_seq = parse_pipe_sequence(parser);
    if (!pipe_seq) {
        return NULL;
    }

    // If negated, mark it somehow (for now, just return the sequence)
    // In a full implementation, we'd wrap this in a special node
    if (negated) {
        // For now, we'll just pass through - negation can be handled at execution
        // TODO: Add proper negation support to node structure
    }

    return pipe_seq;
}

/**
 * Parse pipe_sequence
 * pipe_sequence : pipe_sequence '|' linebreak command
 *               | command
 */
static node_t *parse_pipe_sequence(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    node_t *left = parse_command(parser);
    if (!left) {
        return NULL;
    }

    // Handle pipe continuation
    while (parser->current_token && parser->current_token->type == TOKEN_PIPE) {
        parser_advance(parser);

        // Skip linebreak (newlines) after pipe
        while (parser->current_token && 
               parser->current_token->type == TOKEN_NEWLINE) {
            parser_advance(parser);
        }

        node_t *right = parse_command(parser);
        if (!right) {
            parser_error(parser, "Expected command after pipe");
            free_node_tree(left);
            return NULL;
        }

        // Create pipe node
        node_t *pipe_node = new_node(NODE_PIPE);
        if (!pipe_node) {
            free_node_tree(left);
            free_node_tree(right);
            parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        add_child_node(pipe_node, left);
        add_child_node(pipe_node, right);
        
        left = pipe_node;
    }

    return left;
}

/**
 * Parse command
 * command : simple_command
 *         | compound_command
 *         | compound_command redirect_list
 *         | function_def
 */
static node_t *parse_command(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Check for compound commands first
    switch (parser->current_token->type) {
        case TOKEN_KEYWORD_LBRACE:   // { ... }
        case TOKEN_LEFT_PAREN:       // ( ... )
        case TOKEN_KEYWORD_IF:       // if ... fi
        case TOKEN_KEYWORD_WHILE:    // while ... done
        case TOKEN_KEYWORD_FOR:      // for ... done
        case TOKEN_KEYWORD_CASE:     // case ... esac
        case TOKEN_KEYWORD_UNTIL:    // until ... done
            return parse_compound_command(parser);
        
        case TOKEN_WORD:
            // Could be function definition or simple command
            // Look ahead for function syntax: name '(' ')'
            if (is_function_definition(parser)) {
                return parse_function_def(parser);
            }
            // Fall through to simple command
            __attribute__((fallthrough));
            
        default:
            return parse_simple_command(parser);
    }
}

/**
 * Parse simple_command (simplified version)
 */
static node_t *parse_simple_command(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    node_t *cmd_node = new_node(NODE_COMMAND);
    if (!cmd_node) {
        parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    // For now, handle just basic word sequences
    // Parse command name and arguments
    if (parser->current_token && parser->current_token->type == TOKEN_WORD) {
        // Set command name
        set_node_val_str(cmd_node, parser->current_token->text);
        parser_advance(parser);

        // Parse arguments
        while (parser->current_token && parser->current_token->type == TOKEN_WORD) {
            node_t *arg_node = new_node(NODE_VAR);
            if (!arg_node) {
                free_node_tree(cmd_node);
                parser_error(parser, "Memory allocation failed");
                return NULL;
            }
            
            set_node_val_str(arg_node, parser->current_token->text);
            add_child_node(cmd_node, arg_node);
            
            parser_advance(parser);
        }
    }

    return cmd_node;
}

/**
 * Parse compound_command
 */
static node_t *parse_compound_command(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    switch (parser->current_token->type) {
        case TOKEN_KEYWORD_LBRACE:
            return parse_brace_group(parser);
        case TOKEN_LEFT_PAREN:
            return parse_subshell(parser);
        case TOKEN_KEYWORD_FOR:
            return parse_for_clause(parser);
        case TOKEN_KEYWORD_CASE:
            return parse_case_clause(parser);
        case TOKEN_KEYWORD_IF:
            return parse_if_clause(parser);
        case TOKEN_KEYWORD_WHILE:
            return parse_while_clause(parser);
        case TOKEN_KEYWORD_UNTIL:
            return parse_until_clause(parser);
        default:
            parser_error(parser, "Expected compound command");
            return NULL;
    }
}

/**
 * Parse compound_list (used in compound commands)
 */
static node_t *parse_compound_list(parser_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Skip leading newlines
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        parser_advance(parser);
    }

    return parse_list(parser);
}

/**
 * Parse brace_group: { compound_list }
 */
static node_t *parse_brace_group(parser_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_LBRACE) {
        parser_error(parser, "Expected '{'");
        return NULL;
    }

    parser_advance(parser); // consume '{'

    // Skip newlines
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        parser_advance(parser);
    }

    node_t *body = parse_compound_list(parser);
    if (!body) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_RBRACE) {
        free_node_tree(body);
        parser_error(parser, "Expected '}'");
        return NULL;
    }

    parser_advance(parser); // consume '}'

    // For now, just return the body
    // In a full implementation, we'd wrap this in a brace group node
    return body;
}

/**
 * Parse subshell: ( compound_list )
 */
static node_t *parse_subshell(parser_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_LEFT_PAREN) {
        parser_error(parser, "Expected '('");
        return NULL;
    }

    parser_advance(parser); // consume '('

    // Skip newlines
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        parser_advance(parser);
    }

    node_t *body = parse_compound_list(parser);
    if (!body) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_RIGHT_PAREN) {
        free_node_tree(body);
        parser_error(parser, "Expected ')'");
        return NULL;
    }

    parser_advance(parser); // consume ')'

    // For now, just return the body
    // In a full implementation, we'd wrap this in a subshell node
    return body;
}

/**
 * Parse if_clause: if compound_list then compound_list [else_part] fi
 */
static node_t *parse_if_clause(parser_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_IF) {
        parser_error(parser, "Expected 'if'");
        return NULL;
    }

    parser_advance(parser); // consume 'if'

    node_t *condition = parse_compound_list(parser);
    if (!condition) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_THEN) {
        free_node_tree(condition);
        parser_error(parser, "Expected 'then'");
        return NULL;
    }

    parser_advance(parser); // consume 'then'

    node_t *then_body = parse_compound_list(parser);
    if (!then_body) {
        free_node_tree(condition);
        return NULL;
    }

    node_t *else_body = NULL;

    // Check for else part
    if (parser->current_token) {
        if (parser->current_token->type == TOKEN_KEYWORD_ELIF) {
            // Recursively parse elif as nested if
            else_body = parse_if_clause(parser);
            if (!else_body) {
                free_node_tree(condition);
                free_node_tree(then_body);
                return NULL;
            }
        } else if (parser->current_token->type == TOKEN_KEYWORD_ELSE) {
            parser_advance(parser); // consume 'else'
            else_body = parse_compound_list(parser);
            if (!else_body) {
                free_node_tree(condition);
                free_node_tree(then_body);
                return NULL;
            }
        }
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_FI) {
        free_node_tree(condition);
        free_node_tree(then_body);
        free_node_tree(else_body);
        parser_error(parser, "Expected 'fi'");
        return NULL;
    }

    parser_advance(parser); // consume 'fi'

    node_t *if_node = new_node(NODE_IF);
    if (!if_node) {
        free_node_tree(condition);
        free_node_tree(then_body);
        free_node_tree(else_body);
        parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    add_child_node(if_node, condition);
    add_child_node(if_node, then_body);
    if (else_body) {
        add_child_node(if_node, else_body);
    }

    return if_node;
}

/**
 * Parse while_clause: while compound_list do compound_list done
 */
static node_t *parse_while_clause(parser_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_WHILE) {
        parser_error(parser, "Expected 'while'");
        return NULL;
    }

    parser_advance(parser); // consume 'while'

    node_t *condition = parse_compound_list(parser);
    if (!condition) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_DO) {
        free_node_tree(condition);
        parser_error(parser, "Expected 'do'");
        return NULL;
    }

    parser_advance(parser); // consume 'do'

    node_t *body = parse_compound_list(parser);
    if (!body) {
        free_node_tree(condition);
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_DONE) {
        free_node_tree(condition);
        free_node_tree(body);
        parser_error(parser, "Expected 'done'");
        return NULL;
    }

    parser_advance(parser); // consume 'done'

    node_t *while_node = new_node(NODE_WHILE);
    if (!while_node) {
        free_node_tree(condition);
        free_node_tree(body);
        parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    add_child_node(while_node, condition);
    add_child_node(while_node, body);

    return while_node;
}

/**
 * Parse for_clause: for name [in word_list] do compound_list done
 */
static node_t *parse_for_clause(parser_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_FOR) {
        parser_error(parser, "Expected 'for'");
        return NULL;
    }

    parser_advance(parser); // consume 'for'

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_WORD) {
        parser_error(parser, "Expected variable name after 'for'");
        return NULL;
    }

    // Store variable name
    char *var_name = strdup(parser->current_token->text);
    parser_advance(parser); // consume variable name

    // Parse optional 'in word_list'
    node_t *word_list = NULL;
    if (parser->current_token && 
        parser->current_token->type == TOKEN_KEYWORD_IN) {
        parser_advance(parser); // consume 'in'

        // Create word list node
        word_list = new_node(NODE_VAR);
        if (!word_list) {
            free(var_name);
            parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        // Parse words
        while (parser->current_token && 
               parser->current_token->type == TOKEN_WORD) {
            node_t *word_node = new_node(NODE_VAR);
            if (!word_node) {
                free(var_name);
                free_node_tree(word_list);
                parser_error(parser, "Memory allocation failed");
                return NULL;
            }
            
            set_node_val_str(word_node, parser->current_token->text);
            add_child_node(word_list, word_node);
            
            parser_advance(parser);
        }
    }

    // Skip optional semicolon and newlines
    if (parser->current_token && 
        parser->current_token->type == TOKEN_SEMI) {
        parser_advance(parser);
    }
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        parser_advance(parser);
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_DO) {
        free(var_name);
        free_node_tree(word_list);
        parser_error(parser, "Expected 'do'");
        return NULL;
    }

    parser_advance(parser); // consume 'do'

    node_t *body = parse_compound_list(parser);
    if (!body) {
        free(var_name);
        free_node_tree(word_list);
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_KEYWORD_DONE) {
        free(var_name);
        free_node_tree(word_list);
        free_node_tree(body);
        parser_error(parser, "Expected 'done'");
        return NULL;
    }

    parser_advance(parser); // consume 'done'

    node_t *for_node = new_node(NODE_FOR);
    if (!for_node) {
        free(var_name);
        free_node_tree(word_list);
        free_node_tree(body);
        parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    // Store variable name in node
    set_node_val_str(for_node, var_name);
    free(var_name);

    // Add children: word_list (if any), body
    if (word_list) {
        add_child_node(for_node, word_list);
    }
    add_child_node(for_node, body);

    return for_node;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Check if current token sequence represents a function definition
 */
static bool is_function_definition(parser_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_WORD) {
        return false;
    }

    // This is a simplified check - in a full implementation,
    // we'd need proper lookahead
    return false; // For now, disable function definitions
}

/**
 * Check if token represents an assignment
 */
static bool is_assignment(const token_t *token) {
    if (!token || token->type != TOKEN_WORD || !token->text) {
        return false;
    }

    // Look for '=' in the token text
    const char *eq = strchr(token->text, '=');
    return (eq != NULL && eq != token->text); // '=' not at start
}

/**
 * Check if token represents a redirection
 */
static bool is_redirection(const token_t *token) {
    if (!token) {
        return false;
    }

    switch (token->type) {
        case TOKEN_LESS:       // <
        case TOKEN_GREAT:      // >
        case TOKEN_DLESS:      // <<
        case TOKEN_DGREAT:     // >>
        case TOKEN_LESSAND:    // <&
        case TOKEN_GREATAND:   // >&
        case TOKEN_LESSGREAT:  // <>
        case TOKEN_CLOBBER:    // >|
            return true;
        default:
            return false;
    }
}

// Placeholder implementations for compound commands not yet implemented
static node_t *parse_case_clause(parser_t *parser) {
    parser_error(parser, "Case statements not yet implemented");
    return NULL;
}

static node_t *parse_until_clause(parser_t *parser) {
    parser_error(parser, "Until loops not yet implemented");
    return NULL;
}

static node_t *parse_function_def(parser_t *parser) {
    parser_error(parser, "Function definitions not yet implemented");
    return NULL;
}

// ============================================================================
// PARSER STATE MANAGEMENT
// ============================================================================

/**
 * Create and initialize a new parser
 */
parser_t *parser_create(source_t *source, void *errors) {
    if (!source) {
        return NULL;
    }
    
    parser_t *parser = malloc(sizeof(parser_t));
    if (!parser) {
        return NULL;
    }
    
    parser->source = source;
    parser->current_token = NULL;
    parser->lookahead_token = NULL;
    parser->errors = errors;
    parser->recursion_depth = 0;
    parser->max_recursion = 100;  // Reasonable limit
    parser->in_function = false;
    parser->in_case = false;
    parser->interactive = false;
    
    // Initialize with first two tokens
    parser_advance(parser);  // Load current
    parser_advance(parser);  // Load lookahead
    
    return parser;
}

/**
 * Clean up parser resources
 */
void parser_destroy(parser_t *parser) {
    if (!parser) {
        return;
    }
    
    if (parser->current_token && parser->current_token != &eof_token) {
        free_token(parser->current_token);
    }
    
    if (parser->lookahead_token && parser->lookahead_token != &eof_token) {
        free_token(parser->lookahead_token);
    }
    
    free(parser);
}

/**
 * Reset parser state for new input
 */
void parser_reset(parser_t *parser, source_t *source) {
    if (!parser || !source) {
        return;
    }
    
    // Clean up old tokens
    if (parser->current_token && parser->current_token != &eof_token) {
        free_token(parser->current_token);
    }
    
    if (parser->lookahead_token && parser->lookahead_token != &eof_token) {
        free_token(parser->lookahead_token);
    }
    
    parser->source = source;
    parser->current_token = NULL;
    parser->lookahead_token = NULL;
    parser->recursion_depth = 0;
    parser->in_function = false;
    parser->in_case = false;
    
    // Load first two tokens
    parser_advance(parser);
    parser_advance(parser);
}
