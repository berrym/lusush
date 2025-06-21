#include <stdio.h>
#include "tokenizer_new.h"

int main() {
    const char *input = "a=test; for i in 1; do echo \"hi\"; done";
    printf("Tokenizing: %s\n\n", input);
    
    modern_tokenizer_t *tokenizer = modern_tokenizer_new(input);
    if (!tokenizer) {
        printf("Failed to create tokenizer\n");
        return 1;
    }
    
    int token_count = 0;
    while (!modern_tokenizer_match(tokenizer, MODERN_TOK_EOF)) {
        modern_token_t *token = modern_tokenizer_current(tokenizer);
        if (token) {
            printf("Token %d: Type=%d, Text='%s'\n", 
                   token_count++, token->type, token->text ? token->text : "(null)");
        }
        modern_tokenizer_advance(tokenizer);
    }
    
    modern_tokenizer_free(tokenizer);
    return 0;
}
