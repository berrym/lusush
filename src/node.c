#include "../include/node.h"

#include "../include/errors.h"
#include "../include/scanner.h"
#include "../include/strings.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

node_s *new_node(node_type_e type) {
    node_s *node = NULL;

    if ((node = calloc(1, sizeof(node_s))) == NULL) {
        error_syscall("new_node");
    }

    node->type = type;

    return node;
}

void add_child_node(node_s *parent, node_s *child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    if (parent->first_child == NULL) {
        parent->first_child = child;
    } else {
        node_s *sibling = parent->first_child;

        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }

        sibling->next_sibling = child;
        child->prev_sibling = sibling;
    }

    parent->children++;
}

void set_token_type(token_s *tok) {
    token_type_e t = TOKEN_EMPTY;

    if (tok->text_len == 1) {
        switch (*tok->text) {
        case '(':
            t = TOKEN_LEFT_PAREN;
            break;
        case ')':
            t = TOKEN_RIGHT_PAREN;
            break;
        case '{':
            t = TOKEN_KEYWORD_LBRACE;
            break;
        case '}':
            t = TOKEN_KEYWORD_RBRACE;
            break;
        case '!':
            t = TOKEN_KEYWORD_BANG;
            break;
        case '|':
            t = TOKEN_PIPE;
            break;
        case '<':
            t = TOKEN_LESS;
            break;
        case '>':
            t = TOKEN_GREAT;
            break;
        case '&':
            t = TOKEN_AND;
            break;
        case '\n':
            t = TOKEN_NEWLINE;
            break;
        case ';':
            t = TOKEN_SEMI;
            break;
        default:
            if (isdigit(*tok->text)) {
                t = TOKEN_INTEGER;
            } else {
                t = TOKEN_WORD;
            }
            break;
        }
    }

    tok->type = t;
}

void set_node_val_str(node_s *node, char *val) {
    node->val_type = VAL_STR;

    if (!val) {
        node->val.str = NULL;
    } else {
        char *val2 = strdup(val);
        if (!val2) {
            error_return("set_node_val_str");
            return;
        }
        node->val.str = val2;
    }
}

void free_node_tree(node_s *node) {
    node_s *child = NULL, *next = NULL;

    if (!node) {
        return;
    }

    child = node->first_child;

    while (child) {
        next = child->next_sibling;
        free_node_tree(child);
        child = next;
    }

    if (node->val_type == VAL_STR) {
        if (node->val.str) {
            free_str(node->val.str);
        }
    }

    free(node);
    node = NULL;
}
