/**
 * Debug the exact flow through the modern parser for IF statements
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/executor_modern.h"

int main() {
    const char *test_input = "if true; then echo success; fi";
    printf("Testing executor with: %s\n", test_input);
    
    executor_modern_t *executor = executor_modern_new();
    if (!executor) {
        printf("Failed to create executor\n");
        return 1;
    }
    
    executor_modern_set_debug(executor, true);
    
    int result = executor_modern_execute_command_line(executor, test_input);
    
    if (executor_modern_has_error(executor)) {
        printf("Error: %s\n", executor_modern_error(executor));
    } else {
        printf("Success: exit code %d\n", result);
    }
    
    executor_modern_free(executor);
    return 0;
}
