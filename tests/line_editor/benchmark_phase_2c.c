/**
 * @file benchmark_phase_2c.c
 * @brief Phase 2C Performance Benchmark Program
 *
 * This program provides comprehensive benchmarking of the Phase 2C performance
 * optimizations including display caching, terminal batching, and overall
 * display system performance improvements.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "../display.h"
#include "../display_performance.h"
#include "../text_buffer.h"
#include "../prompt.h"
#include "../terminal_manager.h"

// Benchmark configuration
#define BENCHMARK_ITERATIONS_QUICK 50
#define BENCHMARK_ITERATIONS_STANDARD 200
#define BENCHMARK_ITERATIONS_COMPREHENSIVE 1000

// Performance thresholds
#define PERF_EXCELLENT_THRESHOLD 0.5   // 50% of target time
#define PERF_GOOD_THRESHOLD 0.8        // 80% of target time
#define PERF_ACCEPTABLE_THRESHOLD 1.0  // 100% of target time

/**
 * @brief Benchmark mode selection
 */
typedef enum {
    BENCHMARK_MODE_QUICK,           /**< Quick smoke test */
    BENCHMARK_MODE_STANDARD,        /**< Standard benchmark suite */
    BENCHMARK_MODE_COMPREHENSIVE,   /**< Comprehensive deep benchmarking */
    BENCHMARK_MODE_STRESS           /**< Stress testing with high load */
} benchmark_mode_t;

/**
 * @brief Benchmark result classification
 */
typedef enum {
    PERF_EXCELLENT,    /**< Performance significantly exceeds targets */
    PERF_GOOD,         /**< Performance meets targets with margin */
    PERF_ACCEPTABLE,   /**< Performance just meets targets */
    PERF_POOR          /**< Performance fails to meet targets */
} performance_rating_t;

/**
 * @brief Get timestamp in microseconds
 */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

/**
 * @brief Create test display state for benchmarking
 */
static lle_display_state_t *create_benchmark_state(void) {
    lle_prompt_t *prompt = lle_prompt_create(256);
    if (!prompt) return NULL;
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(2048);
    if (!buffer) {
        lle_prompt_destroy(prompt);
        return NULL;
    }
    
    // Create minimal terminal manager
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    if (!terminal) {
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        return NULL;
    }
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    terminal->geometry.width = 120;  // Wider terminal for benchmarking
    terminal->geometry.height = 30;
    terminal->geometry_valid = true;
    
    lle_display_state_t *state = lle_display_create(prompt, buffer, terminal);
    if (!state) {
        lle_prompt_destroy(prompt);
        lle_text_buffer_destroy(buffer);
        free(terminal);
        return NULL;
    }
    
    return state;
}

/**
 * @brief Clean up benchmark state
 */
