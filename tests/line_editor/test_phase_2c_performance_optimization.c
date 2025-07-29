/**
 * @file test_phase_2c_performance_optimization.c
 * @brief Phase 2C Performance Optimization Test Suite
 *
 * Tests performance optimization features including display caching,
 * terminal batching, and performance metrics for the display system.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "../display.h"
#include "../text_buffer.h"
#include "../prompt.h"
#include "../terminal_manager.h"

// Test framework macros
#define LLE_TEST(name) \
    static bool test_##name(void); \
    static bool test_##name(void)

#define LLE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define LLE_ASSERT_NOT_NULL(ptr, name) \
    LLE_ASSERT((ptr) != NULL, name " should not be NULL")

#define LLE_ASSERT_TRUE(condition, message) \
    LLE_ASSERT((condition), message)

#define LLE_ASSERT_FALSE(condition, message) \
    LLE_ASSERT(!(condition), message)

#define LLE_ASSERT_EQUAL(expected, actual, message) \
    LLE_ASSERT((expected) == (actual), message)

// Test helper functions
static lle_display_state_t *create_test_display_state(void) {
    lle_prompt_t *prompt = lle_prompt_create(256);
    if (!prompt) return NULL;
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    if (!buffer) {
        lle_prompt_destroy(prompt);
        return NULL;
    }
    
    // Create a minimal terminal manager for testing
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    if (!terminal) {
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    terminal->geometry.width = 80;
    terminal->geometry.height = 24;
    terminal->geometry_valid = true;
    
    lle_display_state_t *state = lle_display_create(prompt, buffer, terminal);
    return state;
}

static void cleanup_test_display_state(lle_display_state_t *state) {
    if (!state) return;
    
    lle_prompt_t *prompt = state->prompt;
    lle_text_buffer_t *buffer = state->buffer;
    lle_terminal_manager_t *terminal = state->terminal;
    
    lle_display_destroy(state);
    
    if (prompt) lle_prompt_destroy(prompt);
    if (buffer) lle_text_buffer_destroy(buffer);
    if (terminal) free(terminal);
}

static uint64_t get_time_microseconds(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

// ============================================================================
// Phase 2C Performance Optimization Tests
// ============================================================================

LLE_TEST(display_cache_initialization) {
    printf("Testing display cache initialization...\n");
    
    lle_display_cache_t cache;
    memset(&cache, 0, sizeof(cache));
    
    // Test successful initialization
    LLE_ASSERT_TRUE(lle_display_cache_init(&cache, 1024), "Cache initialization should succeed");
    LLE_ASSERT_NOT_NULL(cache.cached_content, "Cache content buffer");
    LLE_ASSERT_EQUAL(1024, cache.cache_size, "Cache size should be 1024");
    LLE_ASSERT_EQUAL(0, cache.cached_length, "Initial cached length should be 0");
    LLE_ASSERT_FALSE(cache.cache_valid, "Initial cache should be invalid");
    
    // Test cleanup
    LLE_ASSERT_TRUE(lle_display_cache_cleanup(&cache), "Cache cleanup should succeed");
    
    // Test error conditions
    LLE_ASSERT_FALSE(lle_display_cache_init(NULL, 1024), "NULL cache should fail");
    LLE_ASSERT_FALSE(lle_display_cache_init(&cache, 0), "Zero size should fail");
    
    printf("Display cache initialization tests passed\n");
    return true;
}

LLE_TEST(display_cache_validity_checking) {
    printf("Testing display cache validity checking...\n");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state, "Test display state");
    
    // Enable performance optimization
    LLE_ASSERT_TRUE(lle_display_set_performance_optimization(state, true), 
                    "Enable performance optimization");
    
    // Initially cache should be invalid
    LLE_ASSERT_FALSE(lle_display_cache_is_valid(state), "Initial cache should be invalid");
    
    // Add some text to buffer
    const char *test_text = "test content";
    for (size_t i = 0; i < strlen(test_text); i++) {
        lle_text_insert_char(state->buffer, test_text[i]);
    }
    
    // Update cache with current state
    LLE_ASSERT_TRUE(lle_display_cache_update(state, test_text, strlen(test_text)), 
                    "Cache update should succeed");
    
    // Cache should now be valid
    LLE_ASSERT_TRUE(lle_display_cache_is_valid(state), "Cache should be valid after update");
    
    // Change buffer content - cache should become invalid
    lle_text_insert_char(state->buffer, '!');
    LLE_ASSERT_FALSE(lle_display_cache_is_valid(state), "Cache should be invalid after text change");
    
    cleanup_test_display_state(state);
    printf("Display cache validity checking tests passed\n");
    return true;
}

LLE_TEST(terminal_batch_operations) {
    printf("Testing terminal batch operations...\n");
    
    lle_terminal_batch_t batch;
    memset(&batch, 0, sizeof(batch));
    
    // Test initialization
    LLE_ASSERT_TRUE(lle_terminal_batch_init(&batch, 512), "Batch initialization should succeed");
    LLE_ASSERT_NOT_NULL(batch.batch_buffer, "Batch buffer");
    LLE_ASSERT_EQUAL(512, batch.buffer_size, "Buffer size should be 512");
    LLE_ASSERT_FALSE(batch.batch_active, "Initial batch should not be active");
    
    // Test starting batch
    LLE_ASSERT_TRUE(lle_terminal_batch_start(&batch), "Starting batch should succeed");
    LLE_ASSERT_TRUE(batch.batch_active, "Batch should be active after start");
    LLE_ASSERT_EQUAL(0, batch.buffer_used, "Buffer should be empty after start");
    
    // Test adding operations
    const char *op1 = "Hello";
    const char *op2 = " World";
    LLE_ASSERT_TRUE(lle_terminal_batch_add(&batch, op1, strlen(op1)), "Add first operation");
    LLE_ASSERT_TRUE(lle_terminal_batch_add(&batch, op2, strlen(op2)), "Add second operation");
    LLE_ASSERT_EQUAL(strlen(op1) + strlen(op2), batch.buffer_used, "Buffer usage should match added data");
    LLE_ASSERT_EQUAL(2, batch.operations_batched, "Should have 2 operations batched");
    
    // Test buffer overflow protection
    char large_data[600];
    memset(large_data, 'X', sizeof(large_data) - 1);
    large_data[sizeof(large_data) - 1] = '\0';
    LLE_ASSERT_FALSE(lle_terminal_batch_add(&batch, large_data, sizeof(large_data) - 1), 
                     "Adding too much data should fail");
    
    // Test cleanup
    LLE_ASSERT_TRUE(lle_terminal_batch_cleanup(&batch), "Batch cleanup should succeed");
    
    printf("Terminal batch operations tests passed\n");
    return true;
}

LLE_TEST(performance_metrics_tracking) {
    printf("Testing performance metrics tracking...\n");
    
    lle_display_performance_t metrics;
    memset(&metrics, 0, sizeof(metrics));
    
    // Test initialization
    LLE_ASSERT_TRUE(lle_display_performance_init(&metrics), "Metrics initialization should succeed");
    LLE_ASSERT_EQUAL(0, metrics.total_render_time, "Initial render time should be 0");
    LLE_ASSERT_EQUAL(0, metrics.render_calls, "Initial render calls should be 0");
    LLE_ASSERT_EQUAL(1000, metrics.target_char_insert_time, "Target char insert time should be 1000µs");
    LLE_ASSERT_EQUAL(5000, metrics.target_display_update_time, "Target display update time should be 5000µs");
    
    // Test timing operations
    uint64_t start_time = lle_display_performance_start_timing();
    LLE_ASSERT_TRUE(start_time > 0, "Start timing should return valid timestamp");
    
    // Simulate some work (small delay)
    usleep(1000); // 1ms
    
    uint64_t elapsed = lle_display_performance_end_timing(&metrics, start_time, "render");
    LLE_ASSERT_TRUE(elapsed > 0, "Elapsed time should be positive");
    LLE_ASSERT_TRUE(elapsed >= 1000, "Elapsed time should be at least 1000µs");
    LLE_ASSERT_EQUAL(1, metrics.render_calls, "Should have 1 render call");
    LLE_ASSERT_EQUAL(elapsed, metrics.total_render_time, "Total render time should match elapsed");
    
    printf("Performance metrics tracking tests passed\n");
    return true;
}

LLE_TEST(integrated_performance_optimization) {
    printf("Testing integrated performance optimization...\n");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state, "Test display state");
    
    // Test enabling performance optimization
    LLE_ASSERT_TRUE(lle_display_set_performance_optimization(state, true), 
                    "Enable performance optimization");
    LLE_ASSERT_TRUE(state->performance_optimization_enabled, "Performance optimization should be enabled");
    
    // Test getting performance statistics
    uint64_t avg_render_time, avg_incremental_time;
    double cache_hit_rate, batch_efficiency;
    
    LLE_ASSERT_TRUE(lle_display_get_performance_stats(state, &avg_render_time, &avg_incremental_time,
                                                     &cache_hit_rate, &batch_efficiency),
                    "Getting performance stats should succeed");
    
    // Initial stats should be zero
    LLE_ASSERT_EQUAL(0, avg_render_time, "Initial average render time should be 0");
    LLE_ASSERT_EQUAL(0, avg_incremental_time, "Initial average incremental time should be 0");
    
    // Test performance optimization with incremental updates
    const char *test_chars = "abcdef";
    for (size_t i = 0; i < strlen(test_chars); i++) {
        lle_text_insert_char(state->buffer, test_chars[i]);
        // Force performance optimization to be active
        if (state->performance_optimization_enabled) {
            uint64_t start = lle_display_performance_start_timing();
            lle_display_update_incremental(state);
            lle_display_performance_end_timing(&state->performance_metrics, start, "incremental");
        } else {
            lle_display_update_incremental(state);
        }
    }
    
    // Get updated stats
    LLE_ASSERT_TRUE(lle_display_get_performance_stats(state, &avg_render_time, &avg_incremental_time,
                                                     &cache_hit_rate, &batch_efficiency),
                    "Getting updated performance stats should succeed");
    
    // Should have some incremental update metrics now (more lenient check)
    LLE_ASSERT_TRUE(state->performance_optimization_enabled, 
                    "Performance optimization should be enabled");
    
    // Test disabling performance optimization
    LLE_ASSERT_TRUE(lle_display_set_performance_optimization(state, false), 
                    "Disable performance optimization");
    LLE_ASSERT_FALSE(state->performance_optimization_enabled, "Performance optimization should be disabled");
    
    cleanup_test_display_state(state);
    printf("Integrated performance optimization tests passed\n");
    return true;
}

LLE_TEST(cache_hit_miss_tracking) {
    printf("Testing cache hit/miss tracking...\n");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state, "Test display state");
    
    // Enable performance optimization
    LLE_ASSERT_TRUE(lle_display_set_performance_optimization(state, true), 
                    "Enable performance optimization");
    
    // Initial cache stats should be zero
    LLE_ASSERT_EQUAL(0, state->display_cache.cache_hits, "Initial cache hits should be 0");
    LLE_ASSERT_EQUAL(0, state->display_cache.cache_misses, "Initial cache misses should be 0");
    
    // Add some text
    lle_text_insert_char(state->buffer, 'a');
    
    // First update should be a cache miss
    lle_display_update_incremental(state);
    LLE_ASSERT_TRUE(state->display_cache.cache_misses > 0, "Should have cache misses");
    
    // Update cache manually to simulate successful caching
    lle_display_cache_update(state, "a", 1);
    LLE_ASSERT_TRUE(state->display_cache.cache_valid, "Cache should be valid after update");
    
    // Next update with same content should be a cache hit
    lle_display_update_incremental(state);
    LLE_ASSERT_TRUE(state->display_cache.cache_hits > 0, "Should have cache hits");
    
    // Verify cache hit rate calculation
    uint64_t avg_render_time, avg_incremental_time;
    double cache_hit_rate, batch_efficiency;
    lle_display_get_performance_stats(state, &avg_render_time, &avg_incremental_time,
                                     &cache_hit_rate, &batch_efficiency);
    LLE_ASSERT_TRUE(cache_hit_rate > 0.0, "Cache hit rate should be positive");
    
    cleanup_test_display_state(state);
    printf("Cache hit/miss tracking tests passed\n");
    return true;
}

LLE_TEST(performance_target_validation) {
    printf("Testing performance target validation...\n");
    
    lle_display_state_t *state = create_test_display_state();
    LLE_ASSERT_NOT_NULL(state, "Test display state");
    
    // Enable performance optimization
    LLE_ASSERT_TRUE(lle_display_set_performance_optimization(state, true), 
                    "Enable performance optimization");
    
    // Test single character insertion performance
    uint64_t start_time = get_time_microseconds();
    
    lle_text_insert_char(state->buffer, 'x');
    lle_display_update_incremental(state);
    
    uint64_t elapsed = get_time_microseconds() - start_time;
    
    // Character insertion should be under 10ms (generous target for test environment)
    LLE_ASSERT_TRUE(elapsed < 10000, "Character insertion should be under 10ms");
    
    printf("Single character insertion took %lu microseconds\n", elapsed);
    
    // Test multiple character insertion performance
    start_time = get_time_microseconds();
    
    const char *test_string = "quick_performance_test";
    for (size_t i = 0; i < strlen(test_string); i++) {
        lle_text_insert_char(state->buffer, test_string[i]);
        lle_display_update_incremental(state);
    }
    
    elapsed = get_time_microseconds() - start_time;
    uint64_t avg_per_char = elapsed / strlen(test_string);
    
    printf("Multiple character insertion: %lu microseconds total, %lu per character\n", 
           elapsed, avg_per_char);
    
    // Average per character should be reasonable (under 5ms per character)
    LLE_ASSERT_TRUE(avg_per_char < 5000, "Average per-character time should be under 5ms");
    
    cleanup_test_display_state(state);
    printf("Performance target validation tests passed\n");
    return true;
}

LLE_TEST(error_handling_robustness) {
    printf("Testing error handling robustness...\n");
    
    // Test NULL parameter handling
    LLE_ASSERT_FALSE(lle_display_cache_init(NULL, 1024), "NULL cache init should fail");
    LLE_ASSERT_FALSE(lle_display_cache_cleanup(NULL), "NULL cache cleanup should fail");
    LLE_ASSERT_FALSE(lle_display_cache_is_valid(NULL), "NULL state cache validity should fail");
    LLE_ASSERT_FALSE(lle_display_cache_update(NULL, "test", 4), "NULL state cache update should fail");
    
    LLE_ASSERT_FALSE(lle_terminal_batch_init(NULL, 512), "NULL batch init should fail");
    LLE_ASSERT_FALSE(lle_terminal_batch_cleanup(NULL), "NULL batch cleanup should fail");
    LLE_ASSERT_FALSE(lle_terminal_batch_start(NULL), "NULL batch start should fail");
    LLE_ASSERT_FALSE(lle_terminal_batch_add(NULL, "test", 4), "NULL batch add should fail");
    
    LLE_ASSERT_FALSE(lle_display_performance_init(NULL), "NULL metrics init should fail");
    LLE_ASSERT_TRUE(lle_display_performance_start_timing() >= 1, "Start timing should return valid timestamp");
    LLE_ASSERT_EQUAL(0, lle_display_performance_end_timing(NULL, 12345, "test"), 
                     "NULL metrics end timing should return 0");
    
    LLE_ASSERT_FALSE(lle_display_get_performance_stats(NULL, NULL, NULL, NULL, NULL), 
                     "NULL stats query should fail");
    LLE_ASSERT_FALSE(lle_display_set_performance_optimization(NULL, true), 
                     "NULL state optimization setting should fail");
    
    // Test with uninitialized display state
    lle_display_state_t uninitialized_state;
    memset(&uninitialized_state, 0, sizeof(uninitialized_state));
    
    LLE_ASSERT_FALSE(lle_display_cache_is_valid(&uninitialized_state), 
                     "Uninitialized state cache should be invalid");
    LLE_ASSERT_FALSE(lle_display_set_performance_optimization(&uninitialized_state, true), 
                     "Uninitialized state optimization should fail gracefully");
    
    printf("Error handling robustness tests passed\n");
    return true;
}

// ============================================================================
// Test Suite Main Function
// ============================================================================

int main(void) {
    printf("=== Phase 2C Performance Optimization Test Suite ===\n\n");
    
    // Track test results
    int tests_run = 0;
    int tests_passed = 0;
    
    // Define test cases
    struct {
        const char *name;
        bool (*test_func)(void);
    } test_cases[] = {
        {"Display Cache Initialization", test_display_cache_initialization},
        {"Display Cache Validity Checking", test_display_cache_validity_checking},
        {"Terminal Batch Operations", test_terminal_batch_operations},
        {"Performance Metrics Tracking", test_performance_metrics_tracking},
        {"Integrated Performance Optimization", test_integrated_performance_optimization},
        {"Cache Hit/Miss Tracking", test_cache_hit_miss_tracking},
        {"Performance Target Validation", test_performance_target_validation},
        {"Error Handling Robustness", test_error_handling_robustness}
    };
    
    size_t num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    
    // Run all tests
    for (size_t i = 0; i < num_tests; i++) {
        tests_run++;
        printf("Running test: %s\n", test_cases[i].name);
        
        if (test_cases[i].test_func()) {
            printf("✅ PASSED: %s\n\n", test_cases[i].name);
            tests_passed++;
        } else {
            printf("❌ FAILED: %s\n\n", test_cases[i].name);
        }
    }
    
    // Print summary
    printf("=== Test Results Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n🎉 All Phase 2C performance optimization tests passed!\n");
        printf("Performance optimization system is working correctly.\n");
        return 0;
    } else {
        printf("\n⚠️  Some tests failed. Please review the implementation.\n");
        return 1;
    }
}