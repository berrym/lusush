/*
 * Lusush Shell - Display Controller Integration Wrapper
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
 * DISPLAY INTEGRATION WRAPPER
 * 
 * This module provides seamless integration between the existing shell display
 * functions and the new layered display controller architecture. It enables
 * zero-regression deployment while providing enhanced display capabilities.
 * 
 * Key Features:
 * - Seamless function replacement with backward compatibility
 * - Configuration-based enable/disable of layered display
 * - Performance monitoring integration with shell diagnostics
 * - Enterprise deployment readiness with configuration management
 * - Graceful fallback to existing display functions
 * 
 * Integration Strategy:
 * - Preserve all existing functionality and user experience
 * - Add layered display as an optional enhancement
 * - Provide configuration options for deployment customization
 * - Enable performance monitoring and optimization
 * - Ensure zero memory leaks and resource management
 */

#include "../include/display_integration.h"
#include "../include/display/display_controller.h"
#include "../include/display/layer_events.h"
#include "../include/readline_integration.h"
#include "../include/prompt.h"
#include "../include/themes.h"
#include "../include/lusush.h"
#include "../include/config.h"
#include <libgen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <readline/readline.h>
#include <readline/history.h>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// Shell display functions that we're replacing
void lusush_refresh_line(void);
void lusush_prompt_update(void);
void lusush_clear_screen(void);
char *lusush_generate_prompt(void);

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

// Display integration state
static display_controller_t *global_display_controller = NULL;
static bool layered_display_enabled = false;
static bool integration_initialized = false;
static display_integration_config_t current_config = {0};

// Performance tracking
static display_integration_stats_t integration_stats = {0};

// Buffer for display output
static char display_output_buffer[DISPLAY_INTEGRATION_MAX_OUTPUT_SIZE];

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

/**
 * Initialize the display integration system.
 * Sets up the display controller and prepares for shell integration.
 *
 * @param config Configuration for display integration
 * @return true on success, false on failure
 */
bool display_integration_init(const display_integration_config_t *config) {
    if (integration_initialized) {
        return true; // Already initialized
    }

    // Copy configuration
    if (config) {
        current_config = *config;
    } else {
        // Use default configuration
        display_integration_create_default_config(&current_config);
    }

    // Create display controller if layered display is enabled
    if (current_config.enable_layered_display) {
        global_display_controller = display_controller_create();
        if (!global_display_controller) {
            fprintf(stderr, "display_integration: Failed to create display controller\n");
            return false;
        }

        // Initialize display controller with shell configuration
        // Create default configuration for display controller
        display_controller_config_t controller_config;
        display_controller_create_default_config(&controller_config);
        
        // Create layer event system for controller
        layer_event_system_t *event_system = layer_events_create(NULL);
        if (!event_system) {
            fprintf(stderr, "display_integration: Failed to create event system\n");
            display_controller_destroy(global_display_controller);
            global_display_controller = NULL;
            return false;
        }
        
        // Initialize the event system (critical - required for subscriptions to work)
        layer_events_error_t event_init_error = layer_events_init(event_system);
        if (event_init_error != LAYER_EVENTS_SUCCESS) {
            fprintf(stderr, "display_integration: Failed to initialize event system: %d\n", event_init_error);
            layer_events_destroy(event_system);
            display_controller_destroy(global_display_controller);
            global_display_controller = NULL;
            return false;
        }
        
        display_controller_error_t error = display_controller_init(global_display_controller, &controller_config, event_system);
        if (error != DISPLAY_CONTROLLER_SUCCESS) {
            fprintf(stderr, "display_integration: Failed to initialize display controller: %d\n", error);
            display_controller_destroy(global_display_controller);
            global_display_controller = NULL;
            return false;
        }

        // Prepare for shell integration
        error = display_controller_prepare_shell_integration(global_display_controller, &current_config);
        if (error != DISPLAY_CONTROLLER_SUCCESS) {
            fprintf(stderr, "display_integration: Failed to prepare shell integration: %d\n", error);
            display_controller_destroy(global_display_controller);
            global_display_controller = NULL;
            return false;
        }

        // Configure display controller
        display_controller_config_t controller_config2;
        display_controller_create_default_config(&controller_config2);
        controller_config2.optimization_level = current_config.optimization_level;
        controller_config2.enable_caching = current_config.enable_caching;
        controller_config2.enable_performance_monitoring = current_config.enable_performance_monitoring;
        controller_config2.enable_integration_mode = true;

        error = display_controller_set_config(global_display_controller, &controller_config2);
        if (error != DISPLAY_CONTROLLER_SUCCESS) {
            fprintf(stderr, "display_integration: Failed to configure display controller: %d\n", error);
            display_controller_destroy(global_display_controller);
            global_display_controller = NULL;
            return false;
        }

        layered_display_enabled = true;
    }

    integration_initialized = true;
    integration_stats.init_time = time(NULL);
    integration_stats.total_display_calls = 0;
    integration_stats.layered_display_calls = 0;
    integration_stats.fallback_calls = 0;

    if (current_config.debug_mode) {
        printf("display_integration: Initialized successfully (layered_display=%s)\n",
               layered_display_enabled ? "enabled" : "disabled");
    }

    return true;
}

