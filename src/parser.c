#include "../include/alias.h"
#include "../include/alias_expand.h"
#include "../include/errors.h"
#include "../include/node.h"
#include "../include/scanner.h"
#include "../include/strings.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations
static node_t *parse_if_statement(source_t *src);
static bool parse_condition_then_pair(source_t *src, node_t *if_node, const char *clause_type);
static node_t *parse_for_statement(source_t *src);
static node_t *parse_while_statement(source_t *src);
static node_t *parse_until_statement(source_t *src);
static node_t *parse_case_statement(source_t *src);
static node_t *parse_command_list(source_t *src, token_type_t terminator);

// Error recovery context for parser
static parser_error_context_t error_ctx = {0};

/**
 * parse_redirection:
 *      Parse a redirection operator and its target
 */
node_t *parse_redirection(token_t *redir_tok, token_t *target_tok) {
    if (redir_tok == NULL || target_tok == NULL) {
        return NULL;
    }

    node_t *redir = NULL;
    
    switch (redir_tok->type) {
        case TOKEN_LESS:
            redir = new_node(NODE_REDIR_IN);
            break;
        case TOKEN_GREAT:
            redir = new_node(NODE_REDIR_OUT);
            break;
        case TOKEN_DGREAT:
            redir = new_node(NODE_REDIR_APPEND);
            break;
        case TOKEN_DLESS:
            redir = new_node(NODE_REDIR_HEREDOC);
            break;
        case TOKEN_CLOBBER:
            redir = new_node(NODE_REDIR_CLOBBER);
            break;
        default:
            return NULL;
    }

    if (redir == NULL) {
        return NULL;
    }

    // Add the target filename/fd as a child
    node_t *target = new_node(NODE_VAR);
    if (target == NULL) {
        free_node_tree(redir);
        return NULL;
    }
    
    set_node_val_str(target, target_tok->text);
    add_child_node(redir, target);
    
    return redir;
}

/**
 * parse_basic_command:
 *      Parse a basic command with simple pipeline support
 */
node_t *parse_basic_command(token_t *tok) {
    if (tok == NULL) {
        return NULL;
    }

    node_t *cmd = new_node(NODE_COMMAND);
    if (cmd == NULL) {
        free_token(tok);
        return NULL;
    }

    source_t *src = tok->src;
    bool first_word = true;

    do {
        if (*tok->text == '\n') {
            free_token(tok);
            break;
        }

        // Stop at control structure keywords that shouldn't be part of basic commands
        if (tok->type == TOKEN_KEYWORD_THEN || tok->type == TOKEN_KEYWORD_ELSE ||
            tok->type == TOKEN_KEYWORD_FI || tok->type == TOKEN_KEYWORD_DO ||
            tok->type == TOKEN_KEYWORD_DONE || tok->type == TOKEN_KEYWORD_ELIF) {
            // Don't consume these tokens - they belong to control structures
            break;
        }

        // Handle semicolon as command terminator
        if (tok->type == TOKEN_SEMI) {
            free_token(tok);
            break;
        }

        // Handle pipe operator - just add pipe marker  
        if (tok->type == TOKEN_PIPE) {
            node_t *pipe_node = new_node(NODE_PIPE);
            add_child_node(cmd, pipe_node);
            free_token(tok);
            if ((tok = tokenize(src)) == &eof_token) {
                break;
            }
            first_word = true; // Reset for next command in pipeline
            continue;
        }

        // Handle redirection operators
        if (tok->type == TOKEN_LESS || tok->type == TOKEN_GREAT || 
            tok->type == TOKEN_DGREAT || tok->type == TOKEN_DLESS ||
            tok->type == TOKEN_CLOBBER) {
            
            token_t *redir_tok = tok;
            free_token(tok);
            
            // Get the target filename/fd
            if ((tok = tokenize(src)) == &eof_token) {
                free_token(redir_tok);
                break;
            }
            
            node_t *redir = parse_redirection(redir_tok, tok);
            if (redir != NULL) {
                add_child_node(cmd, redir);
            }
            
            free_token(redir_tok);
            free_token(tok);
            if ((tok = tokenize(src)) == &eof_token) {
                break;
            }
            continue;
        }

        // Handle regular words
        node_t *word = new_node(NODE_VAR);
        if (word == NULL) {
            free_node_tree(cmd);
            free_token(tok);
            return NULL;
        }

        // Expand only the first word as an alias (POSIX behavior)
        char *expanded = NULL;
        if (first_word) {
            expanded = expand_first_word_alias(tok->text);
            first_word = false;
            
            // If the alias expanded, tokenize and add all parts
            if (expanded && strcmp(expanded, tok->text) != 0) {
                char *token_start = expanded;
                bool first_token = true;
                
                while (*token_start) {
                    // Skip leading whitespace
                    while (*token_start && isspace(*token_start)) {
                        token_start++;
                    }
                    
                    if (!*token_start) break;
                    
                    // Find end of token
                    char *token_end = token_start;
                    while (*token_end && !isspace(*token_end)) {
                        token_end++;
                    }
                    
                    // Create a word node for this token
                    size_t token_len = token_end - token_start;
                    char *token_str = malloc(token_len + 1);
                    if (token_str) {
                        strncpy(token_str, token_start, token_len);
                        token_str[token_len] = '\0';
                        
                        if (first_token) {
                            set_node_val_str(word, token_str);
                            add_child_node(cmd, word);
                            first_token = false;
                        } else {
                            node_t *alias_word = new_node(NODE_VAR);
                            if (alias_word) {
                                set_node_val_str(alias_word, token_str);
                                add_child_node(cmd, alias_word);
                            }
                        }
                        free(token_str);
                    }
                    
                    token_start = token_end;
                }
                
                free(expanded);
                free_token(tok);
                if ((tok = tokenize(src)) == &eof_token) {
                    break;
                }
                continue;
            }
        } else {
            expanded = strdup(tok->text);
        }
        
        set_node_val_str(word, expanded);
        free(expanded);
        add_child_node(cmd, word);
        free_token(tok);
        
    } while ((tok = tokenize(src)) != &eof_token);

    return cmd;
}

