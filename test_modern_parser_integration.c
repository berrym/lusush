/**
 * Test to verify which tokenizer is being used in the modern parser
 */

#include <stdio.h>
#include "include/parser_modern.h"

int main() {
    const char *test_input = "i=$((i+1))";
    printf("Testing modern parser with: %s\n", test_input);
    
    // Create modern parser (this should use our enhanced tokenizer)
    parser_modern_t *parser = parser_modern_new(test_input);
    if (!parser) {
        printf("Failed to create modern parser\n");
        return 1;
    }
    
    // Check if tokenizer was created
    printf("Modern parser created successfully\n");
    if (parser->tokenizer) {
        printf("Tokenizer is available\n");
        
        // Get current token
        modern_token_t *token = modern_tokenizer_current(parser->tokenizer);
        if (token) {
            printf("First token: type=%d, text='%s'\n", token->type, token->text);
        }
    }
    
    parser_modern_free(parser);
    return 0;
}
