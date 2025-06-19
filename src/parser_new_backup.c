/**
 * LUSUSH PARSER REWRITE - CORE INFRASTRUCTURE
 * 
 * This file implements the foundational parser infrastructure for the new
 * POSIX-compliant recursive descent parser.
 */

#include "../include/parser_new.h"
#include "../include/node_new.h"
#include "../include/errors.h"
#include "../include/scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

// ============================================================================
// PARSER STATE MANAGEMENT
// ============================================================================

/**
 * Create and initialize a new parser
 */
parser_t *parser_create(source_t *source, error_context_t *errors) {
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
    parser->errors = errors ? errors : malloc(sizeof(error_context_t));
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
    
    // Don't free errors if it was passed in externally
    // This would need reference counting in a full implementation
    
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

// ============================================================================
// TOKEN STREAM INTERFACE (NO PUSHBACK COMPLEXITY)
// ============================================================================

/**
 * Get current token without advancing
 */
token_t *parser_current_token(parser_t *parser) {
    if (!parser) {
        return &eof_token;
    }
    
    return parser->current_token ? parser->current_token : &eof_token;
}

/**
 * Get lookahead token without advancing
 */
token_t *parser_peek_token(parser_t *parser) {
    if (!parser) {
        return &eof_token;
    }
    
    return parser->lookahead_token ? parser->lookahead_token : &eof_token;
}

/**
 * Advance to next token
 */
void parser_advance(parser_t *parser) {
    if (!parser || !parser->source) {
        return;
    }
    
    // Move lookahead to current
    if (parser->current_token && parser->current_token != &eof_token) {
        free_token(parser->current_token);
    }
    
    parser->current_token = parser->lookahead_token;
    
    // Get new lookahead token
    parser->lookahead_token = tokenize(parser->source);
    
    // Handle EOF
    if (!parser->lookahead_token) {
        parser->lookahead_token = &eof_token;
    }
}

/**
 * Check if current token matches type without consuming
 */
bool parser_check(parser_t *parser, token_type_t type) {
    if (!parser) {
        return false;
    }
    
    token_t *current = parser_current_token(parser);
    return current && current->type == type;
}

/**
 * Consume token if it matches type, return success
 */
bool parser_match(parser_t *parser, token_type_t type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

/**
 * Expect specific token type, error if not found
 */
bool parser_expect(parser_t *parser, token_type_t type, const char *context) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    
    // Generate error message
    token_t *current = parser_current_token(parser);
    const char *token_name = current ? token_type_to_string(current->type) : "EOF";
    const char *expected_name = token_type_to_string(type);
    
    parser_error(parser, "Expected %s in %s, but found %s", 
                 expected_name, context ? context : "command", token_name);
    
    return false;
}

/**
 * Check if at end of input
 */
bool parser_at_end(parser_t *parser) {
    if (!parser) {
        return true;
    }
    
    token_t *current = parser_current_token(parser);
    return !current || current == &eof_token || current->type == TOKEN_EOF;
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

/**
 * Report parser error with context
 */
void parser_error(parser_t *parser, const char *format, ...) {
    if (!parser || !format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    char message[512];
    vsnprintf(message, sizeof(message), format, args);
    
    va_end(args);
    
    // Get source location
    source_location_t loc = parser_get_location(parser);
    
    // Report error using existing error system
    if (parser->errors) {
        fprintf(stderr, "%s:%d:%d: error: %s\n", 
                loc.filename ? loc.filename : "<input>",
                loc.line, loc.column, message);
    } else {
        fprintf(stderr, "Parser error: %s\n", message);
    }
}

/**
 * Report parser error with suggestion
 */
void parser_error_with_suggestion(parser_t *parser, const char *suggestion, const char *format, ...) {
    if (!parser || !format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    char message[512];
    vsnprintf(message, sizeof(message), format, args);
    
    va_end(args);
    
    source_location_t loc = parser_get_location(parser);
    
    fprintf(stderr, "%s:%d:%d: error: %s\n", 
            loc.filename ? loc.filename : "<input>",
            loc.line, loc.column, message);
    
    if (suggestion) {
        fprintf(stderr, "%s:%d:%d: note: %s\n",
                loc.filename ? loc.filename : "<input>",
                loc.line, loc.column, suggestion);
    }
}

/**
 * Attempt error recovery by skipping to synchronization point
 */
bool parser_synchronize(parser_t *parser) {
    if (!parser) {
        return false;
    }
    
    // Skip tokens until we find a synchronization point
    while (!parser_at_end(parser)) {
        token_t *current = parser_current_token(parser);
        
        // Synchronization points: statement boundaries
        if (current->type == TOKEN_SEMI ||
            current->type == TOKEN_NEWLINE ||
            current->type == TOKEN_KEYWORD_FI ||
            current->type == TOKEN_KEYWORD_DONE ||
            current->type == TOKEN_KEYWORD_ESAC ||
            current->type == TOKEN_KEYWORD_THEN ||
            current->type == TOKEN_KEYWORD_ELSE ||
            current->type == TOKEN_KEYWORD_ELIF) {
            return true;
        }
        
        parser_advance(parser);
    }
    
    return false;
}

/**
 * Get current source location for error reporting
 */
source_location_t parser_get_location(parser_t *parser) {
    source_location_t loc = {0};
    
    if (!parser || !parser->source) {
        return loc;
    }
    
    // Calculate line and column from source position
    source_t *src = parser->source;
    int line = 1;
    int column = 1;
    
    for (int i = 0; i < src->pos && i < src->bufsize; i++) {
        if (src->buf[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
    
    loc.line = line;
    loc.column = column;
    loc.position = src->pos;
    loc.filename = src->srcname;  // If available
    
    return loc;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Convert token type to string for error messages
 */
const char *token_type_to_string(token_type_t type) {
    switch (type) {
        case TOKEN_WORD: return "word";
        case TOKEN_SEMI: return "';'";
        case TOKEN_NEWLINE: return "newline";
        case TOKEN_PIPE: return "'|'";
        case TOKEN_AND_IF: return "'&&'";
        case TOKEN_OR_IF: return "'||'";
        case TOKEN_KEYWORD_IF: return "'if'";
        case TOKEN_KEYWORD_THEN: return "'then'";
        case TOKEN_KEYWORD_ELSE: return "'else'";
        case TOKEN_KEYWORD_ELIF: return "'elif'";
        case TOKEN_KEYWORD_FI: return "'fi'";
        case TOKEN_KEYWORD_FOR: return "'for'";
        case TOKEN_KEYWORD_WHILE: return "'while'";
        case TOKEN_KEYWORD_UNTIL: return "'until'";
        case TOKEN_KEYWORD_DO: return "'do'";
        case TOKEN_KEYWORD_DONE: return "'done'";
        case TOKEN_KEYWORD_CASE: return "'case'";
        case TOKEN_KEYWORD_ESAC: return "'esac'";
        case TOKEN_KEYWORD_IN: return "'in'";
        case TOKEN_LEFT_PAREN: return "'('";
        case TOKEN_RIGHT_PAREN: return "')'";
        case TOKEN_EOF: return "end of file";
        default: return "unknown token";
    }
}

/**
 * Parse linebreak (newlines or empty)
 */
bool parse_linebreak(parser_t *parser) {
    if (!parser) {
        return false;
    }
    
    bool found_newline = false;
    
    while (parser_check(parser, TOKEN_NEWLINE)) {
        parser_advance(parser);
        found_newline = true;
    }
    
    // Linebreak can be empty, so always return true
    return true;
}

// ============================================================================
// RECURSION PROTECTION
// ============================================================================

/**
 * Check recursion depth before entering parsing function
 */
static bool parser_enter_recursion(parser_t *parser) {
    if (!parser) {
        return false;
    }
    
    if (parser->recursion_depth >= parser->max_recursion) {
        parser_error(parser, "Parser recursion limit exceeded (possible infinite recursion)");
        return false;
    }
    
    parser->recursion_depth++;
    return true;
}

/**
 * Exit recursion level
 */
static void parser_exit_recursion(parser_t *parser) {
    if (parser && parser->recursion_depth > 0) {
        parser->recursion_depth--;
    }
}

// ============================================================================
// MAIN ENTRY POINTS (STUBS FOR NOW)
// ============================================================================

/**
 * Parse and execute a complete command from source (main entry point)
 */
int parse_and_execute_new(source_t *src) {
    if (!src) {
        return 1;
    }
    
    error_context_t errors = {0};
    parser_t *parser = parser_create(src, &errors);
    
    if (!parser) {
        return 1;
    }
    
    // TODO: Implement actual parsing and execution
    node_new_t *ast = parse_complete_command(parser);
    
    int exit_code = 0;
    if (ast) {
        // TODO: Execute AST
        // exit_code = execute_new_ast(ast);
        node_new_free_tree(ast);
    } else {
        exit_code = 1;
    }
    
    parser_destroy(parser);
    return exit_code;
}

/**
 * Parse a complete command and return AST (for testing)
 */
node_t *parse_command_new(source_t *src) {
    if (!src) {
        return NULL;
    }
    
    error_context_t errors = {0};
    parser_t *parser = parser_create(src, &errors);
    
    if (!parser) {
        return NULL;
    }
    
    // TODO: Implement actual parsing
    node_new_t *new_ast = parse_complete_command(parser);
    
    // Convert to old format for compatibility
    struct node *old_ast = node_new_to_old(new_ast);
    
    node_new_free_tree(new_ast);
    parser_destroy(parser);
    
    return old_ast;
}

/* ============================================================================
 * POSIX Grammar Parsing Functions
 * Based on POSIX.1-2017 Shell Command Language grammar
 * ============================================================================ */

/**
 * Parse a complete command (top-level production)
 * complete_command : list separator_op
 *                  | list
 */
static ast_node_t *parse_complete_command(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    ast_node_t *list_node = parse_list(parser);
    if (!list_node) {
        return NULL;
    }

    // Check for optional separator_op
    if (parser->current_token && 
        (parser->current_token->type == TOKEN_SEMICOLON ||
         parser->current_token->type == TOKEN_AMPERSAND ||
         parser->current_token->type == TOKEN_NEWLINE)) {
        advance_token(parser);
    }

    return list_node;
}

/**
 * Parse a list (sequence of and_or commands)
 * list : list separator_op and_or
 *      | and_or
 */
static ast_node_t *parse_list(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    ast_node_t *left = parse_and_or(parser);
    if (!left) {
        return NULL;
    }

    // Handle list continuation
    while (parser->current_token && 
           (parser->current_token->type == TOKEN_SEMICOLON ||
            parser->current_token->type == TOKEN_AMPERSAND ||
            parser->current_token->type == TOKEN_NEWLINE)) {
        
        token_type_t separator = parser->current_token->type;
        advance_token(parser);

        // Skip any additional newlines
        while (parser->current_token && 
               parser->current_token->type == TOKEN_NEWLINE) {
            advance_token(parser);
        }

        // Parse next and_or
        ast_node_t *right = parse_and_or(parser);
        if (!right) {
            break; // End of list
        }

        // Create list node
        ast_node_t *list_node = create_ast_node(NODE_LIST);
        if (!list_node) {
            node_free(left);
            node_free(right);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        list_node->data.list.left = left;
        list_node->data.list.right = right;
        list_node->data.list.separator = (separator == TOKEN_AMPERSAND) ? 
                                         SEPARATOR_BACKGROUND : SEPARATOR_SEQUENCE;
        
        left = list_node;
    }

    return left;
}

/**
 * Parse and_or (pipeline with && or ||)
 * and_or : and_or AND_IF pipeline
 *        | and_or OR_IF pipeline  
 *        | pipeline
 */
static ast_node_t *parse_and_or(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    ast_node_t *left = parse_pipeline(parser);
    if (!left) {
        return NULL;
    }

    // Handle && and || operators
    while (parser->current_token && 
           (parser->current_token->type == TOKEN_AND_IF ||
            parser->current_token->type == TOKEN_OR_IF)) {
        
        token_type_t op = parser->current_token->type;
        advance_token(parser);

        // Skip newlines after operator
        while (parser->current_token && 
               parser->current_token->type == TOKEN_NEWLINE) {
            advance_token(parser);
        }

        ast_node_t *right = parse_pipeline(parser);
        if (!right) {
            set_parser_error(parser, "Expected pipeline after logical operator");
            node_free(left);
            return NULL;
        }

        // Create logical node
        ast_node_t *logical_node = create_ast_node(NODE_LOGICAL);
        if (!logical_node) {
            node_free(left);
            node_free(right);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        logical_node->data.logical.left = left;
        logical_node->data.logical.right = right;
        logical_node->data.logical.operator = (op == TOKEN_AND_IF) ? 
                                              LOGICAL_AND : LOGICAL_OR;
        
        left = logical_node;
    }

    return left;
}

/**
 * Parse pipeline
 * pipeline : bang_opt pipe_sequence
 * bang_opt : '!'
 *          | /* empty */
 */
static ast_node_t *parse_pipeline(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    bool negated = false;
    
    // Check for bang (!)
    if (parser->current_token->type == TOKEN_BANG) {
        negated = true;
        advance_token(parser);
    }

    ast_node_t *pipe_seq = parse_pipe_sequence(parser);
    if (!pipe_seq) {
        return NULL;
    }

    // If negated, wrap in pipeline node
    if (negated) {
        ast_node_t *pipeline_node = create_ast_node(NODE_PIPELINE);
        if (!pipeline_node) {
            node_free(pipe_seq);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }
        
        pipeline_node->data.pipeline.commands = malloc(sizeof(ast_node_t *));
        if (!pipeline_node->data.pipeline.commands) {
            node_free(pipeline_node);
            node_free(pipe_seq);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }
        
        pipeline_node->data.pipeline.commands[0] = pipe_seq;
        pipeline_node->data.pipeline.command_count = 1;
        pipeline_node->data.pipeline.negated = true;
        
        return pipeline_node;
    }

    return pipe_seq;
}

/**
 * Parse pipe_sequence
 * pipe_sequence : pipe_sequence '|' linebreak command
 *               | command
 */
static ast_node_t *parse_pipe_sequence(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    ast_node_t **commands = NULL;
    size_t command_count = 0;
    size_t command_capacity = 4;

    commands = malloc(command_capacity * sizeof(ast_node_t *));
    if (!commands) {
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    // Parse first command
    ast_node_t *cmd = parse_command(parser);
    if (!cmd) {
        free(commands);
        return NULL;
    }

    commands[command_count++] = cmd;

    // Handle pipe continuation
    while (parser->current_token && parser->current_token->type == TOKEN_PIPE) {
        advance_token(parser);

        // Skip linebreak (newlines) after pipe
        while (parser->current_token && 
               parser->current_token->type == TOKEN_NEWLINE) {
            advance_token(parser);
        }

        // Expand array if needed
        if (command_count >= command_capacity) {
            command_capacity *= 2;
            ast_node_t **new_commands = realloc(commands, 
                                               command_capacity * sizeof(ast_node_t *));
            if (!new_commands) {
                for (size_t i = 0; i < command_count; i++) {
                    node_free(commands[i]);
                }
                free(commands);
                set_parser_error(parser, "Memory allocation failed");
                return NULL;
            }
            commands = new_commands;
        }

        cmd = parse_command(parser);
        if (!cmd) {
            set_parser_error(parser, "Expected command after pipe");
            for (size_t i = 0; i < command_count; i++) {
                node_free(commands[i]);
            }
            free(commands);
            return NULL;
        }

        commands[command_count++] = cmd;
    }

    // If only one command, return it directly
    if (command_count == 1) {
        ast_node_t *result = commands[0];
        free(commands);
        return result;
    }

    // Create pipeline node
    ast_node_t *pipeline_node = create_ast_node(NODE_PIPELINE);
    if (!pipeline_node) {
        for (size_t i = 0; i < command_count; i++) {
            node_free(commands[i]);
        }
        free(commands);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    pipeline_node->data.pipeline.commands = commands;
    pipeline_node->data.pipeline.command_count = command_count;
    pipeline_node->data.pipeline.negated = false;

    return pipeline_node;
}

/**
 * Parse command
 * command : simple_command
 *         | compound_command
 *         | compound_command redirect_list
 *         | function_def
 */
static ast_node_t *parse_command(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    // Check for compound commands first
    switch (parser->current_token->type) {
        case TOKEN_LBRACE:   // { ... }
        case TOKEN_LPAREN:   // ( ... )
        case TOKEN_IF:       // if ... fi
        case TOKEN_WHILE:    // while ... done
        case TOKEN_FOR:      // for ... done
        case TOKEN_CASE:     // case ... esac
        case TOKEN_UNTIL:    // until ... done
            return parse_compound_command(parser);
        
        case TOKEN_WORD:
            // Could be function definition or simple command
            // Look ahead for function syntax: name '(' ')'
            if (is_function_definition(parser)) {
                return parse_function_def(parser);
            }
            // Fall through to simple command
            
        default:
            return parse_simple_command(parser);
    }
}

/**
 * Parse simple_command
 * simple_command : cmd_prefix cmd_word cmd_suffix
 *                | cmd_prefix cmd_word
 *                | cmd_prefix
 *                | cmd_name cmd_suffix
 *                | cmd_name
 */
static ast_node_t *parse_simple_command(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    ast_node_t *cmd_node = create_ast_node(NODE_SIMPLE_COMMAND);
    if (!cmd_node) {
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    // Initialize command data
    cmd_node->data.simple_command.argc = 0;
    cmd_node->data.simple_command.argv = NULL;
    cmd_node->data.simple_command.assignments = NULL;
    cmd_node->data.simple_command.assignment_count = 0;
    cmd_node->data.simple_command.redirections = NULL;
    cmd_node->data.simple_command.redirection_count = 0;

    size_t argv_capacity = 4;
    cmd_node->data.simple_command.argv = malloc(argv_capacity * sizeof(char *));
    if (!cmd_node->data.simple_command.argv) {
        node_free(cmd_node);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    // Parse cmd_prefix (assignments and redirections)
    while (parser->current_token) {
        if (is_assignment(parser->current_token)) {
            if (!parse_assignment(parser, cmd_node)) {
                node_free(cmd_node);
                return NULL;
            }
        } else if (is_redirection(parser->current_token)) {
            if (!parse_redirection(parser, cmd_node)) {
                node_free(cmd_node);
                return NULL;
            }
        } else {
            break; // End of prefix
        }
    }

    // Parse command name and arguments
    while (parser->current_token && parser->current_token->type == TOKEN_WORD) {
        // Expand argv array if needed
        if (cmd_node->data.simple_command.argc >= argv_capacity) {
            argv_capacity *= 2;
            char **new_argv = realloc(cmd_node->data.simple_command.argv,
                                     argv_capacity * sizeof(char *));
            if (!new_argv) {
                node_free(cmd_node);
                set_parser_error(parser, "Memory allocation failed");
                return NULL;
            }
            cmd_node->data.simple_command.argv = new_argv;
        }

        // Add argument
        cmd_node->data.simple_command.argv[cmd_node->data.simple_command.argc] = 
            strdup(parser->current_token->value);
        if (!cmd_node->data.simple_command.argv[cmd_node->data.simple_command.argc]) {
            node_free(cmd_node);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }
        cmd_node->data.simple_command.argc++;

        advance_token(parser);

        // Parse cmd_suffix (more redirections)
        while (parser->current_token && is_redirection(parser->current_token)) {
            if (!parse_redirection(parser, cmd_node)) {
                node_free(cmd_node);
                return NULL;
            }
        }
    }

    // Null-terminate argv
    if (cmd_node->data.simple_command.argc >= argv_capacity) {
        argv_capacity++;
        char **new_argv = realloc(cmd_node->data.simple_command.argv,
                                 argv_capacity * sizeof(char *));
        if (!new_argv) {
            node_free(cmd_node);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }
        cmd_node->data.simple_command.argv = new_argv;
    }
    cmd_node->data.simple_command.argv[cmd_node->data.simple_command.argc] = NULL;

    // A simple command must have at least a command name or assignments
    if (cmd_node->data.simple_command.argc == 0 && 
        cmd_node->data.simple_command.assignment_count == 0) {
        node_free(cmd_node);
        set_parser_error(parser, "Empty command");
        return NULL;
    }

    return cmd_node;
}

/**
 * Parse compound_command
 * compound_command : brace_group
 *                  | subshell
 *                  | for_clause
 *                  | case_clause
 *                  | if_clause
 *                  | while_clause
 *                  | until_clause
 */
static ast_node_t *parse_compound_command(parser_state_t *parser) {
    if (!parser || !parser->current_token) {
        return NULL;
    }

    switch (parser->current_token->type) {
        case TOKEN_LBRACE:
            return parse_brace_group(parser);
        case TOKEN_LPAREN:
            return parse_subshell(parser);
        case TOKEN_FOR:
            return parse_for_clause(parser);
        case TOKEN_CASE:
            return parse_case_clause(parser);
        case TOKEN_IF:
            return parse_if_clause(parser);
        case TOKEN_WHILE:
            return parse_while_clause(parser);
        case TOKEN_UNTIL:
            return parse_until_clause(parser);
        default:
            set_parser_error(parser, "Expected compound command");
            return NULL;
    }
}

/**
 * Parse brace_group: { compound_list }
 */
static ast_node_t *parse_brace_group(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_LBRACE) {
        set_parser_error(parser, "Expected '{'");
        return NULL;
    }

    advance_token(parser); // consume '{'

    // Skip newlines
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        advance_token(parser);
    }

    ast_node_t *body = parse_compound_list(parser);
    if (!body) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_RBRACE) {
        node_free(body);
        set_parser_error(parser, "Expected '}'");
        return NULL;
    }

    advance_token(parser); // consume '}'

    ast_node_t *group_node = create_ast_node(NODE_BRACE_GROUP);
    if (!group_node) {
        node_free(body);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    group_node->data.brace_group.body = body;
    return group_node;
}

/**
 * Parse subshell: ( compound_list )
 */
static ast_node_t *parse_subshell(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_LPAREN) {
        set_parser_error(parser, "Expected '('");
        return NULL;
    }

    advance_token(parser); // consume '('

    // Skip newlines
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        advance_token(parser);
    }

    ast_node_t *body = parse_compound_list(parser);
    if (!body) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_RPAREN) {
        node_free(body);
        set_parser_error(parser, "Expected ')'");
        return NULL;
    }

    advance_token(parser); // consume ')'

    ast_node_t *subshell_node = create_ast_node(NODE_SUBSHELL);
    if (!subshell_node) {
        node_free(body);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    subshell_node->data.subshell.body = body;
    return subshell_node;
}

/**
 * Parse if_clause: if compound_list then compound_list [else_part] fi
 */
static ast_node_t *parse_if_clause(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_IF) {
        set_parser_error(parser, "Expected 'if'");
        return NULL;
    }

    advance_token(parser); // consume 'if'

    ast_node_t *condition = parse_compound_list(parser);
    if (!condition) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_THEN) {
        node_free(condition);
        set_parser_error(parser, "Expected 'then'");
        return NULL;
    }

    advance_token(parser); // consume 'then'

    ast_node_t *then_body = parse_compound_list(parser);
    if (!then_body) {
        node_free(condition);
        return NULL;
    }

    ast_node_t *else_body = NULL;

    // Check for else part
    if (parser->current_token) {
        if (parser->current_token->type == TOKEN_ELIF) {
            // Recursively parse elif as nested if
            else_body = parse_if_clause(parser);
            if (!else_body) {
                node_free(condition);
                node_free(then_body);
                return NULL;
            }
        } else if (parser->current_token->type == TOKEN_ELSE) {
            advance_token(parser); // consume 'else'
            else_body = parse_compound_list(parser);
            if (!else_body) {
                node_free(condition);
                node_free(then_body);
                return NULL;
            }
        }
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_FI) {
        node_free(condition);
        node_free(then_body);
        node_free(else_body);
        set_parser_error(parser, "Expected 'fi'");
        return NULL;
    }

    advance_token(parser); // consume 'fi'

    ast_node_t *if_node = create_ast_node(NODE_IF);
    if (!if_node) {
        node_free(condition);
        node_free(then_body);
        node_free(else_body);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    if_node->data.if_stmt.condition = condition;
    if_node->data.if_stmt.then_body = then_body;
    if_node->data.if_stmt.else_body = else_body;

    return if_node;
}

/**
 * Parse while_clause: while compound_list do compound_list done
 */
static ast_node_t *parse_while_clause(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_WHILE) {
        set_parser_error(parser, "Expected 'while'");
        return NULL;
    }

    advance_token(parser); // consume 'while'

    ast_node_t *condition = parse_compound_list(parser);
    if (!condition) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_DO) {
        node_free(condition);
        set_parser_error(parser, "Expected 'do'");
        return NULL;
    }

    advance_token(parser); // consume 'do'

    ast_node_t *body = parse_compound_list(parser);
    if (!body) {
        node_free(condition);
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_DONE) {
        node_free(condition);
        node_free(body);
        set_parser_error(parser, "Expected 'done'");
        return NULL;
    }

    advance_token(parser); // consume 'done'

    ast_node_t *while_node = create_ast_node(NODE_WHILE);
    if (!while_node) {
        node_free(condition);
        node_free(body);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    while_node->data.while_loop.condition = condition;
    while_node->data.while_loop.body = body;

    return while_node;
}

/**
 * Parse for_clause: for name [in word_list] do compound_list done
 */
static ast_node_t *parse_for_clause(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_FOR) {
        set_parser_error(parser, "Expected 'for'");
        return NULL;
    }

    advance_token(parser); // consume 'for'

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_WORD) {
        set_parser_error(parser, "Expected variable name after 'for'");
        return NULL;
    }

    char *var_name = strdup(parser->current_token->value);
    if (!var_name) {
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    advance_token(parser); // consume variable name

    // Parse optional 'in word_list'
    char **word_list = NULL;
    size_t word_count = 0;

    if (parser->current_token && 
        parser->current_token->type == TOKEN_IN) {
        advance_token(parser); // consume 'in'

        // Parse word list
        size_t word_capacity = 4;
        word_list = malloc(word_capacity * sizeof(char *));
        if (!word_list) {
            free(var_name);
            set_parser_error(parser, "Memory allocation failed");
            return NULL;
        }

        while (parser->current_token && 
               parser->current_token->type == TOKEN_WORD) {
            if (word_count >= word_capacity) {
                word_capacity *= 2;
                char **new_list = realloc(word_list, 
                                         word_capacity * sizeof(char *));
                if (!new_list) {
                    for (size_t i = 0; i < word_count; i++) {
                        free(word_list[i]);
                    }
                    free(word_list);
                    free(var_name);
                    set_parser_error(parser, "Memory allocation failed");
                    return NULL;
                }
                word_list = new_list;
            }

            word_list[word_count] = strdup(parser->current_token->value);
            if (!word_list[word_count]) {
                for (size_t i = 0; i < word_count; i++) {
                    free(word_list[i]);
                }
                free(word_list);
                free(var_name);
                set_parser_error(parser, "Memory allocation failed");
                return NULL;
            }
            word_count++;

            advance_token(parser);
        }

        // Null-terminate word list
        if (word_count >= word_capacity) {
            word_capacity++;
            char **new_list = realloc(word_list, 
                                     word_capacity * sizeof(char *));
            if (!new_list) {
                for (size_t i = 0; i < word_count; i++) {
                    free(word_list[i]);
                }
                free(word_list);
                free(var_name);
                set_parser_error(parser, "Memory allocation failed");
                return NULL;
            }
            word_list = new_list;
        }
        word_list[word_count] = NULL;
    }

    // Skip optional semicolon and newlines
    if (parser->current_token && 
        parser->current_token->type == TOKEN_SEMICOLON) {
        advance_token(parser);
    }
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        advance_token(parser);
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_DO) {
        for (size_t i = 0; i < word_count; i++) {
            free(word_list[i]);
        }
        free(word_list);
        free(var_name);
        set_parser_error(parser, "Expected 'do'");
        return NULL;
    }

    advance_token(parser); // consume 'do'

    ast_node_t *body = parse_compound_list(parser);
    if (!body) {
        for (size_t i = 0; i < word_count; i++) {
            free(word_list[i]);
        }
        free(word_list);
        free(var_name);
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_DONE) {
        node_free(body);
        for (size_t i = 0; i < word_count; i++) {
            free(word_list[i]);
        }
        free(word_list);
        free(var_name);
        set_parser_error(parser, "Expected 'done'");
        return NULL;
    }

    advance_token(parser); // consume 'done'

    ast_node_t *for_node = create_ast_node(NODE_FOR);
    if (!for_node) {
        node_free(body);
        for (size_t i = 0; i < word_count; i++) {
            free(word_list[i]);
        }
        free(word_list);
        free(var_name);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    for_node->data.for_loop.variable = var_name;
    for_node->data.for_loop.word_list = word_list;
    for_node->data.for_loop.word_count = word_count;
    for_node->data.for_loop.body = body;

    return for_node;
}

/**
 * Parse compound_list (used in compound commands)
 * compound_list : linebreak term
 *               | linebreak term separator_op compound_list
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
 * Check if current token sequence represents a function definition
 */
static bool is_function_definition(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_WORD) {
        return false;
    }

    // Save current position
    size_t saved_pos = parser->token_index;
    
    advance_token(parser); // skip name
    
    bool is_function = false;
    if (parser->current_token && 
        parser->current_token->type == TOKEN_LPAREN) {
        advance_token(parser); // skip '('
        if (parser->current_token && 
            parser->current_token->type == TOKEN_RPAREN) {
            is_function = true;
        }
    }

    // Restore position
    parser->token_index = saved_pos;
    parser->current_token = (parser->token_index < parser->token_count) ?
                           &parser->tokens[parser->token_index] : NULL;

    return is_function;
}

/**
 * Check if token represents an assignment
 */
static bool is_assignment(const token_t *token) {
    if (!token || token->type != TOKEN_WORD || !token->value) {
        return false;
    }

    // Look for '=' in the token value
    const char *eq = strchr(token->value, '=');
    return (eq != NULL && eq != token->value); // '=' not at start
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
        case TOKEN_WORD:
            // Check for numbered redirections like "2>"
            if (token->value && strlen(token->value) >= 2) {
                char *end;
                strtol(token->value, &end, 10);
                return (*end == '<' || *end == '>');
            }
            return false;
        default:
            return false;
    }
}

/**
 * Parse assignment
 */
static bool parse_assignment(parser_state_t *parser, ast_node_t *cmd_node) {
    if (!parser || !parser->current_token || !cmd_node) {
        return false;
    }

    const char *assignment_str = parser->current_token->value;
    const char *eq = strchr(assignment_str, '=');
    if (!eq) {
        return false;
    }

    size_t name_len = eq - assignment_str;
    char *name = malloc(name_len + 1);
    if (!name) {
        set_parser_error(parser, "Memory allocation failed");
        return false;
    }
    
    strncpy(name, assignment_str, name_len);
    name[name_len] = '\0';

    char *value = strdup(eq + 1);
    if (!value) {
        free(name);
        set_parser_error(parser, "Memory allocation failed");
        return false;
    }

    // Expand assignments array
    size_t new_count = cmd_node->data.simple_command.assignment_count + 1;
    assignment_t *new_assignments = realloc(
        cmd_node->data.simple_command.assignments,
        new_count * sizeof(assignment_t));
    if (!new_assignments) {
        free(name);
        free(value);
        set_parser_error(parser, "Memory allocation failed");
        return false;
    }

    cmd_node->data.simple_command.assignments = new_assignments;
    cmd_node->data.simple_command.assignments[cmd_node->data.simple_command.assignment_count].name = name;
    cmd_node->data.simple_command.assignments[cmd_node->data.simple_command.assignment_count].value = value;
    cmd_node->data.simple_command.assignment_count = new_count;

    advance_token(parser);
    return true;
}

/**
 * Parse redirection
 */
static bool parse_redirection(parser_state_t *parser, ast_node_t *cmd_node) {
    if (!parser || !parser->current_token || !cmd_node) {
        return false;
    }

    redirection_t redir = {0};
    
    // Check for numbered redirection (e.g., "2>")
    if (parser->current_token->type == TOKEN_WORD) {
        char *end;
        long fd = strtol(parser->current_token->value, &end, 10);
        if (*end == '<' || *end == '>') {
            redir.fd = (int)fd;
            
            // Determine redirection type from the operator
            if (*end == '<') {
                if (*(end + 1) == '&') {
                    redir.type = REDIR_INPUT_DUP;
                } else {
                    redir.type = REDIR_INPUT;
                }
            } else { // '>'
                if (*(end + 1) == '&') {
                    redir.type = REDIR_OUTPUT_DUP;
                } else if (*(end + 1) == '>') {
                    redir.type = REDIR_APPEND;
                } else {
                    redir.type = REDIR_OUTPUT;
                }
            }
            
            advance_token(parser);
        } else {
            set_parser_error(parser, "Invalid redirection");
            return false;
        }
    } else {
        // Standard redirection operators
        redir.fd = -1; // Use default fd
        
        switch (parser->current_token->type) {
            case TOKEN_LESS:
                redir.type = REDIR_INPUT;
                break;
            case TOKEN_GREAT:
                redir.type = REDIR_OUTPUT;
                break;
            case TOKEN_DGREAT:
                redir.type = REDIR_APPEND;
                break;
            case TOKEN_LESSAND:
                redir.type = REDIR_INPUT_DUP;
                break;
            case TOKEN_GREATAND:
                redir.type = REDIR_OUTPUT_DUP;
                break;
            case TOKEN_LESSGREAT:
                redir.type = REDIR_INPUT_OUTPUT;
                break;
            case TOKEN_CLOBBER:
                redir.type = REDIR_OUTPUT_CLOBBER;
                break;
            default:
                set_parser_error(parser, "Invalid redirection operator");
                return false;
        }
        
        advance_token(parser);
    }

    // Parse target (filename or fd)
    if (!parser->current_token || parser->current_token->type != TOKEN_WORD) {
        set_parser_error(parser, "Expected redirection target");
        return false;
    }

    redir.target = strdup(parser->current_token->value);
    if (!redir.target) {
        set_parser_error(parser, "Memory allocation failed");
        return false;
    }

    advance_token(parser);

    // Add to redirections array
    size_t new_count = cmd_node->data.simple_command.redirection_count + 1;
    redirection_t *new_redirections = realloc(
        cmd_node->data.simple_command.redirections,
        new_count * sizeof(redirection_t));
    if (!new_redirections) {
        free(redir.target);
        set_parser_error(parser, "Memory allocation failed");
        return false;
    }

    cmd_node->data.simple_command.redirections = new_redirections;
    cmd_node->data.simple_command.redirections[cmd_node->data.simple_command.redirection_count] = redir;
    cmd_node->data.simple_command.redirection_count = new_count;

    return true;
}

/**
 * Parse function_def: name '(' ')' linebreak function_body
 */
static ast_node_t *parse_function_def(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_WORD) {
        set_parser_error(parser, "Expected function name");
        return NULL;
    }

    char *name = strdup(parser->current_token->value);
    if (!name) {
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    advance_token(parser); // consume name

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_LPAREN) {
        free(name);
        set_parser_error(parser, "Expected '(' after function name");
        return NULL;
    }

    advance_token(parser); // consume '('

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_RPAREN) {
        free(name);
        set_parser_error(parser, "Expected ')' in function definition");
        return NULL;
    }

    advance_token(parser); // consume ')'

    // Skip linebreak (newlines)
    while (parser->current_token && 
           parser->current_token->type == TOKEN_NEWLINE) {
        advance_token(parser);
    }

    // Parse function body (compound command)
    ast_node_t *body = parse_compound_command(parser);
    if (!body) {
        free(name);
        return NULL;
    }

    ast_node_t *func_node = create_ast_node(NODE_FUNCTION);
    if (!func_node) {
        free(name);
        node_free(body);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    func_node->data.function.name = name;
    func_node->data.function.body = body;

    return func_node;
}

/**
 * Parse case_clause and until_clause (basic implementations)
 */
static ast_node_t *parse_case_clause(parser_state_t *parser) {
    // TODO: Implement full case statement parsing
    // For now, return error to indicate unsupported
    set_parser_error(parser, "Case statements not yet implemented");
    return NULL;
}

static ast_node_t *parse_until_clause(parser_state_t *parser) {
    if (!parser || !parser->current_token || 
        parser->current_token->type != TOKEN_UNTIL) {
        set_parser_error(parser, "Expected 'until'");
        return NULL;
    }

    advance_token(parser); // consume 'until'

    ast_node_t *condition = parse_compound_list(parser);
    if (!condition) {
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_DO) {
        node_free(condition);
        set_parser_error(parser, "Expected 'do'");
        return NULL;
    }

    advance_token(parser); // consume 'do'

    ast_node_t *body = parse_compound_list(parser);
    if (!body) {
        node_free(condition);
        return NULL;
    }

    if (!parser->current_token || 
        parser->current_token->type != TOKEN_DONE) {
        node_free(condition);
        node_free(body);
        set_parser_error(parser, "Expected 'done'");
        return NULL;
    }

    advance_token(parser); // consume 'done'

    ast_node_t *until_node = create_ast_node(NODE_UNTIL);
    if (!until_node) {
        node_free(condition);
        node_free(body);
        set_parser_error(parser, "Memory allocation failed");
        return NULL;
    }

    until_node->data.until_loop.condition = condition;
    until_node->data.until_loop.body = body;

    return until_node;
}
