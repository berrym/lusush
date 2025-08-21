/*
 * Lusush Shell - Display Controller Integration Wrapper Header
 * Week 8 Shell Integration Implementation
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ============================================================================
 * 
 * DISPLAY INTEGRATION WRAPPER HEADER
 * 
 * This header defines the API for integrating the layered display controller
 * with existing shell display functions. It provides seamless function
 * replacement with backward compatibility, configuration management, and
 * performance monitoring integration.
 * 
 * Key Features:
 * - Seamless function replacement with zero regression
 * - Configuration-based enable/disable of layered display
 * - Performance monitoring integration with shell diagnostics
 * - Enterprise deployment readiness with configuration management
 * - Graceful fallback to existing display functions
 * - Resource management and memory safety
 * 
 * Integration Functions:
 * - display_integration_redisplay() replaces lusush_safe_redisplay()
 * - display_integration_prompt_update() replaces lusush_prompt_update()
 * - display_integration_clear_screen() replaces lusush_clear_screen()
 * 
 * Strategic Benefits:
 * - Zero regression deployment of layered display architecture
 * - Enterprise-grade configuration and monitoring capabilities
 * - Production-ready performance optimization and caching
 * - Seamless user experience with enhanced display capabilities
 */

#ifndef DISPLAY_INTEGRATION_H
#define DISPLAY_INTEGRATION_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#include "display/display_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS AND CONFIGURATION
// ============================================================================

#define DISPLAY_INTEGRATION_VERSION_MAJOR 1
#define DISPLAY_INTEGRATION_VERSION_MINOR 0
#define DISPLAY_INTEGRATION_VERSION_PATCH 0

// Buffer and limit constants
#define DISPLAY_INTEGRATION_MAX_OUTPUT_SIZE 16384
#define DISPLAY_INTEGRATION_MAX_CONFIG_STRING 256

// Default configuration values
#define DISPLAY_INTEGRATION_DEFAULT_PERFORMANCE_THRESHOLD_MS 20
#define DISPLAY_INTEGRATION_DEFAULT_CACHE_HIT_RATE_THRESHOLD 0.8
#define DISPLAY_INTEGRATION_DEFAULT_OPTIMIZATION_LEVEL DISPLAY_OPTIMIZATION_BALANCED

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * Display integration health status enumeration.
 */
typedef enum {
    DISPLAY_INTEGRATION_HEALTH_EXCELLENT = 0,     // Operating perfectly with layered display
    DISPLAY_INTEGRATION_HEALTH_GOOD = 1,          // Operating well with minor issues
    DISPLAY_INTEGRATION_HEALTH_DEGRADED = 2,      // Performance issues or high fallback rate
    DISPLAY_INTEGRATION_HEALTH_ERROR = 3,         // Error in layered display system
    DISPLAY_INTEGRATION_HEALTH_DISABLED = 4,      // Layered display intentionally disabled
    DISPLAY_INTEGRATION_HEALTH_NOT_INITIALIZED = 5 // Integration system not initialized
} display_integration_health_t;

/**
 * Display integration configuration structure.
 * Controls all aspects of layered display integration behavior.
 */
typedef struct {
    // Core feature enables
    bool enable_layered_display;           // Enable/disable layered display controller
    bool enable_caching;                   // Enable display caching for performance
    bool enable_performance_monitoring;    // Enable performance tracking and metrics
    
    // Optimization settings
    display_optimization_level_t optimization_level; // Performance optimization level
    uint32_t performance_threshold_ms;     // Performance threshold for health checks
    double cache_hit_rate_threshold;       // Cache hit rate threshold for health
    
    // Behavior settings
    bool fallback_on_error;               // Fallback to original functions on error
    bool debug_mode;                      // Enable debug output and logging
    size_t max_output_size;               // Maximum size for display output buffers
    
    // Enterprise deployment settings
    char deployment_mode[DISPLAY_INTEGRATION_MAX_CONFIG_STRING]; // Deployment mode identifier
    char environment_name[DISPLAY_INTEGRATION_MAX_CONFIG_STRING]; // Environment name
    bool enable_enterprise_logging;        // Enable enterprise-grade logging
    bool strict_compatibility_mode;        // Strict backward compatibility mode
} display_integration_config_t;