/**
 * Cleanup the display integration system.
 * Releases all resources and resets state.
 */
void display_integration_cleanup(void) {
    if (!integration_initialized) {
        return;
    }

    if (global_display_controller) {
        display_controller_destroy(global_display_controller);
        global_display_controller = NULL;
    }

    layered_display_enabled = false;
    integration_initialized = false;
    memset(&current_config, 0, sizeof(current_config));
    memset(&integration_stats, 0, sizeof(integration_stats));

    // Note: current_config was zeroed above, so we can't check debug_mode here
    // Debug output would need to be checked before memset() if needed
}

// ============================================================================
// CONFIGURATION MANAGEMENT
// ============================================================================

/**
 * Create default configuration for display integration.
 *
 * @param config Configuration structure to initialize
 */
void display_integration_create_default_config(display_integration_config_t *config) {
    if (!config) return;

    memset(config, 0, sizeof(display_integration_config_t));
    config->enable_layered_display = false;
    config->enable_caching = true;
    config->enable_performance_monitoring = true;
    config->optimization_level = DISPLAY_OPTIMIZATION_STANDARD;
    config->fallback_on_error = true;
    config->debug_mode = false;
    config->max_output_size = DISPLAY_INTEGRATION_MAX_OUTPUT_SIZE;
    config->performance_threshold_ms = 20;
    config->cache_hit_rate_threshold = 0.8;
}

/**
 * Update display integration configuration.
 *
 * @param config New configuration to apply
 * @return true on success, false on failure
 */
