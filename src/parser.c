#include "../include/node.h"
#include "../include/scanner.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * parse_command:
 *      Parse a token into a new command node.
 */
node_t *parse_command(token_t *tok) {
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

        node_t *word = new_node(NODE_VAR);
        if (word == NULL) {
            free_node_tree(cmd);
            free_token(tok);
            return NULL;
        }

        set_token_type(tok);
        set_node_val_str(word, tok->text);
        add_child_node(cmd, word);
        free_token(tok);
    } while ((tok = tokenize(src)) != &eof_token);

    return cmd;
}
