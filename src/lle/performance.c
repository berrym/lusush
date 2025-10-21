/**
 * @file performance.c
 * @brief LLE Performance Optimization System - Phase 1 Core Implementation
 * 
 * Specification: Spec 14 - Performance Optimization Complete Specification
 * Version: 1.0.0 - Phase 1
 * Phase: Core Performance Monitoring Only
 * 
 * CRITICAL MANDATE: Complete implementations only.
 * Every function in this file has a COMPLETE implementation with real logic.
 * 
 * Phase 1 Scope:
 * 1. Performance monitor initialization and cleanup
 * 2. Measurement start/end operations
 * 3. Statistics calculation and aggregation
 * 4. History recording
 * 5. Threshold handling (warning/critical)
 * 6. Utility functions
 * 
 * Functions deliberately EXCLUDED from Phase 1 (deferred to future phases):
 * - Dashboard and reporting (Phase 2)
 * - Testing and validation (Phase 3)
 * - Integration with other subsystems (Phase 4)
 */

#define _POSIX_C_SOURCE 200809L
#include "lle/performance.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Get current thread ID
 */
static uint32_t lle_get_thread_id(void) {
    return (uint32_t)pthread_self();
}

/**
 * @brief Initialize statistics structure to zero
 */
static void lle_perf_stats_init(lle_perf_statistics_t *stats) {
    memset(stats, 0, sizeof(lle_perf_statistics_t));
    stats->min_duration_ns = UINT64_MAX;
}

/**
 * @brief Update running statistics with new measurement
 */
static void lle_perf_stats_update(lle_perf_statistics_t *stats, uint64_t duration_ns, bool success) {
    stats->call_count++;
    
    if (success) {
        stats->completed_count++;
        stats->consecutive_successes++;
        if (stats->consecutive_successes > stats->max_consecutive_successes) {
            stats->max_consecutive_successes = stats->consecutive_successes;
        }
    } else {
        stats->error_count++;
        stats->consecutive_successes = 0;
    }
    
    stats->total_duration_ns += duration_ns;
    
    if (duration_ns < stats->min_duration_ns) {
        stats->min_duration_ns = duration_ns;
    }
    
    if (duration_ns > stats->max_duration_ns) {
        stats->max_duration_ns = duration_ns;
    }
    
    /* Update mean */
    if (stats->completed_count > 0) {
        stats->mean_duration_ns = stats->total_duration_ns / stats->completed_count;
    }
    
    /* Calculate reliability score */
    if (stats->call_count > 0) {
        stats->reliability_score = (double)stats->completed_count / (double)stats->call_count;
    }
}

/* ============================================================================
 * PHASE 1 CORE FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Initialize performance monitor
 */