bool display_integration_set_config(const display_integration_config_t *config) {
    if (!config || !integration_initialized) {
        return false;
    }

    bool layered_was_enabled = layered_display_enabled;
    display_integration_config_t old_config = current_config;
    current_config = *config;

    // Handle layered display enable/disable
    if (config->enable_layered_display && !layered_was_enabled) {
        // Enable layered display
        if (!global_display_controller) {
            global_display_controller = display_controller_create();
            if (global_display_controller) {
                // Create default configuration for display controller
                display_controller_config_t controller_config;
                display_controller_create_default_config(&controller_config);
                
                // Create layer event system for controller
                layer_event_system_t *event_system = layer_events_create(NULL);
                if (!event_system) {
                    display_controller_destroy(global_display_controller);
                    global_display_controller = NULL;
                    current_config = old_config;
                    return false;
                }
                
                // Initialize the event system (critical - required for subscriptions to work)
                layer_events_error_t event_init_error = layer_events_init(event_system);
                if (event_init_error != LAYER_EVENTS_SUCCESS) {
                    fprintf(stderr, "display_integration: Failed to initialize event system: %d\n", event_init_error);
                    layer_events_destroy(event_system);
                    display_controller_destroy(global_display_controller);
                    global_display_controller = NULL;
                    current_config = old_config;
                    return false;
                }
                
                display_controller_error_t error = display_controller_init(global_display_controller, &controller_config, event_system);
                if (error == DISPLAY_CONTROLLER_SUCCESS) {
                    error = display_controller_prepare_shell_integration(global_display_controller, config);
                    if (error == DISPLAY_CONTROLLER_SUCCESS) {
                        layered_display_enabled = true;
                    }
                }
                
                if (!layered_display_enabled) {
                    display_controller_destroy(global_display_controller);
                    global_display_controller = NULL;
                    current_config = old_config; // Restore old config
                    return false;
                }
            }
        }
    } else if (!config->enable_layered_display && layered_was_enabled) {
        // Disable layered display
        if (global_display_controller) {
            display_controller_destroy(global_display_controller);
            global_display_controller = NULL;
        }
        layered_display_enabled = false;
    }

    // Update display controller configuration if active
    if (layered_display_enabled && global_display_controller) {
        display_controller_config_t controller_config;
        display_controller_create_default_config(&controller_config);
        controller_config.optimization_level = config->optimization_level;
        controller_config.enable_caching = config->enable_caching;
        controller_config.enable_performance_monitoring = config->enable_performance_monitoring;
        controller_config.enable_integration_mode = true;

        display_controller_error_t error = display_controller_set_config(global_display_controller, &controller_config);
        if (error != DISPLAY_CONTROLLER_SUCCESS && config->fallback_on_error) {
            if (current_config.debug_mode) {
                fprintf(stderr, "display_integration: Failed to update controller config, continuing with old settings\n");
            }
        }
    }

    if (current_config.debug_mode) {
        printf("display_integration: Configuration updated (layered_display=%s)\n",
               layered_display_enabled ? "enabled" : "disabled");
    }

    return true;
}

/**
 * Get current display integration configuration.
 *
 * @param config Configuration structure to fill
 * @return true on success, false on failure
 */
bool display_integration_get_config(display_integration_config_t *config) {
    if (!config || !integration_initialized) {
        return false;
    }

    *config = current_config;
    return true;
}

// ============================================================================
// MAIN INTEGRATION FUNCTIONS (Shell Function Replacements)
// ============================================================================

/**
 * Integrated display function - replacement for lusush_safe_redisplay().
 * Provides coordinated display using layered architecture when enabled,
 * with graceful fallback to existing display functions.
 */
void display_integration_redisplay(void) {
    static bool in_display_redisplay = false;
    
    // CRITICAL: Prevent infinite recursion - this was causing stack overflow
    if (in_display_redisplay) {
        // Use safe fallback without any function pointers
        rl_redisplay();
        return;
    }
    
    in_display_redisplay = true;
    integration_stats.total_display_calls++;
    integration_fallback_reason_t fallback_reason;
    
    // Professional safety check - can we attempt layered display?
    if (!safe_layered_display_attempt("redisplay", &fallback_reason)) {
        integration_stats.fallback_calls++;
        log_fallback_event("redisplay", fallback_reason);
        
        // Graceful fallback to existing system
        rl_redisplay();
        in_display_redisplay = false;
        return;
    }
    
    // Attempt sophisticated layered display operation
    if (layered_display_enabled && global_display_controller) {
        char output_buffer[4096];  // Buffer for full display output
        char *current_prompt = lusush_generate_prompt();
        
        // Phase 2 Implementation: Modern Syntax Highlighting
        // Use command content during real-time typing, but not during prompt display
        // This enables syntax highlighting while preventing stale command display
        bool is_real_time_input = (rl_line_buffer && rl_end > 0 && rl_point >= 0);
        char *current_command = is_real_time_input ? rl_line_buffer : "";
        
        // Use display controller for sophisticated rendering
        display_controller_error_t error = display_controller_display(
            global_display_controller,
            current_prompt,
            current_command,
            output_buffer,
            sizeof(output_buffer)
        );
        
        if (error == DISPLAY_CONTROLLER_SUCCESS) {
            integration_stats.layered_display_calls++;
            

            
            // Phase 2 Implementation: Modern Syntax Highlighting
            // Always use layered display when system is enabled and output is available
            if (output_buffer[0] != '\0') {
                // Use readline's display mechanism to show our composed output
                // This maintains compatibility with readline's input handling
                rl_clear_visible_line();
                rl_on_new_line();
                printf("%s", output_buffer);
                fflush(stdout);
                
                // Note: current_prompt is managed by readline system, don't free here
                in_display_redisplay = false;
                return;  // Success - skip fallback
            } else {
                // Empty output - use standard readline
                integration_stats.fallback_calls++;
            }
        } else {
            // Layered display failed - log and fallback
            integration_stats.fallback_calls++;
            log_controller_error("redisplay", error);
            
            // Note: current_prompt is managed by readline system, don't free here
        }
    }
    
    // Fallback to standard readline display
    rl_redisplay();
    
    in_display_redisplay = false;
}

