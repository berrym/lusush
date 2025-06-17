#include "../include/alias.h"
#include "../include/alias_expand.h"
#include "../include/node.h"
#include "../include/scanner.h"
#include "../include/strings.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

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
    cmd = parse_basic_command(tok);

    return cmd;
}
