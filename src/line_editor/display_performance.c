/**
 * @file display_performance.c
 * @brief Phase 2C Display Performance Benchmark and Validation Utilities
 *
 * This module provides comprehensive performance benchmarking and validation
 * utilities for the display system, enabling measurement and optimization
 * of display operations to meet performance targets.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * @brief Performance benchmark structure for display operations
 */
typedef struct {
    const char *operation_name;         /**< Name of the operation being benchmarked */
    uint64_t target_time_microseconds;  /**< Target completion time in microseconds */
    uint64_t measured_time;             /**< Actual measured time in microseconds */
    size_t iterations;                  /**< Number of iterations performed */
    bool target_met;                    /**< Whether performance target was met */
} lle_performance_benchmark_t;

/**
 * @brief Performance validation results
 */
typedef struct {
    size_t total_benchmarks;            /**< Total number of benchmarks run */
    size_t targets_met;                 /**< Number of targets that were met */
    double overall_performance_ratio;   /**< Overall performance as ratio of target */
    uint64_t total_time_spent;          /**< Total time spent in benchmarking */
    char summary_report[1024];          /**< Summary report text */
} lle_performance_validation_t;

// Performance targets (in microseconds)
#define LLE_PERF_TARGET_CHAR_INSERT 1000     // 1ms for character insertion
#define LLE_PERF_TARGET_CHAR_DELETE 1000     // 1ms for character deletion
#define LLE_PERF_TARGET_CURSOR_MOVE 1000     // 1ms for cursor movement
#define LLE_PERF_TARGET_WORD_MOVE 2000       // 2ms for word movement
#define LLE_PERF_TARGET_LINE_CLEAR 3000      // 3ms for line clearing
#define LLE_PERF_TARGET_DISPLAY_UPDATE 5000  // 5ms for display update
#define LLE_PERF_TARGET_CACHE_ACCESS 100     // 100µs for cache access
#define LLE_PERF_TARGET_BATCH_FLUSH 2000     // 2ms for batch flush

// Benchmark iteration counts
#define LLE_PERF_ITERATIONS_CHAR_OPS 100     // Character operations
#define LLE_PERF_ITERATIONS_CURSOR_OPS 50    // Cursor operations
#define LLE_PERF_ITERATIONS_DISPLAY_OPS 25   // Display operations
#define LLE_PERF_ITERATIONS_CACHE_OPS 200    // Cache operations

/**
 * @brief Get high-precision timestamp in microseconds
 * @return Current time in microseconds, 0 on error
 */
