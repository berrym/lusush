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
    
    // Check for control structures
    switch (tok->type) {
        case TOKEN_KEYWORD_IF:
            free_token(tok);
            cmd = parse_if_statement(tok->src);
            break;
        default:
            cmd = parse_basic_command(tok);
            break;
    }

    return cmd;
}

/**
 * parse_if_statement:
 *      Parse an if statement: if condition; then body; [else body;] fi
 */
static node_t *parse_if_statement(source_t *src) {
    node_t *if_node = new_node(NODE_IF);
    if (!if_node) {
        return NULL;
    }
    
    // Parse condition - simplified to parse one command
    token_t *tok = tokenize(src);
    if (!tok || tok == &eof_token) {
        error_message("parse error: expected condition after 'if'");
        free_node_tree(if_node);
        return NULL;
    }
    
    node_t *condition = parse_basic_command(tok);
    if (!condition) {
        error_message("parse error: failed to parse if condition");
        free_node_tree(if_node);
        return NULL;
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
        error_message("parse error: expected 'then' after 'if' condition");
        free_node_tree(if_node);
        if (tok) free_token(tok);
        return NULL;
    }
    
    free_token(tok);
    
    // Parse then body - simplified to parse one command
    tok = tokenize(src);
    if (!tok || tok == &eof_token) {
        error_message("parse error: expected command after 'then'");
        free_node_tree(if_node);
        return NULL;
    }
    
    node_t *then_body = parse_basic_command(tok);
    if (!then_body) {
        error_message("parse error: failed to parse then body");
        free_node_tree(if_node);
        return NULL;
    }
    
    add_child_node(if_node, then_body);
    
    // Skip to 'fi' or 'else' - simplified
    tok = tokenize(src);
    while (tok && tok != &eof_token && 
           (tok->type == TOKEN_SEMI || tok->type == TOKEN_NEWLINE)) {
        free_token(tok);
        tok = tokenize(src);
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
