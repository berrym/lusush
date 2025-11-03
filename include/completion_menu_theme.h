/*
 * Completion Menu Theme Integration (Phase 3)
 * 
 * Integrates the interactive completion menu with Lusush's theme system.
 * Provides themed colors, symbols, and formatting for completion display.
 */

#ifndef COMPLETION_MENU_THEME_H
#define COMPLETION_MENU_THEME_H

#include "completion_types.h"
#include "completion_menu.h"
#include "themes.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// THEME-AWARE CONFIGURATION
// ============================================================================

/**
 * Completion menu theme colors
 * Maps completion types to theme semantic colors
 */
typedef struct {
    char category_header[COLOR_CODE_MAX];  // Category header color
    char category_border[COLOR_CODE_MAX];  // Category border color
    char item_selected[COLOR_CODE_MAX];    // Selected item highlight
    char item_normal[COLOR_CODE_MAX];      // Normal item text
    char item_dimmed[COLOR_CODE_MAX];      // Dimmed/secondary text
    char type_builtin[COLOR_CODE_MAX];     // Builtin command color
    char type_command[COLOR_CODE_MAX];     // External command color
    char type_file[COLOR_CODE_MAX];        // File color
    char type_directory[COLOR_CODE_MAX];   // Directory color
    char type_variable[COLOR_CODE_MAX];    // Variable color
    char type_alias[COLOR_CODE_MAX];       // Alias color
    char type_history[COLOR_CODE_MAX];     // History color
    char scroll_indicator[COLOR_CODE_MAX]; // Scroll indicator color
} completion_menu_colors_t;

/**
 * Symbol set for completion menu display
 */
typedef struct {
    const char *builtin_indicator;      // Builtin symbol (⚙ or *)
    const char *command_indicator;      // Command symbol (⚡ or >)
    const char *file_indicator;         // File symbol (📄 or f)
    const char *directory_indicator;    // Directory symbol (📁 or d)
    const char *variable_indicator;     // Variable symbol ($ or $)
    const char *alias_indicator;        // Alias symbol (@ or @)
    const char *history_indicator;      // History symbol (🕐 or h)
    const char *header_left;            // Header left bracket (--- or [)
    const char *header_right;           // Header right bracket (--- or ])
    const char *scroll_up;              // Scroll up indicator (^ or ^)
    const char *scroll_down;            // Scroll down indicator (v or v)
    const char *selection_prefix;       // Selection prefix (> or >)
} completion_menu_symbols_t;

/**
 * Complete theme configuration for menu
 */
typedef struct {
    completion_menu_colors_t colors;
    completion_menu_symbols_t symbols;
    bool use_colors;                    // Enable/disable colors
    bool use_unicode;                   // Enable/disable Unicode symbols
    bool use_bold;                      // Enable bold text
    bool use_underline;                 // Enable underlines
    bool compact_mode;                  // Compact display mode
} completion_menu_theme_config_t;

// ============================================================================
// THEME INTEGRATION API
// ============================================================================

/**
 * Initialize menu theme system
 * Loads colors and symbols from active theme
 * @return true on success, false on failure
 */
bool completion_menu_theme_init(void);

/**
 * Get themed configuration for menu
 * Automatically syncs with active Lusush theme
 * @return pointer to theme config, NULL on failure
 */
const completion_menu_theme_config_t* completion_menu_theme_get_config(void);

/**
 * Update theme configuration from active theme
 * Call when theme changes to refresh menu appearance
 * @return true on success, false on failure
 */
bool completion_menu_theme_update(void);

/**
 * Get color for completion type
 * @param type Completion type
 * @return ANSI color code
 */
const char* completion_menu_theme_get_type_color(completion_type_t type);

/**
 * Get symbol for completion type
 * @param type Completion type
 * @return symbol string
 */
const char* completion_menu_theme_get_type_symbol(completion_type_t type);

/**
 * Get category header color
 * @return ANSI color code
 */
const char* completion_menu_theme_get_header_color(void);

/**
 * Get selection highlight color
 * @return ANSI color code
 */
const char* completion_menu_theme_get_selection_color(void);

// ============================================================================
// THEMED DISPLAY FUNCTIONS
// ============================================================================

/**
 * Display menu with theme integration
 * Uses colors and symbols from active theme
 * @param menu Menu to display
 * @return true on success, false on failure
 */
bool completion_menu_display_themed(completion_menu_t *menu);

/**
 * Format item with theme colors and symbols
 * @param item Item to format
 * @param is_selected Whether item is selected
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return number of characters written
 */
size_t completion_menu_format_item_themed(const completion_item_t *item,
                                          bool is_selected,
                                          char *buffer,
                                          size_t buffer_size);

/**
 * Format category header with theme
 * @param category Category type
 * @param count Item count in category
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return number of characters written
 */
size_t completion_menu_format_header_themed(completion_type_t category,
                                            size_t count,
                                            char *buffer,
                                            size_t buffer_size);

/**
 * Format scroll indicator with theme
 * @param direction 1 for down, -1 for up
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return number of characters written
 */
size_t completion_menu_format_scroll_indicator_themed(int direction,
                                                      char *buffer,
                                                      size_t buffer_size);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Check if terminal supports required theme features
 * @return true if supported, false otherwise
 */
bool completion_menu_theme_is_supported(void);

/**
 * Get fallback theme configuration
 * For use when theme system is unavailable
 * @return fallback configuration
 */
completion_menu_theme_config_t completion_menu_theme_get_fallback(void);

/**
 * Cleanup theme resources
 */
void completion_menu_theme_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* COMPLETION_MENU_THEME_H */
