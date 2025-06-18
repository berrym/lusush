#include "../include/scanner.h"
#include "../include/token_pushback.h"
#include "../include/lusush.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Example: Enhanced parser function that can look ahead multiple tokens
// This demonstrates how the new token pushback system enables better parsing

typedef enum {
    PARSE_OK,
    PARSE_ERROR,
    PARSE_INCOMPLETE
} parse_result_t;

// Example: Parse a complex for loop with lookahead
parse_result_t parse_enhanced_for_loop(source_t *src) {
    // Look ahead to determine for loop type without consuming tokens
    token_type_t for_pattern[] = {
        TOKEN_KEYWORD_FOR,
        TOKEN_WORD,           // variable name
        TOKEN_KEYWORD_IN      // 'in' keyword
    };
    
    if (match_token_sequence(src, for_pattern, 3)) {
        printf("Detected: for VAR in LIST loop\n");
        
        // Now we can safely consume the tokens we verified
        consume_tokens(src, 3);
        
        // Continue parsing the rest...
        return PARSE_OK;
    }
    
    // Check for C-style for loop: for ((expr; expr; expr))
    token_type_t c_for_pattern[] = {
        TOKEN_KEYWORD_FOR,
        TOKEN_LEFT_PAREN,
        TOKEN_LEFT_PAREN     // Double paren indicates C-style
    };
    
    if (match_token_sequence(src, c_for_pattern, 3)) {
        printf("Detected: C-style for ((;;)) loop\n");
        consume_tokens(src, 3);
        return PARSE_OK;
    }
    
    return PARSE_ERROR;
}

// Example: Parse complex redirection with lookahead
parse_result_t parse_complex_redirection(source_t *src) {
    // Look for patterns like: cmd 2>&1 >file
    // We need to peek ahead to see the full redirection sequence
    
    token_t *tok1 = peek_token_ahead(src, 0);
    token_t *tok2 = peek_token_ahead(src, 1);
    token_t *tok3 = peek_token_ahead(src, 2);
    
    if (tok1 && tok2 && tok3) {
        printf("Next 3 tokens: %s %s %s\n", 
               tok1->text ? tok1->text : "<null>",
               tok2->text ? tok2->text : "<null>",
               tok3->text ? tok3->text : "<null>");
               
        // Example: detect file descriptor redirection
        if (tok1->type == TOKEN_NUMBER && 
            tok2->type == TOKEN_REDIRECT_OUT &&
            tok3->type == TOKEN_WORD) {
            printf("Detected: file descriptor redirection\n");
            return PARSE_OK;
        }
    }
    
    return PARSE_ERROR;
}

// Test function to demonstrate the capabilities
void test_enhanced_parsing(void) {
    printf("=== Enhanced Token Pushback System Test ===\n\n");
    
    // Test 1: For loop detection
    printf("Test 1: For loop detection\n");
    char *test_input1 = "for var in list; do echo $var; done";
    source_t src1 = {
        .buf = test_input1,
        .bufsize = strlen(test_input1),
        .pos = -1
    };
    
    init_scanner();
    parse_result_t result1 = parse_enhanced_for_loop(&src1);
    printf("Result: %s\n\n", result1 == PARSE_OK ? "SUCCESS" : "FAILED");
    
    // Test 2: C-style for loop
    printf("Test 2: C-style for loop detection\n");
    char *test_input2 = "for ((i=0; i<10; i++)); do echo $i; done";
    source_t src2 = {
        .buf = test_input2,
        .bufsize = strlen(test_input2),
        .pos = -1
    };
    
    parse_result_t result2 = parse_enhanced_for_loop(&src2);
    printf("Result: %s\n\n", result2 == PARSE_OK ? "SUCCESS" : "FAILED");
    
    // Test 3: Complex redirection
    printf("Test 3: Complex redirection detection\n");
    char *test_input3 = "2 > /dev/null";
    source_t src3 = {
        .buf = test_input3,
        .bufsize = strlen(test_input3),
        .pos = -1
    };
    
    parse_result_t result3 = parse_complex_redirection(&src3);
    printf("Result: %s\n\n", result3 == PARSE_OK ? "SUCCESS" : "FAILED");
    
    printf("=== Test Complete ===\n");
}

// Example of how this could be integrated into the main shell
void demonstrate_integration(void) {
    printf("\n=== Integration Example ===\n");
    printf("The enhanced token pushback system enables:\n");
    printf("1. Multi-token lookahead for complex syntax detection\n");
    printf("2. Better error recovery in the parser\n");
    printf("3. Support for ambiguous grammar constructs\n");
    printf("4. More sophisticated auto-completion\n");
    printf("5. Better handling of nested control structures\n");
    printf("\nExample improvements:\n");
    printf("- Distinguish between 'for var in list' and 'for ((;;))' patterns\n");
    printf("- Parse complex redirection sequences correctly\n");
    printf("- Handle ambiguous tokens in different contexts\n");
    printf("- Provide better error messages with context\n");
}