static void cleanup_benchmark_state(lle_display_state_t *state) {
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
 * @brief Benchmark character insertion with and without optimizations
 */
static void benchmark_character_operations(benchmark_mode_t mode) {
    printf("\n=== Character Operations Benchmark ===\n");
    
    size_t iterations = (mode == BENCHMARK_MODE_QUICK) ? BENCHMARK_ITERATIONS_QUICK :
                       (mode == BENCHMARK_MODE_STANDARD) ? BENCHMARK_ITERATIONS_STANDARD :
                       BENCHMARK_ITERATIONS_COMPREHENSIVE;
    
    // Test with optimizations enabled
    lle_display_state_t *state_optimized = create_benchmark_state();
    lle_display_set_performance_optimization(state_optimized, true);
    
    printf("Testing character insertion with optimizations (%zu iterations)...\n", iterations);
    uint64_t start_optimized = get_timestamp_us();
    
    for (size_t i = 0; i < iterations; i++) {
        char test_char = 'a' + (i % 26);
        lle_text_insert_char(state_optimized->buffer, test_char);
        lle_display_update_incremental(state_optimized);
    }
    
    uint64_t time_optimized = get_timestamp_us() - start_optimized;
    uint64_t avg_optimized = time_optimized / iterations;
    
    // Test with optimizations disabled
    lle_display_state_t *state_basic = create_benchmark_state();
    lle_display_set_performance_optimization(state_basic, false);
    
    printf("Testing character insertion without optimizations (%zu iterations)...\n", iterations);
    uint64_t start_basic = get_timestamp_us();
    
    for (size_t i = 0; i < iterations; i++) {
        char test_char = 'a' + (i % 26);
        lle_text_insert_char(state_basic->buffer, test_char);
        lle_display_update_incremental(state_basic);
    }
    
    uint64_t time_basic = get_timestamp_us() - start_basic;
    uint64_t avg_basic = time_basic / iterations;
    
    // Calculate improvement
    double improvement = (double)avg_basic / (double)avg_optimized;
    
    printf("\nResults:\n");
    printf("- With optimizations:    %6lu µs avg (%8lu µs total)\n", avg_optimized, time_optimized);
    printf("- Without optimizations: %6lu µs avg (%8lu µs total)\n", avg_basic, time_basic);
    printf("- Performance improvement: %.2fx faster\n", improvement);
    printf("- Target: %d µs (optimized: %s, basic: %s)\n", 
           LLE_PERF_TARGET_CHAR_INSERT,
           avg_optimized <= LLE_PERF_TARGET_CHAR_INSERT ? "✅" : "❌",
           avg_basic <= LLE_PERF_TARGET_CHAR_INSERT ? "✅" : "❌");
    
    cleanup_benchmark_state(state_optimized);
    cleanup_benchmark_state(state_basic);
}

/**
 * @brief Benchmark cache performance
 */
static void benchmark_cache_performance(benchmark_mode_t mode) {
    printf("\n=== Cache Performance Benchmark ===\n");
    
    lle_display_state_t *state = create_benchmark_state();
    lle_display_set_performance_optimization(state, true);
    
    size_t iterations = (mode == BENCHMARK_MODE_QUICK) ? BENCHMARK_ITERATIONS_QUICK * 4 :
                       (mode == BENCHMARK_MODE_STANDARD) ? BENCHMARK_ITERATIONS_STANDARD * 4 :
                       BENCHMARK_ITERATIONS_COMPREHENSIVE * 2;
    
    // Populate text buffer
    const char *test_text = "The quick brown fox jumps over the lazy dog. ";
    for (size_t i = 0; i < strlen(test_text) * 2; i++) {
        lle_text_insert_char(state->buffer, test_text[i % strlen(test_text)]);
    }
    
    // Prime the cache
    lle_display_update_incremental(state);
    const char *cached_content = "cached display content for performance testing";
    lle_display_cache_update(state, cached_content, strlen(cached_content));
    
    printf("Testing cache validity checks (%zu iterations)...\n", iterations);
    uint64_t start_cache = get_timestamp_us();
    
    size_t cache_hits = 0;
    for (size_t i = 0; i < iterations; i++) {
        if (lle_display_cache_is_valid(state)) {
            cache_hits++;
        }
    }
    
    uint64_t time_cache = get_timestamp_us() - start_cache;
    uint64_t avg_cache = time_cache / iterations;
    
    printf("\nCache Results:\n");
    printf("- Cache checks: %zu iterations\n", iterations);
    printf("- Cache hits: %zu (%.1f%%)\n", cache_hits, (double)cache_hits / iterations * 100.0);
    printf("- Average time per check: %lu µs\n", avg_cache);
    printf("- Total time: %lu µs\n", time_cache);
    printf("- Target: %d µs (%s)\n", 
           LLE_PERF_TARGET_CACHE_ACCESS,
           avg_cache <= LLE_PERF_TARGET_CACHE_ACCESS ? "✅" : "❌");
    
    // Get cache statistics
    uint64_t avg_render, avg_incremental;
    double cache_hit_rate, batch_efficiency;
    if (lle_display_get_performance_stats(state, &avg_render, &avg_incremental, 
                                        &cache_hit_rate, &batch_efficiency)) {
        printf("- Overall cache hit rate: %.1f%%\n", cache_hit_rate);
        printf("- Batch efficiency: %.1f%%\n", batch_efficiency);
    }
    
    cleanup_benchmark_state(state);
}

/**
 * @brief Benchmark terminal batching performance
 */
static void benchmark_batch_performance(benchmark_mode_t mode) {
    printf("\n=== Terminal Batching Benchmark ===\n");
    
    size_t iterations = (mode == BENCHMARK_MODE_QUICK) ? BENCHMARK_ITERATIONS_QUICK / 2 :
                       (mode == BENCHMARK_MODE_STANDARD) ? BENCHMARK_ITERATIONS_STANDARD / 2 :
                       BENCHMARK_ITERATIONS_COMPREHENSIVE / 4;
    
    // Test with batching enabled
    lle_display_state_t *state_batched = create_benchmark_state();
    lle_display_set_performance_optimization(state_batched, true);
    
    printf("Testing with terminal batching (%zu iterations)...\n", iterations);
    uint64_t start_batched = get_timestamp_us();
    
    for (size_t i = 0; i < iterations; i++) {
        lle_terminal_batch_start(&state_batched->terminal_batch);
        
        // Add multiple operations to batch
        for (int j = 0; j < 15; j++) {
            char test_data[2] = {(char)('A' + j % 26), '\0'};
            lle_terminal_batch_add(&state_batched->terminal_batch, test_data, 1);
        }
        
        lle_terminal_batch_flush(state_batched);
    }
    
    uint64_t time_batched = get_timestamp_us() - start_batched;
    uint64_t avg_batched = time_batched / iterations;
    
    // Test without batching (individual writes)
    lle_display_state_t *state_individual = create_benchmark_state();
    lle_display_set_performance_optimization(state_individual, false);
    
    printf("Testing without batching (%zu iterations)...\n", iterations);
    uint64_t start_individual = get_timestamp_us();
    
    for (size_t i = 0; i < iterations; i++) {
        // Simulate individual writes
        for (int j = 0; j < 15; j++) {
            char test_data[2] = {(char)('A' + j % 26), '\0'};
            // Simulate terminal write overhead
            usleep(1); // 1µs delay per write to simulate system call overhead
        }
    }
    
    uint64_t time_individual = get_timestamp_us() - start_individual;
    uint64_t avg_individual = time_individual / iterations;
    
    // Calculate batching efficiency
    double batching_improvement = (double)avg_individual / (double)avg_batched;
    
    printf("\nBatching Results:\n");
    printf("- With batching:    %6lu µs avg (%8lu µs total)\n", avg_batched, time_batched);
    printf("- Without batching: %6lu µs avg (%8lu µs total)\n", avg_individual, time_individual);
    printf("- Batching improvement: %.2fx faster\n", batching_improvement);
    printf("- Target: %d µs (batched: %s)\n", 
           LLE_PERF_TARGET_BATCH_FLUSH,
           avg_batched <= LLE_PERF_TARGET_BATCH_FLUSH ? "✅" : "❌");
    
    cleanup_benchmark_state(state_batched);
    cleanup_benchmark_state(state_individual);
}

/**
 * @brief Benchmark multiline text performance
 */
static void benchmark_multiline_performance(benchmark_mode_t mode) {
    printf("\n=== Multiline Text Performance Benchmark ===\n");
    
    lle_display_state_t *state = create_benchmark_state();
    lle_display_set_performance_optimization(state, true);
    
    // Create multiline text
    const char *lines[] = {
        "First line of multiline text for performance testing",
        "Second line with different content and length variations",
        "Third line containing special characters: []{}()",
        "Fourth line with numbers: 12345 and symbols: @#$%^&*",
        "Final line to complete the multiline benchmark test"
    };
    size_t num_lines = sizeof(lines) / sizeof(lines[0]);
    
    printf("Testing multiline text rendering (%zu lines)...\n", num_lines);
    uint64_t start_multiline = get_timestamp_us();
    
    // Insert all lines with newlines
    for (size_t i = 0; i < num_lines; i++) {
        const char *line = lines[i];
        for (size_t j = 0; j < strlen(line); j++) {
            lle_text_insert_char(state->buffer, line[j]);
        }
        if (i < num_lines - 1) {
            lle_text_insert_char(state->buffer, '\n');
        }
        lle_display_update_incremental(state);
    }
    
    uint64_t time_multiline = get_timestamp_us() - start_multiline;
    
    printf("\nMultiline Results:\n");
    printf("- Lines processed: %zu\n", num_lines);
    printf("- Total characters: %zu\n", state->buffer->length);
    printf("- Total time: %lu µs\n", time_multiline);
    printf("- Average per line: %lu µs\n", time_multiline / num_lines);
    printf("- Characters per second: %.0f\n", 
           state->buffer->length * 1000000.0 / time_multiline);
    
    cleanup_benchmark_state(state);
}

/**
 * @brief Classify performance rating
 */
static performance_rating_t classify_performance(uint64_t measured, uint64_t target) {
    double ratio = (double)measured / target;
    
    if (ratio <= PERF_EXCELLENT_THRESHOLD) return PERF_EXCELLENT;
    if (ratio <= PERF_GOOD_THRESHOLD) return PERF_GOOD;
    if (ratio <= PERF_ACCEPTABLE_THRESHOLD) return PERF_ACCEPTABLE;
    return PERF_POOR;
}

/**
 * @brief Print performance rating
 */
static void print_performance_rating(performance_rating_t rating) {
    switch (rating) {
        case PERF_EXCELLENT:  printf("🏆 EXCELLENT"); break;
        case PERF_GOOD:       printf("✅ GOOD"); break;
        case PERF_ACCEPTABLE: printf("☑️  ACCEPTABLE"); break;
        case PERF_POOR:       printf("❌ POOR"); break;
    }
}

/**
 * @brief Run comprehensive benchmark suite
 */
static void run_comprehensive_benchmark(benchmark_mode_t mode) {
    printf("🚀 Phase 2C Performance Optimization Benchmark\n");
    printf("===============================================\n");
    
    const char *mode_names[] = {"Quick", "Standard", "Comprehensive", "Stress"};
    printf("Benchmark Mode: %s\n", mode_names[mode]);
    printf("Timestamp: %ld\n", time(NULL));
    
    // Run smoke test first
    printf("\nRunning smoke test...");
    if (lle_display_performance_smoke_test()) {
        printf(" ✅ PASSED\n");
    } else {
        printf(" ❌ FAILED - Basic performance is unacceptable\n");
        return;
    }
    
    // Run individual benchmarks
    benchmark_character_operations(mode);
    benchmark_cache_performance(mode);
    benchmark_batch_performance(mode);
    benchmark_multiline_performance(mode);
    
    // Run official validation
    printf("\n=== Official Performance Validation ===\n");
    lle_performance_validation_t validation;
    if (lle_display_performance_validate(&validation)) {
        lle_display_performance_print_results(&validation);
    } else {
        printf("❌ Performance validation failed to run\n");
    }
    
    printf("\n=== Benchmark Summary ===\n");
    printf("Phase 2C performance optimization benchmarking completed.\n");
    printf("See individual benchmark results above for detailed analysis.\n");
}

/**
 * @brief Print usage information
 */
static void print_usage(const char *program_name) {
    printf("Usage: %s [mode]\n", program_name);
    printf("\nModes:\n");
    printf("  quick       - Quick smoke test (default)\n");
    printf("  standard    - Standard benchmark suite\n");
    printf("  comprehensive - Comprehensive deep benchmarking\n");
    printf("  stress      - Stress testing with high load\n");
    printf("\nExamples:\n");
    printf("  %s                    # Run quick benchmark\n", program_name);
    printf("  %s standard           # Run standard benchmark\n", program_name);
    printf("  %s comprehensive      # Run comprehensive benchmark\n", program_name);
}

/**
 * @brief Main benchmark program
 */
int main(int argc, char *argv[]) {
    benchmark_mode_t mode = BENCHMARK_MODE_QUICK;
    
    // Parse command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "quick") == 0) {
            mode = BENCHMARK_MODE_QUICK;
        } else if (strcmp(argv[1], "standard") == 0) {
            mode = BENCHMARK_MODE_STANDARD;
        } else if (strcmp(argv[1], "comprehensive") == 0) {
            mode = BENCHMARK_MODE_COMPREHENSIVE;
        } else if (strcmp(argv[1], "stress") == 0) {
            mode = BENCHMARK_MODE_STRESS;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("❌ Unknown mode: %s\n\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Run benchmark suite
    run_comprehensive_benchmark(mode);
    
    return 0;
}