/**
 * @file theme_integration.h
 * @brief Theme integration interface for Lusush Line Editor (LLE)
 * 
 * This module provides the interface between LLE and the Lusush theme system,
 * enabling seamless theme application to line editor components including
 * prompts, input text, cursor, and syntax highlighting.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef LLE_THEME_INTEGRATION_H
#define LLE_THEME_INTEGRATION_H

#include <stdbool.h>
#include <stddef.h>
#include "themes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length for theme element names
 */
#define LLE_THEME_ELEMENT_MAX 64

/**
 * @brief Maximum length for color codes
 */
#define LLE_THEME_COLOR_MAX 32

/**
 * @brief Line editor specific theme colors
 * 
 * These colors are specifically used by the line editor for various
 * visual elements during input editing and display.
 */
typedef struct {
    char prompt_primary[LLE_THEME_COLOR_MAX];    // Primary prompt color
    char prompt_secondary[LLE_THEME_COLOR_MAX];  // Secondary prompt elements
    char input_text[LLE_THEME_COLOR_MAX];        // User input text
    char input_background[LLE_THEME_COLOR_MAX];  // Input background
    char cursor_normal[LLE_THEME_COLOR_MAX];     // Normal cursor
    char cursor_insert[LLE_THEME_COLOR_MAX];     // Insert mode cursor
    char cursor_replace[LLE_THEME_COLOR_MAX];    // Replace mode cursor
    char selection[LLE_THEME_COLOR_MAX];         // Selected text
    char selection_bg[LLE_THEME_COLOR_MAX];      // Selection background
    char completion_match[LLE_THEME_COLOR_MAX];  // Completion matches
    char completion_bg[LLE_THEME_COLOR_MAX];     // Completion background
    char syntax_keyword[LLE_THEME_COLOR_MAX];    // Syntax: keywords
    char syntax_string[LLE_THEME_COLOR_MAX];     // Syntax: strings
    char syntax_comment[LLE_THEME_COLOR_MAX];    // Syntax: comments
    char syntax_operator[LLE_THEME_COLOR_MAX];   // Syntax: operators
    char syntax_variable[LLE_THEME_COLOR_MAX];   // Syntax: variables
    char error_highlight[LLE_THEME_COLOR_MAX];   // Error highlighting
    char warning_highlight[LLE_THEME_COLOR_MAX]; // Warning highlighting
} lle_theme_colors_t;

/**
 * @brief Theme integration state and configuration
 * 
 * Maintains the current theme state, cached colors, and integration
 * status with the main Lusush theme system.
 */
typedef struct {
    // Theme identification
    char theme_name[64];                    // Active theme name
    bool theme_active;                      // Theme system active
    bool colors_cached;                     // Colors are cached and valid
    
    // Color definitions
    lle_theme_colors_t colors;              // LLE-specific colors
    
    // Integration with Lusush theme system
    theme_definition_t *lusush_theme; // Pointer to active Lusush theme
    color_scheme_t *color_scheme;     // Pointer to color scheme
    
    // Terminal capabilities
    int color_support;                      // Terminal color support level
    bool supports_true_color;              // 24-bit color support
    bool supports_256_color;               // 256 color support
    bool supports_basic_color;             // 8/16 color support
    
    // Performance optimization
    bool dirty;                             // Colors need refresh
    char last_element[LLE_THEME_ELEMENT_MAX]; // Last requested element
    char last_color[LLE_THEME_COLOR_MAX];     // Last returned color
    
    // Debug and diagnostics
    bool debug_mode;                        // Enable debug output
    size_t color_requests;                  // Number of color requests
    size_t cache_hits;                      // Number of cache hits
} lle_theme_integration_t;

/**
 * @brief Theme element identifiers for LLE components
 * 
 * These constants identify specific visual elements within the line
 * editor that can be themed independently.
 */
