/**
 * Modern POSIX Shell Parser Implementation
 * 
 * Clean recursive descent parser that properly handles POSIX shell grammar
 * with correct token boundary management and error handling.
 */

#include "parser_modern.h"
#include "tokenizer_new.h"
#include "node.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declarations
static node_t *parse_command_list(parser_modern_t *parser);
static node_t *parse_pipeline(parser_modern_t *parser);
static node_t *parse_simple_command(parser_modern_t *parser);
static node_t *parse_control_structure(parser_modern_t *parser);
static node_t *parse_if_statement(parser_modern_t *parser);
static node_t *parse_while_statement(parser_modern_t *parser);
static node_t *parse_for_statement(parser_modern_t *parser);
static node_t *parse_case_statement(parser_modern_t *parser);
static node_t *parse_function_definition(parser_modern_t *parser);
static bool is_function_definition(parser_modern_t *parser);
static void parser_error(parser_modern_t *parser, const char *message);
static bool expect_token(parser_modern_t *parser, modern_token_type_t expected);

// Create new parser
parser_modern_t *parser_modern_new(const char *input) {
    if (!input) return NULL;
    
    parser_modern_t *parser = malloc(sizeof(parser_modern_t));
    if (!parser) return NULL;
    
    parser->tokenizer = modern_tokenizer_new(input);
    if (!parser->tokenizer) {
        free(parser);
        return NULL;
    }
    
    parser->error_message = NULL;
    parser->has_error = false;
    
    return parser;
}

// Free parser
void parser_modern_free(parser_modern_t *parser) {
    if (!parser) return;
    
    modern_tokenizer_free(parser->tokenizer);
    free(parser);
}

// Check for errors
bool parser_modern_has_error(parser_modern_t *parser) {
    return parser && parser->has_error;
}

const char *parser_modern_error(parser_modern_t *parser) {
    return parser ? parser->error_message : "Invalid parser";
}

// Set parser error
static void parser_error(parser_modern_t *parser, const char *message) {
    if (parser) {
        parser->error_message = message;
        parser->has_error = true;
    }
}

// Expect specific token type
static bool expect_token(parser_modern_t *parser, modern_token_type_t expected) {
    if (!modern_tokenizer_match(parser->tokenizer, expected)) {
        char error_buf[256];
        modern_token_t *current = modern_tokenizer_current(parser->tokenizer);
        snprintf(error_buf, sizeof(error_buf), "Expected %s but got %s", 
                modern_token_type_name(expected), 
                current ? modern_token_type_name(current->type) : "EOF");
        parser_error(parser, strdup(error_buf));
        return false;
    }
    modern_tokenizer_advance(parser->tokenizer);
    return true;
}

// Main parsing entry point
node_t *parser_modern_parse(parser_modern_t *parser) {
    if (!parser) return NULL;
    
    // Skip initial whitespace and comments
    while (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_WHITESPACE) ||
           modern_tokenizer_match(parser->tokenizer, MODERN_TOK_COMMENT) ||
           modern_tokenizer_match(parser->tokenizer, MODERN_TOK_NEWLINE)) {
        modern_tokenizer_advance(parser->tokenizer);
    }
    
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
        return NULL; // Empty input
    }
    
    return parse_command_list(parser);
}

// Parse command line (sequence of commands)
node_t *parser_modern_parse_command_line(parser_modern_t *parser) {
    return parse_command_list(parser);
}

// Helper function to skip separators (semicolons, newlines, whitespace)
static void skip_separators(parser_modern_t *parser) {
    while (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON) ||
           modern_tokenizer_match(parser->tokenizer, MODERN_TOK_NEWLINE) ||
           modern_tokenizer_match(parser->tokenizer, MODERN_TOK_WHITESPACE)) {
        modern_tokenizer_advance(parser->tokenizer);
    }
}

