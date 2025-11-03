/*
 * Lusush - Interactive Completion Menu System
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
 */

/**
 * completion_menu.h - Interactive Completion Menu System
 *
 * Provides interactive completion menu with:
 * - Arrow key navigation (up/down/left/right)
 * - Visual selection highlighting
 * - Category grouping and headers
 * - Type indicators for each completion
 * - Scrolling for long completion lists
 *
 * Part of pragmatic completion system (Phase 2 - Interactive Menu).
 */

#ifndef COMPLETION_MENU_H
#define COMPLETION_MENU_H

#include "completion_types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MENU CONFIGURATION
// ============================================================================

/**
 * Menu display configuration
 */
typedef struct completion_menu_config {
    size_t max_visible_items;     // Maximum items visible at once
    bool show_category_headers;   // Show category headers (COMMANDS, FILES, etc.)
    bool show_type_indicators;    // Show type indicators (⚙ ⚡ etc.)
    bool show_descriptions;       // Show item descriptions
    bool enable_scrolling;        // Enable scrolling for long lists
    bool highlight_selection;     // Highlight selected item
    int min_items_for_menu;       // Minimum items before showing menu (default: 2)
} completion_menu_config_t;

// ============================================================================
// MENU STATE
// ============================================================================

/**
 * Menu navigation state
 */
typedef struct completion_menu {
    completion_result_t *result;  // Completion result with classified items
    
    // Navigation state
    size_t selected_index;        // Currently selected item (global index)
    size_t first_visible;         // First visible item (for scrolling)
    size_t visible_count;         // Number of visible items
    
    // Category navigation
    size_t current_category;      // Current category being viewed
    size_t *category_positions;   // Start position of each category
    size_t category_count;        // Number of categories present
    
    // Display state
    bool menu_active;             // Whether menu is currently displayed
    int terminal_width;           // Terminal width for display
    int terminal_height;          // Terminal height for display
    
    // Configuration
    completion_menu_config_t config;  // Menu display configuration
} completion_menu_t;

/**
 * Menu navigation direction
 */
typedef enum {
    MENU_NAV_UP,                  // Move selection up
    MENU_NAV_DOWN,                // Move selection down
    MENU_NAV_LEFT,                // Move to previous category
    MENU_NAV_RIGHT,               // Move to next category
    MENU_NAV_PAGE_UP,             // Page up
    MENU_NAV_PAGE_DOWN,           // Page down
    MENU_NAV_HOME,                // First item
    MENU_NAV_END                  // Last item
} menu_nav_direction_t;

// ============================================================================
// MENU LIFECYCLE
// ============================================================================

/**
 * Create a new completion menu
 * @param result completion result with classified items
 * @param config menu configuration (NULL for defaults)
 * @return newly allocated menu (must be freed with completion_menu_free)
 */
completion_menu_t* completion_menu_create(completion_result_t *result,
                                         completion_menu_config_t *config);

/**
 * Get default menu configuration
 * @return default configuration
 */
completion_menu_config_t completion_menu_get_default_config(void);

/**
 * Free a completion menu
 * Note: Does not free the result, caller retains ownership
 * @param menu menu to free
 */
void completion_menu_free(completion_menu_t *menu);

// ============================================================================
// MENU DISPLAY
// ============================================================================

/**
 * Display the completion menu
 * Renders menu to stdout with ANSI escape sequences
 * @param menu menu to display
 * @return true on success, false on error
 */
bool completion_menu_display(completion_menu_t *menu);

/**
 * Clear the completion menu from display
 * @param menu menu to clear
 */
void completion_menu_clear(completion_menu_t *menu);

/**
 * Refresh the menu display
 * Call after navigation to update visual state
 * @param menu menu to refresh
 * @return true on success, false on error
 */
bool completion_menu_refresh(completion_menu_t *menu);

// ============================================================================
// MENU NAVIGATION
// ============================================================================

/**
 * Navigate the menu
 * @param menu menu to navigate
 * @param direction navigation direction
 * @return true if navigation succeeded, false if at boundary
 */
bool completion_menu_navigate(completion_menu_t *menu, 
                             menu_nav_direction_t direction);

/**
 * Navigate to specific index
 * @param menu menu to navigate
 * @param index target index
 * @return true on success, false on invalid index
 */
bool completion_menu_goto(completion_menu_t *menu, size_t index);

/**
 * Navigate to specific category
 * @param menu menu to navigate
 * @param category category type to navigate to
 * @return true on success, false if category not found
 */
bool completion_menu_goto_category(completion_menu_t *menu,
                                  completion_type_t category);

// ============================================================================
// MENU QUERIES
// ============================================================================

/**
 * Get currently selected completion item
 * @param menu menu to query
 * @return selected item, or NULL if none selected
 */
const completion_item_t* completion_menu_get_selected(const completion_menu_t *menu);

/**
 * Get selected item text
 * @param menu menu to query
 * @return selected item text, or NULL if none selected
 */
const char* completion_menu_get_selected_text(const completion_menu_t *menu);

/**
 * Get number of visible items
 * @param menu menu to query
 * @return number of visible items
 */
size_t completion_menu_get_visible_count(const completion_menu_t *menu);

/**
 * Check if menu has multiple categories
 * @param menu menu to check
 * @return true if menu has 2+ categories
 */
bool completion_menu_has_categories(const completion_menu_t *menu);

/**
 * Get current category
 * @param menu menu to query
 * @return current category type
 */
completion_type_t completion_menu_get_current_category(const completion_menu_t *menu);

// ============================================================================
// MENU FORMATTING
// ============================================================================

/**
 * Format a completion item for display
 * @param item item to format
 * @param is_selected whether item is currently selected
 * @param show_indicator whether to show type indicator
 * @param buffer output buffer
 * @param buffer_size size of output buffer
 * @return number of characters written (excluding null terminator)
 */
size_t completion_menu_format_item(const completion_item_t *item,
                                  bool is_selected,
                                  bool show_indicator,
                                  char *buffer,
                                  size_t buffer_size);

/**
 * Format a category header
 * @param category category type
 * @param count number of items in category
 * @param buffer output buffer
 * @param buffer_size size of output buffer
 * @return number of characters written
 */
size_t completion_menu_format_header(completion_type_t category,
                                    size_t count,
                                    char *buffer,
                                    size_t buffer_size);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Update terminal dimensions
 * Call when terminal size changes
 * @param menu menu to update
 */
void completion_menu_update_dimensions(completion_menu_t *menu);

/**
 * Check if menu should be displayed
 * @param result completion result
 * @param min_items minimum items required
 * @return true if menu should be shown
 */
bool completion_menu_should_show(const completion_result_t *result,
                                int min_items);

#ifdef __cplusplus
}
#endif

#endif /* COMPLETION_MENU_H */
