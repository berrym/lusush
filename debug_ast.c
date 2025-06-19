#include "include/parser_new_simple.h"
#include "include/node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_node(node_t *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Type: %d", node->type);
    if (node->val.str) {
        printf(", Value: '%s'", node->val.str);
    }
    printf(", Children: %zu\n", node->children);
    
    node_t *child = node->first_child;
    while (child) {
        print_node(child, depth + 1);
        child = child->next_sibling;
    }
}

int main() {
    char *input = "echo hello world";
    source_t src;
    src.buf = input;
    src.bufsize = strlen(input);
    src.pos = INIT_SRC_POS;
    
    parser_t *parser = parser_create(&src, NULL);
    node_t *cmd = parser_parse(parser);
    
    printf("AST structure for: %s\n", input);
    print_node(cmd, 0);
    
    free_node_tree(cmd);
    parser_destroy(parser);
    return 0;
}