// Parse command body for control structures - parses multiple commands until terminator
static node_t *parse_command_body(parser_modern_t *parser, modern_token_type_t terminator) {
    node_t *first_command = NULL;
    node_t *current = NULL;
    
    while (!modern_tokenizer_match(parser->tokenizer, terminator) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF) &&
           !parser->has_error) {
        
        // Skip separators between commands
        skip_separators(parser);
        
        // Check again for terminator after skipping separators
        if (modern_tokenizer_match(parser->tokenizer, terminator) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
            break;
        }
        
        node_t *command = parse_pipeline(parser);
        if (!command) {
            if (!parser->has_error) {
                break; // End of input
            }
            free_node_tree(first_command);
            return NULL;
        }
        
        if (!first_command) {
            first_command = command;
            current = command;
        } else {
            current->next_sibling = command;
            current = command;
        }
    }
    
    return first_command;
}

// Parse command body for IF statements - stops at else, elif, or fi
static node_t *parse_if_body(parser_modern_t *parser) {
    node_t *first_command = NULL;
    node_t *current = NULL;
    
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELSE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELIF) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_FI) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF) &&
           !parser->has_error) {
        
        // Skip separators between commands
        skip_separators(parser);
        
        // Check again for terminators after skipping separators
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELSE) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELIF) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_FI) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
            break;
        }
        
        node_t *command = parse_pipeline(parser);
        if (!command) {
            if (!parser->has_error) {
                break; // End of input
            }
            free_node_tree(first_command);
            return NULL;
        }
        
        if (!first_command) {
            first_command = command;
            current = command;
        } else {
            current->next_sibling = command;
            current = command;
        }
    }
    
    return first_command;
}

// Parse logical operators (and_or level)
static node_t *parse_logical_expression(parser_modern_t *parser) {
    node_t *left = parse_pipeline(parser);
    if (!left) return NULL;
    
    while (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_LOGICAL_AND) ||
           modern_tokenizer_match(parser->tokenizer, MODERN_TOK_LOGICAL_OR)) {
        
        modern_token_type_t op_type = modern_tokenizer_current(parser->tokenizer)->type;
        modern_tokenizer_advance(parser->tokenizer); // consume operator
        
        // Skip whitespace after operator
        skip_separators(parser);
        
        node_t *right = parse_pipeline(parser);
        if (!right) {
            free_node_tree(left);
            return NULL;
        }
        
        // Create logical operator node
        node_t *logical_node = new_node(op_type == MODERN_TOK_LOGICAL_AND ? NODE_LOGICAL_AND : NODE_LOGICAL_OR);
        if (!logical_node) {
            free_node_tree(left);
            free_node_tree(right);
            return NULL;
        }
        
        add_child_node(logical_node, left);
        add_child_node(logical_node, right);
        left = logical_node;
    }
    
    return left;
}

// Parse command list (commands separated by ; or newlines)
static node_t *parse_command_list(parser_modern_t *parser) {
    node_t *first_command = NULL;
    node_t *current = NULL;
    
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF) && !parser->has_error) {
        // Skip separators and newlines
        while (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON) ||
               modern_tokenizer_match(parser->tokenizer, MODERN_TOK_NEWLINE)) {
            modern_tokenizer_advance(parser->tokenizer);
        }
        
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
            break;
        }
        
        node_t *command = parse_logical_expression(parser);
        if (!command) {
            if (!parser->has_error) {
                break; // End of input
            }
            free_node_tree(first_command);
            return NULL;
        }
        
        if (!first_command) {
            first_command = command;
            current = command;
        } else {
            current->next_sibling = command;
            current = command;
        }
        
        // Check for end of command list
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DONE) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_FI) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELSE) ||
            modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELIF)) {
            break;
        }
    }
    
    return first_command;
}

