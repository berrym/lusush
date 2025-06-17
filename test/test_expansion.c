#include "../include/alias.h"
#include "../include/alias_expand.h"
#include "../include/expand.h"
#include "../include/lusush.h"
#include "../include/wordexp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test alias expansion
void test_alias_expansion() {
    printf("Testing alias expansion...\n");
    
    // Initialize aliases
    init_aliases();
    
    // Set some test aliases
    set_alias("ll", "ls -la");
    set_alias("gs", "git status");
    set_alias("gl", "git log");
    set_alias("recursive", "echo recursive");
    set_alias("loop1", "loop2");
    set_alias("loop2", "loop1");
    
    // Test basic expansion
    char *expanded = expand_aliases_recursive("ll", 10);
    assert(expanded != NULL);
    assert(strcmp(expanded, "ls -la") == 0);
    printf("Basic expansion: OK\n");
    free(expanded);
    
    // Test first word expansion
    expanded = expand_first_word_alias("ll -h");
    assert(expanded != NULL);
    assert(strcmp(expanded, "ls -la -h") == 0);
    printf("First word expansion: OK\n");
    free(expanded);
    
    // Test recursion limit
    expanded = expand_aliases_recursive("loop1", 10);
    assert(expanded != NULL);  // Should return something even with recursion
    printf("Recursion handling: OK\n");
    free(expanded);
    
    // Cleanup
    free_aliases();
    printf("Alias expansion tests passed\n\n");
}

// Test expansion context
void test_expansion_context() {
    printf("Testing expansion context...\n");
    
    // Initialize a context
    expand_ctx_t ctx;
    expand_ctx_init(&ctx, EXPAND_NORMAL);
    
    // Test normal context
    assert(expand_ctx_check(&ctx, EXPAND_NORMAL) == false);  // EXPAND_NORMAL is 0
    assert(expand_ctx_check(&ctx, EXPAND_ALIAS) == false);
    assert(expand_ctx_check(&ctx, EXPAND_NOQUOTE) == false);
    
    // Test alias context
    expand_ctx_init(&ctx, EXPAND_ALIAS);
    assert(expand_ctx_check(&ctx, EXPAND_ALIAS) == true);
    assert(expand_ctx_check(&ctx, EXPAND_NOQUOTE) == false);
    
    // Test combined flags
    expand_ctx_init(&ctx, EXPAND_ALIAS | EXPAND_NOQUOTE);
    assert(expand_ctx_check(&ctx, EXPAND_ALIAS) == true);
    assert(expand_ctx_check(&ctx, EXPAND_NOQUOTE) == true);
    assert(expand_ctx_check(&ctx, EXPAND_NOVAR) == false);
    
    printf("Expansion context tests passed\n\n");
}

// Main test function
int main() {
    printf("Running expansion tests...\n\n");
    
    test_expansion_context();
    test_alias_expansion();
    
    printf("All tests passed!\n");
    return 0;
}
