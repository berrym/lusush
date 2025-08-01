/**
 * @file display_stabilization.h
 * @brief Lusush Line Editor - Display System Stabilization
 *
 * This module provides display system stabilization features including
 * terminal escape sequence validation, robust error handling, performance
 * monitoring, and memory safety verification for reliable cross-platform operation.
 *
 * Part of LLE-R002: Display System Stabilization for feature integration readiness.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#ifndef LLE_DISPLAY_STABILIZATION_H
#define LLE_DISPLAY_STABILIZATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declaration
struct lle_display_state;

/**
 * @brief Terminal escape sequence validation levels
 */
typedef enum {
    LLE_ESCAPE_VALIDATION_NONE = 0,     /**< No validation (unsafe) */
    LLE_ESCAPE_VALIDATION_BASIC = 1,    /**< Basic sequence validation */
    LLE_ESCAPE_VALIDATION_STRICT = 2,   /**< Strict validation with sanitization */
    LLE_ESCAPE_VALIDATION_PARANOID = 3  /**< Maximum validation and logging */
} lle_escape_validation_level_t;

/**
 * @brief Terminal escape sequence types for validation
 */
typedef enum {
    LLE_ESCAPE_TYPE_CURSOR_MOVE = 1,    /**< Cursor movement sequences */
    LLE_ESCAPE_TYPE_CLEAR = 2,          /**< Screen/line clearing sequences */
    LLE_ESCAPE_TYPE_COLOR = 4,          /**< Color/formatting sequences */
    LLE_ESCAPE_TYPE_QUERY = 8,          /**< Terminal query sequences */
    LLE_ESCAPE_TYPE_MODE = 16,          /**< Terminal mode changes */
    LLE_ESCAPE_TYPE_ALL = 31            /**< All sequence types */
} lle_escape_type_flags_t;

/**
 * @brief Display system error recovery strategies
 */
typedef enum {
    LLE_RECOVERY_IGNORE,                /**< Ignore error and continue */
    LLE_RECOVERY_RETRY,                 /**< Retry operation once */
    LLE_RECOVERY_FALLBACK,              /**< Use fallback implementation */
    LLE_RECOVERY_RESET,                 /**< Reset display state and retry */
    LLE_RECOVERY_ABORT                  /**< Abort operation safely */
} lle_error_recovery_strategy_t;

/**
 * @brief Display system health status
 */
typedef enum {
    LLE_HEALTH_EXCELLENT,               /**< All systems operating optimally */
    LLE_HEALTH_GOOD,                    /**< Operating normally with minor issues */
    LLE_HEALTH_DEGRADED,                /**< Reduced functionality but stable */
    LLE_HEALTH_CRITICAL,                /**< Major issues, fallback mode */
    LLE_HEALTH_FAILED                   /**< System non-functional */
} lle_display_health_t;

/**
 * @brief Terminal escape sequence validation result
 */
typedef struct {
    bool is_valid;                      /**< Whether sequence is valid */
    bool is_safe;                       /**< Whether sequence is safe to use */
    size_t sequence_length;             /**< Length of validated sequence */
    lle_escape_type_flags_t type;       /**< Type of escape sequence */
    char sanitized_sequence[256];       /**< Sanitized version if needed */
    char error_message[128];            /**< Error description if invalid */
} lle_escape_validation_result_t;

/**
 * @brief Display system performance metrics for stabilization
 */
typedef struct {
    uint64_t total_operations;          /**< Total display operations */
    uint64_t failed_operations;        /**< Number of failed operations */
    uint64_t recovery_activations;      /**< Number of error recoveries */
    uint64_t validation_rejections;     /**< Number of rejected sequences */
    
    uint64_t avg_render_time_ns;        /**< Average render time in nanoseconds */
    uint64_t max_render_time_ns;        /**< Maximum render time observed */
    uint64_t min_render_time_ns;        /**< Minimum render time observed */
    
    double error_rate;                  /**< Current error rate (0.0-1.0) */
    double performance_efficiency;      /**< Performance efficiency metric */
    lle_display_health_t health_status; /**< Current system health */
} lle_display_stability_metrics_t;

/**
 * @brief Display system stabilization configuration
 */
typedef struct {
    lle_escape_validation_level_t validation_level;  /**< Escape sequence validation level */
    lle_escape_type_flags_t allowed_sequences;       /**< Allowed sequence types */
    lle_error_recovery_strategy_t recovery_strategy; /**< Error recovery strategy */
    
    bool enable_performance_monitoring;              /**< Enable performance tracking */
    bool enable_memory_safety_checks;               /**< Enable memory safety validation */
    bool enable_sequence_logging;                   /**< Log escape sequences for debugging */
    
    uint64_t max_render_time_ns;                    /**< Maximum allowed render time */
    double max_error_rate;                          /**< Maximum acceptable error rate */
    size_t memory_safety_check_interval;           /**< Memory check frequency */
} lle_display_stabilization_config_t;

/**
 * @brief Initialize display system stabilization.
 *
 * @param config Stabilization configuration, NULL for defaults
 * @return true on successful initialization, false on error
 */