// Parse pipeline (commands connected by |)
static node_t *parse_pipeline(parser_modern_t *parser) {
    node_t *left = parse_simple_command(parser);
    if (!left) return NULL;
    
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_PIPE)) {
        modern_tokenizer_advance(parser->tokenizer); // consume |
        
        node_t *right = parse_pipeline(parser);
        if (!right) {
            free_node_tree(left);
            return NULL;
        }
        
        node_t *pipe_node = new_node(NODE_PIPE);
        if (!pipe_node) {
            free_node_tree(left);
            free_node_tree(right);
            return NULL;
        }
        
        add_child_node(pipe_node, left);
        add_child_node(pipe_node, right);
        return pipe_node;
    }
    
    return left;
}

// Parse simple command or control structure
static node_t *parse_simple_command(parser_modern_t *parser) {
    modern_token_t *current = modern_tokenizer_current(parser->tokenizer);
    if (!current) return NULL;
    
    // Check for control structures
    if (modern_token_is_keyword(current->type)) {
        if (getenv("NEW_PARSER_DEBUG")) {
            printf("DEBUG: Found keyword token type %d (%s)\n", 
                   current->type, modern_token_type_name(current->type));
        }
        switch (current->type) {
            case MODERN_TOK_IF:
                return parse_if_statement(parser);
            case MODERN_TOK_WHILE:
                return parse_while_statement(parser);
            case MODERN_TOK_FOR:
                return parse_for_statement(parser);
            case MODERN_TOK_CASE:
                return parse_case_statement(parser);
            case MODERN_TOK_FUNCTION:
                return parse_function_definition(parser);
            default:
                // Other keywords not implemented yet
                printf("DEBUG: Unhandled keyword type %d (%s)\n", 
                       current->type, modern_token_type_name(current->type));
                return NULL;
        }
    }
    
    // Check for function definition (word followed by ())
    if (modern_token_is_word_like(current->type) && is_function_definition(parser)) {
        return parse_function_definition(parser);
    }
    
    // Check for assignment (word followed by =)
    if (modern_token_is_word_like(current->type)) {
        modern_token_t *next = modern_tokenizer_peek(parser->tokenizer);
        if (next && next->type == MODERN_TOK_ASSIGN) {
            // This is an assignment: variable=value
            node_t *command = new_node(NODE_COMMAND);
            if (!command) return NULL;
            
            // FIX: Save variable name BEFORE advancing tokenizer
            char *var_name = strdup(current->text);
            if (!var_name) {
                free_node_tree(command);
                return NULL;
            }
            
            modern_tokenizer_advance(parser->tokenizer); // consume variable name
            modern_tokenizer_advance(parser->tokenizer); // consume '='
            
            modern_token_t *value = modern_tokenizer_current(parser->tokenizer);
            if (value && modern_token_is_word_like(value->type)) {
                size_t var_len = strlen(var_name);
                size_t value_len = strlen(value->text);
                char *assignment = malloc(var_len + 1 + value_len + 1);
                if (assignment) {
                    strcpy(assignment, var_name);
                    strcat(assignment, "=");
                    strcat(assignment, value->text);
                    command->val.str = assignment;
                }
                modern_tokenizer_advance(parser->tokenizer); // consume value
            } else {
                // Assignment with empty value: variable=
                size_t var_len = strlen(var_name);
                char *assignment = malloc(var_len + 2);
                if (assignment) {
                    strcpy(assignment, var_name);
                    strcat(assignment, "=");
                    command->val.str = assignment;
                }
            }
            
            free(var_name);
            return command;
        }
    }
    
    // Parse regular command
    if (!modern_token_is_word_like(current->type) && current->type != MODERN_TOK_LBRACKET) {

        parser_error(parser, "Expected command name");
        return NULL;
    }

    
    node_t *command = new_node(NODE_COMMAND);
    if (!command) return NULL;
    
    // Set command name
    command->val.str = strdup(current->text);
    modern_tokenizer_advance(parser->tokenizer);
    
    // Parse arguments
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_NEWLINE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_PIPE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_LOGICAL_AND) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_LOGICAL_OR) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DONE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_FI) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_THEN) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELSE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELIF) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DO)) {
        
        modern_token_t *arg_token = modern_tokenizer_current(parser->tokenizer);
        if (!arg_token) break;
        
        // Handle special case: ] is part of [ command arguments
        if (modern_token_is_word_like(arg_token->type) || 
            arg_token->type == MODERN_TOK_VARIABLE ||
            arg_token->type == MODERN_TOK_EXPANDABLE_STRING ||
            arg_token->type == MODERN_TOK_COMMAND_SUB ||
            arg_token->type == MODERN_TOK_BACKQUOTE ||
            arg_token->type == MODERN_TOK_RBRACKET ||
            arg_token->type == MODERN_TOK_REDIRECT_IN ||
            arg_token->type == MODERN_TOK_REDIRECT_OUT ||
            arg_token->type == MODERN_TOK_ASSIGN) {
            
            node_t *arg_node = new_node(NODE_VAR);
            if (!arg_node) {
                free_node_tree(command);
                return NULL;
            }
            arg_node->val.str = strdup(arg_token->text);
            add_child_node(command, arg_node);
            modern_tokenizer_advance(parser->tokenizer);
        } else {
            break; // Stop parsing arguments
        }
    }
    
    return command;
}