typedef enum {
    LLE_THEME_PROMPT_PRIMARY = 0,
    LLE_THEME_PROMPT_SECONDARY,
    LLE_THEME_INPUT_TEXT,
    LLE_THEME_INPUT_BACKGROUND,
    LLE_THEME_CURSOR_NORMAL,
    LLE_THEME_CURSOR_INSERT,
    LLE_THEME_CURSOR_REPLACE,
    LLE_THEME_SELECTION,
    LLE_THEME_SELECTION_BG,
    LLE_THEME_COMPLETION_MATCH,
    LLE_THEME_COMPLETION_BG,
    LLE_THEME_SYNTAX_KEYWORD,
    LLE_THEME_SYNTAX_STRING,
    LLE_THEME_SYNTAX_COMMENT,
    LLE_THEME_SYNTAX_OPERATOR,
    LLE_THEME_SYNTAX_VARIABLE,
    LLE_THEME_ERROR_HIGHLIGHT,
    LLE_THEME_WARNING_HIGHLIGHT,
    LLE_THEME_ELEMENT_COUNT
} lle_theme_element_t;

/**
 * @brief Theme mapping configuration
 * 
 * Defines how LLE theme elements map to Lusush theme colors,
 * allowing flexible theme integration and fallback behavior.
 */
typedef struct {
    lle_theme_element_t lle_element;        // LLE element identifier
    const char *lusush_color_name;          // Corresponding Lusush color
    const char *fallback_color;            // Fallback ANSI color code
    bool required;                          // Must be present in theme
} lle_theme_mapping_t;

// ============================================================================
// Core Theme Integration Functions
// ============================================================================

/**
 * Initialize theme integration system
 * 
 * Sets up the theme integration system, detects terminal capabilities,
 * and establishes connection with the main Lusush theme system.
 * 
 * @param ti Theme integration structure to initialize
 * @return true on success, false on error
 * 
 * @note Must be called before any other theme functions
 */
bool lle_theme_init(lle_theme_integration_t *ti);

/**
 * Cleanup theme integration resources
 * 
 * Releases all resources allocated by the theme integration system.
 * 
 * @param ti Theme integration structure to cleanup
 * 
 * @note Should be called when shutting down LLE
 */
void lle_theme_cleanup(lle_theme_integration_t *ti);

/**
 * Sync with active Lusush theme
 * 
 * Synchronizes LLE theme colors with the currently active Lusush theme,
 * mapping theme colors to line editor visual elements.
 * 
 * @param ti Theme integration structure
 * @return true on success, false on error
 * 
 * @note Call when Lusush theme changes or on startup
 */
bool lle_theme_sync(lle_theme_integration_t *ti);

/**
 * Apply specific theme by name
 * 
 * Activates a specific theme and updates all LLE color mappings.
 * 
 * @param ti Theme integration structure
 * @param theme_name Name of theme to activate
 * @return true on success, false if theme not found
 */
bool lle_theme_apply(lle_theme_integration_t *ti, const char *theme_name);

// ============================================================================
// Color Access Functions
// ============================================================================

/**
 * Get color for specific LLE element
 * 
 * Retrieves the ANSI color code for a specific line editor visual element.
 * Uses caching for performance optimization.
 * 
 * @param ti Theme integration structure
 * @param element LLE element identifier
 * @return ANSI color code string, empty string on error
 * 
 * @note Returned pointer is valid until next theme operation
 */
const char *lle_theme_get_color(lle_theme_integration_t *ti, lle_theme_element_t element);

/**
 * Get color by element name
 * 
 * Retrieves color using string-based element identification.
 * 
 * @param ti Theme integration structure
 * @param element_name String name of element (e.g., "prompt_primary")
 * @return ANSI color code string, empty string on error
 */
const char *lle_theme_get_color_by_name(lle_theme_integration_t *ti, const char *element_name);

/**
 * Check if theme supports specific element
 * 
 * Determines if the current theme provides a color for the specified element.
 * 
 * @param ti Theme integration structure
 * @param element LLE element identifier
 * @return true if element is supported, false otherwise
 */
bool lle_theme_supports_element(lle_theme_integration_t *ti, lle_theme_element_t element);

/**
 * Get fallback color for element
 * 
 * Returns a suitable fallback color when theme doesn't provide
 * a specific color for an element.
 * 
 * @param ti Theme integration structure
 * @param element LLE element identifier
 * @return Fallback ANSI color code
 */
