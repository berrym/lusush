/**
 * Comprehensive Performance Comparison Test for All Symbol Table Implementations
 * 
 * This program tests and compares all available symbol table implementations:
 * 1. Standard implementation (djb2-like hash)
 * 2. Enhanced implementation (libhashtable generic ht_t)
 * 3. Optimized implementation (libhashtable ht_strstr_t)
 */

#include "include/symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Test configuration
#define PERFORMANCE_ITERATIONS 50000
#define STRESS_VARIABLES 5000
#define SCOPE_DEPTH 20

// Color codes for output
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_BLUE "\033[34m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
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

// Performance measurement utilities
static double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Implementation info structure
typedef struct {
    const char *name;
    const char *description;
    bool available;
    double insert_time;
    double lookup_time;
    double combined_time;
    size_t memory_estimate;
} impl_info_t;

// Test implementations availability
static void test_implementation_availability(void) {
    TEST_SECTION("Implementation Availability");
    
    printf("Standard implementation: %s\n", COLOR_GREEN "Available" COLOR_RESET);
    
    bool enhanced_available = symtable_libht_available();
    printf("Enhanced implementation: %s\n", 
           enhanced_available ? COLOR_GREEN "Available" COLOR_RESET : COLOR_RED "Not Available" COLOR_RESET);
    if (enhanced_available) {
        printf("  Details: %s\n", symtable_implementation_info());
    }
    
    bool optimized_available = symtable_opt_available();
    printf("Optimized implementation: %s\n", 
           optimized_available ? COLOR_GREEN "Available" COLOR_RESET : COLOR_RED "Not Available" COLOR_RESET);
    if (optimized_available) {
        printf("  Details: %s\n", symtable_opt_implementation_info());
    }
    
    if (!enhanced_available && !optimized_available) {
        printf(COLOR_YELLOW "\nNOTE: Enhanced implementations require compilation with:\n");
        printf("  -DSYMTABLE_USE_LIBHASHTABLE=1 (enhanced)\n");
        printf("  -DSYMTABLE_USE_LIBHASHTABLE_V2=1 (optimized)\n" COLOR_RESET);
    }
}

// Benchmark standard implementation
static double benchmark_standard_implementation(int iterations) {
    printf("Benchmarking standard implementation...\n");
    
    init_symtable();
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        char name[32], value[64];
        snprintf(name, sizeof(name), "std_var_%d", i);
        snprintf(value, sizeof(value), "std_value_%d_with_some_content", i);
        
        set_global_var(name, value);
        char *retrieved = get_global_var(name);
        free(retrieved);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = get_time_diff(start, end);
    
    free_global_symtable();
    
    printf("  Time: %.6f seconds\n", time_taken);
    return time_taken;
}

// Benchmark enhanced implementation
static double benchmark_enhanced_implementation(int iterations) {
    if (!symtable_libht_available()) {
        printf("Enhanced implementation not available\n");
        return -1.0;
    }
    
    printf("Benchmarking enhanced implementation...\n");
    
    free_symtable_libht(); // Ensure clean state
    init_symtable_libht();
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        char name[32], value[64];
        snprintf(name, sizeof(name), "enh_var_%d", i);
        snprintf(value, sizeof(value), "enh_value_%d_with_some_content", i);
        
        symtable_set_var_enhanced(name, value, SYMVAR_NONE);
        char *retrieved = symtable_get_var_enhanced(name);
        free(retrieved);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = get_time_diff(start, end);
    
    free_symtable_libht();
    
    printf("  Time: %.6f seconds\n", time_taken);
    return time_taken;
}

// Benchmark optimized implementation
static double benchmark_optimized_implementation(int iterations) {
    if (!symtable_opt_available()) {
        printf("Optimized implementation not available\n");
        return -1.0;
    }
    
    printf("Benchmarking optimized implementation...\n");
    
    free_symtable_opt(); // Ensure clean state
    init_symtable_opt();
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        char name[32], value[64];
        snprintf(name, sizeof(name), "opt_var_%d", i);
        snprintf(value, sizeof(value), "opt_value_%d_with_some_content", i);
        
        symtable_set_var_opt_api(name, value, SYMVAR_NONE);
        char *retrieved = symtable_get_var_opt_api(name);
        free(retrieved);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = get_time_diff(start, end);
    
    free_symtable_opt();
    
    printf("  Time: %.6f seconds\n", time_taken);
    return time_taken;
}

