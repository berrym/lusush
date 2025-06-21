/**
 * Comprehensive tokenizer test to verify robust handling of complex shell constructs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/tokenizer_new.h"

void print_tokens(const char *input) {
    printf("\n=== Tokenizing: %s ===\n", input);
    
    tokenizer_t *tokenizer = tokenizer_new(input);
    if (!tokenizer) {
        printf("ERROR: Failed to create tokenizer\n");
        return;
    }
    
    int token_count = 0;
    while (tokenizer_current(tokenizer) && tokenizer_current(tokenizer)->type != TOK_EOF) {
        token_t *token = tokenizer_current(tokenizer);
        printf("Token %d: %s '%s' (pos: %zu, line: %zu, col: %zu)\n", 
               ++token_count, token_type_name(token->type), token->text,
               token->position, token->line, token->column);
        tokenizer_advance(tokenizer);
    }
    
    printf("Total tokens: %d\n", token_count);
    tokenizer_free(tokenizer);
}

int main() {
    printf("=== TOKENIZER ROBUSTNESS TEST ===\n");
    
    // Test 1: Simple commands
    print_tokens("echo hello");
    print_tokens("ls -la");
    
    // Test 2: Pipelines
    print_tokens("ls | grep test");
    print_tokens("cat file | head -n 10 | tail -n 5");
    
    // Test 3: Variable expansion
    print_tokens("echo $USER");
    print_tokens("echo ${HOME}/bin");
    print_tokens("test \"$var\" = \"value\"");
    
    // Test 4: Test commands and brackets
    print_tokens("[ -f file ]");
    print_tokens("[ \"$i\" -lt 10 ]");
    print_tokens("test $i -eq 0");
    
    // Test 5: Control structures - basic
    print_tokens("if true; then echo ok; fi");
    print_tokens("while [ true ]; do echo loop; done");
    print_tokens("for i in 1 2 3; do echo $i; done");
    
    // Test 6: Complex control structures (the problematic ones)
    print_tokens("while [ \"$i\" -lt 10 ]; do echo $i; i=$((i+1)); done");
    print_tokens("for file in *.txt; do [ -f \"$file\" ] && echo \"$file\"; done");
    print_tokens("if [ -f \"$HOME/.bashrc\" ]; then source \"$HOME/.bashrc\"; fi");
    
    // Test 7: Nested quotes and escaping
    print_tokens("echo \"Hello 'world'\"");
    print_tokens("echo 'Don\\'t panic'");
    
    // Test 8: Redirections
    print_tokens("cat < input.txt > output.txt");
    print_tokens("echo hello >> log.txt");
    print_tokens("cat << EOF");
    
    // Test 9: Logical operators
    print_tokens("true && echo success");
    print_tokens("false || echo failure");
    print_tokens("cmd1 && cmd2 || cmd3");
    
    // Test 10: Complex mixed constructs
    print_tokens("while read line; do [ -n \"$line\" ] && echo \"Line: $line\"; done < file.txt");
    print_tokens("for i in $(seq 1 10); do [ $((i % 2)) -eq 0 ] && echo \"Even: $i\"; done");
    
    // Test 11: Variable assignments
    print_tokens("var=value");
    print_tokens("PATH=\"$PATH:/usr/local/bin\"");
    print_tokens("export USER_HOME=\"$HOME\"");
    
    // Test 12: Background and process control
    print_tokens("sleep 10 &");
    print_tokens("(cd /tmp && ls)");
    print_tokens("{echo start; echo end;}");
    
    printf("\n=== TEST COMPLETE ===\n");
    return 0;
}
