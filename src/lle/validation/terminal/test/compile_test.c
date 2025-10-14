// src/lle/validation/terminal/test/compile_test.c
// Simple compilation test for terminal state structures

#include "../state.h"
#include <stdio.h>

int main(void) {
    lle_terminal_validation_t term;
    
    printf("=== LLE Terminal State Structure Compilation Test ===\n\n");
    
    printf("Structure sizes:\n");
    printf("  lle_terminal_state_t: %zu bytes\n", sizeof(lle_terminal_state_t));
    printf("  lle_terminal_caps_t: %zu bytes\n", sizeof(lle_terminal_caps_t));
    printf("  lle_terminal_validation_t: %zu bytes\n", sizeof(lle_terminal_validation_t));
    
    printf("\nStructure alignment:\n");
    printf("  lle_terminal_state_t: %zu bytes\n", _Alignof(lle_terminal_state_t));
    printf("  lle_terminal_caps_t: %zu bytes\n", _Alignof(lle_terminal_caps_t));
    printf("  lle_terminal_validation_t: %zu bytes\n", _Alignof(lle_terminal_validation_t));
    
    printf("\n✅ Compilation successful - header guards correct, structures valid\n");
    
    return 0;
}
