/**
 * Test to check what our tokenizer produces for the problematic while loop
 */

#include <stdio.h>
#include "include/tokenizer_new.h"

int main() {
    const char *input = "while [ \"$i\" -lt 3 ]; do echo \"i=$i\"; done";
    printf("Testing tokenization of: %s\n\n", input);
    
    tokenizer_t *tokenizer = tokenizer_new(input);
    if (!tokenizer) {
        printf("Failed to create tokenizer\n");
        return 1;
    }
    
    int count = 0;
    while (tokenizer_current(tokenizer) && tokenizer_current(tokenizer)->type != TOK_EOF) {
        token_t *token = tokenizer_current(tokenizer);
        printf("Token %d: type=%d (%s) text='%s'\n", 
               ++count, token->type, token_type_name(token->type), token->text);
        tokenizer_advance(tokenizer);
    }
    
    tokenizer_free(tokenizer);
    return 0;
}
