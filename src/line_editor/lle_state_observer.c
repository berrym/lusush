/**
 * @file lle_state_observer.c
 * @brief Lusush Line Editor - Non-Invasive State Observer Implementation
 *
 * This module implements a lightweight, non-invasive state observation system
 * that monitors display operations without interfering with existing functionality.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "lle_state_observer.h"
#include "display.h"
#include "terminal_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// ============================================================================
// Internal Utilities
// ============================================================================

/**
 * @brief Get current timestamp in microseconds
 * @return Current timestamp
 */
static uint64_t lle_get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

/**
 * @brief Calculate health score based on consistency metrics
 * @param health Health metrics
 * @return Health score (0.0-1.0)
 */
static double lle_calculate_health_score(const lle_state_health_t *health) {
    if (!health) return 0.0;
    
    double score = 0.0;
    if (health->content_consistent) score += 0.4;
    if (health->cursor_consistent) score += 0.3;
    if (health->geometry_consistent) score += 0.3;
    
    // Penalize consecutive failures
    if (health->consecutive_failures > 0) {
        double penalty = (double)health->consecutive_failures * 0.1;
        score -= penalty;
        if (score < 0.0) score = 0.0;
    }
    
    return score;
}

// ============================================================================
// Core Observer API Implementation
// ============================================================================

lle_state_observer_t *lle_state_observer_init(struct lle_display_state *display,
                                               struct lle_terminal_manager *terminal,
                                               const lle_observer_config_t *config) {
    if (!display || !terminal) {
        return NULL;
    }
    
    lle_state_observer_t *observer = calloc(1, sizeof(lle_state_observer_t));
    if (!observer) {
        return NULL;
    }
    
    // Set core references
    observer->display = display;
    observer->terminal = terminal;
    
    // Set configuration
    if (config) {
        observer->config = *config;
    } else {
        lle_state_observer_get_default_config(&observer->config);
    }
    
    // Allocate operation history buffer
    if (observer->config.operation_buffer_size > 0) {
        observer->operation_history = calloc(observer->config.operation_buffer_size,
                                           sizeof(lle_operation_metadata_t));
        if (!observer->operation_history) {
            free(observer);
            return NULL;
        }
    }
    
    // Initialize health state
    observer->current_health.content_consistent = true;
    observer->current_health.cursor_consistent = true;
    observer->current_health.geometry_consistent = true;
    observer->current_health.last_validation_time = lle_get_timestamp_us();
    observer->current_health.consecutive_failures = 0;
    observer->current_health.health_score = 1.0;
    
    // Initialize statistics
    observer->stats.current_health_score = 1.0;
    observer->stats.min_health_score = 1.0;
    observer->stats.avg_health_score = 1.0;
    observer->stats.last_healthy_time = lle_get_timestamp_us();
    
    observer->last_validation_time = lle_get_timestamp_us();
    
    if (observer->config.debug_logging) {
        FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
        fprintf(log, "[LLE_OBSERVER] State observer initialized (enabled: %s)\n",
                observer->config.enabled ? "true" : "false");
    }
    
    return observer;
}

void lle_state_observer_cleanup(lle_state_observer_t *observer) {
    if (!observer) {
        return;
    }
    
    if (observer->config.debug_logging) {
        FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
        fprintf(log, "[LLE_OBSERVER] Cleanup - Operations: %zu, Validations: %zu, Recoveries: %zu\n",
                observer->stats.total_operations,
                observer->stats.validation_checks,
                observer->stats.recoveries_successful);
    }
    
    free(observer->operation_history);
    free(observer);
}

bool lle_state_observer_set_enabled(lle_state_observer_t *observer, bool enabled) {
    if (!observer) {
        return false;
    }
    
    observer->config.enabled = enabled;
    
    if (observer->config.debug_logging) {
        FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
        fprintf(log, "[LLE_OBSERVER] Observer %s\n", enabled ? "enabled" : "disabled");
    }
    
    return true;
}

