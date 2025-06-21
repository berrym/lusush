#include "symtable_modern.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main() {
    printf("Testing modern symbol table...\n");
    
    // Create manager
    symtable_manager_t *mgr = symtable_manager_new();
    assert(mgr != NULL);
    
    printf("✓ Manager created\n");
    
    // Test global variable
    assert(symtable_set_var(mgr, "test", "value", SYMVAR_NONE) == 0);
    char *value = symtable_get_var(mgr, "test");
    assert(value != NULL);
    assert(strcmp(value, "value") == 0);
    free(value);
    
    printf("✓ Global variable set/get works\n");
    
    // Test loop scope
    assert(symtable_push_scope(mgr, SCOPE_LOOP, "test-loop") == 0);
    assert(symtable_set_local_var(mgr, "i", "1") == 0);
    
    value = symtable_get_var(mgr, "i");
    assert(value != NULL);
    assert(strcmp(value, "1") == 0);
    free(value);
    
    printf("✓ Loop scope works\n");
    
    // Pop scope and verify variable is gone
    assert(symtable_pop_scope(mgr) == 0);
    value = symtable_get_var(mgr, "i");
    assert(value == NULL);
    
    printf("✓ Scope cleanup works\n");
    
    // Cleanup
    symtable_manager_free(mgr);
    
    printf("✓ All tests passed!\n");
    return 0;
}