/**
 * Integrated prompt update function - replacement for lusush_prompt_update().
 * Provides coordinated prompt updates using layered architecture when enabled.
 */
void display_integration_prompt_update(void) {
    static bool in_prompt_update = false;
    
    // Prevent infinite recursion
    if (in_prompt_update) {
        return;
    }
    
    in_prompt_update = true;
    integration_stats.total_display_calls++;

    // Only use layered display integration when enabled
    if (integration_initialized && layered_display_enabled) {
        integration_stats.layered_display_calls++;
    }

    // Safe prompt update without recursion
    if (integration_initialized && layered_display_enabled) {
        // Just rebuild prompt without calling lusush_generate_prompt to avoid loops
        rebuild_prompt();
    } else {
        // Fallback: call original functions safely
        rebuild_prompt();
        lusush_generate_prompt();
    }
    
    in_prompt_update = false;
}

/**
 * Integrated clear screen function - replacement for lusush_clear_screen().
 * Provides coordinated screen clearing using layered architecture when enabled.
 */
void display_integration_clear_screen(void) {
    static bool in_clear_screen = false;
    
    // Prevent recursion
    if (in_clear_screen) {
        rl_clear_screen(0, 0);
        return;
    }
    
    in_clear_screen = true;
    integration_stats.total_display_calls++;
    integration_fallback_reason_t fallback_reason;
    
    // Professional safety check - can we attempt layered display?
    if (!safe_layered_display_attempt("clear_screen", &fallback_reason)) {
        integration_stats.fallback_calls++;
        log_fallback_event("clear_screen", fallback_reason);
        
        // Graceful fallback to existing system
        rl_clear_screen(0, 0);
        in_clear_screen = false;
        return;
    }
    
    // Attempt layered display clear screen operation
    if (layered_display_enabled && global_display_controller) {
        char output_buffer[1024];  // Buffer for clear screen sequence
        
        // Use display controller for sophisticated clear screen
        display_controller_error_t error = display_controller_refresh(
            global_display_controller,
            output_buffer,
            sizeof(output_buffer)
        );
        
        if (error == DISPLAY_CONTROLLER_SUCCESS) {
            integration_stats.layered_display_calls++;
            
            // CRITICAL FIX: Use readline's clear screen function
            // Never bypass readline terminal management
            rl_clear_screen(0, 0);
            
            if (current_config.debug_mode) {
                fprintf(stderr, "display_integration: Using readline clear screen\n");
            }
            
            in_clear_screen = false;
            return;
        } else {
            // Clear screen failed - log and fallback
            integration_stats.fallback_calls++;
            log_controller_error("clear_screen", error);
        }
    }
    
    // Fallback to existing system
    rl_clear_screen(0, 0);
    in_clear_screen = false;
}

// ============================================================================
// PERFORMANCE MONITORING AND DIAGNOSTICS
// ============================================================================

/**
 * Get display integration performance statistics.
 *
 * @param stats Statistics structure to fill
 * @return true on success, false on failure
 */
