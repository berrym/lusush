/*
 * Enhanced Terminal Integration Header for Lusush Line Editor (LLE)
 * 
 * This header provides a simple wrapper interface for integrating enhanced
 * terminal detection with existing Lusush components. It allows gradual
 * adoption of enhanced detection without requiring major changes to existing code.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#ifndef LLE_ENHANCED_TERMINAL_INTEGRATION_H
#define LLE_ENHANCED_TERMINAL_INTEGRATION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration Recommendation Structure
// ============================================================================

/**
 * @brief Configuration recommendations based on terminal capabilities
 */
typedef struct {
    bool enable_lle;                    /**< Whether LLE should be enabled */
    bool enable_syntax_highlighting;    /**< Whether syntax highlighting should be enabled */
    bool enable_tab_completion;         /**< Whether tab completion should be enabled */
    bool enable_history;                /**< Whether history should be enabled */
    bool enable_multiline;              /**< Whether multiline editing should be enabled */
    bool enable_undo;                   /**< Whether undo/redo should be enabled */
    bool force_interactive_mode;        /**< Whether to force interactive mode */
    int color_support_level;            /**< Color support level (0=none, 1=basic, 2=256, 3=truecolor) */
} lle_enhanced_config_recommendation_t;

// ============================================================================
// Core Integration Functions
// ============================================================================

/**
 * @brief Initialize enhanced terminal integration
 * 
 * This function initializes the enhanced terminal detection system and
 * configures it for integration with existing shell components.
 * 
 * @param enable_enhanced Whether to enable enhanced detection (true recommended)
 * @return true on successful initialization
 * 
 * @note This function automatically detects debug mode from LLE_DEBUG environment
 *       variable and provides comprehensive debug output when enabled.
 */
bool lle_enhanced_integration_init(bool enable_enhanced);

/**
 * @brief Clean up enhanced terminal integration
 * 
 * Releases all resources and resets integration state. The system can be
 * re-initialized after cleanup.
 */
void lle_enhanced_integration_cleanup(void);

// ============================================================================
// Enhanced Detection Wrapper Functions
// ============================================================================

/**
 * @brief Enhanced version of isatty() check for interactive detection
 * 
 * This function provides a drop-in replacement for traditional isatty() checks
 * that also considers enhanced terminal detection for editor terminals like
 * Zed, VS Code, and others.
 * 
 * @return true if terminal should be treated as interactive
 * 
 * @note This may return true even when isatty(STDIN_FILENO) returns false
 *       for capable editor-embedded terminals.
 */
bool lle_enhanced_is_interactive_terminal(void);

/**
 * @brief Check if terminal supports colors
 * 
 * Uses enhanced detection when available, falls back to environment variable
 * checks for traditional terminals.
 * 
 * @return true if color output is supported
 */
bool lle_enhanced_supports_colors(void);

/**
 * @brief Check if terminal supports advanced features
 * 
 * Advanced features include truecolor, cursor queries, mouse support,
 * and other modern terminal capabilities.
 * 
 * @return true if advanced features are supported
 */
bool lle_enhanced_supports_advanced_features(void);

/**
 * @brief Get terminal information summary
 * 
 * Returns a human-readable summary of detected terminal information
 * including terminal type, capabilities, and interactive status.
 * 
 * @return Pointer to static string with terminal summary
 * 
 * @note The returned string is updated on each call and should be
 *       copied if persistence is needed.
 */
const char *lle_enhanced_get_terminal_summary(void);

// ============================================================================
// Shell Integration Helper Functions
// ============================================================================

/**
 * @brief Wrapper for shell interactive detection
 * 
 * This function can be used to replace traditional shell interactive detection
 * logic while maintaining backwards compatibility. It considers command-line
 * options, script execution, and enhanced terminal capabilities.
 * 
 * @param forced_interactive Whether interactive mode is forced via -i flag
 * @param has_script_file Whether a script file is being executed
 * @param stdin_mode Whether stdin mode (-s flag) is enabled
 * @return true if shell should run in interactive mode
 * 
 * @note This function provides the logic for replacing traditional shell
 *       interactive detection with enhanced capabilities.
 */
