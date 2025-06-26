#include "../include/node.h"
#include "../include/parser.h"
#include "../include/tokenizer.h"

#include <stdio.h>
#include <stdlib.h>

void print_ast_structure(node_t *node, int depth) {
    if (!node) {
        return;
    }

    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    // Print node type and value
    printf("Node type: %d", node->type);
    if (node->val.str) {
        printf(" value: '%s'", node->val.str);
    }
    printf("\n");

    // Print children
    node_t *child = node->first_child;
    while (child) {
        print_ast_structure(child, depth + 1);
        child = child->next_sibling;
    }
}

int main() {
    // Test command: if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then
    // echo two; else echo other; fi
    const char *test_input = "if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 "
                             "]; then echo two; else echo other; fi";

    printf("Parsing: %s\n", test_input);
    printf("AST Structure:\n");

    tokenizer_t *tokenizer = tokenizer_new(test_input);
    if (!tokenizer) {
        printf("Failed to create tokenizer\n");
        return 1;
    }

    parser_t *parser = parser_new(tokenizer);
    if (!parser) {
        printf("Failed to create parser\n");
        tokenizer_free(tokenizer);
        return 1;
    }

    node_t *ast = parse_statement(parser);
    if (!ast) {
        printf("Failed to parse statement\n");
        parser_free(parser);
        tokenizer_free(tokenizer);
        return 1;
    }

    print_ast_structure(ast, 0);

    // Cleanup
    free_node_tree(ast);
    parser_free(parser);
    tokenizer_free(tokenizer);

    return 0;
}