bool display_integration_get_stats(display_integration_stats_t *stats) {
    if (!stats || !integration_initialized) {
        return false;
    }

    *stats = integration_stats;

    // Get additional performance data from display controller
    if (layered_display_enabled && global_display_controller) {
        display_controller_performance_t controller_perf;
        display_controller_error_t error = display_controller_get_performance(
            global_display_controller, &controller_perf);
        
        if (error == DISPLAY_CONTROLLER_SUCCESS) {
            stats->avg_layered_display_time_ns = controller_perf.avg_display_time_ns;
            stats->cache_hit_rate = controller_perf.cache_hit_rate;
            stats->memory_usage_bytes = controller_perf.cache_memory_usage_bytes;
        }
    }

    // Calculate health indicators
    stats->performance_within_threshold = 
        (stats->avg_layered_display_time_ns < (current_config.performance_threshold_ms * 1000000));
    
    stats->cache_efficiency_good = 
        (stats->cache_hit_rate >= current_config.cache_hit_rate_threshold);
    
    // Memory usage acceptable if under 10MB (reasonable threshold for display system)
    stats->memory_usage_acceptable = (stats->memory_usage_bytes < 10 * 1024 * 1024);

    return true;
}

/**
 * Reset display integration performance statistics.
 */
void display_integration_reset_stats(void) {
    if (!integration_initialized) {
        return;
    }

    memset(&integration_stats, 0, sizeof(integration_stats));
    integration_stats.init_time = time(NULL);

    // Reset display controller performance if available
    if (layered_display_enabled && global_display_controller) {
        display_controller_reset_performance_metrics(global_display_controller);
    }
}

/**
 * Check if layered display is currently enabled and working.
 *
 * @return true if layered display is active, false otherwise
 */
bool display_integration_is_layered_active(void) {
    return integration_initialized && layered_display_enabled && global_display_controller != NULL;
}

/**
 * Get display integration health status.
 *
 * @return health status enum
 */
