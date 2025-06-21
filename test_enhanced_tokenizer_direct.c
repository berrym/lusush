/**
 * Direct test of our enhanced tokenizer to verify it's working
 */

#include <stdio.h>
#include <string.h>
#include "include/tokenizer_new.h"

int main() {
    printf("=== TESTING ENHANCED TOKENIZER DIRECTLY ===\n");
    
    // Test 1: Arithmetic expansion 
    const char *test1 = "i=$((i+1))";
    printf("\nTest 1: %s\n", test1);
    
    modern_tokenizer_t *tokenizer1 = modern_tokenizer_new(test1);
    if (!tokenizer1) {
        printf("FAIL: Could not create tokenizer\n");
        return 1;
    }
    
    int token_count = 0;
    while (modern_tokenizer_current(tokenizer1) && 
           modern_tokenizer_current(tokenizer1)->type != MODERN_TOK_EOF) {
        modern_token_t *token = modern_tokenizer_current(tokenizer1);
        printf("  Token %d: %s '%s'\n", ++token_count, 
               modern_token_type_name(token->type), token->text);
        modern_tokenizer_advance(tokenizer1);
    }
    printf("  Result: %s (%d tokens)\n", 
           token_count > 0 && strstr(test1, "$((") ? "PASS - Arithmetic expansion recognized" : "FAIL", 
           token_count);
    
    modern_tokenizer_free(tokenizer1);
    
    // Test 2: Complex while loop
    const char *test2 = "while [ \"$i\" -lt 10 ]; do echo $i; done";
    printf("\nTest 2: %s\n", test2);
    
    modern_tokenizer_t *tokenizer2 = modern_tokenizer_new(test2);
    if (!tokenizer2) {
        printf("FAIL: Could not create tokenizer\n");
        return 1;
    }
    
    token_count = 0;
    int errors = 0;
    while (modern_tokenizer_current(tokenizer2) && 
           modern_tokenizer_current(tokenizer2)->type != MODERN_TOK_EOF) {
        modern_token_t *token = modern_tokenizer_current(tokenizer2);
        if (token->type == MODERN_TOK_ERROR) {
            errors++;
            printf("  ERROR TOKEN: '%s'\n", token->text);
        } else {
            token_count++;
        }
        modern_tokenizer_advance(tokenizer2);
    }
    printf("  Result: %s (%d tokens, %d errors)\n", 
           errors == 0 ? "PASS - No error tokens" : "FAIL", 
           token_count, errors);
    
    modern_tokenizer_free(tokenizer2);
    
    // Test 3: Variable assignment
    const char *test3 = "var=value";
    printf("\nTest 3: %s\n", test3);
    
    modern_tokenizer_t *tokenizer3 = modern_tokenizer_new(test3);
    if (!tokenizer3) {
        printf("FAIL: Could not create tokenizer\n");
        return 1;
    }
    
    token_count = 0;
    bool has_assign = false;
    while (modern_tokenizer_current(tokenizer3) && 
           modern_tokenizer_current(tokenizer3)->type != MODERN_TOK_EOF) {
        modern_token_t *token = modern_tokenizer_current(tokenizer3);
        if (token->type == MODERN_TOK_ASSIGN) {
            has_assign = true;
        }
        token_count++;
        modern_tokenizer_advance(tokenizer3);
    }
    printf("  Result: %s (%d tokens, assign=%s)\n", 
           has_assign ? "PASS - Assignment operator recognized" : "FAIL", 
           token_count, has_assign ? "yes" : "no");
    
    modern_tokenizer_free(tokenizer3);
    
    printf("\n=== ENHANCED TOKENIZER TEST COMPLETE ===\n");
    return 0;
}