// Performance comparison test
static void test_performance_comparison(void) {
    TEST_SECTION("Performance Comparison");
    
    printf("Running comprehensive performance test with %d iterations...\n", PERFORMANCE_ITERATIONS);
    
    impl_info_t implementations[3] = {
        {"Standard", "djb2-like hash, custom implementation", true, 0, 0, 0, 0},
        {"Enhanced", "libhashtable generic ht_t interface", symtable_libht_available(), 0, 0, 0, 0},
        {"Optimized", "libhashtable ht_strstr_t interface", symtable_opt_available(), 0, 0, 0, 0}
    };
    
    // Benchmark each available implementation
    implementations[0].combined_time = benchmark_standard_implementation(PERFORMANCE_ITERATIONS);
    
    if (implementations[1].available) {
        implementations[1].combined_time = benchmark_enhanced_implementation(PERFORMANCE_ITERATIONS);
    }
    
    if (implementations[2].available) {
        implementations[2].combined_time = benchmark_optimized_implementation(PERFORMANCE_ITERATIONS);
    }
    
    // Print comparison results
    printf(COLOR_CYAN "\n=== Performance Results ===" COLOR_RESET "\n");
    printf("%-12s %-15s %-15s %s\n", "Implementation", "Time (seconds)", "Relative Speed", "Description");
    printf("%-12s %-15s %-15s %s\n", "-------------", "-------------", "-------------", "-----------");
    
    double baseline_time = implementations[0].combined_time;
    
    for (int i = 0; i < 3; i++) {
        if (!implementations[i].available) {
            printf("%-12s %-15s %-15s %s\n", 
                   implementations[i].name, "N/A", "N/A", "Not Available");
            continue;
        }
        
        double time = implementations[i].combined_time;
        if (time < 0) {
            printf("%-12s %-15s %-15s %s\n", 
                   implementations[i].name, "ERROR", "ERROR", implementations[i].description);
            continue;
        }
        
        double relative_speed = baseline_time / time;
        const char *color = COLOR_RESET;
        
        if (relative_speed > 1.5) {
            color = COLOR_GREEN;
        } else if (relative_speed < 0.8) {
            color = COLOR_RED;
        } else {
            color = COLOR_YELLOW;
        }
        
        printf("%-12s %s%-15.6f%s %s%.2fx%s       %s\n", 
               implementations[i].name, 
               color, time, COLOR_RESET,
               color, relative_speed, COLOR_RESET,
               implementations[i].description);
    }
    
    // Find the fastest implementation
    int fastest_idx = 0;
    double fastest_time = implementations[0].combined_time;
    
    for (int i = 1; i < 3; i++) {
        if (implementations[i].available && implementations[i].combined_time > 0 && 
            implementations[i].combined_time < fastest_time) {
            fastest_idx = i;
            fastest_time = implementations[i].combined_time;
        }
    }
    
    printf(COLOR_GREEN "\nFastest Implementation: %s" COLOR_RESET "\n", implementations[fastest_idx].name);
}

// Test correctness of all implementations
static void test_correctness_comparison(void) {
    TEST_SECTION("Correctness Comparison");
    
    printf("Testing correctness of all available implementations...\n");
    
    // Test standard implementation
    printf("Testing standard implementation...\n");
    init_symtable();
    set_global_var("test_var", "test_value");
    char *value = get_global_var("test_var");
    TEST_ASSERT(value && strcmp(value, "test_value") == 0, "Standard implementation basic test");
    free(value);
    free_global_symtable();
    
    // Test enhanced implementation
    if (symtable_libht_available()) {
        printf("Testing enhanced implementation...\n");
        free_symtable_libht();
        init_symtable_libht();
        symtable_set_var_enhanced("test_var", "test_value", SYMVAR_NONE);
        value = symtable_get_var_enhanced("test_var");
        TEST_ASSERT(value && strcmp(value, "test_value") == 0, "Enhanced implementation basic test");
        free(value);
        free_symtable_libht();
    }
    
    // Test optimized implementation
    if (symtable_opt_available()) {
        printf("Testing optimized implementation...\n");
        free_symtable_opt();
        init_symtable_opt();
        symtable_set_var_opt_api("test_var", "test_value", SYMVAR_NONE);
        value = symtable_get_var_opt_api("test_var");
        TEST_ASSERT(value && strcmp(value, "test_value") == 0, "Optimized implementation basic test");
        free(value);
        free_symtable_opt();
    }
}

