#include <stdio.h>
#include "executor_modern.h"
#include "parser_modern.h"

int main() {
    // Create executor
    executor_modern_t *executor = executor_modern_new();
    if (!executor) {
        printf("Failed to create executor\n");
        return 1;
    }
    
    // Enable debug mode
    executor_modern_set_debug(executor, true);
    
    // Test FOR loop
    const char *cmd = "for i in one two three; do echo \"Item: $i\"; done";
    printf("Testing: %s\n\n", cmd);
    
    int result = executor_modern_execute_command_line(executor, cmd);
    printf("\nResult: %d\n", result);
    
    executor_modern_free(executor);
    return 0;
}