// Parse if statement
static node_t *parse_if_statement(parser_modern_t *parser) {
    if (!expect_token(parser, MODERN_TOK_IF)) return NULL;
    
    node_t *if_node = new_node(NODE_IF);
    if (!if_node) return NULL;
    
    // Parse condition - parse until we hit 'then' or ';'
    node_t *condition = parse_pipeline(parser);
    if (!condition) {
        free_node_tree(if_node);
        return NULL;
    }
    add_child_node(if_node, condition);
    
    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);
    
    // Now we should see 'then'
    if (!expect_token(parser, MODERN_TOK_THEN)) {
        free_node_tree(if_node);
        return NULL;
    }
    
    // Skip separators after 'then' before parsing body
    skip_separators(parser);
    
    // Parse then body - parse until we hit 'else', 'elif', or 'fi'
    node_t *then_body = parse_if_body(parser);
    if (!then_body) {
        free_node_tree(if_node);
        return NULL;
    }
    add_child_node(if_node, then_body);
    
    // Handle optional semicolon before else/fi
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
        modern_tokenizer_advance(parser->tokenizer);
    }
    
    // Parse optional else
    // Skip separators before checking for else
    skip_separators(parser);
    
    // Handle optional else clause
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELSE)) {
        modern_tokenizer_advance(parser->tokenizer);
        
        // Skip separators after 'else' before parsing body
        skip_separators(parser);
        
        node_t *else_body = parse_if_body(parser);
        if (!else_body) {
            free_node_tree(if_node);
            return NULL;
        }
        add_child_node(if_node, else_body);
    }
    
    // Skip separators before 'fi'
    skip_separators(parser);
    
    // No need for additional semicolon handling here since we handled it above
    
    if (!expect_token(parser, MODERN_TOK_FI)) {
        free_node_tree(if_node);
        return NULL;
    }
    
    return if_node;
}

// Parse while statement  
static node_t *parse_while_statement(parser_modern_t *parser) {
    if (!expect_token(parser, MODERN_TOK_WHILE)) return NULL;
    
    node_t *while_node = new_node(NODE_WHILE);
    if (!while_node) return NULL;
    
    // Parse condition - this is the key fix!
    // We parse until we hit 'do', not until we hit ';'
    node_t *condition = NULL;
    
    // Parse condition as a simple command or pipeline
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_LBRACKET)) {
        // Special handling for [ ... ] test commands
        condition = parse_simple_command(parser);
    } else {
        // Regular command condition
        condition = parse_pipeline(parser);
    }
    
    if (!condition) {
        free_node_tree(while_node);
        parser_error(parser, "Failed to parse while condition");
        return NULL;
    }
    add_child_node(while_node, condition);
    
    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);
    
    // Now we should see 'do'
    if (!expect_token(parser, MODERN_TOK_DO)) {
        free_node_tree(while_node);
        return NULL;
    }
    
    // Skip separators after 'do' before parsing body
    skip_separators(parser);
    
    // Parse body
    node_t *body = parse_command_body(parser, MODERN_TOK_DONE);
    if (!body) {
        free_node_tree(while_node);
        return NULL;
    }
    add_child_node(while_node, body);
    
    // Skip separators before 'done'
    skip_separators(parser);
    
    if (!expect_token(parser, MODERN_TOK_DONE)) {
        free_node_tree(while_node);
        return NULL;
    }
    
    return while_node;
}