/**
 * Display integration performance statistics.
 * Tracks usage patterns and performance metrics for monitoring and optimization.
 */
typedef struct {
    // Usage statistics
    time_t init_time;                     // Time when integration was initialized
    uint64_t total_display_calls;         // Total number of display function calls
    uint64_t layered_display_calls;       // Calls handled by layered display
    uint64_t fallback_calls;              // Calls that fell back to original functions
    
    // Performance metrics
    uint64_t avg_layered_display_time_ns; // Average time for layered display operations
    double cache_hit_rate;                // Current cache hit rate (0.0-1.0)
    size_t memory_usage_bytes;            // Current memory usage of integration system
    
    // Error tracking
    uint64_t layered_display_errors;      // Number of errors in layered display
    uint64_t fallback_triggers;           // Number of times fallback was triggered
    time_t last_error_time;               // Time of last error
    
    // Health indicators
    bool performance_within_threshold;     // Performance meeting threshold requirements
    bool cache_efficiency_good;           // Cache performing efficiently
    bool memory_usage_acceptable;         // Memory usage within acceptable limits
} display_integration_stats_t;

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

/**
 * Initialize the display integration system.
 * Sets up the display controller and prepares for shell integration.
 * 
 * This function must be called during shell startup, before any display
 * operations. It initializes the layered display controller if enabled
 * and prepares the integration wrapper system.
 *
 * @param config Configuration for display integration (NULL for defaults)
 * @return true on success, false on failure
 */
bool display_integration_init(const display_integration_config_t *config);

/**
 * Cleanup the display integration system.
 * Releases all resources and resets state.
 * 
 * This function should be called during shell shutdown to ensure
 * proper cleanup of all display integration resources.
 */
void display_integration_cleanup(void);

// ============================================================================
// CONFIGURATION MANAGEMENT
// ============================================================================

/**
 * Create default configuration for display integration.
 * Initializes configuration structure with sensible defaults for most environments.
 *
 * @param config Configuration structure to initialize
 */
void display_integration_create_default_config(display_integration_config_t *config);

/**
 * Update display integration configuration.
 * Applies new configuration settings and reconfigures the system as needed.
 * 
 * This function can be called at runtime to change integration behavior,
 * including enabling/disabling layered display or changing optimization levels.
 *
 * @param config New configuration to apply
 * @return true on success, false on failure
 */
bool display_integration_set_config(const display_integration_config_t *config);

/**
 * Get current display integration configuration.
 * Retrieves the currently active configuration settings.
 *
 * @param config Configuration structure to fill
 * @return true on success, false on failure
 */
bool display_integration_get_config(display_integration_config_t *config);

// ============================================================================
// MAIN INTEGRATION FUNCTIONS (Shell Function Replacements)
// ============================================================================

/**
 * Integrated display function - replacement for lusush_safe_redisplay().
 * 
 * This function provides coordinated display using the layered architecture
 * when enabled, with graceful fallback to the existing display function.
 * It handles prompt rendering, syntax highlighting, and display composition
 * through the display controller system.
 * 
 * Usage: Replace all calls to lusush_safe_redisplay() with this function.
 * Behavior: Seamless operation with enhanced capabilities when layered display
 *          is enabled, identical behavior when disabled.
 */
void display_integration_redisplay(void);

/**
 * Integrated prompt update function - replacement for lusush_prompt_update().
 * 
 * This function provides coordinated prompt updates using the layered
 * architecture when enabled, with graceful fallback to the existing
 * prompt update function. It ensures proper prompt rendering and
 * coordination with other display layers.
 * 
 * Usage: Replace all calls to lusush_prompt_update() with this function.
 * Behavior: Enhanced prompt update coordination when layered display is
 *          enabled, identical behavior when disabled.
 */
void display_integration_prompt_update(void);

/**
 * Integrated clear screen function - replacement for lusush_clear_screen().
 * 
 * This function provides coordinated screen clearing using the layered
 * architecture when enabled, with graceful fallback to the existing
 * clear screen function. It ensures proper screen clearing and display
 * state reset across all layers.
 * 
 * Usage: Replace all calls to lusush_clear_screen() with this function.
 * Behavior: Coordinated screen clearing when layered display is enabled,
 *          identical behavior when disabled.
 */