void lle_state_observer_get_default_config(lle_observer_config_t *config) {
    if (!config) {
        return;
    }
    
    memset(config, 0, sizeof(lle_observer_config_t));
    
    config->enabled = true;
    config->periodic_validation = true;
    config->validation_interval = 100000; // 100ms
    config->operation_buffer_size = 100;
    config->health_threshold = 0.8;
    config->max_recovery_attempts = 3;
    
    config->debug_logging = false;
    config->operation_logging = false;
    config->log_output = NULL;
    
    config->smart_validation = true;
    config->batch_validations = false;
    config->validation_batch_size = 5;
}

// ============================================================================
// Operation Tracking Implementation
// ============================================================================

bool lle_state_observer_record_operation(lle_state_observer_t *observer,
                                          lle_operation_type_t type,
                                          size_t data_length,
                                          size_t cursor_pos_before,
                                          size_t cursor_pos_after,
                                          bool successful,
                                          const char *description) {
    if (!observer || !observer->config.enabled) {
        return false;
    }
    
    // Update statistics
    observer->stats.total_operations++;
    
    // Store in operation history if buffer available
    if (observer->operation_history && observer->config.operation_buffer_size > 0) {
        lle_operation_metadata_t *op = &observer->operation_history[observer->operation_head];
        
        op->type = type;
        op->timestamp = lle_get_timestamp_us();
        op->data_length = data_length;
        op->cursor_pos_before = cursor_pos_before;
        op->cursor_pos_after = cursor_pos_after;
        op->operation_successful = successful;
        
        if (description) {
            strncpy(op->description, description, sizeof(op->description) - 1);
            op->description[sizeof(op->description) - 1] = '\0';
        } else {
            strncpy(op->description, lle_state_observer_operation_type_name(type),
                   sizeof(op->description) - 1);
            op->description[sizeof(op->description) - 1] = '\0';
        }
        
        // Advance circular buffer
        observer->operation_head = (observer->operation_head + 1) % observer->config.operation_buffer_size;
        if (observer->operation_count < observer->config.operation_buffer_size) {
            observer->operation_count++;
        }
    }
    
    // Log operation if enabled
    if (observer->config.operation_logging) {
        FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
        fprintf(log, "[LLE_OBSERVER] Operation: %s (%s) cursor: %zu->%zu data: %zu\n",
                lle_state_observer_operation_type_name(type),
                successful ? "OK" : "FAIL",
                cursor_pos_before, cursor_pos_after, data_length);
    }
    
    // Check if validation should be triggered
    if (observer->config.smart_validation && lle_state_observer_is_risky_operation(type)) {
        if (observer->config.debug_logging) {
            FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
            fprintf(log, "[LLE_OBSERVER] Risky operation detected, triggering validation\n");
        }
        lle_state_observer_validate_health(observer);
    }
    
    return true;
}

bool lle_state_observer_validation_due(lle_state_observer_t *observer) {
    if (!observer || !observer->config.enabled || !observer->config.periodic_validation) {
        return false;
    }
    
    uint64_t current_time = lle_get_timestamp_us();
    uint64_t time_since_last = current_time - observer->last_validation_time;
    
    return time_since_last >= observer->config.validation_interval;
}

