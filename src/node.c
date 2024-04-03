#include "../include/node.h"

#include "../include/errors.h"
#include "../include/strings.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

node_t *new_node(node_type_t type) {
    node_t *node = NULL;

    if ((node = calloc(1, sizeof(node_t))) == NULL) {
        error_syscall("new_node");
    }

    node->type = type;

    return node;
}

void add_child_node(node_t *parent, node_t *child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    if (parent->first_child == NULL) {
        parent->first_child = child;
    } else {
        node_t *sibling = parent->first_child;

        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }

        sibling->next_sibling = child;
        child->prev_sibling = sibling;
    }

    parent->children++;
}

void set_node_val_str(node_t *node, char *val) {
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

void free_node_tree(node_t *node) {
    node_t *child = NULL, *next = NULL;

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