void display_integration_clear_screen(void);

/**
 * Get enhanced prompt using layered display system.
 * Provides Ferrari engine prompt generation with visual enhancements.
 * 
 * @param enhanced_prompt Pointer to store the generated enhanced prompt
 * @return true on success, false on failure
 */
bool display_integration_get_enhanced_prompt(char **enhanced_prompt);

// ============================================================================
// PERFORMANCE MONITORING AND DIAGNOSTICS
// ============================================================================

/**
 * Get display integration performance statistics.
 * Retrieves comprehensive performance and usage statistics for monitoring
 * and optimization purposes.
 *
 * @param stats Statistics structure to fill
 * @return true on success, false on failure
 */
bool display_integration_get_stats(display_integration_stats_t *stats);

/**
 * Reset display integration performance statistics.
 * Clears all performance counters and statistics, useful for benchmarking
 * or periodic monitoring resets.
 */
void display_integration_reset_stats(void);

/**
 * Check if layered display is currently enabled and working.
 * 
 * @return true if layered display is active and functional, false otherwise
 */
bool display_integration_is_layered_active(void);

/**
 * Get display integration health status.
 * Provides a comprehensive health assessment of the integration system
 * based on performance metrics, error rates, and system status.
 * 
 * @return health status enum indicating system health
 */
display_integration_health_t display_integration_get_health(void);

/**
 * Get human-readable health status string.
 * Converts health status enum to descriptive string for user display.
 *
 * @param health Health status enum
 * @return String description of health status
 */
const char* display_integration_health_string(display_integration_health_t health);

/**
 * Print comprehensive diagnostic information about display integration.
 * Outputs detailed information about integration status, performance,
 * configuration, and health for debugging and monitoring purposes.
 */
void display_integration_print_diagnostics(void);

// ============================================================================
// ENTERPRISE FEATURES AND DEPLOYMENT
// ============================================================================

/**
 * Enable enterprise deployment mode with specific configuration.
 * Configures the integration system for enterprise deployment with
 * appropriate logging, monitoring, and compatibility settings.
 *
 * @param deployment_name Name identifier for the deployment
 * @param environment_name Environment identifier (dev/staging/prod)
 * @param strict_compatibility Enable strict backward compatibility mode
 * @return true on success, false on failure
 */
bool display_integration_enable_enterprise_mode(
    const char *deployment_name,
    const char *environment_name,
    bool strict_compatibility
);

/**
 * Get integration system version information.
 * Returns version string for compatibility and support purposes.
 *
 * @return Version string in "major.minor.patch" format
 */
const char* display_integration_get_version(void);

/**
 * Check compatibility with shell version.
 * Verifies that the integration system is compatible with the current
 * shell version and configuration.
 *
 * @param shell_version Shell version string to check
 * @return true if compatible, false if incompatible
 */
bool display_integration_check_compatibility(const char *shell_version);

// ============================================================================
// UTILITY MACROS AND CONVENIENCE FUNCTIONS
// ============================================================================

/**
 * Convenience macro for safe function replacement.
 * Provides compile-time switching between original and integrated functions.
 */
#ifdef DISPLAY_INTEGRATION_ENABLED
    #define LUSUSH_SAFE_REDISPLAY() display_integration_redisplay()
    #define LUSUSH_PROMPT_UPDATE() display_integration_prompt_update()
    #define LUSUSH_CLEAR_SCREEN() display_integration_clear_screen()
#else
    #define LUSUSH_SAFE_REDISPLAY() lusush_safe_redisplay()
    #define LUSUSH_PROMPT_UPDATE() lusush_prompt_update()
    #define LUSUSH_CLEAR_SCREEN() lusush_clear_screen()
#endif

/**
 * Quick health check macro for conditional behavior.
 */
#define DISPLAY_INTEGRATION_IS_HEALTHY() \
    (display_integration_get_health() <= DISPLAY_INTEGRATION_HEALTH_GOOD)

/**
 * Debug output macro (only active in debug mode).
 */
#define DISPLAY_INTEGRATION_DEBUG(fmt, ...) \
    do { \
        display_integration_config_t config; \
        if (display_integration_get_config(&config) && config.debug_mode) { \
            fprintf(stderr, "display_integration: " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_INTEGRATION_H */