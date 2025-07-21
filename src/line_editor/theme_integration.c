/**
 * @file theme_integration.c
 * @brief Basic theme application implementation for Lusush Line Editor (LLE)
 * 
 * This module implements the theme integration system that connects LLE with
 * the Lusush theme system, providing seamless theme application to line editor
 * components including prompts, input text, cursor, and syntax highlighting.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "theme_integration.h"
#include "prompt.h"
#include "terminal_manager.h"
#include "termcap/lle_termcap.h"
#include "../../include/themes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// Internal Constants and Structures
// ============================================================================

/**
 * @brief Default theme mappings from LLE elements to Lusush colors
 */
static const lle_theme_mapping_t default_mappings[] = {
    {LLE_THEME_PROMPT_PRIMARY,    "primary",     "\033[36m", true},
    {LLE_THEME_PROMPT_SECONDARY,  "secondary",   "\033[37m", false},
    {LLE_THEME_INPUT_TEXT,        "text",        "\033[0m",  true},
    {LLE_THEME_INPUT_BACKGROUND,  "background",  "\033[40m", false},
    {LLE_THEME_CURSOR_NORMAL,     "highlight",   "\033[7m",  false},
    {LLE_THEME_CURSOR_INSERT,     "success",     "\033[5m",  false},
    {LLE_THEME_CURSOR_REPLACE,    "warning",     "\033[4m",  false},
    {LLE_THEME_SELECTION,         "highlight",   "\033[44m", false},
    {LLE_THEME_SELECTION_BG,      "background",  "\033[104m", false},
    {LLE_THEME_COMPLETION_MATCH,  "success",     "\033[32m", false},
    {LLE_THEME_COMPLETION_BG,     "background",  "\033[42m", false},
    {LLE_THEME_SYNTAX_KEYWORD,    "info",        "\033[34m", false},
    {LLE_THEME_SYNTAX_STRING,     "success",     "\033[33m", false},
    {LLE_THEME_SYNTAX_COMMENT,    "text_dim",    "\033[90m", false},
    {LLE_THEME_SYNTAX_OPERATOR,   "secondary",   "\033[35m", false},
    {LLE_THEME_SYNTAX_VARIABLE,   "primary",     "\033[36m", false},
    {LLE_THEME_ERROR_HIGHLIGHT,   "error",       "\033[41m", true},
    {LLE_THEME_WARNING_HIGHLIGHT, "warning",     "\033[43m", true}
};

#define DEFAULT_MAPPINGS_COUNT (sizeof(default_mappings) / sizeof(default_mappings[0]))

/**
 * @brief Element names for string conversion
 */
static const char *element_names[] = {
    "prompt_primary",
    "prompt_secondary", 
    "input_text",
    "input_background",
    "cursor_normal",
    "cursor_insert",
    "cursor_replace",
    "selection",
    "selection_bg",
    "completion_match",
    "completion_bg",
    "syntax_keyword",
    "syntax_string",
    "syntax_comment",
    "syntax_operator",
    "syntax_variable",
    "error_highlight",
    "warning_highlight"
};

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * Load fallback colors into LLE color structure
 */
static bool load_fallback_colors(lle_theme_integration_t *ti) {
    if (!ti) {
        return false;
    }

    // Load fallback colors based on default mappings
    for (size_t i = 0; i < DEFAULT_MAPPINGS_COUNT; i++) {
        const lle_theme_mapping_t *mapping = &default_mappings[i];
        
        // Get pointer to the appropriate color field
        char *color_field = NULL;
        switch (mapping->lle_element) {
            case LLE_THEME_PROMPT_PRIMARY:    color_field = ti->colors.prompt_primary; break;
            case LLE_THEME_PROMPT_SECONDARY:  color_field = ti->colors.prompt_secondary; break;
            case LLE_THEME_INPUT_TEXT:        color_field = ti->colors.input_text; break;
            case LLE_THEME_INPUT_BACKGROUND:  color_field = ti->colors.input_background; break;
            case LLE_THEME_CURSOR_NORMAL:     color_field = ti->colors.cursor_normal; break;
            case LLE_THEME_CURSOR_INSERT:     color_field = ti->colors.cursor_insert; break;
            case LLE_THEME_CURSOR_REPLACE:    color_field = ti->colors.cursor_replace; break;
            case LLE_THEME_SELECTION:         color_field = ti->colors.selection; break;
            case LLE_THEME_SELECTION_BG:      color_field = ti->colors.selection_bg; break;
            case LLE_THEME_COMPLETION_MATCH:  color_field = ti->colors.completion_match; break;
            case LLE_THEME_COMPLETION_BG:     color_field = ti->colors.completion_bg; break;
            case LLE_THEME_SYNTAX_KEYWORD:    color_field = ti->colors.syntax_keyword; break;
            case LLE_THEME_SYNTAX_STRING:     color_field = ti->colors.syntax_string; break;
            case LLE_THEME_SYNTAX_COMMENT:    color_field = ti->colors.syntax_comment; break;
            case LLE_THEME_SYNTAX_OPERATOR:   color_field = ti->colors.syntax_operator; break;
            case LLE_THEME_SYNTAX_VARIABLE:   color_field = ti->colors.syntax_variable; break;
            case LLE_THEME_ERROR_HIGHLIGHT:   color_field = ti->colors.error_highlight; break;
            case LLE_THEME_WARNING_HIGHLIGHT: color_field = ti->colors.warning_highlight; break;
            default: continue;
        }
        
        if (color_field) {
            strncpy(color_field, mapping->fallback_color, LLE_THEME_COLOR_MAX - 1);
            color_field[LLE_THEME_COLOR_MAX - 1] = '\0';
        }
    }
    
    return true;
}

