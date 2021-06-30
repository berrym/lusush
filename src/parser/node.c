#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "errors.h"
#include "lusush.h"
#include "node.h"
#include "scanner.h"
#include "strings.h"

struct node *new_node(enum node_type type)
{
    struct node *node = NULL;

    if ((node = calloc(1, sizeof(struct node))) == NULL)
        error_syscall("new_node: calloc");

    node->type = type;

    return node;
}

void add_child_node(struct node *parent, struct node *child)
{
    if (!parent || !child)
        return;

    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        struct node *sibling = parent->first_child;

        while (sibling->next_sibling)
            sibling = sibling->next_sibling;

        sibling->next_sibling = child;
        child->prev_sibling = sibling;
    }

    parent->children++;
}

void set_token_type(struct token *token)
{
    enum token_type t = TOKEN_EMPTY;

    if (token->text_len == 1) {
        switch (*token->text) {
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
            if (isdigit(*token->text)) {
                t = TOKEN_INTEGER;
            } else {
                t = TOKEN_WORD;
            }
            break;
        }
    }

    token->type = t;
}

void set_node_val_str(struct node *node, char *val)
{
    node->val_type = VAL_STR;

    if (!val) {
        node->val.str = NULL;
    } else {
        char *val2 = NULL;
        val2 = alloc_string(strnlen(val, MAXLINE) + 1, true);
        strncpy(val2, val, strnlen(val, MAXLINE));
        node->val.str = val2;
    }
}

void free_node_tree(struct node *node)
{
    if(!node)
        return;

    struct node *child = node->first_child;

    while (child) {
        struct node *next = child->next_sibling;
        free_node_tree(child);
        child = next;
    }

    if (node->val_type == VAL_STR)
        if (node->val.str)
            free(node->val.str);

    free(node);
}
