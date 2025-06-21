/**
 * Test the modern parser functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/parser_modern.h"

void test_parse(const char *input, const char *description) {
    printf("\n=== %s ===\n", description);
    printf("Input: %s\n", input);
    
    parser_modern_t *parser = parser_modern_new(input);
    if (!parser) {
        printf("ERROR: Failed to create parser\n");
        return;
    }
    
    node_t *ast = parser_modern_parse(parser);
    if (parser_modern_has_error(parser)) {
        printf("PARSE ERROR: %s\n", parser_modern_error(parser));
    } else if (ast) {
        printf("SUCCESS: Parsed successfully (AST created)\n");
        // In a real implementation, we'd print the AST structure
        printf("AST node type: %d\n", ast->type);
        if (ast->val.str) {
            printf("AST value: %s\n", ast->val.str);
        }
        free_node_tree(ast);
    } else {
        printf("RESULT: Empty or null AST\n");
    }
    
    parser_modern_free(parser);
}

int main() {
    printf("=== MODERN PARSER TEST ===\n");
    
    // Test simple commands
    test_parse("echo hello", "Simple command");
    test_parse("i=5", "Simple assignment");
    test_parse("i=$((i+1))", "Arithmetic assignment");
    
    // Test pipelines
    test_parse("echo hello | grep h", "Simple pipeline");
    
    // Test control structures
    test_parse("if [ $i -eq 5 ]; then echo yes; fi", "If statement");
    test_parse("while [ $i -lt 10 ]; do echo $i; done", "While loop");
    test_parse("for i in 1 2 3; do echo $i; done", "For loop");
    
    return 0;
}