bool lle_display_stabilization_init(const lle_display_stabilization_config_t *config);

/**
 * @brief Cleanup display system stabilization.
 */
void lle_display_stabilization_cleanup(void);

/**
 * @brief Validate terminal escape sequence for safety and correctness.
 *
 * @param sequence Escape sequence to validate
 * @param length Length of sequence
 * @param result Validation result structure to populate
 * @return true if validation completed (check result->is_valid for actual validity)
 */
bool lle_escape_sequence_validate(const char *sequence, 
                                  size_t length,
                                  lle_escape_validation_result_t *result);

/**
 * @brief Check if escape sequence type is allowed by current configuration.
 *
 * @param type Escape sequence type flags
 * @return true if type is allowed, false otherwise
 */
bool lle_escape_sequence_is_allowed(lle_escape_type_flags_t type);

/**
 * @brief Sanitize escape sequence for safe terminal output.
 *
 * @param sequence Input sequence to sanitize
 * @param length Length of input sequence
 * @param output Buffer for sanitized output
 * @param output_size Size of output buffer
 * @return Length of sanitized sequence, 0 on error
 */
size_t lle_escape_sequence_sanitize(const char *sequence,
                                    size_t length,
                                    char *output,
                                    size_t output_size);

/**
 * @brief Perform display system health check.
 *
 * @param state Display state to check
 * @param metrics Optional metrics structure to populate
 * @return Current system health status
 */
lle_display_health_t lle_display_system_health_check(struct lle_display_state *state,
                                                     lle_display_stability_metrics_t *metrics);

/**
 * @brief Attempt error recovery for display system.
 *
 * @param state Display state that encountered error
 * @param error_code Error code that triggered recovery
 * @return true if recovery was successful, false otherwise
 */
bool lle_display_error_recovery(struct lle_display_state *state, int error_code);

/**
 * @brief Verify display system memory safety.
 *
 * @param state Display state to check
 * @return true if memory state is safe, false if corruption detected
 */
bool lle_display_memory_safety_check(struct lle_display_state *state);

/**
 * @brief Reset display system to known good state.
 *
 * @param state Display state to reset
 * @return true on successful reset, false on error
 */
bool lle_display_system_reset(struct lle_display_state *state);

/**
 * @brief Get current stabilization metrics.
 *
 * @param metrics Structure to populate with current metrics
 * @return true on success, false on error
 */
bool lle_display_stabilization_get_metrics(lle_display_stability_metrics_t *metrics);

/**
 * @brief Update stabilization configuration at runtime.
 *
 * @param config New configuration to apply
 * @return true on successful update, false on error
 */
bool lle_display_stabilization_update_config(const lle_display_stabilization_config_t *config);

/**
 * @brief Check terminal compatibility for current environment.
 *
 * @return true if terminal is compatible with stabilization features
 */
bool lle_display_terminal_compatibility_check(void);

/**
 * @brief Benchmark display system performance.
 *
 * @param iterations Number of benchmark iterations
 * @param metrics Structure to populate with benchmark results
 * @return true on successful benchmark, false on error
 */
bool lle_display_performance_benchmark(size_t iterations,
                                       lle_display_stability_metrics_t *metrics);

/**
 * @brief Enable or disable specific stabilization features.
 *
 * @param feature_mask Bitmask of features to enable/disable
 * @param enable true to enable, false to disable
 * @return true on success, false on error
 */
bool lle_display_stabilization_toggle_features(uint32_t feature_mask, bool enable);

/**
 * @brief Record a display operation for performance monitoring.
 *
 * @param success Whether the operation succeeded
 * @param duration_ns Operation duration in nanoseconds
 */
void lle_display_stabilization_record_operation(bool success, uint64_t duration_ns);

// Feature toggle masks for lle_display_stabilization_toggle_features
#define LLE_STABILIZATION_FEATURE_VALIDATION     (1U << 0)
#define LLE_STABILIZATION_FEATURE_RECOVERY       (1U << 1)
#define LLE_STABILIZATION_FEATURE_MONITORING     (1U << 2)
#define LLE_STABILIZATION_FEATURE_MEMORY_SAFETY  (1U << 3)
#define LLE_STABILIZATION_FEATURE_LOGGING        (1U << 4)
#define LLE_STABILIZATION_FEATURE_ALL            (0xFFFFFFFFU)

// Default stabilization configuration
#define LLE_DISPLAY_STABILIZATION_DEFAULT_CONFIG {                           \
    .validation_level = LLE_ESCAPE_VALIDATION_BASIC,                        \
    .allowed_sequences = LLE_ESCAPE_TYPE_ALL,                               \
    .recovery_strategy = LLE_RECOVERY_FALLBACK,                             \
    .enable_performance_monitoring = true,                                  \
    .enable_memory_safety_checks = true,                                    \
    .enable_sequence_logging = false,                                       \
    .max_render_time_ns = 5000000,  /* 5ms */                              \
    .max_error_rate = 0.01,         /* 1% */                               \
    .memory_safety_check_interval = 100                                     \
}

#endif // LLE_DISPLAY_STABILIZATION_H