// Stress test all implementations
static void test_stress_comparison(void) {
    TEST_SECTION("Stress Test Comparison");
    
    printf("Running stress test with %d variables on each implementation...\n", STRESS_VARIABLES);
    
    // Standard implementation stress test
    printf("Standard implementation stress test...\n");
    init_symtable();
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < STRESS_VARIABLES; i++) {
        char name[32], value[64];
        snprintf(name, sizeof(name), "stress_std_%d", i);
        snprintf(value, sizeof(value), "stress_value_%d_with_longer_content_for_testing", i);
        set_global_var(name, value);
    }
    
    // Verify all variables
    int verified_std = 0;
    for (int i = 0; i < STRESS_VARIABLES; i++) {
        char name[32], expected[64];
        snprintf(name, sizeof(name), "stress_std_%d", i);
        snprintf(expected, sizeof(expected), "stress_value_%d_with_longer_content_for_testing", i);
        char *value = get_global_var(name);
        if (value && strcmp(value, expected) == 0) {
            verified_std++;
        }
        free(value);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double std_time = get_time_diff(start, end);
    printf("  Standard: %d/%d variables verified in %.6f seconds\n", verified_std, STRESS_VARIABLES, std_time);
    TEST_ASSERT(verified_std == STRESS_VARIABLES, "Standard implementation stress test");
    free_global_symtable();
    
    // Enhanced implementation stress test
    if (symtable_libht_available()) {
        printf("Enhanced implementation stress test...\n");
        free_symtable_libht();
        init_symtable_libht();
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int i = 0; i < STRESS_VARIABLES; i++) {
            char name[32], value[64];
            snprintf(name, sizeof(name), "stress_enh_%d", i);
            snprintf(value, sizeof(value), "stress_value_%d_with_longer_content_for_testing", i);
            symtable_set_var_enhanced(name, value, SYMVAR_NONE);
        }
        
        int verified_enh = 0;
        for (int i = 0; i < STRESS_VARIABLES; i++) {
            char name[32], expected[64];
            snprintf(name, sizeof(name), "stress_enh_%d", i);
            snprintf(expected, sizeof(expected), "stress_value_%d_with_longer_content_for_testing", i);
            char *value = symtable_get_var_enhanced(name);
            if (value && strcmp(value, expected) == 0) {
                verified_enh++;
            }
            free(value);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double enh_time = get_time_diff(start, end);
        printf("  Enhanced: %d/%d variables verified in %.6f seconds\n", verified_enh, STRESS_VARIABLES, enh_time);
        TEST_ASSERT(verified_enh == STRESS_VARIABLES, "Enhanced implementation stress test");
        free_symtable_libht();
    }
    
    // Optimized implementation stress test
    if (symtable_opt_available()) {
        printf("Optimized implementation stress test...\n");
        free_symtable_opt();
        init_symtable_opt();
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int i = 0; i < STRESS_VARIABLES; i++) {
            char name[32], value[64];
            snprintf(name, sizeof(name), "stress_opt_%d", i);
            snprintf(value, sizeof(value), "stress_value_%d_with_longer_content_for_testing", i);
            symtable_set_var_opt_api(name, value, SYMVAR_NONE);
        }
        
        int verified_opt = 0;
        for (int i = 0; i < STRESS_VARIABLES; i++) {
            char name[32], expected[64];
            snprintf(name, sizeof(name), "stress_opt_%d", i);
            snprintf(expected, sizeof(expected), "stress_value_%d_with_longer_content_for_testing", i);
            char *value = symtable_get_var_opt_api(name);
            if (value && strcmp(value, expected) == 0) {
                verified_opt++;
            }
            free(value);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double opt_time = get_time_diff(start, end);
        printf("  Optimized: %d/%d variables verified in %.6f seconds\n", verified_opt, STRESS_VARIABLES, opt_time);
        TEST_ASSERT(verified_opt == STRESS_VARIABLES, "Optimized implementation stress test");
        free_symtable_opt();
    }
}

