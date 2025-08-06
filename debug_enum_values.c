/**
 * @file debug_enum_values.c
 * @brief Debug utility to print actual enum values for arrow keys
 * 
 * This utility helps debug the mismatch between expected and actual
 * enum values for arrow keys in the line editor.
 */

#include <stdio.h>
#include "src/line_editor/input_handler.h"

int main(void) {
    printf("=== LLE KEY ENUM VALUES DEBUG ===\n");
    printf("\n");
    
    printf("Basic keys:\n");
    printf("LLE_KEY_CHAR = %d\n", LLE_KEY_CHAR);
    printf("LLE_KEY_BACKSPACE = %d\n", LLE_KEY_BACKSPACE);
    printf("LLE_KEY_DELETE = %d\n", LLE_KEY_DELETE);
    printf("LLE_KEY_ENTER = %d\n", LLE_KEY_ENTER);
    printf("LLE_KEY_TAB = %d\n", LLE_KEY_TAB);
    printf("LLE_KEY_ESCAPE = %d\n", LLE_KEY_ESCAPE);
    printf("\n");
    
    printf("Arrow keys:\n");
    printf("LLE_KEY_ARROW_LEFT = %d\n", LLE_KEY_ARROW_LEFT);
    printf("LLE_KEY_ARROW_RIGHT = %d\n", LLE_KEY_ARROW_RIGHT);
    printf("LLE_KEY_ARROW_UP = %d\n", LLE_KEY_ARROW_UP);
    printf("LLE_KEY_ARROW_DOWN = %d\n", LLE_KEY_ARROW_DOWN);
    printf("\n");
    
    printf("Navigation keys:\n");
    printf("LLE_KEY_HOME = %d\n", LLE_KEY_HOME);
    printf("LLE_KEY_END = %d\n", LLE_KEY_END);
    printf("LLE_KEY_PAGE_UP = %d\n", LLE_KEY_PAGE_UP);
    printf("LLE_KEY_PAGE_DOWN = %d\n", LLE_KEY_PAGE_DOWN);
    printf("\n");
    
    printf("Control keys:\n");
    printf("LLE_KEY_CTRL_A = %d\n", LLE_KEY_CTRL_A);
    printf("LLE_KEY_CTRL_B = %d\n", LLE_KEY_CTRL_B);
    printf("LLE_KEY_CTRL_C = %d\n", LLE_KEY_CTRL_C);
    printf("LLE_KEY_CTRL_D = %d\n", LLE_KEY_CTRL_D);
    printf("LLE_KEY_CTRL_E = %d\n", LLE_KEY_CTRL_E);
    printf("LLE_KEY_CTRL_F = %d\n", LLE_KEY_CTRL_F);
    printf("LLE_KEY_CTRL_G = %d\n", LLE_KEY_CTRL_G);
    printf("LLE_KEY_CTRL_H = %d\n", LLE_KEY_CTRL_H);
    printf("LLE_KEY_CTRL_I = %d\n", LLE_KEY_CTRL_I);
    printf("LLE_KEY_CTRL_J = %d\n", LLE_KEY_CTRL_J);
    printf("LLE_KEY_CTRL_K = %d\n", LLE_KEY_CTRL_K);
    printf("LLE_KEY_CTRL_L = %d\n", LLE_KEY_CTRL_L);
    printf("LLE_KEY_CTRL_M = %d\n", LLE_KEY_CTRL_M);
    printf("LLE_KEY_CTRL_N = %d\n", LLE_KEY_CTRL_N);
    printf("LLE_KEY_CTRL_O = %d\n", LLE_KEY_CTRL_O);
    printf("LLE_KEY_CTRL_P = %d\n", LLE_KEY_CTRL_P);
    printf("\n");
    
    printf("=== ANALYSIS ===\n");
    printf("From debug log: Arrow UP detected as type=8\n");
    printf("Expected: LLE_KEY_ARROW_UP = %d\n", LLE_KEY_ARROW_UP);
    printf("Match: %s\n", (LLE_KEY_ARROW_UP == 8) ? "YES" : "NO");
    printf("\n");
    printf("From debug log: Arrow DOWN detected as type=9\n");
    printf("Expected: LLE_KEY_ARROW_DOWN = %d\n", LLE_KEY_ARROW_DOWN);
    printf("Match: %s\n", (LLE_KEY_ARROW_DOWN == 9) ? "YES" : "NO");
    printf("\n");
    
    return 0;
}