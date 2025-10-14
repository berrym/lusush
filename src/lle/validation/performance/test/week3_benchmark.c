/*
 * LLE Week 3 Performance & Memory Benchmark - Day 11
 * Comprehensive validation of Week 3 success criteria
 */

#define _POSIX_C_SOURCE 199309L

#include "../benchmark.h"
#include "../../display/client.h"
#include "../../display/scenarios.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Benchmark: Display client operations
static int benchmark_display_operations(void) {
    printf("\n=== Benchmarking Display Client Operations ===\n\n");
    
    lle_display_client_t client;
    if (lle_display_client_init(&client, 24, 80) != 0) {
        fprintf(stderr, "Failed to initialize display client\n");
        return -1;
    }
    
    const int iterations = 100000;
    printf("Running %d display operations...\n", iterations);
    
    struct timespec start, end;
    
    // Benchmark simple rendering
    for (int i = 0; i < iterations; i++) {
        const char *line = "echo hello world";
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_display_client_render(&client, line);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        uint64_t time_ns = (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
                           (uint64_t)(end.tv_nsec - start.tv_nsec);
        
        lle_perf_record_timing(time_ns);
    }
    
    lle_display_client_cleanup(&client);
    
    printf("Completed %d iterations\n", iterations);
    return 0;
}

// Benchmark: All 7 rendering scenarios
static int benchmark_all_scenarios(void) {
    printf("\n=== Benchmarking All 7 Rendering Scenarios ===\n\n");
    
    lle_display_client_t client;
    lle_display_client_init(&client, 24, 80);
    
    const int iterations_per_scenario = 10000;
    printf("Running %d iterations per scenario (70,000 total)...\n", iterations_per_scenario);
    
    struct timespec start, end;
    
    for (int i = 0; i < iterations_per_scenario; i++) {
        // Scenario 1: Single-line
        scenario_single_line_t s1 = { .input = "test", .cursor_pos = 4 };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_single_line(&client, &s1);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
        
        // Scenario 2: Multi-line
        char long_line[200];
        memset(long_line, 'A', 150);
        long_line[150] = '\0';
        scenario_multiline_t s2 = { .input = long_line, .cursor_pos = 75, .expected_rows = 2 };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_multiline(&client, &s2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
        
        // Scenario 3: Prompt
        scenario_prompt_t s3 = { .prompt = "$ ", .input = "ls", .cursor_pos = 2 };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_prompt(&client, &s3);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
        
        // Scenario 4: Highlighting
        const char *input = "echo hello";
        uint8_t attrs[] = {
            LLE_ATTR_BOLD, LLE_ATTR_BOLD, LLE_ATTR_BOLD, LLE_ATTR_BOLD,
            0, 0, 0, 0, 0, 0
        };
        scenario_highlighting_t s4 = { .input = input, .highlight_attrs = attrs,
                                       .attr_count = 10, .cursor_pos = 10 };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_highlighting(&client, &s4);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
        
        // Scenario 5: Completion
        scenario_completion_t s5 = { .input = "ec", .completion = "ho", .cursor_pos = 2 };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_completion(&client, &s5);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
        
        // Scenario 6: Scroll region
        const char *history[] = { "line1", "line2", "line3" };
        scenario_scroll_region_t s6 = { .lines = history, .line_count = 3,
                                        .visible_start = 0, .visible_count = 3, .cursor_line = 2 };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_scroll_region(&client, &s6);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
        
        // Scenario 7: Atomic update
        scenario_atomic_update_t s7 = { .old_content = "test", .new_content = "test2", .should_diff = true };
        clock_gettime(CLOCK_MONOTONIC, &start);
        lle_scenario_render_atomic_update(&client, &s7);
        clock_gettime(CLOCK_MONOTONIC, &end);
        lle_perf_record_timing(
            (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
            (uint64_t)(end.tv_nsec - start.tv_nsec));
    }
    
    lle_display_client_cleanup(&client);
    
    printf("Completed all scenario iterations\n");
    return 0;
}

int main(void) {
    printf("===================================================\n");
    printf("  LLE Week 3 Performance & Memory Benchmark\n");
    printf("===================================================\n");
    
    // Initialize performance tracking
    if (lle_perf_init() != 0) {
        fprintf(stderr, "Failed to initialize performance tracking\n");
        return 1;
    }
    
    // Run benchmarks
    printf("\nPhase 1: Display Client Operations\n");
    if (benchmark_display_operations() != 0) {
        lle_perf_cleanup();
        return 1;
    }
    
    printf("\nPhase 2: All Rendering Scenarios\n");
    if (benchmark_all_scenarios() != 0) {
        lle_perf_cleanup();
        return 1;
    }
    
    // Calculate metrics
    lle_perf_metrics_t perf_metrics;
    if (lle_perf_calculate_metrics(&perf_metrics) != 0) {
        fprintf(stderr, "Failed to calculate performance metrics\n");
        lle_perf_cleanup();
        return 1;
    }
    
    lle_memory_metrics_t mem_metrics;
    if (lle_perf_get_memory_metrics(&mem_metrics) != 0) {
        fprintf(stderr, "Failed to get memory metrics\n");
        lle_perf_cleanup();
        return 1;
    }
    
    // Print reports
    lle_perf_print_report(&perf_metrics, true);
    lle_perf_print_memory_report(&mem_metrics);
    
    // Validate against Week 3 criteria
    int passed, total;
    bool all_passed = lle_perf_validate_week3_criteria(&perf_metrics, &mem_metrics,
                                                        &passed, &total);
    
    printf("\n===================================================\n");
    printf("                 Week 3 Results\n");
    printf("===================================================\n\n");
    
    if (all_passed) {
        printf("[PASS] Week 3 Performance & Memory: VALIDATED\n\n");
        printf("Week 3 Assessment: PROCEED to Week 4\n\n");
        printf("Key Achievements:\n");
        printf("- Input latency targets exceeded\n");
        printf("- Memory footprint within limits\n");
        printf("- Zero memory leaks detected\n");
        printf("- Performance validates architectural soundness\n");
        printf("\nReady for Week 4: Event-Driven Architecture\n");
        
        lle_perf_cleanup();
        return 0;
    } else {
        printf("[FAIL] Week 3 validation failed\n");
        printf("Passed: %d/%d criteria\n\n", passed, total);
        
        if (passed >= 3) {
            printf("Assessment: PIVOT (extend Week 3, optimize)\n");
        } else {
            printf("Assessment: Critical issues detected\n");
        }
        
        lle_perf_cleanup();
        return 1;
    }
}
