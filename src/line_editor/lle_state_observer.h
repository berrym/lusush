/**
 * @file lle_state_observer.h
 * @brief Lusush Line Editor - Non-Invasive State Observer System
 *
 * This module provides a lightweight, non-invasive state observation system
 * that monitors display operations without interfering with existing functionality.
 * Uses observer pattern to detect state divergences and provide targeted recovery.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#ifndef LLE_STATE_OBSERVER_H
#define LLE_STATE_OBSERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

// Forward declarations - avoid circular includes
struct lle_display_state;
struct lle_terminal_manager;

// ============================================================================
// Operation Tracking Types
// ============================================================================

/**
 * @brief Types of display operations to observe
 */
typedef enum {
    LLE_OP_WRITE,           /**< Text write operation */
    LLE_OP_CLEAR,           /**< Clear operation */
    LLE_OP_CURSOR_MOVE,     /**< Cursor movement */
    LLE_OP_RENDER,          /**< Full display render */
    LLE_OP_UPDATE,          /**< Incremental update */
    LLE_OP_BACKSPACE,       /**< Backspace operation */
    LLE_OP_INSERT,          /**< Character insertion */
    LLE_OP_NAVIGATION       /**< History navigation */
} lle_operation_type_t;

/**
 * @brief Operation metadata for tracking
 */
typedef struct {
    lle_operation_type_t type;      /**< Type of operation */
    uint64_t timestamp;             /**< When operation occurred */
    size_t data_length;             /**< Length of data involved */
    size_t cursor_pos_before;       /**< Cursor position before op */
    size_t cursor_pos_after;        /**< Cursor position after op */
    bool operation_successful;      /**< Whether operation succeeded */
    char description[64];           /**< Human-readable description */
} lle_operation_metadata_t;

/**
 * @brief State health metrics
 */
typedef struct {
    bool content_consistent;        /**< Content matches expected */
    bool cursor_consistent;         /**< Cursor position matches */
    bool geometry_consistent;       /**< Terminal geometry matches */
    uint64_t last_validation_time;  /**< Last health check time */
    size_t consecutive_failures;    /**< Consecutive validation failures */
    double health_score;            /**< Overall health (0.0-1.0) */
} lle_state_health_t;

// ============================================================================
// Observer Configuration
// ============================================================================

/**
 * @brief Observer configuration settings
 */
typedef struct {
    bool enabled;                   /**< Whether observer is active */
    bool periodic_validation;       /**< Enable periodic health checks */
    uint64_t validation_interval;   /**< Validation interval in microseconds */
    size_t operation_buffer_size;   /**< Size of operation history buffer */
    double health_threshold;        /**< Health threshold for intervention */
    size_t max_recovery_attempts;   /**< Max recovery attempts before giving up */
    
    // Debug and logging
    bool debug_logging;             /**< Enable debug output */
    bool operation_logging;         /**< Log all operations */
    FILE *log_output;               /**< Log output file (NULL for stderr) */
    
    // Performance tuning
    bool smart_validation;          /**< Only validate after risky operations */
    bool batch_validations;         /**< Batch multiple validations */
    size_t validation_batch_size;   /**< Number of operations per batch */
} lle_observer_config_t;

/**
 * @brief Observer statistics
 */
typedef struct {
    size_t total_operations;        /**< Total operations observed */
    size_t validation_checks;       /**< Total validation checks performed */
    size_t divergences_detected;    /**< State divergences found */
    size_t recoveries_attempted;    /**< Recovery operations attempted */
    size_t recoveries_successful;   /**< Successful recovery operations */
    
    // Performance metrics
    uint64_t total_validation_time; /**< Total time spent validating */
    uint64_t total_recovery_time;   /**< Total time spent recovering */
    double avg_validation_time;     /**< Average validation time */
    double avg_recovery_time;       /**< Average recovery time */
    
    // Health metrics
    double current_health_score;    /**< Current system health */
    double min_health_score;        /**< Minimum health score seen */
    double avg_health_score;        /**< Average health score */
    uint64_t last_healthy_time;     /**< Last time system was healthy */
} lle_observer_stats_t;

// ============================================================================
// State Observer Context
// ============================================================================

/**
 * @brief Main state observer context
 */