lle_result_t lle_perf_monitor_init(
    lle_performance_monitor_t *monitor,
    lle_performance_config_t *config)
{
    if (!monitor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Zero-initialize the entire structure */
    memset(monitor, 0, sizeof(lle_performance_monitor_t));
    
    /* Set configuration from config or defaults */
    if (config) {
        monitor->monitoring_enabled = config->monitoring_enabled;
        monitor->monitoring_level = config->monitoring_level;
        monitor->warning_threshold_ns = config->warning_threshold_ns;
        monitor->critical_threshold_ns = config->critical_threshold_ns;
        
        /* Initialize filters if configured */
        if (config->enable_operation_filtering) {
            monitor->active_filters.filter_by_operation = true;
            monitor->active_filters.operation_mask = config->operation_filter_mask;
        }
    } else {
        /* Use defaults */
        monitor->monitoring_enabled = true;
        monitor->monitoring_level = LLE_PERF_MONITORING_NORMAL;
        monitor->warning_threshold_ns = LLE_PERF_TARGET_RESPONSE_TIME_NS;
        monitor->critical_threshold_ns = LLE_PERF_TARGET_RESPONSE_TIME_NS * 2;
    }
    
    /* Initialize all operation statistics */
    lle_perf_stats_init(&monitor->global_stats);
    for (int i = 0; i < LLE_PERF_OPERATION_COUNT; i++) {
        lle_perf_stats_init(&monitor->operation_stats[i]);
    }
    
    /* Initialize history ring buffer */
    monitor->history_ring.capacity = 100; /* Fixed size for Phase 1 */
    monitor->history_ring.entries = calloc(monitor->history_ring.capacity, sizeof(lle_perf_statistics_t));
    if (!monitor->history_ring.entries) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    monitor->measurement_count = 0;
    monitor->measurement_index = 0;
    monitor->total_operations = 0;
    
    return LLE_SUCCESS;
}

/**
 * @brief Destroy performance monitor and free resources
 */
lle_result_t lle_perf_monitor_destroy(lle_performance_monitor_t *monitor) {
    if (!monitor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free history ring buffer */
    if (monitor->history_ring.entries) {
        free(monitor->history_ring.entries);
        monitor->history_ring.entries = NULL;
    }
    
    /* Zero out the structure */
    memset(monitor, 0, sizeof(lle_performance_monitor_t));
    
    return LLE_SUCCESS;
}

/**
 * @brief Calculate time difference in nanoseconds
 */
uint64_t lle_perf_timespec_diff_ns(const struct timespec *start, const struct timespec *end) {
    uint64_t start_ns = (uint64_t)start->tv_sec * 1000000000ULL + (uint64_t)start->tv_nsec;
    uint64_t end_ns = (uint64_t)end->tv_sec * 1000000000ULL + (uint64_t)end->tv_nsec;
    
    if (end_ns > start_ns) {
        return end_ns - start_ns;
    }
    return 0;
}

/**
 * @brief Check if operation should be monitored based on filters
 */
bool lle_perf_should_monitor_operation(
    const lle_performance_monitor_t *monitor,
    lle_perf_operation_type_t op_type)
{
    if (!monitor || !monitor->monitoring_enabled) {
        return false;
    }
    
    /* Check monitoring level */
    if (monitor->monitoring_level == LLE_PERF_MONITORING_DISABLED) {
        return false;
    }
    
    /* Check operation filter */
    if (monitor->active_filters.filter_by_operation) {
        if (op_type >= LLE_PERF_OPERATION_COUNT) {
            return false;
        }
        
        uint32_t op_bit = 1U << op_type;
        if (!(monitor->active_filters.operation_mask & op_bit)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Check if operation is on critical path
 */
bool lle_perf_is_critical_path(lle_perf_operation_type_t op_type) {
    /* Critical path operations that must be fast */
    switch (op_type) {
        case LLE_PERF_OP_TERMINAL_INPUT:
        case LLE_PERF_OP_TERMINAL_OUTPUT:
        case LLE_PERF_OP_BUFFER_INSERT:
        case LLE_PERF_OP_BUFFER_DELETE:
        case LLE_PERF_OP_EVENT_DISPATCH:
        case LLE_PERF_OP_DISPLAY_UPDATE:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Start a performance measurement
 */
lle_result_t lle_perf_measurement_start(
    lle_performance_monitor_t *monitor,
    lle_perf_operation_type_t op_type,
    const char *op_name,
    lle_performance_context_t *context,
    lle_perf_measurement_id_t *measurement_id)
{
    if (!monitor || !op_name || !measurement_id) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if monitoring is enabled */
    if (!monitor->monitoring_enabled) {
        *measurement_id = (lle_perf_measurement_id_t)-1;
        return LLE_SUCCESS;
    }
    
    /* Check if we should monitor this operation */
    if (!lle_perf_should_monitor_operation(monitor, op_type)) {
        *measurement_id = (lle_perf_measurement_id_t)-1;
        return LLE_SUCCESS;
    }
    
    /* Validate operation type */
    if (op_type >= LLE_PERF_OPERATION_COUNT) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate measurement slot (atomic circular buffer) */
    uint32_t index = __atomic_fetch_add(&monitor->measurement_index, 1, __ATOMIC_RELAXED);
    index %= LLE_PERF_MAX_MEASUREMENTS;
    
    lle_performance_measurement_t *measurement = &monitor->measurements[index];
    
    /* Initialize measurement */
    memset(measurement, 0, sizeof(lle_performance_measurement_t));
    measurement->operation_type = op_type;
    measurement->operation_name = op_name;
    
    if (context) {
        measurement->context = *context;
    }
    
    measurement->thread_id = lle_get_thread_id();
    measurement->is_critical_path = lle_perf_is_critical_path(op_type);
    
    /* High-precision timing start */
    if (clock_gettime(CLOCK_MONOTONIC, &measurement->start_time) != 0) {
        return LLE_ERROR_SYSTEM;
    }
    
    /* Increment total operations counter */
    __atomic_fetch_add(&monitor->total_operations, 1, __ATOMIC_RELAXED);
    
    *measurement_id = index;
    return LLE_SUCCESS;
}

/**
 * @brief End a performance measurement
 */
lle_result_t lle_perf_measurement_end(
    lle_performance_monitor_t *monitor,
    lle_perf_measurement_id_t measurement_id,
    lle_perf_result_t result_code)
{
    if (!monitor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check for disabled monitoring sentinel */
    if (measurement_id == (lle_perf_measurement_id_t)-1) {
        return LLE_SUCCESS;
    }
    
    /* Validate measurement ID */
    if (measurement_id >= LLE_PERF_MAX_MEASUREMENTS) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_performance_measurement_t *measurement = &monitor->measurements[measurement_id];
    
    /* High-precision timing end */
    if (clock_gettime(CLOCK_MONOTONIC, &measurement->end_time) != 0) {
        return LLE_ERROR_SYSTEM;
    }
    
    /* Calculate duration */
    measurement->duration_ns = lle_perf_timespec_diff_ns(&measurement->start_time, &measurement->end_time);
    
    /* Determine success */
    bool success = (result_code == LLE_PERF_RESULT_SUCCESS);
    
    /* Update global statistics */
    lle_perf_stats_update(&monitor->global_stats, measurement->duration_ns, success);
    
    /* Update operation-specific statistics */
    if (measurement->operation_type < LLE_PERF_OPERATION_COUNT) {
        lle_perf_stats_update(&monitor->operation_stats[measurement->operation_type], 
                             measurement->duration_ns, success);
    }
    
    /* Check thresholds */
    if (measurement->duration_ns >= monitor->critical_threshold_ns) {
        lle_perf_handle_critical_threshold_exceeded(monitor, measurement_id);
    } else if (measurement->duration_ns >= monitor->warning_threshold_ns) {
        lle_perf_handle_warning_threshold_exceeded(monitor, measurement_id);
    }
    
    /* Increment measurement count */
    __atomic_fetch_add(&monitor->measurement_count, 1, __ATOMIC_RELAXED);
    
    return LLE_SUCCESS;
}

/**
 * @brief Get statistics for a specific operation type
 */
lle_result_t lle_perf_get_statistics(
    lle_performance_monitor_t *monitor,
    lle_perf_operation_type_t op_type,
    lle_perf_statistics_t *stats)
{
    if (!monitor || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Get global stats if op_type is special value */
    if (op_type == LLE_PERF_OP_COUNT) {
        *stats = monitor->global_stats;
        return LLE_SUCCESS;
    }
    
    /* Validate operation type */
    if (op_type >= LLE_PERF_OPERATION_COUNT) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Copy operation-specific statistics */
    *stats = monitor->operation_stats[op_type];
    
    return LLE_SUCCESS;
}

/**
 * @brief Calculate advanced statistics (percentiles, std dev)
 */
lle_result_t lle_perf_calculate_statistics(
    lle_performance_monitor_t *monitor,
    lle_perf_operation_type_t op_type,
    lle_perf_statistics_t *stats)
{
    if (!monitor || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* First get basic statistics */
    lle_result_t result = lle_perf_get_statistics(monitor, op_type, stats);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Collect duration samples for percentile calculation */
    uint64_t *durations = calloc(LLE_PERF_MAX_MEASUREMENTS, sizeof(uint64_t));
    if (!durations) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    size_t sample_count = 0;
    uint64_t sum_squared_diff = 0;
    
    /* Gather samples from measurements array */
    for (uint32_t i = 0; i < LLE_PERF_MAX_MEASUREMENTS; i++) {
        lle_performance_measurement_t *m = &monitor->measurements[i];
        
        /* Filter by operation type */
        if (op_type != LLE_PERF_OP_COUNT && m->operation_type != op_type) {
            continue;
        }
        
        /* Skip if no duration recorded */
        if (m->duration_ns == 0) {
            continue;
        }
        
        durations[sample_count++] = m->duration_ns;
        
        /* Calculate variance for standard deviation */
        if (stats->mean_duration_ns > 0) {
            int64_t diff = (int64_t)m->duration_ns - (int64_t)stats->mean_duration_ns;
            sum_squared_diff += (uint64_t)(diff * diff);
        }
    }
    
    if (sample_count == 0) {
        free(durations);
        return LLE_SUCCESS; /* No samples, return basic stats */
    }
    
    /* Sort durations for percentile calculation */
    for (size_t i = 0; i < sample_count - 1; i++) {
        for (size_t j = i + 1; j < sample_count; j++) {
            if (durations[i] > durations[j]) {
                uint64_t temp = durations[i];
                durations[i] = durations[j];
                durations[j] = temp;
            }
        }
    }
    
    /* Calculate percentiles */
    stats->median_duration_ns = durations[sample_count / 2];
    stats->p95_duration_ns = durations[(sample_count * 95) / 100];
    stats->p99_duration_ns = durations[(sample_count * 99) / 100];
    
    /* Calculate standard deviation */
    if (sample_count > 1) {
        uint64_t variance = sum_squared_diff / (sample_count - 1);
        /* Simple integer square root */
        uint64_t sqrt_var = 0;
        uint64_t bit = 1ULL << 30;
        while (bit > variance) {
            bit >>= 2;
        }
        while (bit != 0) {
            if (variance >= sqrt_var + bit) {
                variance -= sqrt_var + bit;
                sqrt_var = (sqrt_var >> 1) + bit;
            } else {
                sqrt_var >>= 1;
            }
            bit >>= 2;
        }
        stats->standard_deviation_ns = sqrt_var;
    }
    
    free(durations);
    return LLE_SUCCESS;
}

/**
 * @brief Record statistics in history ring buffer
 */
lle_result_t lle_perf_history_record(
    lle_performance_monitor_t *monitor,
    const lle_perf_statistics_t *stats)
{
    if (!monitor || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!monitor->history_ring.entries) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Add to ring buffer */
    uint32_t write_index = monitor->history_ring.head;
    monitor->history_ring.entries[write_index] = *stats;
    
    /* Update head */
    monitor->history_ring.head = (write_index + 1) % monitor->history_ring.capacity;
    
    /* Update size and tail */
    if (monitor->history_ring.size < monitor->history_ring.capacity) {
        monitor->history_ring.size++;
    } else {
        /* Ring buffer is full, move tail */
        monitor->history_ring.tail = (monitor->history_ring.tail + 1) % monitor->history_ring.capacity;
    }
    
    monitor->history_ring.total_entries_recorded++;
    
    return LLE_SUCCESS;
}

/**
 * @brief Handle warning threshold exceeded
 */
lle_result_t lle_perf_handle_warning_threshold_exceeded(
    lle_performance_monitor_t *monitor,
    lle_perf_measurement_id_t measurement_id)
{
    if (!monitor || measurement_id >= LLE_PERF_MAX_MEASUREMENTS) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* In Phase 1, we simply track that a warning occurred */
    /* Phase 2 will add logging, alerting, and dashboard updates */
    
    lle_performance_measurement_t *measurement = &monitor->measurements[measurement_id];
    
    /* Could log to stderr for visibility */
    if (monitor->monitoring_level >= LLE_PERF_MONITORING_DETAILED) {
        fprintf(stderr, "[LLE PERF WARNING] Operation '%s' took %lu ns (threshold: %lu ns)\n",
                measurement->operation_name,
                (unsigned long)measurement->duration_ns,
                (unsigned long)monitor->warning_threshold_ns);
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Handle critical threshold exceeded
 */
lle_result_t lle_perf_handle_critical_threshold_exceeded(
    lle_performance_monitor_t *monitor,
    lle_perf_measurement_id_t measurement_id)
{
    if (!monitor || measurement_id >= LLE_PERF_MAX_MEASUREMENTS) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* In Phase 1, we track critical threshold violations */
    /* Phase 2 will add comprehensive logging and alerting */
    
    lle_performance_measurement_t *measurement = &monitor->measurements[measurement_id];
    
    /* Always log critical threshold violations */
    fprintf(stderr, "[LLE PERF CRITICAL] Operation '%s' took %lu ns (threshold: %lu ns)\n",
            measurement->operation_name,
            (unsigned long)measurement->duration_ns,
            (unsigned long)monitor->critical_threshold_ns);
    
    return LLE_SUCCESS;
}