bool lle_enhanced_should_shell_be_interactive(bool forced_interactive,
                                             bool has_script_file,
                                             bool stdin_mode);

/**
 * @brief Check if LLE features should be enabled
 * 
 * This function determines whether advanced LLE features like syntax
 * highlighting and tab completion should be enabled based on terminal
 * capabilities and interactive mode detection.
 * 
 * @return true if LLE features should be enabled
 */
bool lle_enhanced_should_enable_lle_features(void);

/**
 * @brief Get configuration recommendations for LLE
 * 
 * This function provides configuration recommendations based on detected
 * terminal capabilities. Use this to automatically configure LLE features
 * for optimal user experience.
 * 
 * @param config Pointer to configuration structure to populate
 * 
 * @note The recommendations consider terminal capabilities, interactive mode,
 *       and performance characteristics to provide optimal settings.
 */
void lle_enhanced_get_recommended_config(lle_enhanced_config_recommendation_t *config);

// ============================================================================
// Debugging and Testing Support
// ============================================================================

/**
 * @brief Print comprehensive integration debug information
 * 
 * Outputs detailed information about integration status, detection results,
 * terminal capabilities, and configuration recommendations. Useful for
 * debugging terminal detection issues.
 */
void lle_enhanced_integration_debug_print(void);

/**
 * @brief Test enhanced detection against traditional detection
 * 
 * This function compares enhanced detection results with traditional isatty()
 * based detection to identify cases where enhanced detection provides different
 * (and presumably better) results.
 * 
 * @return true if enhanced detection differs from traditional detection
 * 
 * @note A return value of true indicates that enhanced detection is providing
 *       value by detecting interactive capability that traditional methods miss.
 */
bool lle_enhanced_integration_test_detection_differences(void);

// ============================================================================
// Convenience Macros for Easy Adoption
// ============================================================================

/**
 * @brief Drop-in replacement for isatty() checks in shell code
 */
#define LLE_IS_INTERACTIVE() lle_enhanced_is_interactive_terminal()

/**
 * @brief Check if colors should be used in output
 */
#define LLE_USE_COLORS() lle_enhanced_supports_colors()

/**
 * @brief Check if advanced terminal features are available
 */
#define LLE_HAS_ADVANCED_FEATURES() lle_enhanced_supports_advanced_features()

/**
 * @brief Get quick terminal info for debug output
 */
#define LLE_TERMINAL_INFO() lle_enhanced_get_terminal_summary()

// ============================================================================
// Integration Patterns
// ============================================================================

/**
 * @brief Example integration pattern for shell initialization
 * 
 * Replace traditional shell interactive detection:
 * 
 * // OLD CODE:
 * // bool interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
 * 
 * // NEW CODE:
 * lle_enhanced_integration_init(true);
 * bool interactive = lle_enhanced_should_shell_be_interactive(
 *     forced_interactive, has_script_file, stdin_mode);
 * 
 * // Configure LLE based on capabilities:
 * lle_enhanced_config_recommendation_t config;
 * lle_enhanced_get_recommended_config(&config);
 * 
 * if (config.enable_lle) {
 *     // Initialize LLE with recommended settings
 * }
 */

/**
 * @brief Example integration pattern for LLE configuration
 * 
 * Configure LLE based on terminal capabilities:
 * 
 * lle_enhanced_config_recommendation_t rec;
 * lle_enhanced_get_recommended_config(&rec);
 * 
 * lle_config_t lle_config = {
 *     .enable_syntax_highlighting = rec.enable_syntax_highlighting,
 *     .enable_auto_completion = rec.enable_tab_completion,
 *     .enable_history = rec.enable_history,
 *     .enable_multiline = rec.enable_multiline,
 *     .enable_undo = rec.enable_undo
 * };
 */

#ifdef __cplusplus
}
#endif

#endif /* LLE_ENHANCED_TERMINAL_INTEGRATION_H */