typedef struct lle_state_observer {
    // Core references
    struct lle_display_state *display;     /**< Display state reference */
    struct lle_terminal_manager *terminal; /**< Terminal manager reference */
    
    // Configuration
    lle_observer_config_t config;           /**< Observer configuration */
    
    // Operation tracking
    lle_operation_metadata_t *operation_history; /**< Circular buffer of operations */
    size_t operation_head;                  /**< Head of circular buffer */
    size_t operation_count;                 /**< Number of operations stored */
    
    // Health monitoring
    lle_state_health_t current_health;      /**< Current state health */
    uint64_t last_validation_time;          /**< Last validation timestamp */
    bool validation_in_progress;           /**< True if validation running */
    
    // Statistics
    lle_observer_stats_t stats;             /**< Observer statistics */
    
    // Recovery state
    bool recovery_in_progress;              /**< True if recovery running */
    size_t recovery_attempts;               /**< Current recovery attempts */
    char last_recovery_reason[128];         /**< Reason for last recovery */
    
    // Callbacks
    void (*on_divergence_detected)(const char *reason, double health_score);
    void (*on_recovery_started)(const char *reason);
    void (*on_recovery_completed)(bool successful, const char *result);
    void (*on_health_change)(double old_score, double new_score);
} lle_state_observer_t;

// ============================================================================
// Core Observer API
// ============================================================================

/**
 * @brief Initialize state observer system
 * 
 * @param display Display state to observe
 * @param terminal Terminal manager to monitor
 * @param config Observer configuration (NULL for defaults)
 * @return Allocated observer context, NULL on error
 */
lle_state_observer_t *lle_state_observer_init(struct lle_display_state *display,
                                               struct lle_terminal_manager *terminal,
                                               const lle_observer_config_t *config);

/**
 * @brief Cleanup state observer system
 * 
 * @param observer Observer context to cleanup
 */
void lle_state_observer_cleanup(lle_state_observer_t *observer);

/**
 * @brief Enable or disable state observation
 * 
 * @param observer Observer context
 * @param enabled Whether to enable observation
 * @return true on success, false on error
 */
bool lle_state_observer_set_enabled(lle_state_observer_t *observer, bool enabled);

/**
 * @brief Get default observer configuration
 * 
 * @param config Output configuration structure
 */
void lle_state_observer_get_default_config(lle_observer_config_t *config);

// ============================================================================
// Operation Tracking API
// ============================================================================

/**
 * @brief Record completion of a display operation
 * 
 * @param observer Observer context
 * @param type Type of operation completed
 * @param data_length Length of data involved in operation
 * @param cursor_pos_before Cursor position before operation
 * @param cursor_pos_after Cursor position after operation
 * @param successful Whether operation completed successfully
 * @param description Human-readable description
 * @return true on success, false on error
 */
bool lle_state_observer_record_operation(lle_state_observer_t *observer,
                                          lle_operation_type_t type,
                                          size_t data_length,
                                          size_t cursor_pos_before,
                                          size_t cursor_pos_after,
                                          bool successful,
                                          const char *description);

/**
 * @brief Check if periodic validation is due
 * 
 * @param observer Observer context
 * @return true if validation should be performed
 */
bool lle_state_observer_validation_due(lle_state_observer_t *observer);

/**
 * @brief Perform state health validation
 * 
 * @param observer Observer context
 * @return true if state is healthy, false if divergence detected
 */
bool lle_state_observer_validate_health(lle_state_observer_t *observer);

/**
 * @brief Get current state health information
 * 
 * @param observer Observer context
 * @param health Output health information
 * @return true on success, false on error
 */
bool lle_state_observer_get_health(lle_state_observer_t *observer,
                                   lle_state_health_t *health);

// ============================================================================
// Recovery API
// ============================================================================

/**
 * @brief Attempt to recover from detected state divergence
 * 
 * @param observer Observer context
 * @param reason Reason for recovery attempt
 * @return true if recovery successful, false if failed
 */
bool lle_state_observer_attempt_recovery(lle_state_observer_t *observer,
                                          const char *reason);

/**
 * @brief Check if observer is currently performing recovery
 * 
 * @param observer Observer context
 * @return true if recovery in progress, false otherwise
 */
bool lle_state_observer_recovery_in_progress(lle_state_observer_t *observer);

/**
 * @brief Force immediate state validation and recovery if needed
 * 
 * @param observer Observer context
 * @return true if state is healthy after check, false if problems remain
 */
