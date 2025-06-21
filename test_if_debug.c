/**
 * Debug test to see what tokens are being generated for 'if'
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/tokenizer_new.h"

int main() {
    const char *test_input = "if true; then echo yes; fi";
    printf("Testing tokenization of: %s\n\n", test_input);
    
    modern_tokenizer_t *tokenizer = modern_tokenizer_new(test_input);
    if (!tokenizer) {
        printf("Failed to create tokenizer\n");
        return 1;
    }
    
    int token_count = 0;
    while (modern_tokenizer_current(tokenizer) && 
           modern_tokenizer_current(tokenizer)->type != MODERN_TOK_EOF) {
        modern_token_t *token = modern_tokenizer_current(tokenizer);
        printf("Token %d: type=%d (%s) text='%s' is_keyword=%s\n", 
               ++token_count, 
               token->type,
               modern_token_type_name(token->type),
               token->text,
               modern_token_is_keyword(token->type) ? "YES" : "NO");
        modern_tokenizer_advance(tokenizer);
    }
    
    modern_tokenizer_free(tokenizer);
    return 0;
}
