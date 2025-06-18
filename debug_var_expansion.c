#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Simple test program to debug variable expansion
int main() {
    printf("=== Variable Expansion Debug Test ===\n");
    
    // Test 1: Set a variable and try to expand it
    setenv("TEST_VAR", "hello_world", 1);
    printf("TEST_VAR environment variable set to: %s\n", getenv("TEST_VAR"));
    
    // Test 2: Check if our shell can access this variable
    printf("\nTesting shell variable access...\n");
    
    return 0;
}
