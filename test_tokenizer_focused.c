/**
 * Focused test on previously problematic tokenizer constructs
 */

#include <stdio.h>
#include "include/tokenizer_new.h"

void test_construct(const char *input, const char *description) {
    printf("\n=== %s ===\n", description);
    printf("Input: %s\n", input);
    
    tokenizer_t *tokenizer = tokenizer_new(input);
    if (!tokenizer) {
        printf("ERROR: Failed to create tokenizer\n");
        return;
    }
    
    int errors = 0;
    while (tokenizer_current(tokenizer) && tokenizer_current(tokenizer)->type != TOK_EOF) {
        token_t *token = tokenizer_current(tokenizer);
        if (token->type == TOK_ERROR) {
            printf("ERROR TOKEN: '%s' at pos %zu\n", token->text, token->position);
            errors++;
        } else {
            printf("%s '%s' ", token_type_name(token->type), token->text);
        }
        tokenizer_advance(tokenizer);
    }
    
    printf("\n%s: %d error tokens\n", errors == 0 ? "PASS" : "FAIL", errors);
    tokenizer_free(tokenizer);
}

int main() {
    printf("=== FOCUSED TOKENIZER IMPROVEMENT TEST ===\n");
    
    test_construct("var=value", "Variable assignment");
    test_construct("PATH=\"$PATH:/usr/local/bin\"", "Path assignment with expansion");
    test_construct("i=$((i+1))", "Arithmetic expansion assignment");
    test_construct("while [ \"$i\" -lt 10 ]; do echo $i; i=$((i+1)); done", "While loop with arithmetic");
    test_construct("for file in *.txt; do echo \"$file\"; done", "For loop with globbing");
    test_construct("[ $((i % 2)) -eq 0 ]", "Modulo arithmetic in test");
    test_construct("for i in $(seq 1 10); do echo $i; done", "Command substitution in for loop");
    test_construct("echo 'Don\\'t panic'", "Escaped quotes");
    test_construct("test -f /path/to/file", "Test with flags and paths");
    test_construct("cmd1 && cmd2 || cmd3", "Logical operators");
    
    printf("\n=== TEST COMPLETE ===\n");
    return 0;
}
