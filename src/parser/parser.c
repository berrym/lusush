#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "lusush.h"
#include "node.h"
#include "parser.h"
#include "scanner.h"

struct node *parse_command(struct token *tok)
{
    if (!tok)
        return NULL;

    struct node *cmd = new_node(NODE_COMMAND);

    if (!cmd) {
        free_token(tok);
        return NULL;
    }

    struct source *src = tok->src;

    do {
        if (*tok->text == '\n') {
            free_token(tok);
            break;
        }

        struct node *word = new_node(NODE_VAR);

        if (!word) {
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
