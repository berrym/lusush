#include <stdio.h>
#include <string.h>
#include "include/scanner.h"
#include "include/token_pushback.h"
#include "include/lusush.h"

int main() {
    printf("=== Token Pushback System Test ===\n");
    
    // Initialize scanner
    init_scanner();
    
    // Test input
    char *test_str = "for var in list do";
    source_t src = {
        .buf = test_str,
        .bufsize = strlen(test_str),
        .pos = -1
    };
    
    printf("Input: %s\n", test_str);
    printf("Tokenizing and pushing back...\n");
    
    // Tokenize several tokens
    token_t *tok1 = tokenize(&src);
    token_t *tok2 = tokenize(&src);
    token_t *tok3 = tokenize(&src);
    token_t *tok4 = tokenize(&src);
    
    if (tok1) printf("Token 1: %s (type: %d)\n", tok1->text, tok1->type);
    if (tok2) printf("Token 2: %s (type: %d)\n", tok2->text, tok2->type);
    if (tok3) printf("Token 3: %s (type: %d)\n", tok3->text, tok3->type);
    if (tok4) printf("Token 4: %s (type: %d)\n", tok4->text, tok4->type);
    
    // Push back tokens in reverse order
    printf("\nPushing back tokens...\n");
    if (tok4) unget_token(tok4);
    if (tok3) unget_token(tok3);
    if (tok2) unget_token(tok2);
    if (tok1) unget_token(tok1);
    
    // Now re-read them
    printf("\nRe-reading tokens:\n");
    token_t *check1 = tokenize(&src);
    token_t *check2 = tokenize(&src);
    token_t *check3 = tokenize(&src);
    token_t *check4 = tokenize(&src);
    
    if (check1) printf("Check 1: %s (type: %d)\n", check1->text, check1->type);
    if (check2) printf("Check 2: %s (type: %d)\n", check2->text, check2->type);
    if (check3) printf("Check 3: %s (type: %d)\n", check3->text, check3->type);
    if (check4) printf("Check 4: %s (type: %d)\n", check4->text, check4->type);
    
    // Clean up
    if (check1 && check1 != &eof_token) free_token(check1);
    if (check2 && check2 != &eof_token) free_token(check2);
    if (check3 && check3 != &eof_token) free_token(check3);
    if (check4 && check4 != &eof_token) free_token(check4);
    
    free_tok_buf();
    
    printf("\n=== Test Complete ===\n");
    return 0;
}