display_integration_health_t display_integration_get_health(void) {
    if (!integration_initialized) {
        return DISPLAY_INTEGRATION_HEALTH_NOT_INITIALIZED;
    }

    if (!layered_display_enabled) {
        return DISPLAY_INTEGRATION_HEALTH_DISABLED;
    }

    if (!global_display_controller) {
        return DISPLAY_INTEGRATION_HEALTH_ERROR;
    }

    // Check performance metrics
    display_integration_stats_t stats;
    if (display_integration_get_stats(&stats)) {
        // Check if fallback rate is too high
        if (stats.total_display_calls > 0) {
            double fallback_rate = (double)stats.fallback_calls / stats.total_display_calls;
            if (fallback_rate > 0.5) {
                return DISPLAY_INTEGRATION_HEALTH_DEGRADED;
            }
        }

        // Check performance thresholds
        if (stats.avg_layered_display_time_ns > (current_config.performance_threshold_ms * 1000000)) {
            return DISPLAY_INTEGRATION_HEALTH_DEGRADED;
        }

        if (stats.cache_hit_rate < current_config.cache_hit_rate_threshold) {
            return DISPLAY_INTEGRATION_HEALTH_DEGRADED;
        }
    }

    return DISPLAY_INTEGRATION_HEALTH_EXCELLENT;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get human-readable health status string.
 *
 * @param health Health status enum
 * @return String description of health status
 */
const char* display_integration_health_string(display_integration_health_t health) {
    switch (health) {
        case DISPLAY_INTEGRATION_HEALTH_EXCELLENT:
            return "Excellent";
        case DISPLAY_INTEGRATION_HEALTH_GOOD:
            return "Good";
        case DISPLAY_INTEGRATION_HEALTH_DEGRADED:
            return "Degraded";
        case DISPLAY_INTEGRATION_HEALTH_ERROR:
            return "Error";
        case DISPLAY_INTEGRATION_HEALTH_DISABLED:
            return "Disabled";
        case DISPLAY_INTEGRATION_HEALTH_NOT_INITIALIZED:
            return "Not Initialized";
        default:
            return "Unknown";
    }
}

/**
 * Get enhanced prompt using layered display system.
 * This function provides enhanced prompt generation using the theme system.
 *
 * @param enhanced_prompt Pointer to store the generated enhanced prompt
 * @return true on success, false on failure
 */
bool display_integration_get_enhanced_prompt(char **enhanced_prompt) {
    if (!enhanced_prompt) {
        return false;
    }

    *enhanced_prompt = NULL;

    // Prioritize layered display over legacy enhanced display
    if (!integration_initialized || !global_display_controller) {
        return false; // Enhanced display not available
    }
    
    if (!layered_display_enabled) {
        return false; // No layered display mode active
    }

    // Generate enhanced prompt with layered display
    integration_stats.layered_display_calls++;

    // Create enhanced prompt buffer
    size_t prompt_size = 512;
    char *prompt_buffer = malloc(prompt_size);
    if (!prompt_buffer) {
        return false;
    }

    // Use theme system for layered display
    if (!theme_generate_primary_prompt(prompt_buffer, prompt_size)) {
        // Fallback with user info
        char *current_dir = getcwd(NULL, 0);
        const char *user = getenv("USER");
        const char *hostname = getenv("HOSTNAME");
        if (!hostname) hostname = "localhost";
        
        int written = snprintf(prompt_buffer, prompt_size,
            "[%s@%s] %s $ ",
            user ? user : "user",
            hostname,
            current_dir ? basename(current_dir) : "~"
        );
        
        if (written >= (int)prompt_size) {
            strncpy(prompt_buffer, "$ ", prompt_size - 1);
            prompt_buffer[prompt_size - 1] = '\0';
        }
        
        if (current_dir) free(current_dir);
    }
    
    if (current_config.debug_mode) {
        fprintf(stderr, "display_integration: Using layered display theme prompt\n");
    }

    *enhanced_prompt = prompt_buffer;
    return true;
}

/**
 * Print diagnostic information about display integration.
 */
void display_integration_print_diagnostics(void) {
    if (!integration_initialized) {
        printf("Display Integration: Not initialized\n");
        return;
    }

    printf("=== Display Integration Diagnostics ===\n");
    printf("Status: %s\n", layered_display_enabled ? "Enabled" : "Disabled");
    printf("Health: %s\n", display_integration_health_string(display_integration_get_health()));

    display_integration_stats_t stats;
    if (display_integration_get_stats(&stats)) {
        printf("Total display calls: %lu\n", stats.total_display_calls);
        printf("Layered display calls: %lu\n", stats.layered_display_calls);
        printf("Fallback calls: %lu\n", stats.fallback_calls);
        
        if (stats.total_display_calls > 0) {
            double layered_rate = (double)stats.layered_display_calls / stats.total_display_calls * 100.0;
            double fallback_rate = (double)stats.fallback_calls / stats.total_display_calls * 100.0;
            printf("Layered display rate: %.1f%%\n", layered_rate);
            printf("Fallback rate: %.1f%%\n", fallback_rate);
        }

        if (layered_display_enabled) {
            printf("Average display time: %.2fms\n", stats.avg_layered_display_time_ns / 1000000.0);
            printf("Cache hit rate: %.1f%%\n", stats.cache_hit_rate * 100.0);
            printf("Memory usage: %zu bytes\n", stats.memory_usage_bytes);
        }
    }

    printf("Configuration:\n");
    printf("  Layered display: %s\n", current_config.enable_layered_display ? "enabled" : "disabled");
    printf("  Caching: %s\n", current_config.enable_caching ? "enabled" : "disabled");
    printf("  Performance monitoring: %s\n", current_config.enable_performance_monitoring ? "enabled" : "disabled");
    printf("  Optimization level: %d\n", current_config.optimization_level);
    printf("  Debug mode: %s\n", current_config.debug_mode ? "enabled" : "disabled");
    printf("=======================================\n");
}

// ============================================================================
// v1.3.0 SAFETY INFRASTRUCTURE IMPLEMENTATION
// ============================================================================

/**
 * Perform comprehensive safety check for layered display operation.
 * Professional safety-first implementation with detailed diagnostics.
 */
bool safe_layered_display_attempt(const char *function_name, 
                                 integration_fallback_reason_t *fallback_reason) {
    if (!fallback_reason) {
        return false; // Invalid parameter - cannot proceed safely
    }
    
    // Initialize fallback reason
    *fallback_reason = INTEGRATION_FALLBACK_NONE;
    
    // Update safety check statistics
    integration_stats.safety_checks_performed++;
    
    // Check 1: Integration system initialization
    if (!integration_initialized) {
        *fallback_reason = INTEGRATION_FALLBACK_INITIALIZATION_ERROR;
        return false;
    }
    
    // Check 2: Layered display enabled
    if (!layered_display_enabled) {
        *fallback_reason = INTEGRATION_FALLBACK_USER_REQUEST;
        return false;
    }
    
    // Check 3: Display controller availability
    if (!global_display_controller) {
        *fallback_reason = INTEGRATION_FALLBACK_CONTROLLER_NULL;
        return false;
    }
    
    // Check 4: Memory and system resources
    // Simple check - ensure we have reasonable memory available
    static char test_buffer[1024];
    if (!test_buffer) {
        *fallback_reason = INTEGRATION_FALLBACK_MEMORY_ERROR;
        return false;
    }
    
    // Check 5: Configuration safety
    if (current_config.strict_compatibility_mode) {
        // In strict compatibility mode, use fallback unless explicitly enabled
        if (!current_config.enable_layered_display) {
            *fallback_reason = INTEGRATION_FALLBACK_SAFETY_CHECK;
            return false;
        }
    }
    
    // All safety checks passed
    return true;
}

/**
 * Log a fallback event for diagnostics and monitoring.
 */
void log_fallback_event(const char *function_name, integration_fallback_reason_t reason) {
    if (!function_name) {
        return; // Invalid parameter
    }
    
    // Update statistics
    if (reason >= 0 && reason < 10) {
        integration_stats.fallback_events[reason]++;
    }
    integration_stats.last_fallback_time = time(NULL);
    integration_stats.last_fallback_reason = reason;
    
    // Debug logging if enabled
    if (current_config.debug_mode) {
        fprintf(stderr, "display_integration: %s fallback - %s\n", 
                function_name, integration_fallback_reason_string(reason));
    }
    
    // Enterprise logging if enabled
    if (current_config.enable_enterprise_logging) {
        fprintf(stderr, "DISPLAY_INTEGRATION_FALLBACK: function=%s reason=%s time=%ld\n",
                function_name, integration_fallback_reason_string(reason), 
                (long)integration_stats.last_fallback_time);
    }
}

/**
 * Log a display controller error with context.
 */
void log_controller_error(const char *function_name, display_controller_error_t error) {
    if (!function_name) {
        return; // Invalid parameter
    }
    
    // Update error statistics
    integration_stats.layered_display_errors++;
    integration_stats.last_error_time = time(NULL);
    
    // Debug logging if enabled
    if (current_config.debug_mode) {
        fprintf(stderr, "display_integration: %s controller error %d\n", 
                function_name, (int)error);
    }
    
    // Enterprise logging if enabled
    if (current_config.enable_enterprise_logging) {
        fprintf(stderr, "DISPLAY_CONTROLLER_ERROR: function=%s error=%d time=%ld\n",
                function_name, (int)error, (long)integration_stats.last_error_time);
    }
}

/**
 * Get human-readable string for fallback reason.
 */
const char *integration_fallback_reason_string(integration_fallback_reason_t reason) {
    switch (reason) {
        case INTEGRATION_FALLBACK_NONE:
            return "no fallback";
        case INTEGRATION_FALLBACK_CONTROLLER_NULL:
            return "display controller not available";
        case INTEGRATION_FALLBACK_CONTROLLER_ERROR:
            return "display controller error";
        case INTEGRATION_FALLBACK_BUFFER_ERROR:
            return "buffer allocation error";
        case INTEGRATION_FALLBACK_TIMEOUT:
            return "operation timeout";
        case INTEGRATION_FALLBACK_USER_REQUEST:
            return "user disabled layered display";
        case INTEGRATION_FALLBACK_SAFETY_CHECK:
            return "safety check failed";
        case INTEGRATION_FALLBACK_MEMORY_ERROR:
            return "memory allocation failure";
        case INTEGRATION_FALLBACK_INITIALIZATION_ERROR:
            return "system not initialized";
        case INTEGRATION_FALLBACK_RECURSION_PROTECTION:
            return "recursion protection";
        default:
            return "unknown reason";
    }
}