/**
 * parse_command:
 *      Parse a token into a new command node.
 */
node_t *parse_command(token_t *tok) {
    // skip newline and ; operators
    while (tok->type == TOKEN_NEWLINE || tok->type == TOKEN_SEMI) {
        /* save the start of this line */
        tok->src->wstart = tok->src->pos;
        tok = tokenize(tok->src);
    }

    if (tok == NULL || tok->type == TOKEN_EOF) {
        return NULL;
    }

    node_t *cmd = NULL;
    
    // Initialize error context if not already done
    if (error_ctx.max_errors == 0) {
        parser_reset_errors(&error_ctx);
    }
    
    // Check for control structures
    switch (tok->type) {
        case TOKEN_KEYWORD_IF:
            free_token(tok);
            cmd = parse_if_statement(tok->src);
            break;
        case TOKEN_KEYWORD_FOR:
            free_token(tok);
            cmd = parse_for_statement(tok->src);
            break;
        case TOKEN_KEYWORD_WHILE:
            free_token(tok);
            cmd = parse_while_statement(tok->src);
            break;
        case TOKEN_KEYWORD_UNTIL:
            free_token(tok);
            cmd = parse_until_statement(tok->src);
            break;
        case TOKEN_KEYWORD_CASE:
            free_token(tok);
            cmd = parse_case_statement(tok->src);
            break;
        default:
            cmd = parse_basic_command(tok);
            break;
    }

    return cmd;
}

/**
 * parse_if_statement:
 *      Parse a complete if statement: if condition; then body; [elif condition; then body;]... [else body;] fi
 *      
 *      AST Structure:
 *      NODE_IF
 *      ├── condition1 (NODE_COMMAND)
 *      ├── then_body1 (NODE_COMMAND)
 *      ├── condition2 (NODE_COMMAND) [for elif]
 *      ├── then_body2 (NODE_COMMAND) [for elif]
 *      ├── ...
 *      └── else_body (NODE_COMMAND) [optional, always last]
 */
