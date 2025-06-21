/**
 * Comprehensive test of the modern execution engine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/executor_modern.h"

void test_execute(const char *input, const char *description) {
    printf("\n=== %s ===\n", description);
    printf("Input: %s\n", input);
    
    executor_modern_t *executor = executor_modern_new();
    if (!executor) {
        printf("ERROR: Failed to create executor\n");
        return;
    }
    
    // Enable debug mode for detailed output
    executor_modern_set_debug(executor, true);
    
    int result = executor_modern_execute_command_line(executor, input);
    
    if (executor_modern_has_error(executor)) {
        printf("EXECUTION ERROR: %s\n", executor_modern_error(executor));
    } else {
        printf("SUCCESS: Exit status %d\n", result);
    }
    
    executor_modern_free(executor);
}

int main() {
    printf("=== MODERN EXECUTION ENGINE TEST ===\n");
    
    // Test simple commands
    test_execute("echo hello world", "Simple echo command");
    test_execute("pwd", "Simple builtin command");
    
    // Test assignments
    test_execute("i=5", "Simple assignment");
    test_execute("name=test", "String assignment");
    
    // Test variable expansion (if environment supports it)
    test_execute("echo $USER", "Variable expansion");
    test_execute("echo $HOME", "Environment variable");
    
    // Test arithmetic (basic)
    setenv("counter", "5", 1); // Set up test variable
    test_execute("echo $counter", "Counter variable");
    test_execute("result=$((counter+1))", "Arithmetic assignment");
    test_execute("echo $result", "Display arithmetic result");
    
    // Test pipelines
    test_execute("echo hello | cat", "Simple pipeline");
    test_execute("echo test | grep test", "Pipeline with grep");
    
    // Test control structures
    test_execute("if true; then echo 'success'; fi", "Simple if statement");
    test_execute("if false; then echo 'fail'; else echo 'success'; fi", "If-else statement");
    
    // Test for loops
    test_execute("for i in 1 2 3; do echo \"Number: $i\"; done", "Simple for loop");
    
    // Test while loops (with safety limit)
    test_execute("i=1; while [ $i -lt 3 ]; do echo \"Loop $i\"; i=$((i+1)); done", "While loop with counter");
    
    printf("\n=== TEST COMPLETE ===\n");
    return 0;
}
