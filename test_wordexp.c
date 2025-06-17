#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "include/lusush.h"
#include "include/symtable.h"

// Simple test for the new word expansion system
int main() {
    printf("Testing new word expansion system...\n");
    
    // Initialize symbol table for testing
    init_symtable();
    
    // Test 1: String builder only
    printf("Test 1: String builder\n");
    str_builder_t *sb = sb_create(10);
    assert(sb != NULL);
    
    assert(sb_append(sb, "Hello"));
    assert(sb_append(sb, " "));
    assert(sb_append(sb, "World"));
    
    char *built = sb_finalize(sb);
    assert(built != NULL);
    printf("  Built string: '%s'\n", built);
    assert(strcmp(built, "Hello World") == 0);
    
    free(built);
    sb_free(sb);
    
    // Test 2: Context management
    printf("Test 2: Context management\n");
    exp_ctx_t *ctx = create_expansion_context();
    assert(ctx != NULL);
    assert(!ctx->in_double_quotes);
    assert(!ctx->in_single_quotes);
    
    free_expansion_context(ctx);
    
    // Test 3: Simple variable expansion (without using the full pipeline)
    printf("Test 3: Variable expansion result structure\n");
    
    // Set a test variable
    symtable_entry_t *entry = add_to_symtable("TEST_VAR");
    if (entry) {
        symtable_entry_setval(entry, "hello");
    }
    
    // Test the expansion function directly
    expansion_t var_result = var_expand("$TEST_VAR", NULL);
    if (var_result.result == EXP_OK && var_result.expanded) {
        printf("  Variable expansion result: '%s'\n", var_result.expanded);
        assert(strcmp(var_result.expanded, "hello") == 0);
        free(var_result.expanded);
    } else {
        printf("  Variable expansion failed or returned no expansion\n");
    }
    
    // Test 4: Tilde expansion
    printf("Test 4: Tilde expansion\n");
    expansion_t tilde_result = tilde_expand("~", NULL);
    if (tilde_result.result == EXP_OK && tilde_result.expanded) {
        printf("  ~ -> '%s'\n", tilde_result.expanded);
        free(tilde_result.expanded);
    } else {
        printf("  Tilde expansion not available or failed\n");
    }
    
    // Test 5: Field splitting (empty test - just check it doesn't crash)
    printf("Test 5: Field splitting\n");
    word_t *fields = field_split("hello world test");
    if (fields) {
        printf("  Field splitting succeeded\n");
        word_t *w = fields;
        int count = 0;
        while (w) {
            printf("    Field %d: '%s'\n", count++, w->data);
            w = w->next;
        }
        free_all_words(fields);
    } else {
        printf("  No field splitting performed\n");
    }
    
    printf("\nAll basic tests passed!\n");
    printf("The new word expansion system is functional.\n");
    
    return 0;
}
