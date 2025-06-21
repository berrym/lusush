/**
 * Test to see how our modern parser represents variables in the AST
 */

#include <stdio.h>
#include "include/parser_modern.h"
#include "include/node.h"

void print_node_tree(node_t *node, int depth) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    printf("Node type: %d", node->type);
    if (node->val.str) {
        printf(", value: '%s'", node->val.str);
    }
    printf("\n");
    
    // Print children
    node_t *child = node->first_child;
    while (child) {
        print_node_tree(child, depth + 1);
        child = child->next_sibling;
    }
}

int main() {
    printf("=== TESTING MODERN PARSER AST FOR VARIABLES ===\n");
    
    // Test 1: Simple command with variable
    const char *test1 = "echo $USER";
    printf("\nTest 1: %s\n", test1);
    
    parser_modern_t *parser1 = parser_modern_new(test1);
    if (parser1) {
        node_t *ast1 = parser_modern_parse(parser1);
        if (ast1) {
            print_node_tree(ast1, 0);
            free_node_tree(ast1);
        } else {
            printf("Failed to parse\n");
        }
        parser_modern_free(parser1);
    }
    
    // Test 2: Command with quoted variable
    const char *test2 = "echo \"$i\"";
    printf("\nTest 2: %s\n", test2);
    
    parser_modern_t *parser2 = parser_modern_new(test2);
    if (parser2) {
        node_t *ast2 = parser_modern_parse(parser2);
        if (ast2) {
            print_node_tree(ast2, 0);
            free_node_tree(ast2);
        } else {
            printf("Failed to parse\n");
        }
        parser_modern_free(parser2);
    }
    
    return 0;
}
