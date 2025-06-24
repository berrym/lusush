/**
 * Simple Debug Test for Enhanced Symbol Table
 * 
 * This is a minimal test to isolate the double-free issue
 */

#include "include/symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== Simple Enhanced Symbol Table Debug Test ===\n");
    
    if (!symtable_libht_available()) {
        printf("Enhanced implementation not available\n");
        return 1;
    }
    
    printf("1. Initializing enhanced symbol table...\n");
    init_symtable_libht();
    
    printf("2. Setting a variable...\n");
    int result = symtable_set_var_enhanced("test", "value", 0);
    if (result != 0) {
        printf("FAIL: Could not set variable\n");
        return 1;
    }
    
    printf("3. Getting the variable...\n");
    char *value = symtable_get_var_enhanced("test");
    if (!value) {
        printf("FAIL: Could not get variable\n");
        return 1;
    }
    printf("Got value: %s\n", value);
    free(value);
    
    printf("4. Setting another variable...\n");
    result = symtable_set_var_enhanced("test2", "value2", 0);
    if (result != 0) {
        printf("FAIL: Could not set second variable\n");
        return 1;
    }
    
    printf("5. Overwriting first variable...\n");
    result = symtable_set_var_enhanced("test", "new_value", 0);
    if (result != 0) {
        printf("FAIL: Could not overwrite variable\n");
        return 1;
    }
    
    printf("6. Getting overwritten variable...\n");
    value = symtable_get_var_enhanced("test");
    if (!value) {
        printf("FAIL: Could not get overwritten variable\n");
        return 1;
    }
    printf("Got overwritten value: %s\n", value);
    free(value);
    
    printf("7. Cleaning up...\n");
    free_symtable_libht();
    
    printf("SUCCESS: All operations completed\n");
    return 0;
}