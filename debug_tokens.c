#include <stdio.h>
#include <string.h>
#include "include/scanner.h"
#include "include/parser.h"
#include "include/node.h"

void debug_tokens(const char *input) {
    printf("Input: '%s'\n", input);
    
    source_t *src = malloc(sizeof(source_t));
    src->buf = strdup(input);
    src->bufsize = strlen(input);
    src->pos = -1;
    src->curline = 1;
    src->curchar = 0;
    src->curlinestart = 0;
    
    printf("Tokens:\n");
    int token_count = 0;
    while (1) {
        token_t *tok = tokenize(src);
        if (!tok || tok->type == TOKEN_EOF) {
            printf("  [%d] EOF\n", token_count);
            if (tok && tok != &eof_token) free_token(tok);
            break;
        }
        
        printf("  [%d] Type: %d, Text: '%s'\n", token_count, tok->type, tok->text);
        token_count++;
        
        if (tok != &eof_token) {
            free_token(tok);
        }
        
        if (token_count > 10) break; // Safety limit
    }
    
    free(src->buf);
    free(src);
}

int main() {
    init_scanner();
    
    printf("=== Debug Token Analysis ===\n\n");
    
    debug_tokens("y=test echo $y");
    printf("\n");
    debug_tokens("y=test");
    printf("\n");
    debug_tokens("echo $y");
    
    return 0;
}
