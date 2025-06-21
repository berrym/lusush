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
        
        node_t *command = parse_pipeline(parser);
        if (!command) {
            if (!parser->has_error) {
                parser_error(parser, "Failed to parse command");
            }
            break;
        }
        
        if (!first_command) {
            first_command = current = command;
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
            default:
                // Other keywords not implemented yet
                printf("DEBUG: Unhandled keyword type %d (%s)\n", 
                       current->type, modern_token_type_name(current->type));
                parser_error(parser, "Control structure not implemented");
                return NULL;
        }
    }
    
    // Check for assignment (word followed by =)
    if (modern_token_is_word_like(current->type)) {
        modern_token_t *next = modern_tokenizer_peek(parser->tokenizer);
        if (next && next->type == MODERN_TOK_ASSIGN) {
            // This is an assignment: variable=value
            node_t *command = new_node(NODE_COMMAND);
            if (!command) return NULL;
            
            // Create assignment string: "variable=value"
            size_t var_len = strlen(current->text);
            modern_tokenizer_advance(parser->tokenizer); // consume variable name
            modern_tokenizer_advance(parser->tokenizer); // consume '='
            
            modern_token_t *value = modern_tokenizer_current(parser->tokenizer);
            if (value && modern_token_is_word_like(value->type)) {
                size_t value_len = strlen(value->text);
                char *assignment = malloc(var_len + 1 + value_len + 1);
                if (assignment) {
                    strcpy(assignment, current->text);
                    strcat(assignment, "=");
                    strcat(assignment, value->text);
                    command->val.str = assignment;
                }
                modern_tokenizer_advance(parser->tokenizer); // consume value
            } else {
                // Assignment with empty value: variable=
                char *assignment = malloc(var_len + 2);
                if (assignment) {
                    strcpy(assignment, current->text);
                    strcat(assignment, "=");
                    command->val.str = assignment;
                }
            }
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
            arg_token->type == MODERN_TOK_RBRACKET ||
            arg_token->type == MODERN_TOK_REDIRECT_IN ||
            arg_token->type == MODERN_TOK_REDIRECT_OUT) {
            
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
    
    // Handle flexible separator: semicolon can substitute for THEN
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
        modern_tokenizer_advance(parser->tokenizer);
        // After semicolon, optionally accept THEN keyword
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_THEN)) {
            modern_tokenizer_advance(parser->tokenizer);
        }
    } else if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_THEN)) {
        modern_tokenizer_advance(parser->tokenizer);
    } else {
        parser_error(parser, "Expected ';' or 'then' after if condition");
        free_node_tree(if_node);
        return NULL;
    }
    
    // Parse then body - parse until we hit 'else', 'elif', or 'fi'
    node_t *then_body = parse_pipeline(parser);
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
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_ELSE)) {
        modern_tokenizer_advance(parser->tokenizer);
        node_t *else_body = parse_pipeline(parser);
        if (!else_body) {
            free_node_tree(if_node);
            return NULL;
        }
        add_child_node(if_node, else_body);
        
        // Handle optional semicolon after else body
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
            modern_tokenizer_advance(parser->tokenizer);
        }
    }
    
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
    
    // Handle flexible separator: semicolon can substitute for DO
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
        modern_tokenizer_advance(parser->tokenizer);
        // After semicolon, optionally accept DO keyword
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DO)) {
            modern_tokenizer_advance(parser->tokenizer);
        }
    } else if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DO)) {
        modern_tokenizer_advance(parser->tokenizer);
    } else {
        parser_error(parser, "Expected ';' or 'do' after while condition");
        free_node_tree(while_node);
        return NULL;
    }
    
    // Parse body
    node_t *body = parse_pipeline(parser);
    if (!body) {
        free_node_tree(while_node);
        return NULL;
    }
    add_child_node(while_node, body);
    
    // Handle optional semicolon before DONE
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
        modern_tokenizer_advance(parser->tokenizer);
    }
    
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
    
    // Collect all words until ';' or 'do'
    while (!modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON) &&
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
    
    // Handle flexible separator: semicolon can substitute for DO
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
        modern_tokenizer_advance(parser->tokenizer);
        // After semicolon, optionally accept DO keyword
        if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DO)) {
            modern_tokenizer_advance(parser->tokenizer);
        }
    } else if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_DO)) {
        modern_tokenizer_advance(parser->tokenizer);
    } else {
        parser_error(parser, "Expected ';' or 'do' after for word list");
        free_node_tree(for_node);
        return NULL;
    }
    
    // Parse body
    node_t *body = parse_pipeline(parser);
    if (!body) {
        free_node_tree(for_node);
        return NULL;
    }
    add_child_node(for_node, body);
    
    // Handle optional semicolon before DONE
    if (modern_tokenizer_match(parser->tokenizer, MODERN_TOK_SEMICOLON)) {
        modern_tokenizer_advance(parser->tokenizer);
    }
    
    if (!expect_token(parser, MODERN_TOK_DONE)) {
        free_node_tree(for_node);
        return NULL;
    }
    
    return for_node;
}
