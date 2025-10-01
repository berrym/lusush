/**
 * Modern POSIX Shell Parser Implementation
 *
 * Clean recursive descent parser that properly handles POSIX shell grammar
 * with correct token boundary management and error handling.
 */

#include "../include/parser.h"

#include "../include/node.h"
#include "../include/tokenizer.h"
#include "../include/executor.h"
#include "../include/lusush.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations
static node_t *parse_command_list(parser_t *parser);
static node_t *parse_pipeline(parser_t *parser);
static node_t *parse_simple_command(parser_t *parser);
static node_t *parse_control_structure(parser_t *parser);
static node_t *parse_brace_group(parser_t *parser);
static node_t *parse_subshell(parser_t *parser);
static node_t *parse_if_statement(parser_t *parser);
static node_t *parse_while_statement(parser_t *parser);
static node_t *parse_until_statement(parser_t *parser);
static node_t *parse_for_statement(parser_t *parser);
static node_t *parse_case_statement(parser_t *parser);
static node_t *parse_function_definition(parser_t *parser);
static bool is_function_definition(parser_t *parser);
static node_t *parse_logical_expression(parser_t *parser);
static node_t *parse_redirection(parser_t *parser);

// Forward declarations for POSIX compliance
bool is_posix_mode_enabled(void);
static char *collect_heredoc_content(parser_t *parser, const char *delimiter,
                                     bool strip_tabs, bool expand_variables);
static void set_parser_error(parser_t *parser, const char *message);
static bool expect_token(parser_t *parser, token_type_t expected);



// Create new parser
parser_t *parser_new(const char *input) {
    if (!input) {
        return NULL;
    }

    parser_t *parser = malloc(sizeof(parser_t));
    if (!parser) {
        return NULL;
    }

    parser->tokenizer = tokenizer_new(input);
    if (!parser->tokenizer) {
        free(parser);
        return NULL;
    }

    parser->error_message = NULL;
    parser->has_error = false;

    return parser;
}

// Free parser
void parser_free(parser_t *parser) {
    if (!parser) {
        return;
    }

    tokenizer_free(parser->tokenizer);
    free(parser);
}

// Check for errors
// Error handling
bool parser_has_error(parser_t *parser) { return parser && parser->has_error; }

const char *parser_error(parser_t *parser) {
    return parser ? parser->error_message : "Invalid parser";
}

// Set parser error
static void set_parser_error(parser_t *parser, const char *message) {
    if (parser) {
        parser->error_message = message;
        parser->has_error = true;
    }
}

// Expect specific token type
static bool expect_token(parser_t *parser, token_type_t expected) {
    if (!tokenizer_match(parser->tokenizer, expected)) {
        char error_buf[256];
        token_t *current = tokenizer_current(parser->tokenizer);
        snprintf(error_buf, sizeof(error_buf), "Expected %s but got %s",
                 token_type_name(expected),
                 current ? token_type_name(current->type) : "EOF");
        set_parser_error(parser, strdup(error_buf));
        return false;
    }
    tokenizer_advance(parser->tokenizer);
    return true;
}

// Main parsing entry point
node_t *parser_parse(parser_t *parser) {
    if (!parser) {
        return NULL;
    }

    // Skip initial whitespace and comments
    while (tokenizer_match(parser->tokenizer, TOK_WHITESPACE) ||
           tokenizer_match(parser->tokenizer, TOK_COMMENT) ||
           tokenizer_match(parser->tokenizer, TOK_NEWLINE)) {
        tokenizer_advance(parser->tokenizer);
    }

    if (tokenizer_match(parser->tokenizer, TOK_EOF)) {
        return NULL; // Empty input
    }

    return parse_command_list(parser);
}

// Parse command line (sequence of commands)
node_t *parser_parse_command_line(parser_t *parser) {
    return parse_command_list(parser);
}

// Helper function to skip separators (semicolons, newlines, whitespace)
static void skip_separators(parser_t *parser) {
    while (tokenizer_match(parser->tokenizer, TOK_SEMICOLON) ||
           tokenizer_match(parser->tokenizer, TOK_NEWLINE) ||
           tokenizer_match(parser->tokenizer, TOK_WHITESPACE) ||
           tokenizer_match(parser->tokenizer, TOK_COMMENT)) {
        tokenizer_advance(parser->tokenizer);
    }
}

// Parse command body for control structures - parses multiple commands until
// terminator
static node_t *parse_command_body(parser_t *parser, token_type_t terminator) {
    node_t *first_command = NULL;
    node_t *current = NULL;

    while (!tokenizer_match(parser->tokenizer, terminator) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF) && !parser->has_error) {

        // Skip separators between commands
        skip_separators(parser);

        // Check again for terminator after skipping separators
        if (tokenizer_match(parser->tokenizer, terminator) ||
            tokenizer_match(parser->tokenizer, TOK_EOF)) {
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
    }

    return first_command;
}

// Parse command body for IF statements - stops at else, elif, or fi
static node_t *parse_if_body(parser_t *parser) {
    // Create a command list node to hold all commands
    node_t *command_list = new_node(NODE_COMMAND_LIST);
    if (!command_list) {
        return NULL;
    }

    while (!tokenizer_match(parser->tokenizer, TOK_ELSE) &&
           !tokenizer_match(parser->tokenizer, TOK_ELIF) &&
           !tokenizer_match(parser->tokenizer, TOK_FI) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF) && !parser->has_error) {

        // Skip separators between commands
        skip_separators(parser);

        // Check again for terminators after skipping separators
        if (tokenizer_match(parser->tokenizer, TOK_ELSE) ||
            tokenizer_match(parser->tokenizer, TOK_ELIF) ||
            tokenizer_match(parser->tokenizer, TOK_FI) ||
            tokenizer_match(parser->tokenizer, TOK_EOF)) {
            break;
        }

        node_t *command = parse_logical_expression(parser);
        if (!command) {
            if (!parser->has_error) {
                break; // End of input
            }
            free_node_tree(command_list);
            return NULL;
        }

        // Add command as child of the command list
        add_child_node(command_list, command);

        // Skip separators after command
        skip_separators(parser);
    }

    return command_list;
}