// Parse for statement
static node_t *parse_for_statement(parser_modern_t *parser) {
    if (!expect_token(parser, MODERN_TOK_FOR)) return NULL;
    
    node_t *for_node = new_node(NODE_FOR);
    if (!for_node) return NULL;
    
    // Parse variable name
    if (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_WORD)) {
        free_node_tree(for_node);
        parser_error(parser, "Expected variable name after 'for'");
        return NULL;
    }
    
    modern_token_t *var_token = modern_tokenizer_current(parser->tokenizer);
    for_node->val.str = strdup(var_token->text);
    modern_tokenizer_advance(parser->tokenizer);
    
    if (!expect_token(parser, MODERN_TOK_IN)) {
        free_node_tree(for_node);
        return NULL;
    }
    
    // Parse word list
    node_t *word_list = new_node(NODE_VAR); // Use as container
    if (!word_list) {
        free_node_tree(for_node);
        return NULL;
    }
    
    // Collect all words until ';', newline, or 'do'
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_NEWLINE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DO) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
        
        modern_token_t *word_token = modern_tokenizer_current(parser->tokenizer);
        if (modern_token_is_word_like(word_token->type) || word_token->type == MODERN_TOK_VARIABLE) {
            node_t *word_node = new_node(NODE_VAR);
            if (!word_node) {
                free_node_tree(for_node);
                free_node_tree(word_list);
                return NULL;
            }
            word_node->val.str = strdup(word_token->text);
            add_child_node(word_list, word_node);
            modern_tokenizer_advance(parser->tokenizer);
        } else {
            break;
        }
    }
    
    add_child_node(for_node, word_list);
    
    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);
    
    // Now we should see 'do'
    if (!expect_token(parser, MODERN_TOK_DO)) {
        free_node_tree(for_node);
        return NULL;
    }
    
    // Skip separators after 'do' before parsing body
    skip_separators(parser);
    
    // Parse body
    node_t *body = parse_command_body(parser, MODERN_TOK_DONE);
    if (!body) {
        free_node_tree(for_node);
        return NULL;
    }
    add_child_node(for_node, body);
    
    // Skip separators before 'done'
    skip_separators(parser);
    
    if (!expect_token(parser, MODERN_TOK_DONE)) {
        free_node_tree(for_node);
        return NULL;
    }
    
    return for_node;
}

