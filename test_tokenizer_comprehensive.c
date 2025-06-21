/**
 * Comprehensive test of modern tokenizer for control structures
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/tokenizer_new.h"

void test_tokenize(const char *input, const char *description) {
    printf("\n=== %s ===\n", description);
    printf("Input: %s\n", input);
    
    modern_tokenizer_t *tokenizer = modern_tokenizer_new(input);
    if (!tokenizer) {
        printf("ERROR: Failed to create tokenizer\n");
        return;
    }
    
    int token_count = 0;
    while (modern_tokenizer_current(tokenizer) && 
           modern_tokenizer_current(tokenizer)->type != MODERN_TOK_EOF) {
        modern_token_t *token = modern_tokenizer_current(tokenizer);
        printf("  %d: %s '%s'\n", 
               ++token_count, 
               modern_token_type_name(token->type), 
               token->text);
        modern_tokenizer_advance(tokenizer);
    }
    
    printf("Total tokens: %d\n", token_count);
    modern_tokenizer_free(tokenizer);
}

int main() {
    printf("=== MODERN TOKENIZER COMPREHENSIVE TEST ===\n");
    
    // Test assignments
    test_tokenize("i=5", "Simple assignment");
    test_tokenize("i=$((i+1))", "Arithmetic assignment");
    test_tokenize("PATH=\"$PATH:/usr/bin\"", "Path assignment with expansion");
    
    // Test control structures
    test_tokenize("if [ $i -eq 5 ]; then echo yes; fi", "If statement");
    test_tokenize("while [ $i -lt 10 ]; do echo $i; i=$((i+1)); done", "While loop");
    test_tokenize("for i in 1 2 3; do echo $i; done", "For loop");
    
    // Test complex expressions
    test_tokenize("[ $((i % 2)) -eq 0 ]", "Modulo arithmetic in test");
    test_tokenize("$(echo hello)", "Command substitution");
    test_tokenize("echo 'Don\\'t panic'", "Escaped quotes");
    
    return 0;
}
