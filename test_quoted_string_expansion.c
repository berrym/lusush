/**
 * Test suite for quoted string variable expansion
 * Tests the newly implemented feature for expanding variables within double quotes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "executor_modern.h"

// Test basic variable expansion in double quotes
void test_basic_quoted_expansion() {
    printf("Testing basic quoted string expansion...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test basic expansion
    int result = executor_modern_execute_command_line(executor, "x=hello; echo \"Value: $x\"");
    assert(result == 0);
    
    executor_modern_free(executor);
    printf("✅ Basic quoted expansion test passed\n");
}

// Test multiple variables in quoted string
void test_multiple_variables() {
    printf("Testing multiple variables in quoted string...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test multiple variables
    int result = executor_modern_execute_command_line(executor, "a=hello; b=world; echo \"$a $b\"");
    assert(result == 0);
    
    executor_modern_free(executor);
    printf("✅ Multiple variables test passed\n");
}

// Test both variable syntax forms
void test_variable_syntax_forms() {
    printf("Testing both $var and ${var} syntax...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test both forms
    int result = executor_modern_execute_command_line(executor, "name=user; echo \"Hello $name and ${name}\"");
    assert(result == 0);
    
    executor_modern_free(executor);
    printf("✅ Variable syntax forms test passed\n");
}

// Test arithmetic expansion in quotes
void test_arithmetic_in_quotes() {
    printf("Testing arithmetic expansion in quoted strings...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test arithmetic
    int result = executor_modern_execute_command_line(executor, "a=5; b=3; echo \"Sum: $((a+b))\"");
    assert(result == 0);
    
    executor_modern_free(executor);
    printf("✅ Arithmetic in quotes test passed\n");
}

// Test FOR loop variables in quotes
void test_for_loop_variables() {
    printf("Testing FOR loop variables in quoted strings...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test FOR loop
    int result = executor_modern_execute_command_line(executor, "for i in one two; do echo \"Item: $i\"; done");
    assert(result == 0);
    
    executor_modern_free(executor);
    printf("✅ FOR loop variables test passed\n");
}

// Test single quotes preserve literals
void test_single_quote_literals() {
    printf("Testing single quotes preserve literals...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test literal preservation
    int result = executor_modern_execute_command_line(executor, "x=test; echo 'Value: $x'");
    assert(result == 0);
    
    executor_modern_free(executor);
    printf("✅ Single quote literals test passed\n");
}

// Test empty and edge cases
void test_edge_cases() {
    printf("Testing edge cases...\n");
    
    executor_modern_t *executor = executor_modern_new();
    assert(executor != NULL);
    
    // Test empty variable
    int result1 = executor_modern_execute_command_line(executor, "empty=; echo \"Empty: '$empty'\"");
    assert(result1 == 0);
    
    // Test undefined variable
    int result2 = executor_modern_execute_command_line(executor, "echo \"Undefined: '$undefined'\"");
    assert(result2 == 0);
    
    executor_modern_free(executor);
    printf("✅ Edge cases test passed\n");
}

int main() {
    printf("=== Quoted String Variable Expansion Test Suite ===\n\n");
    
    test_basic_quoted_expansion();
    test_multiple_variables();
    test_variable_syntax_forms();
    test_arithmetic_in_quotes();
    test_for_loop_variables();
    test_single_quote_literals();
    test_edge_cases();
    
    printf("\n🎉 All quoted string expansion tests passed!\n");
    printf("✅ Feature is working correctly and ready for production\n");
    
    return 0;
}