/**
 * Update cache and performance counters
 */
static void update_cache(lle_theme_integration_t *ti, const char *element, const char *color) {
    if (!ti || !element || !color) return;
    
    ti->color_requests++;
    
    // Check if this is a cache hit
    if (strcmp(ti->last_element, element) == 0) {
        ti->cache_hits++;
        return;
    }
    
    // Update cache
    strncpy(ti->last_element, element, LLE_THEME_ELEMENT_MAX - 1);
    ti->last_element[LLE_THEME_ELEMENT_MAX - 1] = '\0';
    
    strncpy(ti->last_color, color, LLE_THEME_COLOR_MAX - 1);
    ti->last_color[LLE_THEME_COLOR_MAX - 1] = '\0';
}

// ============================================================================
// Core Theme Integration Functions
// ============================================================================

bool lle_theme_init(lle_theme_integration_t *ti) {
    if (!ti) return false;
    
    // Initialize structure
    memset(ti, 0, sizeof(lle_theme_integration_t));
    
    // Initialize basic state
    ti->theme_active = false;
    ti->colors_cached = false;
    ti->dirty = true;
    ti->debug_mode = false;
    ti->color_requests = 0;
    ti->cache_hits = 0;
    
    // Initialize cache
    ti->last_element[0] = '\0';
    ti->last_color[0] = '\0';
    
    // Detect terminal capabilities
    if (!lle_theme_detect_capabilities(ti)) {
        if (ti->debug_mode) {
            fprintf(stderr, "LLE Theme: Warning - could not detect terminal capabilities\n");
        }
        // Continue with basic capabilities
        ti->color_support = 8;
        ti->supports_basic_color = true;
        ti->supports_256_color = false;
        ti->supports_true_color = false;
    }
    
    // For standalone LLE, use fallback colors
    strcpy(ti->theme_name, "lle_fallback");
    if (load_fallback_colors(ti)) {
        ti->colors_cached = true;
        ti->dirty = false;
        ti->theme_active = true;
        if (ti->debug_mode) {
            printf("LLE Theme: Initialized with fallback colors\n");
        }
    } else {
        if (ti->debug_mode) {
            fprintf(stderr, "LLE Theme: Failed to load fallback colors\n");
        }
        return false;
    }
    
    return true;
}

void lle_theme_cleanup(lle_theme_integration_t *ti) {
    if (!ti) return;
    
    if (ti->debug_mode) {
        printf("LLE Theme: Cleanup - %zu requests, %zu cache hits (%.1f%%)\n",
               ti->color_requests, ti->cache_hits,
               ti->color_requests > 0 ? (100.0 * ti->cache_hits / ti->color_requests) : 0.0);
    }
    
    // Clear all fields
    memset(ti, 0, sizeof(lle_theme_integration_t));
}

bool lle_theme_sync(lle_theme_integration_t *ti) {
    if (!ti) return false;
    
    // For standalone LLE, sync means refresh fallback colors
    ti->lusush_theme = NULL;
    ti->color_scheme = NULL;
    
    // Load fallback colors
    if (!load_fallback_colors(ti)) {
        ti->theme_active = false;
        return false;
    }
    
    ti->colors_cached = true;
    ti->dirty = false;
    ti->theme_active = true;
    
    if (ti->debug_mode) {
        printf("LLE Theme: Synced with fallback colors\n");
    }
    
    return true;
}