// Parse case statement: case WORD in pattern) commands ;; ... esac
static node_t *parse_case_statement(parser_modern_t *parser) {
    if (!expect_token(parser, MODERN_TOK_CASE)) {
        return NULL;
    }
    
    node_t *case_node = new_node(NODE_CASE);
    if (!case_node) {
        return NULL;
    }
    
    // Parse the word to test
    modern_token_t *word_token = modern_tokenizer_current(parser->tokenizer);
    if (!modern_token_is_word_like(word_token->type) && 
        word_token->type != MODERN_TOK_VARIABLE) {
        free_node_tree(case_node);
        parser_error(parser, "Expected word after 'case'");
        return NULL;
    }
    
    // Store the test word
    case_node->val.str = strdup(word_token->text);
    if (!case_node->val.str) {
        free_node_tree(case_node);
        return NULL;
    }
    modern_tokenizer_advance(parser->tokenizer);
    
    // Skip separators
    skip_separators(parser);
    
    // Expect 'in' keyword
    if (!expect_token(parser, MODERN_TOK_IN)) {
        free_node_tree(case_node);
        return NULL;
    }
    
    // Skip separators after 'in'
    skip_separators(parser);
    
    // Parse case items until 'esac'
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ESAC) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
        
        // Parse pattern(s)
        node_t *case_item = new_node(NODE_COMMAND); // Reuse NODE_COMMAND for case items
        if (!case_item) {
            free_node_tree(case_node);
            return NULL;
        }
        
        // Build pattern string (can be multiple patterns separated by |)
        char *pattern = NULL;
        size_t pattern_len = 0;
        
        do {
            // Build pattern from multiple tokens until ) or |
            char *single_pattern = NULL;
            size_t single_pattern_len = 0;
            
            // Collect tokens for a single pattern until ) or |
            while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_RPAREN) &&
                   !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_PIPE) &&
                   !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF) &&
                   !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ESAC)) {
                
                modern_token_t *pattern_token = modern_tokenizer_current(parser->tokenizer);
                
                // Accept word-like tokens, wildcards, and variables for patterns
                if (modern_token_is_word_like(pattern_token->type) ||
                    pattern_token->type == MODERN_TOK_MULTIPLY ||
                    pattern_token->type == MODERN_TOK_QUESTION ||
                    pattern_token->type == MODERN_TOK_VARIABLE) {
                    
                    size_t token_len = strlen(pattern_token->text);
                    char *new_single_pattern = realloc(single_pattern, single_pattern_len + token_len + 1);
                    if (!new_single_pattern) {
                        free(single_pattern);
                        free_node_tree(case_item);
                        free_node_tree(case_node);
                        return NULL;
                    }
                    single_pattern = new_single_pattern;
                    strcpy(single_pattern + single_pattern_len, pattern_token->text);
                    single_pattern_len += token_len;
                    
                    modern_tokenizer_advance(parser->tokenizer);
                } else {
                    // Unexpected token in pattern
                    break;
                }
            }
            
            // If we didn't collect any pattern tokens, that's an error
            if (!single_pattern) {
                free_node_tree(case_item);
                free_node_tree(case_node);
                parser_error(parser, "Expected pattern in case statement");
                return NULL;
            }
            
            // Append this single pattern to the overall pattern string
            if (pattern) {
                // Add | separator and new pattern
                char *new_pattern = realloc(pattern, pattern_len + 1 + single_pattern_len + 1);
                if (!new_pattern) {
                    free(single_pattern);
                    free(pattern);
                    free_node_tree(case_item);
                    free_node_tree(case_node);
                    return NULL;
                }
                pattern = new_pattern;
                pattern[pattern_len] = '|';
                strcpy(pattern + pattern_len + 1, single_pattern);
                pattern_len += 1 + single_pattern_len;
            } else {
                // First pattern
                pattern = single_pattern;
                pattern_len = single_pattern_len;
                single_pattern = NULL; // Transfer ownership
            }
            
            free(single_pattern);
            
            // Check for | to continue with more patterns
            } while (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_PIPE) &&
                     (modern_tokenizer_advance(parser->tokenizer), true));
        
        // Store pattern in case item
        case_item->val.str = pattern;
        
        // Expect )
        if (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_RPAREN)) {
            free_node_tree(case_item);
            free_node_tree(case_node);
            parser_error(parser, "Expected ')' after case pattern");
            return NULL;
        }
        modern_tokenizer_advance(parser->tokenizer);
        
        // Skip separators
        skip_separators(parser);
        
        // Parse commands until ;; or esac
        node_t *commands = NULL;
        while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ESAC) &&
               !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
            
            // Check for ;; pattern at start of loop
            if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
                modern_token_t *next = modern_tokenizer_peek(parser->tokenizer);
                if (next && next->type == MODERN_TOK_SEMICOLON) {
                    break; // Found ;; - end this case item
                }
                // Single semicolon - consume it and continue parsing commands
                modern_tokenizer_advance(parser->tokenizer);
                continue;
            }
            
            node_t *command = parse_simple_command(parser);
            if (!command) {
                break; // Can't parse more commands
            }
            if (!commands) {
                commands = command;
            } else {
                // Link commands as siblings
                node_t *last = commands;
                while (last->next_sibling) last = last->next_sibling;
                last->next_sibling = command;
            }
            
            // Don't skip separators here - we need to detect ;; explicitly
        }
        
        // Add commands as child of case item
        if (commands) {
            add_child_node(case_item, commands);
        }
        
        // Expect ;;
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
            modern_tokenizer_advance(parser->tokenizer); // Consume first ;
            if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
                modern_tokenizer_advance(parser->tokenizer); // Consume second ;
            }
        }
        
        // Only skip non-semicolon separators (newlines, whitespace)
        while (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_NEWLINE) ||
               modern_tokenizer_match(parser->tokenizer, MODERN_TOK_WHITESPACE)) {
            modern_tokenizer_advance(parser->tokenizer);
        }
        
        // Add case item to case statement
        add_child_node(case_node, case_item);
    }
    
    // Expect 'esac'
    if (!expect_token(parser, MODERN_TOK_ESAC)) {
        free_node_tree(case_node);
        return NULL;
    }
    return case_node;
}