static node_t *parse_if_statement(source_t *src) {
    node_t *if_node = new_node(NODE_IF);
    if (!if_node) {
        return NULL;
    }
    
    // Parse initial if condition and then body
    if (!parse_condition_then_pair(src, if_node, "if")) {
        free_node_tree(if_node);
        return NULL;
    }
    
    // Parse elif clauses
    token_t *tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    // Handle elif clauses
    while (tok && tok->type == TOKEN_KEYWORD_ELIF) {
        free_token(tok);
        
        if (!parse_condition_then_pair(src, if_node, "elif")) {
            free_node_tree(if_node);
            return NULL;
        }
        
        // Get next token to check for more elif/else/fi
        tok = tokenize(src);
        while (tok && tok != &eof_token && 
               (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
            free_token(tok);
            tok = tokenize(src);
        }
    }
    
    // Handle optional else clause
    if (tok && tok->type == TOKEN_KEYWORD_ELSE) {
        free_token(tok);
        
        tok = tokenize(src);
        if (!tok || tok == &eof_token) {
            error_message("parse error: expected command after 'else'");
            free_node_tree(if_node);
            return NULL;
        }
        
        node_t *else_body = parse_basic_command(tok);
        if (!else_body) {
            error_message("parse error: failed to parse else body");
            free_node_tree(if_node);
            return NULL;
        }
        
        add_child_node(if_node, else_body);
        
        // Skip to 'fi'
        tok = tokenize(src);
        while (tok && tok != &eof_token && 
               (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
            free_token(tok);
            tok = tokenize(src);
        }
    }
    
    // Expect 'fi' to close the if statement
    if (!tok || tok->type != TOKEN_KEYWORD_FI) {
        error_message("parse error: expected 'fi' to close 'if' statement");
        free_node_tree(if_node);
        if (tok) free_token(tok);
        return NULL;
    }
    
    free_token(tok);
    return if_node;
}

/**
 * parse_condition_then_pair:
 *      Helper function to parse a condition; then body; pair for if/elif
 */
static bool parse_condition_then_pair(source_t *src, node_t *if_node, const char *clause_type) {
    // Parse condition
    token_t *tok = tokenize(src);
    if (!tok || tok == &eof_token) {
        error_message("parse error: expected condition after '%s'", clause_type);
        return false;
    }
    
    node_t *condition = parse_basic_command(tok);
    if (!condition) {
        error_message("parse error: failed to parse %s condition", clause_type);
        return false;
    }
    
    add_child_node(if_node, condition);
    
    // Expect ';' or newline, then 'then'
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    if (!tok || tok->type != TOKEN_KEYWORD_THEN) {
        error_message("parse error: expected 'then' after '%s' condition", clause_type);
        if (tok) free_token(tok);
        return false;
    }
    
    free_token(tok);
    
    // Parse then body
    tok = tokenize(src);
    if (!tok || tok == &eof_token) {
        error_message("parse error: expected command after 'then'");
        return false;
    }
    
    node_t *then_body = parse_basic_command(tok);
    if (!then_body) {
        error_message("parse error: failed to parse then body");
        return false;
    }
    
    add_child_node(if_node, then_body);
    return true;
}

/**
 * parse_for_statement:
 *      Parse a for loop: for var in list; do body; done
 *      
 *      AST Structure:
 *      NODE_FOR
 *      ├── variable (NODE_VAR)
 *      ├── list_item1 (NODE_VAR)
 *      ├── list_item2 (NODE_VAR)
 *      ├── ...
 *      └── body (NODE_COMMAND)
 */
static node_t *parse_for_statement(source_t *src) {
    node_t *for_node = new_node(NODE_FOR);
    if (!for_node) {
        return NULL;
    }
    
    // Parse variable name
    token_t *tok = tokenize(src);
    if (!tok || tok == &eof_token || tok->type != TOKEN_WORD) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected variable name after 'for'");
        if (tok) free_token(tok);
        free_node_tree(for_node);
        return NULL;
    }
    
    node_t *var_node = new_node(NODE_VAR);
    if (!var_node) {
        free_token(tok);
        free_node_tree(for_node);
        return NULL;
    }
    set_node_val_str(var_node, tok->text);
    add_child_node(for_node, var_node);
    free_token(tok);
    
    // Expect 'in'
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    if (!tok || tok->type != TOKEN_KEYWORD_IN) {
        parser_error_with_suggestion(&error_ctx, src, EXPECTED_TOKEN,
                                    "try: for var in item1 item2...; do...; done",
                                    "expected 'in' after for variable");
        if (tok) free_token(tok);
        free_node_tree(for_node);
        return NULL;
    }
    free_token(tok);
    
    // Parse list items
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           tok->type != TOKEN_KEYWORD_DO && 
           tok->type != TOKEN_SEMI && tok->type != TOKEN_NEWLINE) {
        
        if (tok->type == TOKEN_WORD) {
            node_t *item_node = new_node(NODE_VAR);
            if (item_node) {
                set_node_val_str(item_node, tok->text);
                add_child_node(for_node, item_node);
            }
        }
        free_token(tok);
        tok = tokenize(src);
    }
    
    // Skip to 'do'
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    if (!tok || tok->type != TOKEN_KEYWORD_DO) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected 'do' after for list");
        if (tok) free_token(tok);
        free_node_tree(for_node);
        return NULL;
    }
    free_token(tok);
    
    // Parse body as command list until 'done'
    node_t *body = parse_command_list(src, TOKEN_KEYWORD_DONE);
    if (!body) {
        parser_error(&error_ctx, src, SYNTAX_ERROR, ERROR_RECOVERABLE,
                    "failed to parse for loop body");
        free_node_tree(for_node);
        return NULL;
    }
    
    add_child_node(for_node, body);
    
    // Expect 'done'
    tok = tokenize(src);
    if (!tok || tok->type != TOKEN_KEYWORD_DONE) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected 'done' to close for loop");
        if (tok) free_token(tok);
        free_node_tree(for_node);
        return NULL;
    }
    
    free_token(tok);
    return for_node;
}

