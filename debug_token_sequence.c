#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simplified token types for debugging
typedef enum {
    TOK_IF,
    TOK_THEN, 
    TOK_FI,
    TOK_WORD,
    TOK_SEMICOLON,
    TOK_EOF
} simple_token_type_t;

int main() {
    const char *input = "if true; then echo hello; fi";
    printf("Input: %s\n", input);
    printf("Expected tokens: IF WORD SEMICOLON THEN WORD WORD SEMICOLON FI\n");
    printf("Problem: Parser expects THEN but gets SEMICOLON\n");
    printf("Solution: Allow semicolon as substitute for THEN in if statements\n");
    return 0;
}
