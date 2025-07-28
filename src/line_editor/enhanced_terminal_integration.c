/*
 * Enhanced Terminal Integration for Lusush Line Editor (LLE)
 * 
 * This module provides a simple wrapper interface for integrating the enhanced
 * terminal detection system with existing Lusush components. It allows gradual
 * adoption of enhanced detection without requiring major changes to existing code.
 *
 * Key features:
 * - Drop-in replacement for traditional isatty() checks
 * - Enhanced interactive mode detection for editor terminals
 * - Backwards compatible with existing shell logic
 * - Configurable integration levels
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "enhanced_terminal_integration.h"
#include "enhanced_terminal_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// Integration State Management
// ============================================================================

static bool g_integration_initialized = false;
static bool g_enhanced_mode_enabled = true;
static bool g_debug_mode = false;

/**
 * @brief Initialize enhanced terminal integration
 * 
 * @param enable_enhanced Whether to enable enhanced detection
 * @return true on success
 */
bool lle_enhanced_integration_init(bool enable_enhanced) {
    if (g_integration_initialized) {
        return true;
    }
    
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    g_debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (g_debug_mode) {
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Initializing enhanced terminal integration\n");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Enhanced mode: %s\n", enable_enhanced ? "enabled" : "disabled");
    }
    
    g_enhanced_mode_enabled = enable_enhanced;
    
    // Initialize enhanced detection if enabled
    if (g_enhanced_mode_enabled) {
        if (!lle_enhanced_terminal_detection_init()) {
            if (g_debug_mode) {
                fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Failed to initialize enhanced detection, falling back to traditional\n");
            }
            g_enhanced_mode_enabled = false;
        } else {
            if (g_debug_mode) {
                fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Enhanced detection initialized successfully\n");
                fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Terminal report:\n%s\n", 
                        lle_enhanced_terminal_get_debug_info());
            }
        }
    }
    
    g_integration_initialized = true;
    return true;
}

/**
 * @brief Clean up enhanced terminal integration
 */
void lle_enhanced_integration_cleanup(void) {
    if (!g_integration_initialized) {
        return;
    }
    
    if (g_enhanced_mode_enabled) {
        lle_enhanced_terminal_detection_cleanup();
    }
    
    g_integration_initialized = false;
    g_enhanced_mode_enabled = true;
    g_debug_mode = false;
}

// ============================================================================
// Enhanced Detection Wrapper Functions
// ============================================================================

/**
 * @brief Enhanced version of isatty() check for interactive detection
 * 
 * This function provides a drop-in replacement for traditional isatty() checks
 * that also considers enhanced terminal detection for editor terminals.
 * 
 * @return true if terminal should be treated as interactive
 */
bool lle_enhanced_is_interactive_terminal(void) {
    // Ensure initialization
    if (!g_integration_initialized) {
        lle_enhanced_integration_init(true);
    }
    
    // Traditional TTY check
    bool traditional_interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    
    // Enhanced detection if enabled
    bool enhanced_interactive = false;
    if (g_enhanced_mode_enabled) {
        enhanced_interactive = lle_enhanced_terminal_should_be_interactive();
    }
    
    // Combine results - use enhanced if available, fall back to traditional
    bool result = g_enhanced_mode_enabled ? enhanced_interactive : traditional_interactive;
    
    if (g_debug_mode && (result != traditional_interactive)) {
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Detection override:\n");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Traditional: %s\n", 
                traditional_interactive ? "interactive" : "non-interactive");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Enhanced:    %s\n", 
                enhanced_interactive ? "interactive" : "non-interactive");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Final:       %s\n", 
                result ? "interactive" : "non-interactive");
    }
    
    return result;
}

/**
 * @brief Check if terminal supports colors
 * 
 * @return true if color output is supported
 */
bool lle_enhanced_supports_colors(void) {
    if (!g_integration_initialized) {
        lle_enhanced_integration_init(true);
    }
    
    if (g_enhanced_mode_enabled) {
        return lle_enhanced_terminal_supports_colors();
    }
    
    // Fall back to basic environment check
    const char *term = getenv("TERM");
    const char *colorterm = getenv("COLORTERM");
    
    return (term && (strstr(term, "color") || strstr(term, "xterm") || strstr(term, "256"))) ||
           (colorterm && *colorterm);
}

/**
 * @brief Check if terminal supports advanced features
 * 
 * @return true if advanced features like truecolor are supported
 */