const char *lle_theme_get_fallback_color(lle_theme_integration_t *ti, lle_theme_element_t element);

// ============================================================================
// Theme Validation and Capabilities
// ============================================================================

/**
 * Validate theme compatibility
 * 
 * Checks if a theme is compatible with current terminal capabilities
 * and LLE requirements.
 * 
 * @param ti Theme integration structure
 * @param theme_name Theme name to validate
 * @return true if compatible, false otherwise
 */
bool lle_theme_validate_compatibility(lle_theme_integration_t *ti, const char *theme_name);

/**
 * Detect terminal color capabilities
 * 
 * Analyzes terminal capabilities and updates integration state.
 * 
 * @param ti Theme integration structure
 * @return true on successful detection, false on error
 */
bool lle_theme_detect_capabilities(lle_theme_integration_t *ti);

/**
 * Check if theme requires specific features
 * 
 * Determines if theme requires features like true color, powerline fonts, etc.
 * 
 * @param ti Theme integration structure
 * @param theme_name Theme to check
 * @return true if requirements are met, false otherwise
 */
bool lle_theme_check_requirements(lle_theme_integration_t *ti, const char *theme_name);

// ============================================================================
// Utility and Debug Functions
// ============================================================================

/**
 * Refresh theme colors
 * 
 * Forces a refresh of all cached theme colors from the Lusush theme system.
 * 
 * @param ti Theme integration structure
 * @return true on success, false on error
 */
bool lle_theme_refresh(lle_theme_integration_t *ti);

/**
 * Get theme statistics
 * 
 * Retrieves performance and usage statistics for theme integration.
 * 
 * @param ti Theme integration structure
 * @param color_requests Output: number of color requests
 * @param cache_hits Output: number of cache hits
 * @param cache_ratio Output: cache hit ratio (0.0-1.0)
 */
void lle_theme_get_stats(lle_theme_integration_t *ti, size_t *color_requests, 
                         size_t *cache_hits, double *cache_ratio);

/**
 * Enable or disable debug mode
 * 
 * Controls debug output for theme integration operations.
 * 
 * @param ti Theme integration structure
 * @param enable true to enable debug mode, false to disable
 */
void lle_theme_set_debug(lle_theme_integration_t *ti, bool enable);

/**
 * Get active theme name
 * 
 * Returns the name of the currently active theme.
 * 
 * @param ti Theme integration structure
 * @return Theme name string, NULL if no theme active
 */
const char *lle_theme_get_active_name(lle_theme_integration_t *ti);

/**
 * Check if theme system is active
 * 
 * Determines if theme integration is active and functional.
 * 
 * @param ti Theme integration structure
 * @return true if active, false if disabled or failed
 */
bool lle_theme_is_active(lle_theme_integration_t *ti);

// ============================================================================
// Element Name Mapping
// ============================================================================

/**
 * Convert element enum to string name
 * 
 * @param element LLE element identifier
 * @return String name of element, NULL for invalid element
 */
const char *lle_theme_element_to_string(lle_theme_element_t element);

/**
 * Convert string name to element enum
 * 
 * @param element_name String name of element
 * @return LLE element identifier, LLE_THEME_ELEMENT_COUNT for invalid name
 */
lle_theme_element_t lle_theme_string_to_element(const char *element_name);

// ============================================================================
// Integration Points with Lusush Theme System
// ============================================================================

/**
 * Map LLE element to Lusush color name
 * 
 * Provides mapping between LLE visual elements and Lusush theme color names.
 * 
 * @param element LLE element identifier
 * @return Lusush color name, NULL if no mapping exists
 */
const char *lle_theme_map_to_lusush_color(lle_theme_element_t element);

/**
 * Get default theme mappings
 * 
 * Returns the default mapping configuration between LLE elements
 * and Lusush theme colors.
 * 
 * @param count Output: number of mappings returned
 * @return Array of theme mappings, NULL on error
 */
const lle_theme_mapping_t *lle_theme_get_default_mappings(size_t *count);

#ifdef __cplusplus
}
#endif

#endif // LLE_THEME_INTEGRATION_H