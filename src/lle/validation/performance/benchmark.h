/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Performance Benchmarking Framework - Week 3 Day 11
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#ifndef LLE_VALIDATION_PERFORMANCE_BENCHMARK_H
#define LLE_VALIDATION_PERFORMANCE_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * PERFORMANCE BENCHMARKING FRAMEWORK
 * 
 * Week 3 Success Criteria:
 * - Input latency (p50): <50μs
 * - Input latency (p99): <100μs
 * - Memory footprint: <1MB incremental
 * - Allocation latency: <100μs per operation
 * - Memory leaks: 0 bytes
 */

// Performance metrics structure
typedef struct {
    uint64_t operation_count;          // Total operations performed
    uint64_t total_time_ns;            // Total time in nanoseconds
    uint64_t min_time_ns;              // Minimum operation time
    uint64_t max_time_ns;              // Maximum operation time
    uint64_t p50_time_ns;              // 50th percentile (median)
    uint64_t p95_time_ns;              // 95th percentile
    uint64_t p99_time_ns;              // 99th percentile
    double avg_time_ns;                // Average time
    double std_dev_ns;                 // Standard deviation
} lle_perf_metrics_t;

// Memory metrics structure
typedef struct {
    size_t initial_rss_bytes;          // RSS before LLE init
    size_t current_rss_bytes;          // Current RSS
    size_t incremental_bytes;          // LLE incremental memory
    size_t allocated_bytes;            // Total allocated
    size_t freed_bytes;                // Total freed
    size_t peak_bytes;                 // Peak memory usage
    uint64_t allocation_count;         // Number of allocations
    uint64_t deallocation_count;       // Number of deallocations
    bool has_leaks;                    // Memory leak detected
    size_t leaked_bytes;               // Total leaked bytes
} lle_memory_metrics_t;

/**
 * Initialize performance benchmarking system.
 * Sets up timing infrastructure and memory tracking.
 * 
 * @return 0 on success, -1 on failure
 */
int lle_perf_init(void);

/**
 * Record a single operation timing.
 * Call this for each operation you want to benchmark.
 * 
 * @param time_ns Operation time in nanoseconds
 * @return 0 on success, -1 on failure
 */
int lle_perf_record_timing(uint64_t time_ns);

/**
 * Calculate performance metrics from recorded timings.
 * Computes percentiles, average, standard deviation, etc.
 * 
 * @param metrics Output structure for calculated metrics
 * @return 0 on success, -1 on failure
 */
int lle_perf_calculate_metrics(lle_perf_metrics_t *metrics);

/**
 * Get current memory usage metrics.
 * Measures RSS, allocations, and detects leaks.
 * 
 * @param metrics Output structure for memory metrics
 * @return 0 on success, -1 on failure
 */
int lle_perf_get_memory_metrics(lle_memory_metrics_t *metrics);

/**
 * Reset performance tracking.
 * Clears all recorded timings and resets counters.
 */
void lle_perf_reset(void);

/**
 * Cleanup performance benchmarking system.
 * Releases all resources.
 */
void lle_perf_cleanup(void);

/**
 * Print performance report.
 * Outputs formatted performance metrics to stdout.
 * 
 * @param metrics Performance metrics to display
 * @param detailed Whether to show detailed statistics
 */
void lle_perf_print_report(const lle_perf_metrics_t *metrics, bool detailed);

/**
 * Print memory report.
 * Outputs formatted memory metrics to stdout.
 * 
 * @param metrics Memory metrics to display
 */
void lle_perf_print_memory_report(const lle_memory_metrics_t *metrics);

/**
 * Validate against Week 3 success criteria.
 * Checks all metrics against targets and returns result.
 * 
 * @param perf_metrics Performance metrics
 * @param mem_metrics Memory metrics
 * @param passed_count Output: number of criteria passed
 * @param total_count Output: total number of criteria
 * @return true if all criteria met, false otherwise
 */
bool lle_perf_validate_week3_criteria(const lle_perf_metrics_t *perf_metrics,
                                       const lle_memory_metrics_t *mem_metrics,
                                       int *passed_count,
                                       int *total_count);

#endif // LLE_VALIDATION_PERFORMANCE_BENCHMARK_H