// Helper function to check if current position is a function definition
static bool is_function_definition(parser_modern_t *parser) {
    if (!parser || !parser->tokenizer) return false;
    
    modern_token_t *current = modern_tokenizer_current(parser->tokenizer);
    if (!current || !modern_token_is_word_like(current->type)) {
        return false;
    }
    
    modern_token_t *next = modern_tokenizer_peek(parser->tokenizer);
    if (!next || next->type != MODERN_TOK_LPAREN) {
        return false;
    }
    
    // We have word() - this looks like a function definition
    return true;
}

// Parse function definition: name() { commands; } or function name() { commands; }
static node_t *parse_function_definition(parser_modern_t *parser) {
    modern_token_t *current = modern_tokenizer_current(parser->tokenizer);
    
    // Handle "function" keyword form
    if (current && current->type == MODERN_TOK_FUNCTION) {
        modern_tokenizer_advance(parser->tokenizer);
        current = modern_tokenizer_current(parser->tokenizer);
    }
    
    if (!current || !modern_token_is_word_like(current->type)) {
        parser_error(parser, "Expected function name");
        return NULL;
    }
    
    // Create function node
    node_t *function_node = new_node(NODE_FUNCTION);
    if (!function_node) return NULL;
    
    // Store function name
    function_node->val.str = strdup(current->text);
    if (!function_node->val.str) {
        free_node_tree(function_node);
        return NULL;
    }
    modern_tokenizer_advance(parser->tokenizer);
    
    // Expect '('
    if (!expect_token(parser, MODERN_TOK_LPAREN)) {
        free_node_tree(function_node);
        return NULL;
    }
    
    // Expect ')'
    if (!expect_token(parser, MODERN_TOK_RPAREN)) {
        free_node_tree(function_node);
        return NULL;
    }
    
    // Skip separators before '{'
    skip_separators(parser);
    
    // Expect '{'
    if (!expect_token(parser, MODERN_TOK_LBRACE)) {
        free_node_tree(function_node);
        return NULL;
    }
    
    // Skip separators after '{'
    skip_separators(parser);
    
    // Parse function body until '}'
    node_t *body = NULL;
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_RBRACE) &&
           !modern_tokenizer_match(parser->tokenizer, MODERN_TOK_EOF)) {
        
        node_t *command = parse_simple_command(parser);
        if (!command) {
            break; // Can't parse more commands
        }
        
        if (!body) {
            body = command;
        } else {
            // Link commands as siblings
            node_t *last = body;
            while (last->next_sibling) last = last->next_sibling;
            last->next_sibling = command;
        }
        
        // Skip separators after command
        skip_separators(parser);
    }
    
    // Add body as child of function
    if (body) {
        add_child_node(function_node, body);
    }
    
    // Expect '}'
    if (!expect_token(parser, MODERN_TOK_RBRACE)) {
        free_node_tree(function_node);
        return NULL;
    }
    
    return function_node;
}