// Memory usage estimation (rough)
static void test_memory_usage_estimation(void) {
    TEST_SECTION("Memory Usage Estimation");
    
    printf("Estimating memory usage patterns (approximate)...\n");
    
    const int test_vars = 1000;
    const int avg_name_len = 15;
    const int avg_value_len = 30;
    
    // Standard implementation estimate
    size_t std_estimate = test_vars * (sizeof(void*) + // Hash table slot
                                      avg_name_len + avg_value_len + // Strings
                                      sizeof(void*) + // Linked list pointer
                                      64); // Structure overhead
    
    // Enhanced implementation estimate (higher due to copying)
    size_t enh_estimate = test_vars * (sizeof(void*) + // Hash table slot
                                      (avg_name_len + avg_value_len) * 2 + // String copies
                                      sizeof(void*) * 2 + // Structure pointers
                                      128); // Higher structure overhead
    
    // Optimized implementation estimate (should be most efficient)
    size_t opt_estimate = test_vars * (sizeof(void*) + // Hash table slot
                                      avg_name_len + avg_value_len + 20 + // Serialized data
                                      32); // Lower overhead
    
    printf("Estimated memory usage for %d variables:\n", test_vars);
    printf("  Standard:  ~%zu bytes (~%.1f KB)\n", std_estimate, std_estimate / 1024.0);
    printf("  Enhanced:  ~%zu bytes (~%.1f KB)\n", enh_estimate, enh_estimate / 1024.0);
    printf("  Optimized: ~%zu bytes (~%.1f KB)\n", opt_estimate, opt_estimate / 1024.0);
    
    printf("\nNote: These are rough estimates. Actual usage may vary.\n");
}

// Test summary
static void print_test_summary(void) {
    printf(COLOR_BLUE "\n=== COMPREHENSIVE TEST SUMMARY ===" COLOR_RESET "\n");
    printf("Tests passed: " COLOR_GREEN "%d" COLOR_RESET "\n", tests_passed);
    printf("Tests failed: " COLOR_RED "%d" COLOR_RESET "\n", tests_failed);
    printf("Total tests:  %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "\n🎉 All tests passed! Symbol table implementations are working correctly." COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "\n⚠️  Some tests failed. Review the results above." COLOR_RESET "\n");
    }
    
    printf("\nRecommendations:\n");
    if (symtable_opt_available()) {
        printf("• Use the optimized implementation for best performance\n");
    } else if (symtable_libht_available()) {
        printf("• Consider using the enhanced implementation for better hash distribution\n");
    } else {
        printf("• Compile with enhanced features for improved performance\n");
    }
    printf("• All implementations maintain POSIX shell compatibility\n");
    printf("• Performance differences may vary based on usage patterns\n");
}

// Main test runner
int main(int argc, char *argv[]) {
    printf(COLOR_CYAN "=== COMPREHENSIVE SYMBOL TABLE IMPLEMENTATION COMPARISON ===" COLOR_RESET "\n");
    printf("This test compares all available symbol table implementations.\n");
    printf("Performance test iterations: %d\n", PERFORMANCE_ITERATIONS);
    printf("Stress test variables: %d\n", STRESS_VARIABLES);
    printf("========================================================================\n");
    
    // Check command line arguments
    bool skip_performance = false;
    bool skip_stress = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--skip-performance") == 0) {
            skip_performance = true;
        } else if (strcmp(argv[i], "--skip-stress") == 0) {
            skip_stress = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --skip-performance  Skip performance benchmarks\n");
            printf("  --skip-stress       Skip stress tests\n");
            printf("  --help              Show this help\n");
            return 0;
        }
    }
    
    // Run all tests
    test_implementation_availability();
    test_correctness_comparison();
    
    if (!skip_stress) {
        test_stress_comparison();
    }
    
    if (!skip_performance) {
        test_performance_comparison();
    }
    
    test_memory_usage_estimation();
    
    // Print final summary
    print_test_summary();
    
    return tests_failed > 0 ? 1 : 0;
}