static uint64_t lle_perf_get_timestamp(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

/**
 * @brief Create a test display state for benchmarking
 * @return Display state for testing, NULL on error
 */
static lle_display_state_t *lle_perf_create_test_state(void) {
    lle_prompt_t *prompt = lle_prompt_create(256);
    if (!prompt) return NULL;
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(1024);
    if (!buffer) {
        lle_prompt_destroy(prompt);
        return NULL;
    }
    
    // Create minimal terminal manager for testing
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
    if (!state) {
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        free(terminal);
        return NULL;
    }
    
    // Enable performance optimization for benchmarking
    lle_display_set_performance_optimization(state, true);
    
    return state;
}

/**
 * @brief Clean up test display state
 * @param state Display state to clean up
 */
static void lle_perf_cleanup_test_state(lle_display_state_t *state) {
    if (!state) return;
    
    lle_prompt_t *prompt = state->prompt;
    lle_text_buffer_t *buffer = state->buffer;
    lle_terminal_manager_t *terminal = state->terminal;
    
    lle_display_destroy(state);
    
    if (prompt) lle_prompt_destroy(prompt);
    if (buffer) lle_text_buffer_destroy(buffer);
    if (terminal) free(terminal);
}

/**
 * @brief Benchmark character insertion performance
 * @param benchmark Benchmark structure to populate
 * @return true on success, false on error
 */
static bool lle_perf_benchmark_char_insert(lle_performance_benchmark_t *benchmark) {
    lle_display_state_t *state = lle_perf_create_test_state();
    if (!state) return false;
    
    benchmark->operation_name = "Character Insertion";
    benchmark->target_time_microseconds = LLE_PERF_TARGET_CHAR_INSERT;
    benchmark->iterations = LLE_PERF_ITERATIONS_CHAR_OPS;
    
    uint64_t start_time = lle_perf_get_timestamp();
    
    // Perform character insertions
    for (size_t i = 0; i < benchmark->iterations; i++) {
        char test_char = 'a' + (i % 26);
        lle_text_insert_char(state->buffer, test_char);
        lle_display_update_incremental(state);
    }
    
    uint64_t end_time = lle_perf_get_timestamp();
    benchmark->measured_time = (end_time - start_time) / benchmark->iterations;
    benchmark->target_met = (benchmark->measured_time <= benchmark->target_time_microseconds);
    
    lle_perf_cleanup_test_state(state);
    return true;
}

/**
 * @brief Benchmark character deletion performance
 * @param benchmark Benchmark structure to populate
 * @return true on success, false on error
 */
static bool lle_perf_benchmark_char_delete(lle_performance_benchmark_t *benchmark) {
    lle_display_state_t *state = lle_perf_create_test_state();
    if (!state) return false;
    
    benchmark->operation_name = "Character Deletion";
    benchmark->target_time_microseconds = LLE_PERF_TARGET_CHAR_DELETE;
    benchmark->iterations = LLE_PERF_ITERATIONS_CHAR_OPS;
    
    // Pre-populate buffer with text
    const char *test_text = "The quick brown fox jumps over the lazy dog. ";
    for (size_t i = 0; i < strlen(test_text) * 3; i++) {
        lle_text_insert_char(state->buffer, test_text[i % strlen(test_text)]);
    }
    
    uint64_t start_time = lle_perf_get_timestamp();
    
    // Perform character deletions
    for (size_t i = 0; i < benchmark->iterations && state->buffer->length > 0; i++) {
        lle_text_delete_char(state->buffer);
        lle_display_update_incremental(state);
    }
    
    uint64_t end_time = lle_perf_get_timestamp();
    benchmark->measured_time = (end_time - start_time) / benchmark->iterations;
    benchmark->target_met = (benchmark->measured_time <= benchmark->target_time_microseconds);
    
    lle_perf_cleanup_test_state(state);
    return true;
}

/**
 * @brief Benchmark cursor movement performance
 * @param benchmark Benchmark structure to populate
 * @return true on success, false on error
 */
static bool lle_perf_benchmark_cursor_movement(lle_performance_benchmark_t *benchmark) {
    lle_display_state_t *state = lle_perf_create_test_state();
    if (!state) return false;
    
    benchmark->operation_name = "Cursor Movement";
    benchmark->target_time_microseconds = LLE_PERF_TARGET_CURSOR_MOVE;
    benchmark->iterations = LLE_PERF_ITERATIONS_CURSOR_OPS;
    
    // Pre-populate buffer with text
    const char *test_text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    for (size_t i = 0; i < strlen(test_text); i++) {
        lle_text_insert_char(state->buffer, test_text[i]);
    }
    
    uint64_t start_time = lle_perf_get_timestamp();
    
    // Perform cursor movements
    for (size_t i = 0; i < benchmark->iterations; i++) {
        lle_cursor_movement_t movements[] = {
            LLE_MOVE_HOME, LLE_MOVE_END, LLE_MOVE_LEFT, LLE_MOVE_RIGHT
        };
        lle_cursor_movement_t movement = movements[i % 4];
        
        lle_text_move_cursor(state->buffer, movement);
        lle_display_update_cursor(state);
    }
    
    uint64_t end_time = lle_perf_get_timestamp();
    benchmark->measured_time = (end_time - start_time) / benchmark->iterations;
    benchmark->target_met = (benchmark->measured_time <= benchmark->target_time_microseconds);
    
    lle_perf_cleanup_test_state(state);
    return true;
}

/**
 * @brief Benchmark display cache performance
 * @param benchmark Benchmark structure to populate
 * @return true on success, false on error
 */
static bool lle_perf_benchmark_cache_performance(lle_performance_benchmark_t *benchmark) {
    lle_display_state_t *state = lle_perf_create_test_state();
    if (!state) return false;
    
    benchmark->operation_name = "Cache Access";
    benchmark->target_time_microseconds = LLE_PERF_TARGET_CACHE_ACCESS;
    benchmark->iterations = LLE_PERF_ITERATIONS_CACHE_OPS;
    
    // Pre-populate cache
    const char *test_content = "cached display content";
    lle_display_cache_update(state, test_content, strlen(test_content));
    
    uint64_t start_time = lle_perf_get_timestamp();
    
    // Perform cache validity checks
    for (size_t i = 0; i < benchmark->iterations; i++) {
        lle_display_cache_is_valid(state);
    }
    
    uint64_t end_time = lle_perf_get_timestamp();
    benchmark->measured_time = (end_time - start_time) / benchmark->iterations;
    benchmark->target_met = (benchmark->measured_time <= benchmark->target_time_microseconds);
    
    lle_perf_cleanup_test_state(state);
    return true;
}

/**
 * @brief Benchmark terminal batching performance
 * @param benchmark Benchmark structure to populate
 * @return true on success, false on error
 */
static bool lle_perf_benchmark_batch_performance(lle_performance_benchmark_t *benchmark) {
    lle_display_state_t *state = lle_perf_create_test_state();
    if (!state) return false;
    
    benchmark->operation_name = "Batch Operations";
    benchmark->target_time_microseconds = LLE_PERF_TARGET_BATCH_FLUSH;
    benchmark->iterations = LLE_PERF_ITERATIONS_DISPLAY_OPS;
    
    uint64_t start_time = lle_perf_get_timestamp();
    
    // Perform batch operations
    for (size_t i = 0; i < benchmark->iterations; i++) {
        lle_terminal_batch_start(&state->terminal_batch);
        
        // Add multiple operations to batch
        for (int j = 0; j < 10; j++) {
            char test_data[2] = {(char)('a' + j), '\0'};
            lle_terminal_batch_add(&state->terminal_batch, test_data, 1);
        }
        
        lle_terminal_batch_flush(state);
    }
    
    uint64_t end_time = lle_perf_get_timestamp();
    benchmark->measured_time = (end_time - start_time) / benchmark->iterations;
    benchmark->target_met = (benchmark->measured_time <= benchmark->target_time_microseconds);
    
    lle_perf_cleanup_test_state(state);
    return true;
}

/**
 * @brief Run comprehensive performance validation suite
 * @param validation Validation results structure to populate
 * @return true on success, false on error
 */
bool lle_display_performance_validate(lle_performance_validation_t *validation) {
    if (!validation) return false;
    
    memset(validation, 0, sizeof(lle_performance_validation_t));
    
    // Define benchmark suite
    lle_performance_benchmark_t benchmarks[6];
    bool (*benchmark_functions[])(lle_performance_benchmark_t *) = {
        lle_perf_benchmark_char_insert,
        lle_perf_benchmark_char_delete,
        lle_perf_benchmark_cursor_movement,
        lle_perf_benchmark_cache_performance,
        lle_perf_benchmark_batch_performance
    };
    
    size_t num_benchmarks = sizeof(benchmark_functions) / sizeof(benchmark_functions[0]);
    validation->total_benchmarks = num_benchmarks;
    
    uint64_t validation_start = lle_perf_get_timestamp();
    
    // Run all benchmarks
    for (size_t i = 0; i < num_benchmarks; i++) {
        if (benchmark_functions[i](&benchmarks[i])) {
            if (benchmarks[i].target_met) {
                validation->targets_met++;
            }
        }
    }
    
    uint64_t validation_end = lle_perf_get_timestamp();
    validation->total_time_spent = validation_end - validation_start;
    
    // Calculate overall performance ratio
    double total_ratio = 0.0;
    for (size_t i = 0; i < num_benchmarks; i++) {
        if (benchmarks[i].target_time_microseconds > 0) {
            double ratio = (double)benchmarks[i].target_time_microseconds / 
                          (double)benchmarks[i].measured_time;
            total_ratio += ratio;
        }
    }
    validation->overall_performance_ratio = total_ratio / num_benchmarks;
    
    // Generate summary report
    snprintf(validation->summary_report, sizeof(validation->summary_report),
        "Phase 2C Performance Validation Results:\n"
        "=========================================\n"
        "Total benchmarks: %zu\n"
        "Targets met: %zu/%zu (%.1f%%)\n"
        "Overall performance ratio: %.2fx target\n"
        "Total validation time: %lu µs\n\n"
        "Individual Results:\n"
        "- %s: %lu µs (target: %lu µs) %s\n"
        "- %s: %lu µs (target: %lu µs) %s\n"
        "- %s: %lu µs (target: %lu µs) %s\n"
        "- %s: %lu µs (target: %lu µs) %s\n"
        "- %s: %lu µs (target: %lu µs) %s\n",
        validation->total_benchmarks,
        validation->targets_met, validation->total_benchmarks,
        (double)validation->targets_met / validation->total_benchmarks * 100.0,
        validation->overall_performance_ratio,
        validation->total_time_spent,
        benchmarks[0].operation_name, benchmarks[0].measured_time, 
        benchmarks[0].target_time_microseconds, benchmarks[0].target_met ? "✅" : "❌",
        benchmarks[1].operation_name, benchmarks[1].measured_time, 
        benchmarks[1].target_time_microseconds, benchmarks[1].target_met ? "✅" : "❌",
        benchmarks[2].operation_name, benchmarks[2].measured_time, 
        benchmarks[2].target_time_microseconds, benchmarks[2].target_met ? "✅" : "❌",
        benchmarks[3].operation_name, benchmarks[3].measured_time, 
        benchmarks[3].target_time_microseconds, benchmarks[3].target_met ? "✅" : "❌",
        benchmarks[4].operation_name, benchmarks[4].measured_time, 
        benchmarks[4].target_time_microseconds, benchmarks[4].target_met ? "✅" : "❌"
    );
    
    return true;
}

/**
 * @brief Print performance validation results to stdout
 * @param validation Validation results to print
 */
void lle_display_performance_print_results(const lle_performance_validation_t *validation) {
    if (!validation) return;
    
    printf("%s", validation->summary_report);
    
    if (validation->targets_met == validation->total_benchmarks) {
        printf("\n🎉 All performance targets met! Phase 2C optimization successful.\n");
    } else {
        printf("\n⚠️  %zu/%zu targets missed. Consider further optimization.\n",
               validation->total_benchmarks - validation->targets_met,
               validation->total_benchmarks);
    }
    
    printf("\nPerformance Summary:\n");
    printf("- Average response time: %.2fx faster than target\n", validation->overall_performance_ratio);
    printf("- Validation completed in: %lu µs\n", validation->total_time_spent);
    printf("- Success rate: %.1f%%\n", 
           (double)validation->targets_met / validation->total_benchmarks * 100.0);
}

/**
 * @brief Run a quick performance smoke test
 * @return true if basic performance is acceptable, false otherwise
 */
bool lle_display_performance_smoke_test(void) {
    lle_display_state_t *state = lle_perf_create_test_state();
    if (!state) return false;
    
    // Quick character insertion test
    uint64_t start = lle_perf_get_timestamp();
    lle_text_insert_char(state->buffer, 'x');
    lle_display_update_incremental(state);
    uint64_t elapsed = lle_perf_get_timestamp() - start;
    
    lle_perf_cleanup_test_state(state);
    
    // Accept up to 10ms for smoke test (very generous)
    return elapsed < 10000;
}