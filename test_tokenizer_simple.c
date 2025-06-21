/**
 * Simple tokenizer test to verify the modern tokenizer works
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/tokenizer_new.h"

int main() {
    const char *test_input = "i=$((i+1))";
    printf("Testing modern tokenizer with: %s\n", test_input);
    
    // Create modern tokenizer
    modern_tokenizer_t *tokenizer = modern_tokenizer_new(test_input);
    if (!tokenizer) {
        printf("Failed to create tokenizer\n");
        return 1;
    }
    
    printf("Tokenizer created successfully\n");
    
    // Walk through tokens
    int token_count = 0;
    while (modern_tokenizer_current(tokenizer) && 
           modern_tokenizer_current(tokenizer)->type != MODERN_TOK_EOF) {
        modern_token_t *token = modern_tokenizer_current(tokenizer);
        printf("Token %d: %s '%s'\n", 
               ++token_count, 
               modern_token_type_name(token->type), 
               token->text);
        modern_tokenizer_advance(tokenizer);
    }
    
    printf("Total tokens: %d\n", token_count);
    modern_tokenizer_free(tokenizer);
    return 0;
}
