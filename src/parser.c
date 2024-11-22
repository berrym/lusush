#include "../include/alias.h"
#include "../include/node.h"
#include "../include/scanner.h"
#include "../include/strings.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool exec_compound_command = false;

/**
 * parse_basic_command:
 *      Parse a basic command;
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

    do {
        if (*tok->text == '\n') {
            free_token(tok);
            break;
        }

        if (tok->type == TOKEN_PIPE) {
            exec_compound_command = true;
            node_t *op = new_node(NODE_PIPE);
            add_child_node(cmd, op);
            node_t *next = new_node(NODE_COMMAND);
            add_child_node(cmd, next);
            free_token(tok);
            if ((tok = tokenize(src)) == &eof_token) {
                break;
            }
        }

        node_t *word = new_node(NODE_VAR);
        if (word == NULL) {
            free_node_tree(cmd);
            free_token(tok);
            return NULL;
        }

        // Perform recursive alias expansion now
        char *alias = NULL;
        for (;;) {
            alias = lookup_alias(tok->text);
            if (alias) {
                free_str(tok->text);
                tok->text = strdup(alias);
            } else {
                break;
            }
        }
        set_node_val_str(word, tok->text);
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
    exec_compound_command = false;

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
