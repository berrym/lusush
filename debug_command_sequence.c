#include <stdio.h>
#include "parser_modern.h"
#include "tokenizer_new.h"

int main() {
    const char *problematic_input = "a=test; for i in 1; do echo \"hi\"; done";
    printf("Testing problematic input: %s\n\n", problematic_input);
    
    // Create tokenizer and examine tokens
    modern_tokenizer_t *tokenizer = modern_tokenizer_new(problematic_input);
    if (!tokenizer) {
        printf("Failed to create tokenizer\n");
        return 1;
    }
    
    printf("=== TOKEN SEQUENCE ===\n");
    int token_count = 0;
    while (!modern_tokenizer_match(tokenizer, MODERN_TOK_EOF)) {
        modern_token_t *token = modern_tokenizer_current(tokenizer);
        if (token) {
            printf("Token %d: Type=%d, Text='%s'\n", 
                   token_count++, token->type, token->text ? token->text : "(null)");
        }
        modern_tokenizer_advance(tokenizer);
    }
    
    printf("\n=== PARSING TEST ===\n");
    modern_tokenizer_free(tokenizer);
    
    // Now test parsing
    parser_modern_t *parser = parser_modern_new(problematic_input);
    if (!parser) {
        printf("Failed to create parser\n");
        return 1;
    }
    
    node_t *ast = parser_modern_parse(parser);
    if (parser_modern_has_error(parser)) {
        printf("Parser error: %s\n", parser_modern_error(parser));
    } else if (ast) {
        printf("Parsing succeeded - AST created\n");
        // Print AST structure
        print_ast(ast, 0);
        free_node_tree(ast);
    } else {
        printf("Parsing returned NULL AST (no error reported)\n");
    }
    
    parser_modern_free(parser);
    return 0;
}