bool lle_enhanced_supports_advanced_features(void) {
    if (!g_integration_initialized) {
        lle_enhanced_integration_init(true);
    }
    
    if (g_enhanced_mode_enabled) {
        return lle_enhanced_terminal_supports_advanced_features();
    }
    
    // Fall back to basic check
    const char *colorterm = getenv("COLORTERM");
    return colorterm && (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit"));
}

/**
 * @brief Get terminal information summary
 * 
 * @return Static string with terminal information
 */
const char *lle_enhanced_get_terminal_summary(void) {
    static char summary[512];
    
    if (!g_integration_initialized) {
        lle_enhanced_integration_init(true);
    }
    
    if (g_enhanced_mode_enabled) {
        const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
        snprintf(summary, sizeof(summary),
                "Terminal: %s (%s), Interactive: %s, Colors: %s/%s/%s",
                info->terminal_signature ? info->terminal_signature->name : "unknown",
                info->term_program,
                lle_enhanced_is_interactive_terminal() ? "yes" : "no",
                info->supports_colors ? "basic" : "none",
                info->supports_256_colors ? "256" : "no",
                info->supports_truecolor ? "true" : "no");
    } else {
        const char *term = getenv("TERM");
        const char *term_program = getenv("TERM_PROGRAM");
        snprintf(summary, sizeof(summary),
                "Terminal: %s (%s), Interactive: %s (traditional detection)",
                term ? term : "unknown",
                term_program ? term_program : "unknown",
                (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) ? "yes" : "no");
    }
    
    return summary;
}

// ============================================================================
// Integration Helper Functions
// ============================================================================

/**
 * @brief Wrapper for shell interactive detection
 * 
 * This function can be used to replace traditional shell interactive detection
 * logic while maintaining backwards compatibility.
 * 
 * @param forced_interactive Whether interactive mode is forced via command line
 * @param has_script_file Whether a script file is being executed
 * @param stdin_mode Whether stdin mode is enabled
 * @return true if shell should run in interactive mode
 */
bool lle_enhanced_should_shell_be_interactive(bool forced_interactive,
                                             bool has_script_file,
                                             bool stdin_mode) {
    // Command-line script execution is never interactive
    if (has_script_file) {
        return false;
    }
    
    // Forced interactive mode always wins
    if (forced_interactive) {
        return true;
    }
    
    // stdin mode disables interactive features
    if (stdin_mode) {
        return false;
    }
    
    // Use enhanced detection for the final decision
    return lle_enhanced_is_interactive_terminal();
}

/**
 * @brief Check if LLE features should be enabled
 * 
 * This function determines whether advanced LLE features like syntax
 * highlighting and tab completion should be enabled based on terminal
 * capabilities and interactive mode.
 * 
 * @return true if LLE features should be enabled
 */
bool lle_enhanced_should_enable_lle_features(void) {
    if (!lle_enhanced_is_interactive_terminal()) {
        return false;
    }
    
    // Enable features if we have color support and interactive mode
    return lle_enhanced_supports_colors();
}

/**
 * @brief Get configuration recommendations for LLE
 * 
 * This function provides configuration recommendations based on detected
 * terminal capabilities.
 * 
 * @param config Pointer to configuration structure to populate
 */
void lle_enhanced_get_recommended_config(lle_enhanced_config_recommendation_t *config) {
    if (!config) return;
    
    // Initialize with defaults
    memset(config, 0, sizeof(lle_enhanced_config_recommendation_t));
    
    // Base recommendations on capabilities
    bool interactive = lle_enhanced_is_interactive_terminal();
    bool colors = lle_enhanced_supports_colors();
    bool advanced = lle_enhanced_supports_advanced_features();
    
    config->enable_lle = interactive;
    config->enable_syntax_highlighting = interactive && colors;
    config->enable_tab_completion = interactive;
    config->enable_history = interactive;
    config->enable_multiline = interactive;
    config->enable_undo = interactive;
    config->force_interactive_mode = interactive && !isatty(STDIN_FILENO);
    config->color_support_level = advanced ? 3 : (colors ? 1 : 0);
    
    if (g_debug_mode) {
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION] Configuration recommendations:\n");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Enable LLE: %s\n", config->enable_lle ? "yes" : "no");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Syntax highlighting: %s\n", config->enable_syntax_highlighting ? "yes" : "no");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Tab completion: %s\n", config->enable_tab_completion ? "yes" : "no");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Force interactive: %s\n", config->force_interactive_mode ? "yes" : "no");
        fprintf(stderr, "[LLE_ENHANCED_INTEGRATION]   Color level: %d\n", config->color_support_level);
    }
}

// ============================================================================
// Debugging and Testing Support
// ============================================================================

/**
 * @brief Print comprehensive integration debug information
 */
void lle_enhanced_integration_debug_print(void) {
    printf("Enhanced Terminal Integration Debug Report\n");
    printf("==========================================\n\n");
    
    printf("Integration Status:\n");
    printf("  Initialized: %s\n", g_integration_initialized ? "yes" : "no");
    printf("  Enhanced mode: %s\n", g_enhanced_mode_enabled ? "enabled" : "disabled");
    printf("  Debug mode: %s\n", g_debug_mode ? "enabled" : "disabled");
    
    printf("\nDetection Results:\n");
    printf("  Interactive (enhanced): %s\n", lle_enhanced_is_interactive_terminal() ? "yes" : "no");
    printf("  Interactive (traditional): %s\n", (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) ? "yes" : "no");
    printf("  Colors supported: %s\n", lle_enhanced_supports_colors() ? "yes" : "no");
    printf("  Advanced features: %s\n", lle_enhanced_supports_advanced_features() ? "yes" : "no");
    
    printf("\nTerminal Summary: %s\n", lle_enhanced_get_terminal_summary());
    
    if (g_enhanced_mode_enabled) {
        printf("\nDetailed Terminal Information:\n");
        printf("%s\n", lle_enhanced_terminal_get_debug_info());
    }
    
    printf("\nConfiguration Recommendations:\n");
    lle_enhanced_config_recommendation_t config;
    lle_enhanced_get_recommended_config(&config);
    printf("  Enable LLE: %s\n", config.enable_lle ? "yes" : "no");
    printf("  Syntax highlighting: %s\n", config.enable_syntax_highlighting ? "yes" : "no");
    printf("  Tab completion: %s\n", config.enable_tab_completion ? "yes" : "no");
    printf("  Force interactive: %s\n", config.force_interactive_mode ? "yes" : "no");
    printf("  Color support level: %d\n", config.color_support_level);
}

/**
 * @brief Test enhanced detection against traditional detection
 * 
 * @return true if enhanced detection differs from traditional
 */
bool lle_enhanced_integration_test_detection_differences(void) {
    if (!g_integration_initialized) {
        lle_enhanced_integration_init(true);
    }
    
    bool traditional = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    bool enhanced = lle_enhanced_is_interactive_terminal();
    
    return enhanced != traditional;
}