/**
 * parse_while_statement:
 *      Parse a while loop: while condition; do body; done
 */
static node_t *parse_while_statement(source_t *src) {
    node_t *while_node = new_node(NODE_WHILE);
    if (!while_node) {
        return NULL;
    }
    
    // Parse condition
    token_t *tok = tokenize(src);
    if (!tok || tok == &eof_token) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected condition after 'while'");
        free_node_tree(while_node);
        return NULL;
    }
    
    node_t *condition = parse_basic_command(tok);
    if (!condition) {
        parser_error(&error_ctx, src, SYNTAX_ERROR, ERROR_RECOVERABLE,
                    "failed to parse while condition");
        free_node_tree(while_node);
        return NULL;
    }
    
    add_child_node(while_node, condition);
    
    // Expect ';' or newline, then 'do'
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    if (!tok || tok->type != TOKEN_KEYWORD_DO) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected 'do' after while condition");
        if (tok) free_token(tok);
        free_node_tree(while_node);
        return NULL;
    }
    free_token(tok);
    
    // Parse body as command list until 'done'
    node_t *body = parse_command_list(src, TOKEN_KEYWORD_DONE);
    if (!body) {
        parser_error(&error_ctx, src, SYNTAX_ERROR, ERROR_RECOVERABLE,
                    "failed to parse while loop body");
        free_node_tree(while_node);
        return NULL;
    }
    
    add_child_node(while_node, body);
    
    // 'done' is already consumed by parse_command_list
    return while_node;
}

/**
 * parse_until_statement:
 *      Parse an until loop: until condition; do body; done
 */
static node_t *parse_until_statement(source_t *src) {
    node_t *until_node = new_node(NODE_UNTIL);
    if (!until_node) {
        return NULL;
    }
    
    // Parse condition (same structure as while, but semantics differ)
    token_t *tok = tokenize(src);
    if (!tok || tok == &eof_token) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected condition after 'until'");
        free_node_tree(until_node);
        return NULL;
    }
    
    node_t *condition = parse_basic_command(tok);
    if (!condition) {
        parser_error(&error_ctx, src, SYNTAX_ERROR, ERROR_RECOVERABLE,
                    "failed to parse until condition");
        free_node_tree(until_node);
        return NULL;
    }
    
    add_child_node(until_node, condition);
    
    // Expect ';' or newline, then 'do'
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    if (!tok || tok->type != TOKEN_KEYWORD_DO) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected 'do' after until condition");
        if (tok) free_token(tok);
        free_node_tree(until_node);
        return NULL;
    }
    free_token(tok);
    
    // Parse body as command list until 'done'
    node_t *body = parse_command_list(src, TOKEN_KEYWORD_DONE);
    if (!body) {
        parser_error(&error_ctx, src, SYNTAX_ERROR, ERROR_RECOVERABLE,
                    "failed to parse until loop body");
        free_node_tree(until_node);
        return NULL;
    }
    
    add_child_node(until_node, body);
    
    // 'done' is already consumed by parse_command_list
    return until_node;
}

/**
 * parse_case_statement:
 *      Parse a case statement: case word in pattern) commands;; ... esac
 *      
 *      AST Structure:
 *      NODE_CASE
 *      ├── word (NODE_VAR)
 *      ├── pattern1 (NODE_VAR)
 *      ├── commands1 (NODE_COMMAND)
 *      ├── pattern2 (NODE_VAR)
 *      ├── commands2 (NODE_COMMAND)
 *      └── ...
 */