// Parse logical operators (and_or level)
static node_t *parse_logical_expression(parser_t *parser) {
    node_t *left = parse_pipeline(parser);
    if (!left) {
        return NULL;
    }

    while (tokenizer_match(parser->tokenizer, TOK_LOGICAL_AND) ||
           tokenizer_match(parser->tokenizer, TOK_LOGICAL_OR)) {

        token_type_t op_type = tokenizer_current(parser->tokenizer)->type;
        tokenizer_advance(parser->tokenizer); // consume operator

        // Skip whitespace after operator
        skip_separators(parser);

        node_t *right = parse_pipeline(parser);
        if (!right) {
            free_node_tree(left);
            return NULL;
        }

        // Create logical operator node
        node_t *logical_node = new_node(
            op_type == TOK_LOGICAL_AND ? NODE_LOGICAL_AND : NODE_LOGICAL_OR);
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
static node_t *parse_command_list(parser_t *parser) {
    node_t *first_command = NULL;
    node_t *current = NULL;

    while (!tokenizer_match(parser->tokenizer, TOK_EOF) && !parser->has_error) {
        // Skip separators, newlines, and comments
        while (tokenizer_match(parser->tokenizer, TOK_SEMICOLON) ||
               tokenizer_match(parser->tokenizer, TOK_NEWLINE) ||
               tokenizer_match(parser->tokenizer, TOK_COMMENT)) {
            tokenizer_advance(parser->tokenizer);
        }

        if (tokenizer_match(parser->tokenizer, TOK_EOF)) {
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
        if (tokenizer_match(parser->tokenizer, TOK_EOF) ||
            tokenizer_match(parser->tokenizer, TOK_DONE) ||
            tokenizer_match(parser->tokenizer, TOK_FI) ||
            tokenizer_match(parser->tokenizer, TOK_ELSE) ||
            tokenizer_match(parser->tokenizer, TOK_ELIF)) {
            break;
        }
    }

    return first_command;
}

// Parse pipeline (commands connected by |)
static node_t *parse_pipeline(parser_t *parser) {
    node_t *left = parse_simple_command(parser);
    if (!left) {
        return NULL;
    }

    if (tokenizer_match(parser->tokenizer, TOK_PIPE)) {
        tokenizer_advance(parser->tokenizer); // consume |

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
        left = pipe_node;
    }

    // Check for background execution
    if (tokenizer_match(parser->tokenizer, TOK_AND)) {
        tokenizer_advance(parser->tokenizer); // consume &

        node_t *background_node = new_node(NODE_BACKGROUND);
        if (!background_node) {
            free_node_tree(left);
            return NULL;
        }

        add_child_node(background_node, left);
        return background_node;
    }

    return left;
}

// Parse simple command or control structure
static node_t *parse_simple_command(parser_t *parser) {
    token_t *current = tokenizer_current(parser->tokenizer);
    if (!current) {
        return NULL;
    }

    // Check for brace group
    if (current->type == TOK_LBRACE) {
        return parse_brace_group(parser);
    }

    // Check for subshell
    if (current->type == TOK_LPAREN) {
        return parse_subshell(parser);
    }

    // Check for control structures
    if (token_is_keyword(current->type)) {
        if (getenv("NEW_PARSER_DEBUG")) {
            printf("DEBUG: Found keyword token type %d (%s)\n", current->type,
                   token_type_name(current->type));
        }
        switch (current->type) {
        case TOK_IF:
            return parse_if_statement(parser);
        case TOK_WHILE:
            return parse_while_statement(parser);
        case TOK_UNTIL:
            return parse_until_statement(parser);
        case TOK_FOR:
            return parse_for_statement(parser);
        case TOK_CASE:
            return parse_case_statement(parser);
        case TOK_FUNCTION:
            return parse_function_definition(parser);
        default:
            // Other keywords not implemented yet
            printf("DEBUG: Unhandled keyword type %d (%s)\n", current->type,
                   token_type_name(current->type));
            return NULL;
        }
    }

    // Check for function definition (word followed by ())
    if (token_is_word_like(current->type) && is_function_definition(parser)) {
        return parse_function_definition(parser);
    }

    // Check for assignment (word followed by =)
    if (token_is_word_like(current->type)) {
        token_t *next = tokenizer_peek(parser->tokenizer);
        if (next && next->type == TOK_ASSIGN) {
            // This is an assignment: variable=value
            node_t *command = new_node(NODE_COMMAND);
            if (!command) {
                return NULL;
            }

            // FIX: Save variable name BEFORE advancing tokenizer
            char *var_name = strdup(current->text);
            if (!var_name) {
                free_node_tree(command);
                return NULL;
            }

            tokenizer_advance(parser->tokenizer); // consume variable name
            tokenizer_advance(parser->tokenizer); // consume '='

            token_t *value = tokenizer_current(parser->tokenizer);
            if (value &&
                (token_is_word_like(value->type) ||
                 value->type == TOK_VARIABLE || value->type == TOK_ARITH_EXP ||
                 value->type == TOK_COMMAND_SUB ||
                 value->type == TOK_BACKQUOTE)) {
                size_t var_len = strlen(var_name);
                size_t value_len = strlen(value->text);
                char *assignment = malloc(var_len + 1 + value_len + 1);
                if (assignment) {
                    strcpy(assignment, var_name);
                    strcat(assignment, "=");
                    strcat(assignment, value->text);
                    command->val.str = assignment;
                }
                tokenizer_advance(parser->tokenizer); // consume value
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
    if (current->type == TOK_ERROR) {
        set_parser_error(parser, "syntax error: unterminated quoted string");
        return NULL;
    }

    if (!token_is_word_like(current->type) && current->type != TOK_LBRACKET) {
        set_parser_error(parser, "Expected command name");
        return NULL;
    }

    node_t *command = new_node(NODE_COMMAND);
    if (!command) {
        return NULL;
    }

    // Set command name
    command->val.str = strdup(current->text);
    tokenizer_advance(parser->tokenizer);

    // Parse arguments and redirections
    while (!tokenizer_match(parser->tokenizer, TOK_EOF) &&
           !tokenizer_match(parser->tokenizer, TOK_SEMICOLON) &&
           !tokenizer_match(parser->tokenizer, TOK_NEWLINE) &&
           !tokenizer_match(parser->tokenizer, TOK_PIPE) &&
           !tokenizer_match(parser->tokenizer, TOK_AND) &&
           !tokenizer_match(parser->tokenizer, TOK_LOGICAL_AND) &&
           !tokenizer_match(parser->tokenizer, TOK_LOGICAL_OR)) {

        token_t *arg_token = tokenizer_current(parser->tokenizer);
        if (!arg_token) {
            break;
        }

        // Check for redirection tokens
        if (arg_token->type == TOK_REDIRECT_OUT ||
            arg_token->type == TOK_REDIRECT_IN ||
            arg_token->type == TOK_APPEND || arg_token->type == TOK_HEREDOC ||
            arg_token->type == TOK_HEREDOC_STRIP ||
            arg_token->type == TOK_HERESTRING ||
            arg_token->type == TOK_REDIRECT_ERR ||
            arg_token->type == TOK_REDIRECT_BOTH ||
            arg_token->type == TOK_APPEND_ERR ||
            arg_token->type == TOK_REDIRECT_FD ||
            arg_token->type == TOK_REDIRECT_CLOBBER) {

            node_t *redir_node = parse_redirection(parser);
            if (!redir_node) {
                free_node_tree(command);
                return NULL;
            }

            add_child_node(command, redir_node);
        }
        // Handle all argument tokens with unified concatenation logic
        else if (arg_token->type == TOK_STRING ||
                 arg_token->type == TOK_EXPANDABLE_STRING ||
                 arg_token->type == TOK_ARITH_EXP ||
                 arg_token->type == TOK_COMMAND_SUB ||
                 arg_token->type == TOK_BACKQUOTE ||
                 token_is_word_like(arg_token->type) ||
                 token_is_keyword(arg_token->type) ||
                 arg_token->type == TOK_VARIABLE ||
                 arg_token->type == TOK_RBRACKET ||
                 arg_token->type == TOK_ASSIGN || arg_token->type == TOK_GLOB ||
                 arg_token->type == TOK_QUESTION ||
                 arg_token->type == TOK_NOT_EQUAL) {

            // Check for consecutive tokens that should be concatenated
            typedef struct {
                token_type_t type;
                char *text;
            } token_info_t;

            token_info_t *collected_tokens = NULL;
            int token_count = 0;
            size_t last_end_pos = arg_token->position + strlen(arg_token->text);

            // Collect all consecutive tokens without whitespace
            while (arg_token && (arg_token->type == TOK_STRING ||
                                 arg_token->type == TOK_EXPANDABLE_STRING ||
                                 arg_token->type == TOK_ARITH_EXP ||
                                 arg_token->type == TOK_COMMAND_SUB ||
                                 arg_token->type == TOK_BACKQUOTE ||
                                 token_is_word_like(arg_token->type) ||
                                 token_is_keyword(arg_token->type) ||
                                 arg_token->type == TOK_VARIABLE ||
                                 arg_token->type == TOK_RBRACKET ||
                                 arg_token->type == TOK_ASSIGN ||
                                 arg_token->type == TOK_GLOB ||
                                 arg_token->type == TOK_QUESTION ||
                                 arg_token->type == TOK_NOT_EQUAL)) {

                // Expand collected_tokens array
                token_info_t *new_tokens = realloc(
                    collected_tokens, (token_count + 1) * sizeof(token_info_t));
                if (!new_tokens) {
                    for (int i = 0; i < token_count; i++) {
                        free(collected_tokens[i].text);
                    }
                    free(collected_tokens);
                    free_node_tree(command);
                    return NULL;
                }
                collected_tokens = new_tokens;

                // Store token information
                collected_tokens[token_count].type = arg_token->type;
                collected_tokens[token_count].text = strdup(arg_token->text);
                token_count++;

                last_end_pos = arg_token->position + strlen(arg_token->text);
                tokenizer_advance(parser->tokenizer);
                token_t *next_token = tokenizer_current(parser->tokenizer);

                // Check if the next token is adjacent (no whitespace between)
                if (next_token && next_token->position != last_end_pos) {
                    break; // There's whitespace between tokens
                }

                arg_token = next_token;
            }

            // Create nodes based on what we collected
            if (token_count == 1) {
                // Single token - create appropriate node type
                node_t *arg_node = NULL;
                switch (collected_tokens[0].type) {
                case TOK_STRING:
                    arg_node = new_node(NODE_STRING_LITERAL);
                    break;
                case TOK_EXPANDABLE_STRING:
                    arg_node = new_node(NODE_STRING_EXPANDABLE);
                    break;
                case TOK_ARITH_EXP:
                    arg_node = new_node(NODE_ARITH_EXP);
                    break;
                case TOK_COMMAND_SUB:
                case TOK_BACKQUOTE:
                    arg_node = new_node(NODE_COMMAND_SUB);
                    break;
                default:
                    arg_node = new_node(NODE_VAR);
                    break;
                }

                if (arg_node) {
                    arg_node->val.str = strdup(collected_tokens[0].text);
                    add_child_node(command, arg_node);
                }
            } else if (token_count > 1) {
                // Multiple tokens - create concatenated string
                size_t total_len = 0;
                for (int i = 0; i < token_count; i++) {
                    total_len += strlen(collected_tokens[i].text);
                }

                char *concatenated = malloc(total_len + 1);
                if (concatenated) {
                    concatenated[0] = '\0';
                    for (int i = 0; i < token_count; i++) {
                        strcat(concatenated, collected_tokens[i].text);
                    }

                    node_t *arg_node = new_node(NODE_STRING_EXPANDABLE);
                    if (arg_node) {
                        arg_node->val.str = concatenated;
                        add_child_node(command, arg_node);
                    } else {
                        free(concatenated);
                    }
                }
            }

            // Clean up collected tokens
            for (int i = 0; i < token_count; i++) {
                free(collected_tokens[i].text);
            }
            free(collected_tokens);
        } else {
            break; // Stop parsing arguments
        }
    }

    return command;
}

// Parse brace group { commands; }
static node_t *parse_brace_group(parser_t *parser) {
    token_t *current = tokenizer_current(parser->tokenizer);
    if (!current || current->type != TOK_LBRACE) {
        set_parser_error(parser, "Expected '{'");
        return NULL;
    }

    // Create brace group node
    node_t *group_node = new_node(NODE_BRACE_GROUP);
    if (!group_node) {
        set_parser_error(parser, "Failed to create brace group node");
        return NULL;
    }

    // Consume '{'
    tokenizer_advance(parser->tokenizer);

    // Skip whitespace and newlines after '{'
    skip_separators(parser);

    // Parse commands until '}'
    while (!tokenizer_match(parser->tokenizer, TOK_RBRACE) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF) && !parser->has_error) {

        node_t *command = parse_logical_expression(parser);
        if (!command) {
            if (!parser->has_error) {
                break; // End of input
            }
            free_node_tree(group_node);
            return NULL;
        }

        add_child_node(group_node, command);

        // Skip separators between commands
        skip_separators(parser);
    }

    // Expect '}'
    if (!expect_token(parser, TOK_RBRACE)) {
        free_node_tree(group_node);
        return NULL;
    }

    return group_node;
}

// Parse subshell ( commands )
static node_t *parse_subshell(parser_t *parser) {
    token_t *current = tokenizer_current(parser->tokenizer);
    if (!current || current->type != TOK_LPAREN) {
        set_parser_error(parser, "Expected '('");
        return NULL;
    }

    // Create subshell node
    node_t *subshell_node = new_node(NODE_SUBSHELL);
    if (!subshell_node) {
        set_parser_error(parser, "Failed to create subshell node");
        return NULL;
    }

    // Consume '('
    tokenizer_advance(parser->tokenizer);

    // Skip whitespace and newlines after '('
    skip_separators(parser);

    // Parse commands until ')'
    while (!tokenizer_match(parser->tokenizer, TOK_RPAREN) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF) && !parser->has_error) {

        node_t *command = parse_logical_expression(parser);
        if (!command) {
            if (!parser->has_error) {
                break; // End of input
            }
            free_node_tree(subshell_node);
            return NULL;
        }

        add_child_node(subshell_node, command);

        // Skip separators between commands
        skip_separators(parser);
    }

    // Expect ')'
    if (!expect_token(parser, TOK_RPAREN)) {
        free_node_tree(subshell_node);
        return NULL;
    }

    return subshell_node;
}

// Parse redirection
static node_t *parse_redirection(parser_t *parser) {
    token_t *redir_token = tokenizer_current(parser->tokenizer);
    if (!redir_token) {
        return NULL;
    }

    node_type_t node_type;
    switch (redir_token->type) {
    case TOK_REDIRECT_OUT:
        node_type = NODE_REDIR_OUT;
        break;
    case TOK_REDIRECT_IN:
        node_type = NODE_REDIR_IN;
        break;
    case TOK_APPEND:
        node_type = NODE_REDIR_APPEND;
        break;
    case TOK_HEREDOC:
        node_type = NODE_REDIR_HEREDOC;
        break;
    case TOK_HEREDOC_STRIP:
        node_type = NODE_REDIR_HEREDOC_STRIP;
        break;
    case TOK_HERESTRING:
        node_type = NODE_REDIR_HERESTRING;
        break;
    case TOK_REDIRECT_ERR:
        node_type = NODE_REDIR_ERR;
        break;
    case TOK_REDIRECT_BOTH:
        node_type = NODE_REDIR_BOTH;
        break;
    case TOK_APPEND_ERR:
        node_type = NODE_REDIR_ERR_APPEND;
        break;
    case TOK_REDIRECT_FD:
        node_type = NODE_REDIR_FD;
        break;
    case TOK_REDIRECT_CLOBBER:
        node_type = NODE_REDIR_CLOBBER;
        break;
    default:
        set_parser_error(parser, "Unknown redirection token");
        return NULL;
    }

    node_t *redir_node = new_node(node_type);
    if (!redir_node) {
        return NULL;
    }

    // Store the redirection operator
    redir_node->val.str = strdup(redir_token->text);
    tokenizer_advance(parser->tokenizer);

    // Parse the target (filename or here document content)
    token_t *target_token = tokenizer_current(parser->tokenizer);

    // For NODE_REDIR_FD, the target is embedded in the redirection token itself
    if (node_type == NODE_REDIR_FD) {
        // No separate target token needed for file descriptor redirections
        return redir_node;
    }

    if (!target_token || !token_is_word_like(target_token->type)) {
        if (node_type == NODE_REDIR_HEREDOC ||
            node_type == NODE_REDIR_HEREDOC_STRIP) {
            // For here documents, the delimiter might be quoted or special
            if (target_token && (target_token->type == TOK_STRING ||
                                 target_token->type == TOK_EXPANDABLE_STRING ||
                                 token_is_word_like(target_token->type))) {
                // Valid here document delimiter
            } else {
                set_parser_error(parser, "Expected here document delimiter");
                free_node_tree(redir_node);
                return NULL;
            }
        } else {
            set_parser_error(parser, "Expected redirection target");
            free_node_tree(redir_node);
            return NULL;
        }
    }

    // For here documents, collect the content
    if (node_type == NODE_REDIR_HEREDOC ||
        node_type == NODE_REDIR_HEREDOC_STRIP) {
        // Store delimiter before advancing tokenizer
        char *delimiter = strdup(target_token->text);
        bool strip_tabs = (node_type == NODE_REDIR_HEREDOC_STRIP);

        // Check if delimiter is quoted (any quoted delimiter disables expansion)
        bool expand_variables = true;
        if (target_token->type == TOK_STRING || target_token->type == TOK_EXPANDABLE_STRING) {
            // Any quoted string - disable expansion per POSIX
            expand_variables = false;
        }
        // Only unquoted delimiters allow variable expansion

        // Advance past the delimiter token first
        tokenizer_advance(parser->tokenizer);

        // Collect the here document content (this will advance the tokenizer
        // further)
        char *content = collect_heredoc_content(parser, delimiter, strip_tabs, expand_variables);
        if (!content) {
            free(delimiter);
            free_node_tree(redir_node);
            return NULL;
        }

        // Store delimiter in the redirection node value
        redir_node->val.str = delimiter; // Transfer ownership

        // Create content node with the collected content
        node_t *content_node = new_node(NODE_VAR);
        if (!content_node) {
            free(content);
            free_node_tree(redir_node);
            return NULL;
        }
        content_node->val.str = content; // Transfer ownership
        add_child_node(redir_node, content_node);

        // Create a second child node to store the expand_variables flag
        node_t *expand_flag_node = new_node(NODE_VAR);
        if (!expand_flag_node) {
            free_node_tree(redir_node);
            return NULL;
        }
        expand_flag_node->val.str = strdup(expand_variables ? "1" : "0");
        add_child_node(redir_node, expand_flag_node);

        return redir_node;
    } else {
        // Regular redirection - handle token concatenation for variables
        char *concatenated_target = NULL;
        size_t total_len = 0;
        size_t last_end_pos =
            target_token->position + strlen(target_token->text);

        // Collect all consecutive tokens without whitespace (like
        // /tmp/file_$VAR)
        token_t *current_token = target_token;
        while (current_token && (token_is_word_like(current_token->type) ||
                                 current_token->type == TOK_VARIABLE ||
                                 current_token->type == TOK_ARITH_EXP ||
                                 current_token->type == TOK_COMMAND_SUB ||
                                 current_token->type == TOK_BACKQUOTE)) {

            size_t token_len = strlen(current_token->text);
            char *new_target =
                realloc(concatenated_target, total_len + token_len + 1);
            if (!new_target) {
                free(concatenated_target);
                free_node_tree(redir_node);
                return NULL;
            }
            concatenated_target = new_target;

            strcpy(concatenated_target + total_len, current_token->text);
            total_len += token_len;
            last_end_pos = current_token->position + token_len;

            tokenizer_advance(parser->tokenizer);
            current_token = tokenizer_current(parser->tokenizer);

            // Check if the next token is adjacent (no whitespace between)
            if (current_token && current_token->position != last_end_pos) {
                break; // There's whitespace between tokens
            }
        }

        if (concatenated_target) {
            concatenated_target[total_len] = '\0';
        } else {
            // Fallback to single token
            concatenated_target = strdup(target_token->text);
            tokenizer_advance(parser->tokenizer);
        }

        node_t *target_node = new_node(NODE_VAR);
        if (!target_node) {
            free(concatenated_target);
            free_node_tree(redir_node);
            return NULL;
        }
        target_node->val.str = concatenated_target;
        add_child_node(redir_node, target_node);

        return redir_node;
    }
}

// Collect here document content until delimiter is found
static char *collect_heredoc_content(parser_t *parser, const char *delimiter,
                                     bool strip_tabs, bool expand_variables) {
    if (!parser || !delimiter) {
        return NULL;
    }

    // expand_variables is now passed as parameter from tokenizer analysis

    tokenizer_t *tokenizer = parser->tokenizer;

    // Find the start of the here document content by searching for << delimiter
    // in input
    size_t content_start = 0;

    // Look for "<<" followed by the delimiter in the input
    // For quoted delimiters, we need to match without quotes
    const char *match_delimiter = delimiter;
    char *unquoted_delimiter = NULL;
    if ((delimiter[0] == '"' || delimiter[0] == '\'') &&
        strlen(delimiter) > 2 &&
        delimiter[strlen(delimiter) - 1] == delimiter[0]) {
        size_t delim_len = strlen(delimiter);
        unquoted_delimiter = malloc(delim_len - 1);
        if (unquoted_delimiter) {
            strncpy(unquoted_delimiter, delimiter + 1, delim_len - 2);
            unquoted_delimiter[delim_len - 2] = '\0';
            match_delimiter = unquoted_delimiter;
        }
    }



    size_t delimiter_len = strlen(match_delimiter);

    for (size_t i = 0; i < tokenizer->input_length - 1; i++) {
        if (tokenizer->input[i] == '<' && tokenizer->input[i + 1] == '<') {
            // Found <<, now check if delimiter follows
            size_t delimiter_pos = i + 2;

            // Skip optional '-' for <<-
            if (delimiter_pos < tokenizer->input_length &&
                tokenizer->input[delimiter_pos] == '-') {
                delimiter_pos++;
            }

            // Skip whitespace
            while (delimiter_pos < tokenizer->input_length &&
                   (tokenizer->input[delimiter_pos] == ' ' ||
                    tokenizer->input[delimiter_pos] == '\t')) {
                delimiter_pos++;
            }

            // Try to match delimiter - first check if it's quoted in the input
            bool found_delimiter = false;
            size_t delim_end_pos = delimiter_pos;
            
            // Check for quoted delimiter in input (like 'EOF' or "EOF")
            if (delimiter_pos < tokenizer->input_length &&
                (tokenizer->input[delimiter_pos] == '\'' || tokenizer->input[delimiter_pos] == '"')) {
                char quote = tokenizer->input[delimiter_pos];
                delim_end_pos = delimiter_pos + 1;
                
                // Find matching quote
                while (delim_end_pos < tokenizer->input_length &&
                       tokenizer->input[delim_end_pos] != quote) {
                    delim_end_pos++;
                }
                
                if (delim_end_pos < tokenizer->input_length &&
                    tokenizer->input[delim_end_pos] == quote) {
                    // Extract the quoted delimiter content
                    size_t quoted_len = delim_end_pos - delimiter_pos - 1;
                    if (quoted_len == strlen(match_delimiter) &&
                        strncmp(&tokenizer->input[delimiter_pos + 1], match_delimiter, quoted_len) == 0) {
                        found_delimiter = true;
                        delim_end_pos++; // Include the closing quote
                    }
                }
            } else {
                // Check for unquoted delimiter
                size_t match_len = strlen(match_delimiter);
                if (delimiter_pos + match_len <= tokenizer->input_length &&
                    strncmp(&tokenizer->input[delimiter_pos], match_delimiter, match_len) == 0) {
                    found_delimiter = true;
                    delim_end_pos = delimiter_pos + match_len;
                }
            }

            if (found_delimiter) {
                // Found our << delimiter, find the end of this line
                content_start = delim_end_pos;
                while (content_start < tokenizer->input_length &&
                       tokenizer->input[content_start] != '\n') {
                    content_start++;
                }
                if (content_start < tokenizer->input_length) {
                    content_start++; // Skip the newline
                }
                break;
            }
        }
    }


    // Collect lines until we find the delimiter
    size_t content_size = 0;
    size_t content_capacity = 1024;
    char *content = malloc(content_capacity);
    if (!content) {
        return NULL;
    }
    content[0] = '\0';

    size_t line_start = content_start;
    while (line_start < tokenizer->input_length) {
        // Find end of current line
        size_t line_end = line_start;
        while (line_end < tokenizer->input_length &&
               tokenizer->input[line_end] != '\n') {
            line_end++;
        }

        // Extract the line (without newline)
        size_t line_len = line_end - line_start;
        char *line = malloc(line_len + 1);
        if (!line) {
            free(content);
            return NULL;
        }
        strncpy(line, &tokenizer->input[line_start], line_len);
        line[line_len] = '\0';

        // Strip leading tabs if requested (<<- variant)
        const char *line_content = line;
        if (strip_tabs) {
            while (*line_content == '\t') {
                line_content++;
            }
        }

        // Check if this line matches the delimiter
        if (strcmp(line_content, match_delimiter) == 0) {
            // Found delimiter - stop collecting
            free(line);
            break;
        }

        // Add line to content (with newline)
        size_t needed =
            content_size + line_len + 2; // +1 for newline, +1 for null
        if (needed > content_capacity) {
            content_capacity = needed * 2;
            char *new_content = realloc(content, content_capacity);
            if (!new_content) {
                free(line);
                free(content);
                return NULL;
            }
            content = new_content;
        }

        // Append the line (stripped if <<- variant) plus newline
        if (strip_tabs) {
            strcat(content, line_content);
        } else {
            strcat(content, line);
        }
        strcat(content, "\n");
        content_size = strlen(content);

        free(line);

        // Move to next line
        line_start = line_end + 1;
    }

    // Update tokenizer position to after the delimiter line
    tokenizer->position = line_start;

    // Update line and column tracking
    for (size_t i = content_start; i < tokenizer->position; i++) {
        if (tokenizer->input[i] == '\n') {
            tokenizer->line++;
            tokenizer->column = 1;
        } else {
            tokenizer->column++;
        }
    }

    // Refresh tokenizer cache from the updated position
    tokenizer_refresh_from_position(tokenizer);

    // Clean up temporary delimiter
    if (unquoted_delimiter) {
        free(unquoted_delimiter);
    }

    return content;
}

// Parse if statement
static node_t *parse_if_statement(parser_t *parser) {
    if (!expect_token(parser, TOK_IF)) {
        return NULL;
    }

    node_t *if_node = new_node(NODE_IF);
    if (!if_node) {
        return NULL;
    }

    // Parse condition - parse until we hit 'then' or ';'
    node_t *condition = parse_logical_expression(parser);
    if (!condition) {
        free_node_tree(if_node);
        return NULL;
    }
    add_child_node(if_node, condition);

    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);

    // Now we should see 'then'
    if (!expect_token(parser, TOK_THEN)) {
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

    // Handle optional semicolon before elif/else/fi
    if (tokenizer_match(parser->tokenizer, TOK_SEMICOLON)) {
        tokenizer_advance(parser->tokenizer);
    }

    // Parse optional elif clauses
    // Skip separators before checking for elif
    skip_separators(parser);

    // Handle multiple elif clauses
    while (tokenizer_match(parser->tokenizer, TOK_ELIF)) {
        tokenizer_advance(parser->tokenizer);

        // Parse elif condition
        node_t *elif_condition = parse_logical_expression(parser);
        if (!elif_condition) {
            free_node_tree(if_node);
            return NULL;
        }
        add_child_node(if_node, elif_condition);

        // Skip separators before 'then'
        skip_separators(parser);

        // Expect 'then' after elif condition
        if (!expect_token(parser, TOK_THEN)) {
            free_node_tree(if_node);
            return NULL;
        }

        // Skip separators after 'then'
        skip_separators(parser);

        // Parse elif body
        node_t *elif_body = parse_if_body(parser);
        if (!elif_body) {
            free_node_tree(if_node);
            return NULL;
        }
        add_child_node(if_node, elif_body);

        // Handle optional semicolon after elif body
        if (tokenizer_match(parser->tokenizer, TOK_SEMICOLON)) {
            tokenizer_advance(parser->tokenizer);
        }

        // Skip separators before next elif/else/fi
        skip_separators(parser);
    }

    // Handle optional else clause
    if (tokenizer_match(parser->tokenizer, TOK_ELSE)) {
        tokenizer_advance(parser->tokenizer);

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

    if (!expect_token(parser, TOK_FI)) {
        free_node_tree(if_node);
        return NULL;
    }

    return if_node;
}

// Parse while statement
static node_t *parse_while_statement(parser_t *parser) {
    if (!expect_token(parser, TOK_WHILE)) {
        return NULL;
    }

    node_t *while_node = new_node(NODE_WHILE);
    if (!while_node) {
        return NULL;
    }

    // Parse condition - this is the key fix!
    // We parse until we hit 'do', not until we hit ';'
    node_t *condition = NULL;

    // Parse condition as a simple command or pipeline
    if (tokenizer_match(parser->tokenizer, TOK_LBRACKET)) {
        // Special handling for [ ... ] test commands
        condition = parse_simple_command(parser);
    } else {
        // Regular command condition
        condition = parse_pipeline(parser);
    }

    if (!condition) {
        free_node_tree(while_node);
        set_parser_error(parser, "Failed to parse while condition");
        return NULL;
    }
    add_child_node(while_node, condition);

    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);

    // Now we should see 'do'
    if (!expect_token(parser, TOK_DO)) {
        free_node_tree(while_node);
        return NULL;
    }

    // Skip separators after 'do' before parsing body
    skip_separators(parser);

    // Parse body
    node_t *body = parse_command_body(parser, TOK_DONE);
    if (!body) {
        free_node_tree(while_node);
        return NULL;
    }
    add_child_node(while_node, body);

    // Skip separators before 'done'
    skip_separators(parser);

    if (!expect_token(parser, TOK_DONE)) {
        free_node_tree(while_node);
        return NULL;
    }

    return while_node;
}

// Parse until statement
static node_t *parse_until_statement(parser_t *parser) {
    if (!expect_token(parser, TOK_UNTIL)) {
        return NULL;
    }

    node_t *until_node = new_node(NODE_UNTIL);
    if (!until_node) {
        return NULL;
    }

    // Parse condition - same logic as while
    node_t *condition = NULL;

    // Parse condition as a simple command or pipeline
    if (tokenizer_match(parser->tokenizer, TOK_LBRACKET)) {
        // Special handling for [ ... ] test commands
        condition = parse_simple_command(parser);
    } else {
        // Regular command condition
        condition = parse_pipeline(parser);
    }

    if (!condition) {
        free_node_tree(until_node);
        set_parser_error(parser, "Failed to parse until condition");
        return NULL;
    }
    add_child_node(until_node, condition);

    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);

    // Now we should see 'do'
    if (!expect_token(parser, TOK_DO)) {
        free_node_tree(until_node);
        return NULL;
    }

    // Skip separators after 'do' before parsing body
    skip_separators(parser);

    // Parse body
    node_t *body = parse_command_body(parser, TOK_DONE);
    if (!body) {
        free_node_tree(until_node);
        return NULL;
    }
    add_child_node(until_node, body);

    // Skip separators before 'done'
    skip_separators(parser);

    if (!expect_token(parser, TOK_DONE)) {
        free_node_tree(until_node);
        return NULL;
    }

    return until_node;
}

// Parse for statement
static node_t *parse_for_statement(parser_t *parser) {
    if (!expect_token(parser, TOK_FOR)) {
        return NULL;
    }

    node_t *for_node = new_node(NODE_FOR);
    if (!for_node) {
        return NULL;
    }

    // Parse variable name
    if (!tokenizer_match(parser->tokenizer, TOK_WORD)) {
        free_node_tree(for_node);
        set_parser_error(parser, "Expected variable name after 'for'");
        return NULL;
    }

    token_t *var_token = tokenizer_current(parser->tokenizer);
    for_node->val.str = strdup(var_token->text);
    tokenizer_advance(parser->tokenizer);

    if (!expect_token(parser, TOK_IN)) {
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
    while (!tokenizer_match(parser->tokenizer, TOK_SEMICOLON) &&
           !tokenizer_match(parser->tokenizer, TOK_NEWLINE) &&
           !tokenizer_match(parser->tokenizer, TOK_DO) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF)) {

        token_t *word_token = tokenizer_current(parser->tokenizer);
        if (token_is_word_like(word_token->type) ||
            word_token->type == TOK_VARIABLE ||
            word_token->type == TOK_COMMAND_SUB ||
            word_token->type == TOK_ARITH_EXP ||
            word_token->type == TOK_BACKQUOTE) {

            node_t *word_node = NULL;

            // Create appropriate node type based on token type
            if (word_token->type == TOK_COMMAND_SUB) {
                word_node = new_node(NODE_COMMAND_SUB);
            } else if (word_token->type == TOK_ARITH_EXP) {
                word_node = new_node(NODE_ARITH_EXP);
            } else if (word_token->type == TOK_EXPANDABLE_STRING) {
                word_node = new_node(NODE_STRING_EXPANDABLE);
            } else if (word_token->type == TOK_STRING) {
                word_node = new_node(NODE_STRING_LITERAL);
            } else {
                word_node = new_node(NODE_VAR);
            }

            if (!word_node) {
                free_node_tree(for_node);
                free_node_tree(word_list);
                return NULL;
            }
            word_node->val.str = strdup(word_token->text);
            add_child_node(word_list, word_node);
            tokenizer_advance(parser->tokenizer);
        } else {
            break;
        }
    }

    add_child_node(for_node, word_list);

    // Skip any separators (semicolons, newlines, whitespace)
    skip_separators(parser);

    // Now we should see 'do'
    if (!expect_token(parser, TOK_DO)) {
        free_node_tree(for_node);
        return NULL;
    }

    // Skip separators after 'do' before parsing body
    skip_separators(parser);

    // Parse body
    node_t *body = parse_command_body(parser, TOK_DONE);
    if (!body) {
        free_node_tree(for_node);
        return NULL;
    }
    add_child_node(for_node, body);

    // Skip separators before 'done'
    skip_separators(parser);

    if (!expect_token(parser, TOK_DONE)) {
        free_node_tree(for_node);
        return NULL;
    }

    return for_node;
}

// Parse case statement: case WORD in pattern) commands ;; ... esac
static node_t *parse_case_statement(parser_t *parser) {
    if (!expect_token(parser, TOK_CASE)) {
        return NULL;
    }

    node_t *case_node = new_node(NODE_CASE);
    if (!case_node) {
        return NULL;
    }

    // Parse the word to test
    token_t *word_token = tokenizer_current(parser->tokenizer);
    if (!token_is_word_like(word_token->type) &&
        word_token->type != TOK_VARIABLE) {
        free_node_tree(case_node);
        set_parser_error(parser, "Expected word after 'case'");
        return NULL;
    }

    // Store the test word
    case_node->val.str = strdup(word_token->text);
    if (!case_node->val.str) {
        free_node_tree(case_node);
        return NULL;
    }
    tokenizer_advance(parser->tokenizer);

    // Skip separators
    skip_separators(parser);

    // Expect 'in' keyword
    if (!expect_token(parser, TOK_IN)) {
        free_node_tree(case_node);
        return NULL;
    }

    // Skip separators after 'in'
    skip_separators(parser);

    // Parse case items until 'esac'
    while (!tokenizer_match(parser->tokenizer, TOK_ESAC) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF)) {

        // Parse pattern(s)
        node_t *case_item =
            new_node(NODE_COMMAND); // Reuse NODE_COMMAND for case items
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
            while (!tokenizer_match(parser->tokenizer, TOK_RPAREN) &&
                   !tokenizer_match(parser->tokenizer, TOK_PIPE) &&
                   !tokenizer_match(parser->tokenizer, TOK_EOF) &&
                   !tokenizer_match(parser->tokenizer, TOK_ESAC)) {

                token_t *pattern_token = tokenizer_current(parser->tokenizer);

                // Accept word-like tokens, wildcards, brackets, variables, and
                // equals for patterns (equals needed for patterns like HOME=*)
                if (token_is_word_like(pattern_token->type) ||
                    pattern_token->type == TOK_MULTIPLY ||
                    pattern_token->type == TOK_QUESTION ||
                    pattern_token->type == TOK_GLOB ||
                    pattern_token->type == TOK_LBRACKET ||
                    pattern_token->type == TOK_RBRACKET ||
                    pattern_token->type == TOK_VARIABLE ||
                    pattern_token->type == TOK_ASSIGN) {

                    size_t token_len = strlen(pattern_token->text);
                    char *new_single_pattern = realloc(
                        single_pattern, single_pattern_len + token_len + 1);
                    if (!new_single_pattern) {
                        free(single_pattern);
                        free_node_tree(case_item);
                        free_node_tree(case_node);
                        return NULL;
                    }
                    single_pattern = new_single_pattern;
                    strcpy(single_pattern + single_pattern_len,
                           pattern_token->text);
                    single_pattern_len += token_len;

                    tokenizer_advance(parser->tokenizer);
                } else {
                    // Unexpected token in pattern
                    break;
                }
            }

            // If we didn't collect any pattern tokens, that's an error
            if (!single_pattern) {
                free_node_tree(case_item);
                free_node_tree(case_node);
                set_parser_error(parser, "Expected pattern in case statement");
                return NULL;
            }

            // Append this single pattern to the overall pattern string
            if (pattern) {
                // Add | separator and new pattern
                char *new_pattern =
                    realloc(pattern, pattern_len + 1 + single_pattern_len + 1);
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
        } while (tokenizer_match(parser->tokenizer, TOK_PIPE) &&
                 (tokenizer_advance(parser->tokenizer), true));

        // Store pattern in case item
        case_item->val.str = pattern;

        // Expect )
        if (!tokenizer_match(parser->tokenizer, TOK_RPAREN)) {
            free_node_tree(case_item);
            free_node_tree(case_node);
            set_parser_error(parser, "Expected ')' after case pattern");
            return NULL;
        }
        tokenizer_advance(parser->tokenizer);

        // Skip separators
        skip_separators(parser);

        // Parse commands until ;; or esac
        node_t *commands = NULL;
        while (!tokenizer_match(parser->tokenizer, TOK_ESAC) &&
               !tokenizer_match(parser->tokenizer, TOK_EOF)) {

            // Check for terminators before processing command
            if (tokenizer_match(parser->tokenizer, TOK_ESAC) ||
                tokenizer_match(parser->tokenizer, TOK_EOF)) {
                break;
            }

            // Skip only newlines and whitespace, NOT semicolons (preserve ;; for detection)
            while (tokenizer_match(parser->tokenizer, TOK_NEWLINE) ||
                   tokenizer_match(parser->tokenizer, TOK_WHITESPACE)) {
                tokenizer_advance(parser->tokenizer);
            }

            // Check for ;; pattern AFTER skipping separators
            if (tokenizer_match(parser->tokenizer, TOK_SEMICOLON)) {
                token_t *next = tokenizer_peek(parser->tokenizer);
                if (next && next->type == TOK_SEMICOLON) {
                    break; // Found ;; - end this case item
                }
                // Single semicolon - consume it and continue parsing commands
                tokenizer_advance(parser->tokenizer);
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
                while (last->next_sibling) {
                    last = last->next_sibling;
                }
                last->next_sibling = command;
            }

            // Don't skip separators here - we need to detect ;; explicitly
        }

        // Add commands as child of case item
        if (commands) {
            add_child_node(case_item, commands);
        }

        // Expect ;;
        if (tokenizer_match(parser->tokenizer, TOK_SEMICOLON)) {
            tokenizer_advance(parser->tokenizer); // Consume first ;
            if (tokenizer_match(parser->tokenizer, TOK_SEMICOLON)) {
                tokenizer_advance(parser->tokenizer); // Consume second ;
            }
        }

        // Only skip non-semicolon separators (newlines, whitespace)
        while (tokenizer_match(parser->tokenizer, TOK_NEWLINE) ||
               tokenizer_match(parser->tokenizer, TOK_WHITESPACE)) {
            tokenizer_advance(parser->tokenizer);
        }

        // Add case item to case statement
        add_child_node(case_node, case_item);
    }

    // Expect 'esac'
    if (!expect_token(parser, TOK_ESAC)) {
        free_node_tree(case_node);
        return NULL;
    }
    return case_node;
}

// Helper function to check if current position is a function definition
static bool is_function_definition(parser_t *parser) {
    if (!parser || !parser->tokenizer) {
        return false;
    }

    token_t *current = tokenizer_current(parser->tokenizer);
    if (!current || !token_is_word_like(current->type)) {
        return false;
    }

    token_t *next = tokenizer_peek(parser->tokenizer);
    if (!next || next->type != TOK_LPAREN) {
        return false;
    }

    // We have word() - this looks like a function definition
    return true;
}

// Validate function name for POSIX compliance
static bool is_valid_posix_function_name(const char *name) {
    if (!name || !*name) {
        return false;
    }
    
    // First character must be letter or underscore
    if (!isalpha(*name) && *name != '_') {
        return false;
    }
    
    // Remaining characters must be alphanumeric or underscore
    for (const char *p = name + 1; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return false;
        }
    }
    
    return true;
}

// Parse function definition: name() { commands; } or function name() {
// commands; }
static node_t *parse_function_definition(parser_t *parser) {
    token_t *current = tokenizer_current(parser->tokenizer);

    // Handle "function" keyword form
    if (current && current->type == TOK_FUNCTION) {
        tokenizer_advance(parser->tokenizer);
        current = tokenizer_current(parser->tokenizer);
    }

    if (!current || !token_is_word_like(current->type)) {
        set_parser_error(parser, "Expected ')' after function parameters");
        return NULL;
    }

    // Create function node
    node_t *function_node = new_node(NODE_FUNCTION);
    if (!function_node) {
        return NULL;
    }

    // Store function name
    function_node->val.str = strdup(current->text);
    if (!function_node->val.str) {
        free_node_tree(function_node);
        return NULL;
    }
    
    // POSIX compliance: validate function name in posix mode
    if (is_posix_mode_enabled() && !is_valid_posix_function_name(current->text)) {
        set_parser_error(parser, "Invalid function name in POSIX mode: function names must contain only letters, digits, and underscores, and cannot start with a digit");
        free_node_tree(function_node);
        return NULL;
    }
    tokenizer_advance(parser->tokenizer);

    // Expect '('
    if (!expect_token(parser, TOK_LPAREN)) {
        free_node_tree(function_node);
        return NULL;
    }

    // Parse parameters between ( and )
    function_param_t *params = NULL;
    function_param_t *last_param = NULL;
    int param_count = 0;

    // Check if we have parameters (not immediate ')')
    current = tokenizer_current(parser->tokenizer);
    while (current && current->type != TOK_RPAREN && current->type != TOK_EOF) {
        // Expect parameter name (word token)
        if (!token_is_word_like(current->type)) {
            set_parser_error(parser, "Expected parameter name");
            free_function_params(params);
            free_node_tree(function_node);
            return NULL;
        }

        char *param_name = strdup(current->text);
        if (!param_name) {
            free_function_params(params);
            free_node_tree(function_node);
            return NULL;
        }

        tokenizer_advance(parser->tokenizer);
        current = tokenizer_current(parser->tokenizer);

        // Check for default value (= token)
        char *default_value = NULL;
        if (current && current->type == TOK_ASSIGN) {
            tokenizer_advance(parser->tokenizer); // Skip '='
            current = tokenizer_current(parser->tokenizer);

            if (!current || (!token_is_word_like(current->type) && current->type != TOK_STRING && current->type != TOK_EXPANDABLE_STRING)) {
                set_parser_error(parser, "Expected default value after '='");
                free(param_name);
                free_function_params(params);
                free_node_tree(function_node);
                return NULL;
            }

            default_value = strdup(current->text);
            if (!default_value) {
                free(param_name);
                free_function_params(params);
                free_node_tree(function_node);
                return NULL;
            }

            tokenizer_advance(parser->tokenizer);
            current = tokenizer_current(parser->tokenizer);
        }

        // Create parameter structure
        function_param_t *param = create_function_param(param_name, default_value);
        if (!param) {
            free(param_name);
            free(default_value);
            free_function_params(params);
            free_node_tree(function_node);
            return NULL;
        }

        // Add to parameter list
        if (!params) {
            params = param;
        } else {
            last_param->next = param;
        }
        last_param = param;
        param_count++;

        free(param_name);
        free(default_value);

        // Check for comma or end
        if (current && current->type == TOK_RPAREN) {
            // End of parameters
            break;
        } else if (current && current->text && strcmp(current->text, ",") == 0) {
            tokenizer_advance(parser->tokenizer); // Skip comma
            current = tokenizer_current(parser->tokenizer);
            // Continue to next parameter
        } else {
            set_parser_error(parser, "Expected ',' or ')' after parameter");
            free_function_params(params);
            free_node_tree(function_node);
            return NULL;
        }
    }

    // Expect ')'
    if (!expect_token(parser, TOK_RPAREN)) {
        free_function_params(params);
        free_node_tree(function_node);
        return NULL;
    }

    // Store parameters in the function node
    // We need a way to pass this to the executor
    // Create a special parameter info string to embed in the node
    if (params) {
        // Encode parameter info as JSON-like string for later parsing
        char *param_info = malloc(2048);
        if (param_info) {
            strcpy(param_info, "PARAMS{");
            function_param_t *p = params;
            bool first = true;
            while (p) {
                if (!first) strcat(param_info, ",");
                strcat(param_info, p->name);
                if (p->default_value) {
                    strcat(param_info, "=");
                    strcat(param_info, p->default_value);
                }
                p = p->next;
                first = false;
            }
            strcat(param_info, "}");
            
            // Store in function node's string value temporarily
            if (function_node->val.str) {
                char *old_name = function_node->val.str;
                function_node->val.str = malloc(strlen(old_name) + strlen(param_info) + 2);
                strcpy(function_node->val.str, old_name);
                strcat(function_node->val.str, "|");
                strcat(function_node->val.str, param_info);
                free(old_name);
            }
            free(param_info);
        }
        
        // Clean up params since we've encoded them
        free_function_params(params);
    }

    // Skip separators before '{'
    skip_separators(parser);

    // Expect '{'
    if (!expect_token(parser, TOK_LBRACE)) {
        free_node_tree(function_node);
        return NULL;
    }

    // Skip separators after '{'
    skip_separators(parser);

    // Parse function body until '}'
    node_t *body = NULL;
    while (!tokenizer_match(parser->tokenizer, TOK_RBRACE) &&
           !tokenizer_match(parser->tokenizer, TOK_EOF)) {

        node_t *command = parse_logical_expression(parser);
        if (!command) {
            break; // Can't parse more commands
        }

        if (!body) {
            body = command;
        } else {
            // Link commands as siblings
            node_t *last = body;
            while (last->next_sibling) {
                last = last->next_sibling;
            }
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
    if (!expect_token(parser, TOK_RBRACE)) {
        free_node_tree(function_node);
        return NULL;
    }

    return function_node;
}