bool lle_theme_apply(lle_theme_integration_t *ti, const char *theme_name) {
    if (!ti || !theme_name) return false;
    
    // For standalone LLE, just update theme name and sync
    strncpy(ti->theme_name, theme_name, sizeof(ti->theme_name) - 1);
    ti->theme_name[sizeof(ti->theme_name) - 1] = '\0';
    
    // Sync with fallback colors
    if (!lle_theme_sync(ti)) {
        if (ti->debug_mode) {
            fprintf(stderr, "LLE Theme: Failed to sync after setting theme '%s'\n", theme_name);
        }
        return false;
    }
    
    if (ti->debug_mode) {
        printf("LLE Theme: Applied theme '%s' (using fallback colors)\n", theme_name);
    }
    
    return true;
}

// ============================================================================
// Color Access Functions
// ============================================================================

const char *lle_theme_get_color(lle_theme_integration_t *ti, lle_theme_element_t element) {
    if (!ti || element >= LLE_THEME_ELEMENT_COUNT) return "";
    
    // Check cache first
    const char *element_name = lle_theme_element_to_string(element);
    if (element_name && strcmp(ti->last_element, element_name) == 0) {
        ti->color_requests++;
        ti->cache_hits++;
        return ti->last_color;
    }
    
    // Get color based on element
    const char *color = "";
    switch (element) {
        case LLE_THEME_PROMPT_PRIMARY:    color = ti->colors.prompt_primary; break;
        case LLE_THEME_PROMPT_SECONDARY:  color = ti->colors.prompt_secondary; break;
        case LLE_THEME_INPUT_TEXT:        color = ti->colors.input_text; break;
        case LLE_THEME_INPUT_BACKGROUND:  color = ti->colors.input_background; break;
        case LLE_THEME_CURSOR_NORMAL:     color = ti->colors.cursor_normal; break;
        case LLE_THEME_CURSOR_INSERT:     color = ti->colors.cursor_insert; break;
        case LLE_THEME_CURSOR_REPLACE:    color = ti->colors.cursor_replace; break;
        case LLE_THEME_SELECTION:         color = ti->colors.selection; break;
        case LLE_THEME_SELECTION_BG:      color = ti->colors.selection_bg; break;
        case LLE_THEME_COMPLETION_MATCH:  color = ti->colors.completion_match; break;
        case LLE_THEME_COMPLETION_BG:     color = ti->colors.completion_bg; break;
        case LLE_THEME_SYNTAX_KEYWORD:    color = ti->colors.syntax_keyword; break;
        case LLE_THEME_SYNTAX_STRING:     color = ti->colors.syntax_string; break;
        case LLE_THEME_SYNTAX_COMMENT:    color = ti->colors.syntax_comment; break;
        case LLE_THEME_SYNTAX_OPERATOR:   color = ti->colors.syntax_operator; break;
        case LLE_THEME_SYNTAX_VARIABLE:   color = ti->colors.syntax_variable; break;
        case LLE_THEME_ERROR_HIGHLIGHT:   color = ti->colors.error_highlight; break;
        case LLE_THEME_WARNING_HIGHLIGHT: color = ti->colors.warning_highlight; break;
        default: color = "";
    }
    
    // Update cache
    if (element_name) {
        update_cache(ti, element_name, color);
    }
    
    return color;
}

const char *lle_theme_get_color_by_name(lle_theme_integration_t *ti, const char *element_name) {
    if (!ti || !element_name) return "";
    
    // Check cache first
    if (strcmp(ti->last_element, element_name) == 0) {
        ti->color_requests++;
        ti->cache_hits++;
        return ti->last_color;
    }
    
    // Convert name to element and get color
    lle_theme_element_t element = lle_theme_string_to_element(element_name);
    if (element == LLE_THEME_ELEMENT_COUNT) {
        return "";
    }
    
    return lle_theme_get_color(ti, element);
}

bool lle_theme_supports_element(lle_theme_integration_t *ti, lle_theme_element_t element) {
    if (!ti || element >= LLE_THEME_ELEMENT_COUNT) return false;
    
    const char *color = lle_theme_get_color(ti, element);
    return color && strlen(color) > 0;
}

const char *lle_theme_get_fallback_color(lle_theme_integration_t *ti, lle_theme_element_t element) {
    if (!ti) return "\033[0m";
    
    // Return ultimate fallback for invalid elements
    if (element >= LLE_THEME_ELEMENT_COUNT) return "\033[0m";
    
    // Find fallback color in default mappings
    for (size_t i = 0; i < DEFAULT_MAPPINGS_COUNT; i++) {
        if (default_mappings[i].lle_element == element) {
            return default_mappings[i].fallback_color;
        }
    }
    
    return "\033[0m"; // Reset as ultimate fallback
}

// ============================================================================
// Theme Validation and Capabilities
// ============================================================================

