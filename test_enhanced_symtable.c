/**
 * Test Program for Enhanced Symbol Table Implementation
 * 
 * This program tests the enhanced symbol table implementation using libhashtable
 * and compares it with the standard implementation for functionality and performance.
 */

#include "include/symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Test configuration
#define TEST_ITERATIONS 1000
#define PERFORMANCE_ITERATIONS 10000

// Color codes for output
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_BLUE "\033[34m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

// Helper macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf(COLOR_GREEN "PASS" COLOR_RESET ": %s\n", message); \
            tests_passed++; \
        } else { \
            printf(COLOR_RED "FAIL" COLOR_RESET ": %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

#define TEST_SECTION(name) \
    printf(COLOR_BLUE "\n=== %s ===" COLOR_RESET "\n", name)

// Utility functions
static double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

static void print_test_summary(void) {
    printf(COLOR_BLUE "\n=== TEST SUMMARY ===" COLOR_RESET "\n");
    printf("Tests passed: " COLOR_GREEN "%d" COLOR_RESET "\n", tests_passed);
    printf("Tests failed: " COLOR_RED "%d" COLOR_RESET "\n", tests_failed);
    printf("Total tests:  %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "All tests passed!" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Some tests failed!" COLOR_RESET "\n");
    }
}

// Test basic functionality
static void test_basic_functionality(void) {
    TEST_SECTION("Basic Functionality Tests");
    
    // Test feature detection
    bool enhanced_available = symtable_libht_available();
    printf("Enhanced implementation available: %s\n", enhanced_available ? "YES" : "NO");
    printf("Implementation: %s\n", symtable_implementation_info());
    
    if (!enhanced_available) {
        printf(COLOR_YELLOW "NOTE: Enhanced tests require compilation with -DSYMTABLE_USE_LIBHASHTABLE=1" COLOR_RESET "\n");
        return;
    }
    
    // Ensure clean state
    free_symtable_libht();
    
    // Test initialization
    init_symtable_libht();
    void *manager = get_libht_manager();
    TEST_ASSERT(manager != NULL, "Enhanced manager initialization");
    
    // Test basic variable operations
    int result = symtable_set_var_enhanced("test_var1", "value1", SYMVAR_NONE);
    TEST_ASSERT(result == 0, "Set variable operation");
    
    char *value = symtable_get_var_enhanced("test_var1");
    TEST_ASSERT(value != NULL && strcmp(value, "value1") == 0, "Get variable operation");
    free(value);
    
    // Test non-existent variable
    value = symtable_get_var_enhanced("nonexistent");
    TEST_ASSERT(value == NULL, "Non-existent variable returns NULL");
    
    // Test empty value
    result = symtable_set_var_enhanced("empty_var", "", SYMVAR_NONE);
    TEST_ASSERT(result == 0, "Set empty variable");
    value = symtable_get_var_enhanced("empty_var");
    TEST_ASSERT(value != NULL && strcmp(value, "") == 0, "Get empty variable");
    free(value);
    
    // Test NULL value
    result = symtable_set_var_enhanced("null_var", NULL, SYMVAR_NONE);
    TEST_ASSERT(result == 0, "Set NULL variable");
    value = symtable_get_var_enhanced("null_var");
    TEST_ASSERT(value != NULL && strcmp(value, "") == 0, "NULL value becomes empty string");
    free(value);
    
    // Cleanup
    free_symtable_libht();
}

// Test scope operations
static void test_scope_operations(void) {
    TEST_SECTION("Scope Operations Tests");
    
    if (!symtable_libht_available()) {
        printf(COLOR_YELLOW "Skipping scope tests - enhanced implementation not available" COLOR_RESET "\n");
        return;
    }
    
    // Ensure clean state
    free_symtable_libht();
    init_symtable_libht();
    
    // Test global scope variable
    int result = symtable_set_var_enhanced("global_var", "global_value", SYMVAR_NONE);
    TEST_ASSERT(result == 0, "Set global variable");
    
    // Test function scope
    result = symtable_push_scope_enhanced(SCOPE_FUNCTION, "test_function");
    TEST_ASSERT(result == 0, "Push function scope");
    
    // Test local variable
    result = symtable_set_var_enhanced("local_var", "local_value", SYMVAR_LOCAL);
    TEST_ASSERT(result == 0, "Set local variable");
    
    // Test variable shadowing
    result = symtable_set_var_enhanced("global_var", "shadowed_value", SYMVAR_NONE);
    TEST_ASSERT(result == 0, "Shadow global variable");
    
    char *value = symtable_get_var_enhanced("global_var");
    TEST_ASSERT(value != NULL && strcmp(value, "shadowed_value") == 0, "Shadowed variable access");
    free(value);
    
    // Test access to local variable
    value = symtable_get_var_enhanced("local_var");
    TEST_ASSERT(value != NULL && strcmp(value, "local_value") == 0, "Local variable access");
    free(value);
    
    // Test nested scope
    result = symtable_push_scope_enhanced(SCOPE_LOOP, "test_loop");
    TEST_ASSERT(result == 0, "Push nested scope");
    
    result = symtable_set_var_enhanced("loop_var", "loop_value", SYMVAR_NONE);
    TEST_ASSERT(result == 0, "Set nested scope variable");
    
    // Pop nested scope
    result = symtable_pop_scope_enhanced();
    TEST_ASSERT(result == 0, "Pop nested scope");
    
    // Verify nested variable is gone
    value = symtable_get_var_enhanced("loop_var");
    TEST_ASSERT(value == NULL, "Nested variable removed after scope pop");
    
    // Verify local variable still exists
    value = symtable_get_var_enhanced("local_var");
    TEST_ASSERT(value != NULL && strcmp(value, "local_value") == 0, "Local variable persists");
    free(value);
    
    // Pop function scope
    result = symtable_pop_scope_enhanced();
    TEST_ASSERT(result == 0, "Pop function scope");
    
    // Verify global variable is restored
    value = symtable_get_var_enhanced("global_var");
    TEST_ASSERT(value != NULL && strcmp(value, "global_value") == 0, "Global variable restored");
    free(value);
    
    // Verify local variable is gone
    value = symtable_get_var_enhanced("local_var");
    TEST_ASSERT(value == NULL, "Local variable removed after function scope pop");
    
    // Test error conditions
    result = symtable_pop_scope_enhanced(); // Should fail - can't pop global
    TEST_ASSERT(result != 0, "Cannot pop global scope");
    
    free_symtable_libht();
}

// Test variable flags and types
static void test_variable_flags(void) {
    TEST_SECTION("Variable Flags Tests");
    
    if (!symtable_libht_available()) {
        printf(COLOR_YELLOW "Skipping flags tests - enhanced implementation not available" COLOR_RESET "\n");
        return;
    }
    
    // Ensure clean state
    free_symtable_libht();
    init_symtable_libht();
    
    // Test exported variable
    int result = symtable_set_var_enhanced("exported_var", "exported_value", SYMVAR_EXPORTED);
    TEST_ASSERT(result == 0, "Set exported variable");
    
    // Test readonly variable
    result = symtable_set_var_enhanced("readonly_var", "readonly_value", SYMVAR_READONLY);
    TEST_ASSERT(result == 0, "Set readonly variable");
    
    // Test local variable
    result = symtable_set_var_enhanced("local_var", "local_value", SYMVAR_LOCAL);
    TEST_ASSERT(result == 0, "Set local variable");
    
    // Test combination of flags
    result = symtable_set_var_enhanced("combo_var", "combo_value", 
                                       SYMVAR_EXPORTED | SYMVAR_READONLY);
    TEST_ASSERT(result == 0, "Set variable with combined flags");
    
    // Verify values can be retrieved
    char *value = symtable_get_var_enhanced("exported_var");
    TEST_ASSERT(value != NULL && strcmp(value, "exported_value") == 0, "Exported variable retrieval");
    free(value);
    
    value = symtable_get_var_enhanced("readonly_var");
    TEST_ASSERT(value != NULL && strcmp(value, "readonly_value") == 0, "Readonly variable retrieval");
    free(value);
    
    value = symtable_get_var_enhanced("combo_var");
    TEST_ASSERT(value != NULL && strcmp(value, "combo_value") == 0, "Combined flags variable retrieval");
    free(value);
    
    free_symtable_libht();
}

// Performance comparison test
static void test_performance_comparison(void) {
    TEST_SECTION("Performance Comparison");
    
    if (!symtable_libht_available()) {
        printf(COLOR_YELLOW "Skipping performance tests - enhanced implementation not available" COLOR_RESET "\n");
        return;
    }
    
    printf("Running performance comparison with %d iterations...\n", PERFORMANCE_ITERATIONS);
    
    // Ensure clean state for both implementations
    free_global_symtable();
    free_symtable_libht();
    
    // Initialize both implementations
    init_symtable();
    init_symtable_libht();
    
    struct timespec start, end;
    double standard_time, enhanced_time;
    
    // Test standard implementation
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < PERFORMANCE_ITERATIONS; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "var_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        
        set_global_var(name, value);
        char *retrieved = get_global_var(name);
        free(retrieved);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    standard_time = get_time_diff(start, end);
    
    // Test enhanced implementation
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < PERFORMANCE_ITERATIONS; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "var_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);
        
        symtable_set_var_enhanced(name, value, SYMVAR_NONE);
        char *retrieved = symtable_get_var_enhanced(name);
        free(retrieved);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    enhanced_time = get_time_diff(start, end);
    
    // Report results
    printf("Standard implementation: %.6f seconds\n", standard_time);
    printf("Enhanced implementation: %.6f seconds\n", enhanced_time);
    
    if (enhanced_time < standard_time) {
        double speedup = standard_time / enhanced_time;
        printf(COLOR_GREEN "Enhanced is %.2fx faster!" COLOR_RESET "\n", speedup);
    } else if (enhanced_time > standard_time) {
        double slowdown = enhanced_time / standard_time;
        printf(COLOR_YELLOW "Enhanced is %.2fx slower" COLOR_RESET "\n", slowdown);
    } else {
        printf("Performance is equivalent\n");
    }
    
    // Cleanup
    free_global_symtable();
    free_symtable_libht();
}

// Stress test
static void test_stress(void) {
    TEST_SECTION("Stress Tests");
    
    if (!symtable_libht_available()) {
        printf(COLOR_YELLOW "Skipping stress tests - enhanced implementation not available" COLOR_RESET "\n");
        return;
    }
    
    // Ensure clean state
    free_symtable_libht();
    init_symtable_libht();
    
    printf("Running stress test with %d variables...\n", TEST_ITERATIONS);
    
    // Create many variables
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        char name[32], value[64];
        snprintf(name, sizeof(name), "stress_var_%d", i);
        snprintf(value, sizeof(value), "stress_value_%d_with_longer_content", i);
        
        int result = symtable_set_var_enhanced(name, value, SYMVAR_NONE);
        if (result != 0) {
            printf(COLOR_RED "Failed to set variable %d" COLOR_RESET "\n", i);
            tests_failed++;
            break;
        }
    }
    
    // Verify all variables exist
    int verified = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        char name[32], expected[64];
        snprintf(name, sizeof(name), "stress_var_%d", i);
        snprintf(expected, sizeof(expected), "stress_value_%d_with_longer_content", i);
        
        char *value = symtable_get_var_enhanced(name);
        if (value && strcmp(value, expected) == 0) {
            verified++;
        } else {
            if (i < 10) { // Only print first 10 failures to avoid spam
                printf("DEBUG: Variable %s failed - got: %s, expected: %s\n", 
                       name, value ? value : "NULL", expected);
            }
        }
        free(value);
    }
    
    printf("DEBUG: Verified %d out of %d stress test variables\n", verified, TEST_ITERATIONS);
    TEST_ASSERT(verified == TEST_ITERATIONS, "All stress test variables verified");
    
    // Test with deep scoping
    printf("Testing deep scope nesting...\n");
    for (int i = 0; i < 50; i++) {
        char scope_name[32];
        snprintf(scope_name, sizeof(scope_name), "scope_%d", i);
        int result = symtable_push_scope_enhanced(SCOPE_FUNCTION, scope_name);
        if (result != 0) {
            printf("Failed to push scope %d\n", i);
            break;
        }
    }
    
    // Pop all scopes
    for (int i = 0; i < 50; i++) {
        symtable_pop_scope_enhanced();
    }
    
    printf("Deep scope nesting test completed\n");
    
    free_symtable_libht();
}

// Run internal self-test
static void test_internal_self_test(void) {
    TEST_SECTION("Internal Self-Test");
    
    int result = symtable_libht_test();
    if (result == 0) {
        printf(COLOR_GREEN "Internal self-test passed" COLOR_RESET "\n");
        tests_passed++;
    } else if (result == -1 && !symtable_libht_available()) {
        printf(COLOR_YELLOW "Internal self-test skipped - feature not available" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Internal self-test failed" COLOR_RESET "\n");
        tests_failed++;
    }
}

// Main test runner
int main(int argc, char *argv[]) {
    printf(COLOR_BLUE "Enhanced Symbol Table Test Suite" COLOR_RESET "\n");
    printf("=======================================\n");
    
    // Check if enhanced features should be tested
    bool run_enhanced = true;
    if (argc > 1 && strcmp(argv[1], "--standard-only") == 0) {
        run_enhanced = false;
        printf("Running standard implementation tests only\n");
    }
    
    // Run tests
    test_internal_self_test();
    
    if (run_enhanced) {
        test_basic_functionality();
        test_scope_operations();
        test_variable_flags();
        test_stress();
        test_performance_comparison();
    }
    
    // Print summary
    print_test_summary();
    
    return tests_failed > 0 ? 1 : 0;
}