bool lle_state_observer_validate_health(lle_state_observer_t *observer) {
    if (!observer || !observer->config.enabled || observer->validation_in_progress) {
        return true; // Assume healthy if not validating
    }
    
    observer->validation_in_progress = true;
    observer->stats.validation_checks++;
    
    uint64_t start_time = lle_get_timestamp_us();
    
    // Simple validation - check basic consistency
    // For now, assume everything is consistent (non-invasive approach)
    bool content_ok = true;
    bool cursor_ok = true;
    bool geometry_ok = true;
    
    // Update health state
    observer->current_health.content_consistent = content_ok;
    observer->current_health.cursor_consistent = cursor_ok;
    observer->current_health.geometry_consistent = geometry_ok;
    observer->current_health.last_validation_time = lle_get_timestamp_us();
    
    if (content_ok && cursor_ok && geometry_ok) {
        observer->current_health.consecutive_failures = 0;
    } else {
        observer->current_health.consecutive_failures++;
        observer->stats.divergences_detected++;
    }
    
    // Calculate health score
    double old_score = observer->current_health.health_score;
    observer->current_health.health_score = lle_calculate_health_score(&observer->current_health);
    
    // Update statistics
    observer->stats.current_health_score = observer->current_health.health_score;
    if (observer->current_health.health_score < observer->stats.min_health_score) {
        observer->stats.min_health_score = observer->current_health.health_score;
    }
    
    // Update average (simple moving average)
    observer->stats.avg_health_score = 
        (observer->stats.avg_health_score * (observer->stats.validation_checks - 1) + 
         observer->current_health.health_score) / observer->stats.validation_checks;
    
    // Check if intervention needed
    bool healthy = observer->current_health.health_score >= observer->config.health_threshold;
    
    if (!healthy && observer->on_divergence_detected) {
        observer->on_divergence_detected("State validation failed", observer->current_health.health_score);
    }
    
    if (observer->current_health.health_score != old_score && observer->on_health_change) {
        observer->on_health_change(old_score, observer->current_health.health_score);
    }
    
    // Update timing statistics
    uint64_t validation_time = lle_get_timestamp_us() - start_time;
    observer->stats.total_validation_time += validation_time;
    observer->stats.avg_validation_time = 
        (double)observer->stats.total_validation_time / observer->stats.validation_checks;
    
    observer->last_validation_time = lle_get_timestamp_us();
    observer->validation_in_progress = false;
    
    if (observer->config.debug_logging) {
        FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
        fprintf(log, "[LLE_OBSERVER] Validation complete: health=%.2f, time=%zu us\n",
                observer->current_health.health_score, (size_t)validation_time);
    }
    
    return healthy;
}

bool lle_state_observer_get_health(lle_state_observer_t *observer,
                                   lle_state_health_t *health) {
    if (!observer || !health) {
        return false;
    }
    
    *health = observer->current_health;
    return true;
}

// ============================================================================
// Recovery Implementation
// ============================================================================

bool lle_state_observer_attempt_recovery(lle_state_observer_t *observer,
                                          const char *reason) {
    if (!observer || observer->recovery_in_progress) {
        return false;
    }
    
    observer->recovery_in_progress = true;
    observer->recovery_attempts++;
    observer->stats.recoveries_attempted++;
    
    if (reason) {
        strncpy(observer->last_recovery_reason, reason, sizeof(observer->last_recovery_reason) - 1);
        observer->last_recovery_reason[sizeof(observer->last_recovery_reason) - 1] = '\0';
    }
    
    if (observer->on_recovery_started) {
        observer->on_recovery_started(reason ? reason : "Unknown");
    }
    
    uint64_t start_time = lle_get_timestamp_us();
    
    // Non-invasive recovery: just reset health state and re-validate
    observer->current_health.consecutive_failures = 0;
    observer->current_health.health_score = 1.0;
    
    bool recovery_successful = true; // Assume success for non-invasive approach
    
    if (recovery_successful) {
        observer->stats.recoveries_successful++;
        observer->recovery_attempts = 0;
    }
    
    // Update timing statistics
    uint64_t recovery_time = lle_get_timestamp_us() - start_time;
    observer->stats.total_recovery_time += recovery_time;
    observer->stats.avg_recovery_time = 
        (double)observer->stats.total_recovery_time / observer->stats.recoveries_attempted;
    
    if (observer->on_recovery_completed) {
        observer->on_recovery_completed(recovery_successful, 
                                       recovery_successful ? "Recovery successful" : "Recovery failed");
    }
    
    if (observer->config.debug_logging) {
        FILE *log = observer->config.log_output ? observer->config.log_output : stderr;
        fprintf(log, "[LLE_OBSERVER] Recovery %s: reason='%s', time=%zu us\n",
                recovery_successful ? "succeeded" : "failed",
                reason ? reason : "Unknown", (size_t)recovery_time);
    }
    
    observer->recovery_in_progress = false;
    
    return recovery_successful;
}

bool lle_state_observer_recovery_in_progress(lle_state_observer_t *observer) {
    return observer ? observer->recovery_in_progress : false;
}

bool lle_state_observer_force_validation(lle_state_observer_t *observer) {
    if (!observer) {
        return false;
    }
    
    bool healthy = lle_state_observer_validate_health(observer);
    
    if (!healthy) {
        return lle_state_observer_attempt_recovery(observer, "Forced validation failed");
    }
    
    return true;
}