bool lle_theme_validate_compatibility(lle_theme_integration_t *ti, const char *theme_name) {
    if (!ti || !theme_name) return false;
    
    // For standalone LLE, all themes are compatible with fallbacks
    if (ti->debug_mode) {
        printf("LLE Theme: Theme '%s' is compatible (using fallbacks)\n", theme_name);
    }
    
    return true;
}

bool lle_theme_detect_capabilities(lle_theme_integration_t *ti) {
    if (!ti) return false;
    
    // Standalone capability detection based on COLORTERM and TERM
    const char *colorterm = getenv("COLORTERM");
    const char *term = getenv("TERM");
    
    int color_support = 8; // Default basic color support
    
    // Check for true color support
    if (colorterm && (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit"))) {
        color_support = 16777216;
    }
    // Check for 256 color support
    else if (term && strstr(term, "256")) {
        color_support = 256;
    }
    // Check for xterm-like terminals
    else if (term && strstr(term, "xterm")) {
        color_support = 256;
    }
    
    ti->color_support = color_support;
    ti->supports_true_color = (color_support >= 16777216);
    ti->supports_256_color = (color_support >= 256);
    ti->supports_basic_color = (color_support >= 8);
    
    if (ti->debug_mode) {
        printf("LLE Theme: Detected %d color support (true:%s, 256:%s, basic:%s)\n",
               color_support,
               ti->supports_true_color ? "yes" : "no",
               ti->supports_256_color ? "yes" : "no", 
               ti->supports_basic_color ? "yes" : "no");
    }
    
    return true;
}

bool lle_theme_check_requirements(lle_theme_integration_t *ti, const char *theme_name) {
    if (!ti || !theme_name) return false;
    
    // For standalone LLE, all requirements are met with fallbacks
    if (ti->debug_mode) {
        printf("LLE Theme: Theme '%s' requirements met (using fallbacks)\n", theme_name);
    }
    
    return true;
}

// ============================================================================
// Utility and Debug Functions
// ============================================================================

bool lle_theme_refresh(lle_theme_integration_t *ti) {
    if (!ti) return false;
    
    ti->dirty = true;
    ti->colors_cached = false;
    
    // Clear cache
    ti->last_element[0] = '\0';
    ti->last_color[0] = '\0';
    
    // For standalone LLE, always reload fallback colors
    if (load_fallback_colors(ti)) {
        ti->colors_cached = true;
        ti->dirty = false;
        return true;
    }
    
    return false;
}

void lle_theme_get_stats(lle_theme_integration_t *ti, size_t *color_requests, 
                         size_t *cache_hits, double *cache_ratio) {
    if (!ti) return;
    
    if (color_requests) *color_requests = ti->color_requests;
    if (cache_hits) *cache_hits = ti->cache_hits;
    if (cache_ratio) {
        *cache_ratio = ti->color_requests > 0 ? 
                       (double)ti->cache_hits / ti->color_requests : 0.0;
    }
}

void lle_theme_set_debug(lle_theme_integration_t *ti, bool enable) {
    if (!ti) return;
    
    ti->debug_mode = enable;
    
    if (enable) {
        printf("LLE Theme: Debug mode enabled\n");
    }
}

const char *lle_theme_get_active_name(lle_theme_integration_t *ti) {
    if (!ti || !ti->theme_active) return NULL;
    
    return ti->theme_name;
}

bool lle_theme_is_active(lle_theme_integration_t *ti) {
    return ti && ti->theme_active;
}

// ============================================================================
// Element Name Mapping
// ============================================================================

const char *lle_theme_element_to_string(lle_theme_element_t element) {
    if (element >= LLE_THEME_ELEMENT_COUNT) return NULL;
    
    return element_names[element];
}

lle_theme_element_t lle_theme_string_to_element(const char *element_name) {
    if (!element_name) return LLE_THEME_ELEMENT_COUNT;
    
    for (size_t i = 0; i < LLE_THEME_ELEMENT_COUNT; i++) {
        if (strcmp(element_names[i], element_name) == 0) {
            return (lle_theme_element_t)i;
        }
    }
    
    return LLE_THEME_ELEMENT_COUNT;
}

// ============================================================================
// Integration Points with Lusush Theme System
// ============================================================================

const char *lle_theme_map_to_lusush_color(lle_theme_element_t element) {
    if (element >= LLE_THEME_ELEMENT_COUNT) return NULL;
    
    for (size_t i = 0; i < DEFAULT_MAPPINGS_COUNT; i++) {
        if (default_mappings[i].lle_element == element) {
            return default_mappings[i].lusush_color_name;
        }
    }
    
    return NULL;
}

const lle_theme_mapping_t *lle_theme_get_default_mappings(size_t *count) {
    if (count) *count = DEFAULT_MAPPINGS_COUNT;
    return default_mappings;
}