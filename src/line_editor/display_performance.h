#ifndef LLE_DISPLAY_PERFORMANCE_H
#define LLE_DISPLAY_PERFORMANCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file display_performance.h
 * @brief Phase 2C Display Performance Benchmark and Validation Utilities
 *
 * This module provides comprehensive performance benchmarking and validation
 * utilities for the display system, enabling measurement and optimization
 * of display operations to meet performance targets.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief Performance validation results structure
 *
 * Contains comprehensive results from performance validation including
 * benchmark results, timing information, and success metrics.
 */
typedef struct {
    size_t total_benchmarks;            /**< Total number of benchmarks run */
    size_t targets_met;                 /**< Number of targets that were met */
    double overall_performance_ratio;   /**< Overall performance as ratio of target */
    uint64_t total_time_spent;          /**< Total time spent in benchmarking */
    char summary_report[1024];          /**< Summary report text */
} lle_performance_validation_t;

/**
 * @brief Run comprehensive performance validation suite
 *
 * Executes a complete suite of performance benchmarks to validate that
 * the display system meets all performance targets. Measures character
 * operations, cursor movement, cache performance, and batch operations.
 *
 * @param validation Validation results structure to populate
 * @return true on success, false on error
 *
 * @note This function may take several seconds to complete as it runs
 *       multiple iterations of each benchmark for accurate measurements.
 */
bool lle_display_performance_validate(lle_performance_validation_t *validation);

/**
 * @brief Print performance validation results to stdout
 *
 * Prints a formatted report of performance validation results including
 * individual benchmark results, overall success rate, and summary.
 *
 * @param validation Validation results to print
 *
 * @note Output includes colored indicators (✅/❌) for target achievement
 *       and formatted timing information for easy interpretation.
 */
void lle_display_performance_print_results(const lle_performance_validation_t *validation);

/**
 * @brief Run a quick performance smoke test
 *
 * Performs a minimal performance test to verify basic display functionality
 * is working at acceptable speeds. Much faster than full validation.
 *
 * @return true if basic performance is acceptable, false otherwise
 *
 * @note This test uses very generous thresholds (10ms) and is intended
 *       for quick verification rather than detailed performance analysis.
 */
bool lle_display_performance_smoke_test(void);

// Performance target constants (in microseconds)
#define LLE_PERF_TARGET_CHAR_INSERT 1000     /**< 1ms for character insertion */
#define LLE_PERF_TARGET_CHAR_DELETE 1000     /**< 1ms for character deletion */
#define LLE_PERF_TARGET_CURSOR_MOVE 1000     /**< 1ms for cursor movement */
#define LLE_PERF_TARGET_WORD_MOVE 2000       /**< 2ms for word movement */
#define LLE_PERF_TARGET_LINE_CLEAR 3000      /**< 3ms for line clearing */
#define LLE_PERF_TARGET_DISPLAY_UPDATE 5000  /**< 5ms for display update */
#define LLE_PERF_TARGET_CACHE_ACCESS 100     /**< 100µs for cache access */
#define LLE_PERF_TARGET_BATCH_FLUSH 2000     /**< 2ms for batch flush */

// Benchmark iteration counts for consistent measurements
#define LLE_PERF_ITERATIONS_CHAR_OPS 100     /**< Character operations iterations */
#define LLE_PERF_ITERATIONS_CURSOR_OPS 50    /**< Cursor operations iterations */
#define LLE_PERF_ITERATIONS_DISPLAY_OPS 25   /**< Display operations iterations */
#define LLE_PERF_ITERATIONS_CACHE_OPS 200    /**< Cache operations iterations */

#endif // LLE_DISPLAY_PERFORMANCE_H