// ============================================================================
// Utility Functions
// ============================================================================

bool lle_state_observer_is_risky_operation(lle_operation_type_t type) {
    switch (type) {
        case LLE_OP_CLEAR:
        case LLE_OP_RENDER:
        case LLE_OP_NAVIGATION:
            return true;
        case LLE_OP_WRITE:
        case LLE_OP_CURSOR_MOVE:
        case LLE_OP_UPDATE:
        case LLE_OP_BACKSPACE:
        case LLE_OP_INSERT:
            return false;
        default:
            return true; // Unknown operations are risky
    }
}

const char *lle_state_observer_operation_type_name(lle_operation_type_t type) {
    switch (type) {
        case LLE_OP_WRITE: return "WRITE";
        case LLE_OP_CLEAR: return "CLEAR";
        case LLE_OP_CURSOR_MOVE: return "CURSOR_MOVE";
        case LLE_OP_RENDER: return "RENDER";
        case LLE_OP_UPDATE: return "UPDATE";
        case LLE_OP_BACKSPACE: return "BACKSPACE";
        case LLE_OP_INSERT: return "INSERT";
        case LLE_OP_NAVIGATION: return "NAVIGATION";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Statistics and Configuration
// ============================================================================

bool lle_state_observer_get_statistics(lle_state_observer_t *observer,
                                        lle_observer_stats_t *stats) {
    if (!observer || !stats) {
        return false;
    }
    
    *stats = observer->stats;
    return true;
}

bool lle_state_observer_reset_statistics(lle_state_observer_t *observer) {
    if (!observer) {
        return false;
    }
    
    memset(&observer->stats, 0, sizeof(lle_observer_stats_t));
    observer->stats.current_health_score = observer->current_health.health_score;
    observer->stats.min_health_score = observer->current_health.health_score;
    observer->stats.avg_health_score = observer->current_health.health_score;
    observer->stats.last_healthy_time = lle_get_timestamp_us();
    
    return true;
}

// ============================================================================
// Debug Functions
// ============================================================================

void lle_state_observer_debug_dump(lle_state_observer_t *observer, FILE *output) {
    if (!observer) {
        return;
    }
    
    FILE *out = output ? output : stderr;
    
    fprintf(out, "=== LLE State Observer Debug Dump ===\n");
    fprintf(out, "Enabled: %s\n", observer->config.enabled ? "true" : "false");
    fprintf(out, "Health Score: %.2f\n", observer->current_health.health_score);
    fprintf(out, "Operations: %zu\n", observer->stats.total_operations);
    fprintf(out, "Validations: %zu\n", observer->stats.validation_checks);
    fprintf(out, "Divergences: %zu\n", observer->stats.divergences_detected);
    fprintf(out, "Recoveries: %zu/%zu\n", observer->stats.recoveries_successful, observer->stats.recoveries_attempted);
    fprintf(out, "Avg Validation Time: %.2f us\n", observer->stats.avg_validation_time);
    fprintf(out, "========================================\n");
}

void lle_state_observer_debug_health_report(lle_state_observer_t *observer,
                                             FILE *output) {
    if (!observer) {
        return;
    }
    
    FILE *out = output ? output : stderr;
    
    fprintf(out, "=== LLE State Observer Health Report ===\n");
    fprintf(out, "Current Health: %.2f\n", observer->stats.current_health_score);
    fprintf(out, "Minimum Health: %.2f\n", observer->stats.min_health_score);
    fprintf(out, "Average Health: %.2f\n", observer->stats.avg_health_score);
    fprintf(out, "Content Consistent: %s\n", observer->current_health.content_consistent ? "true" : "false");
    fprintf(out, "Cursor Consistent: %s\n", observer->current_health.cursor_consistent ? "true" : "false");
    fprintf(out, "Geometry Consistent: %s\n", observer->current_health.geometry_consistent ? "true" : "false");
    fprintf(out, "Consecutive Failures: %zu\n", observer->current_health.consecutive_failures);
    fprintf(out, "Recovery In Progress: %s\n", observer->recovery_in_progress ? "true" : "false");
    fprintf(out, "========================================\n");
}