static node_t *parse_case_statement(source_t *src) {
    node_t *case_node = new_node(NODE_CASE);
    if (!case_node) {
        return NULL;
    }
    
    // Parse case word
    token_t *tok = tokenize(src);
    if (!tok || tok == &eof_token || tok->type != TOKEN_WORD) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected word after 'case'");
        if (tok) free_token(tok);
        free_node_tree(case_node);
        return NULL;
    }
    
    node_t *word_node = new_node(NODE_VAR);
    if (!word_node) {
        free_token(tok);
        free_node_tree(case_node);
        return NULL;
    }
    set_node_val_str(word_node, tok->text);
    add_child_node(case_node, word_node);
    free_token(tok);
    
    // Expect 'in'
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
    }
    
    if (!tok || tok->type != TOKEN_KEYWORD_IN) {
        parser_error_with_suggestion(&error_ctx, src, EXPECTED_TOKEN,
                                    "try: case word in pattern) commands;; ... esac",
                                    "expected 'in' after case word");
        if (tok) free_token(tok);
        free_node_tree(case_node);
        return NULL;
    }
    free_token(tok);
    
    // Parse case patterns and commands
    tok = tokenize(src);
    while (tok && tok != &eof_token && tok->type != TOKEN_KEYWORD_ESAC) {
        // Skip newlines and semicolons
        while (tok && tok != &eof_token && 
               (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
            free_token(tok);
            tok = tokenize(src);
        }
        
        if (!tok || tok == &eof_token || tok->type == TOKEN_KEYWORD_ESAC) {
            break;
        }
        
        // Parse pattern
        if (tok->type == TOKEN_WORD) {
            node_t *pattern_node = new_node(NODE_VAR);
            if (pattern_node) {
                set_node_val_str(pattern_node, tok->text);
                add_child_node(case_node, pattern_node);
            }
            free_token(tok);
            
            // Expect ')'
            tok = tokenize(src);
            if (!tok || tok->type != TOKEN_RIGHT_PAREN) {
                parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                            "expected ')' after case pattern");
                if (tok) free_token(tok);
                // Continue trying to parse
                continue;
            }
            free_token(tok);
            
            // Parse commands until ';;'
            tok = tokenize(src);
            if (tok && tok != &eof_token && tok->type != TOKEN_DSEMI) {
                node_t *commands = parse_basic_command(tok);
                if (commands) {
                    add_child_node(case_node, commands);
                }
                
                // Get next token after parsing commands
                tok = tokenize(src);
            }
            
            // Expect ';;'
            while (tok && tok != &eof_token && 
                   (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
                free_token(tok);
                tok = tokenize(src);
            }
            
            if (tok && tok->type == TOKEN_DSEMI) {
                free_token(tok);
                tok = tokenize(src);
            }
        } else {
            // Skip unexpected token
            free_token(tok);
            tok = tokenize(src);
        }
    }
    
    // Expect 'esac'
    if (!tok || tok->type != TOKEN_KEYWORD_ESAC) {
        parser_error(&error_ctx, src, EXPECTED_TOKEN, ERROR_RECOVERABLE,
                    "expected 'esac' to close case statement");
        if (tok) free_token(tok);
        free_node_tree(case_node);
        return NULL;
    }
    
    free_token(tok);
    return case_node;
}

/**
 * parse_command_list:
 *      Parse a sequence of commands until a terminator token is found
 *      Used for parsing loop bodies and other compound constructs
 */
static node_t *parse_command_list(source_t *src, token_type_t terminator) {
    node_t *list_node = new_node(NODE_COMMAND);
    if (!list_node) {
        return NULL;
    }
    
    while (true) {
        // Skip whitespace and separators
        token_t *tok = tokenize(src);
        while (tok && tok != &eof_token && 
               (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
            free_token(tok);
            tok = tokenize(src);
        }
        
        // Check for terminator
        if (!tok || tok == &eof_token || tok->type == terminator) {
            if (tok && tok->type == terminator) {
                free_token(tok);  // Consume the terminator
            }
            break;
        }
        
        // Parse a command
        node_t *cmd = parse_basic_command(tok);
        if (cmd) {
            add_child_node(list_node, cmd);
        }
    }
    
    return list_node;
}