bool lle_state_observer_force_validation(lle_state_observer_t *observer);

// ============================================================================
// Statistics and Monitoring API
// ============================================================================

/**
 * @brief Get observer statistics
 * 
 * @param observer Observer context
 * @param stats Output statistics structure
 * @return true on success, false on error
 */
bool lle_state_observer_get_statistics(lle_state_observer_t *observer,
                                        lle_observer_stats_t *stats);

/**
 * @brief Reset observer statistics
 * 
 * @param observer Observer context
 * @return true on success, false on error
 */
bool lle_state_observer_reset_statistics(lle_state_observer_t *observer);

/**
 * @brief Get recent operation history
 * 
 * @param observer Observer context
 * @param operations Output array for operations
 * @param max_operations Maximum number of operations to return
 * @param actual_count Output: actual number of operations returned
 * @return true on success, false on error
 */
bool lle_state_observer_get_operation_history(lle_state_observer_t *observer,
                                               lle_operation_metadata_t *operations,
                                               size_t max_operations,
                                               size_t *actual_count);

// ============================================================================
// Configuration API
// ============================================================================

/**
 * @brief Update observer configuration
 * 
 * @param observer Observer context
 * @param config New configuration
 * @return true on success, false on error
 */
bool lle_state_observer_update_config(lle_state_observer_t *observer,
                                       const lle_observer_config_t *config);

/**
 * @brief Set validation interval
 * 
 * @param observer Observer context
 * @param interval_us Validation interval in microseconds
 * @return true on success, false on error
 */
bool lle_state_observer_set_validation_interval(lle_state_observer_t *observer,
                                                 uint64_t interval_us);

/**
 * @brief Set health threshold for intervention
 * 
 * @param observer Observer context
 * @param threshold Health threshold (0.0-1.0)
 * @return true on success, false on error
 */
bool lle_state_observer_set_health_threshold(lle_state_observer_t *observer,
                                              double threshold);

/**
 * @brief Enable or disable debug logging
 * 
 * @param observer Observer context
 * @param enabled Whether to enable debug logging
 * @param log_file Log file (NULL for stderr)
 * @return true on success, false on error
 */
bool lle_state_observer_set_debug_logging(lle_state_observer_t *observer,
                                           bool enabled,
                                           FILE *log_file);

// ============================================================================
// Integration Helpers
// ============================================================================

/**
 * @brief Convenience macro for recording operation completion
 */
#define LLE_OBSERVER_RECORD_OP(observer, type, data_len, cursor_before, cursor_after, success, desc) \
    do { \
        if ((observer) && (observer)->config.enabled) { \
            lle_state_observer_record_operation(observer, type, data_len, cursor_before, cursor_after, success, desc); \
        } \
    } while(0)

/**
 * @brief Convenience macro for periodic validation check
 */
#define LLE_OBSERVER_CHECK_HEALTH(observer) \
    do { \
        if ((observer) && (observer)->config.enabled && lle_state_observer_validation_due(observer)) { \
            lle_state_observer_validate_health(observer); \
        } \
    } while(0)

/**
 * @brief Check if operation type is considered "risky" for validation
 * 
 * @param type Operation type
 * @return true if operation is risky and should trigger validation
 */
bool lle_state_observer_is_risky_operation(lle_operation_type_t type);

/**
 * @brief Get human-readable operation type name
 * 
 * @param type Operation type
 * @return String name of operation type
 */
const char *lle_state_observer_operation_type_name(lle_operation_type_t type);

// ============================================================================
// Debug and Diagnostic API
// ============================================================================

/**
 * @brief Dump observer state for debugging
 * 
 * @param observer Observer context
 * @param output Output file (NULL for stderr)
 */
void lle_state_observer_debug_dump(lle_state_observer_t *observer, FILE *output);

/**
 * @brief Dump operation history for debugging
 * 
 * @param observer Observer context
 * @param output Output file (NULL for stderr)
 * @param max_operations Maximum operations to dump (0 for all)
 */
void lle_state_observer_debug_dump_operations(lle_state_observer_t *observer,
                                               FILE *output,
                                               size_t max_operations);

/**
 * @brief Generate health report
 * 
 * @param observer Observer context
 * @param output Output file (NULL for stderr)
 */
void lle_state_observer_debug_health_report(lle_state_observer_t *observer,
                                             FILE *output);

#endif // LLE_STATE_OBSERVER_H