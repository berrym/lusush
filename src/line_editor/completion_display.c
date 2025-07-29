/*
 * Lusush Line Editor - Completion Display Implementation
 * 
 * This file implements the completion display system for the Lusush Line Editor.
 * It provides visual presentation of completion candidates with scrolling,
 * selection highlighting, and configurable layout options.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "completion.h"
#include "display.h"
#include "terminal_manager.h"
#include "theme_integration.h"
#include "cursor_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Default configuration constants */
#define LLE_COMPLETION_DISPLAY_DEFAULT_MAX_ITEMS 10
#define LLE_COMPLETION_DISPLAY_DEFAULT_SELECTION_INDICATOR ">"
#define LLE_COMPLETION_DISPLAY_DEFAULT_SEPARATOR " "
#define LLE_COMPLETION_DISPLAY_MAX_LINE_LENGTH 256

/* ANSI color codes for fallback display */
#define LLE_COMPLETION_COLOR_SELECTION "\033[7m"  // Reverse video
#define LLE_COMPLETION_COLOR_TEXT "\033[0m"       // Normal text
#define LLE_COMPLETION_COLOR_DESC "\033[2m"       // Dim text
#define LLE_COMPLETION_COLOR_RESET "\033[0m"      // Reset

/**
 * @brief Create completion display structure
 *
 * Creates and initializes a completion display structure with default
 * configuration suitable for most terminal environments. The display
 * can be configured after creation for specific needs.
 *
 * @param completions Completion list to display (must not be NULL)
 * @param max_items Maximum number of items to show at once (0 for auto)
 * @return New completion display or NULL on error
 *
 * @note Display takes ownership of the completion list reference
 * @note Use lle_completion_display_destroy() to clean up
 */
lle_completion_display_t *lle_completion_display_create(
    lle_completion_list_t *completions,
    size_t max_items
) {
    if (!completions) {
        return NULL;
    }
    
    lle_completion_display_t *display = malloc(sizeof(lle_completion_display_t));
    if (!display) {
        return NULL;
    }
    
    if (!lle_completion_display_init(display, completions, max_items)) {
        free(display);
        return NULL;
    }
    
    return display;
}

/**
 * @brief Initialize completion display structure
 *
 * Initializes an existing completion display structure with the given
 * completion list and configuration. Provides fine-grained control
 * over display parameters and memory management.
 *
 * @param display Display structure to initialize (must not be NULL)
 * @param completions Completion list to display (must not be NULL)
 * @param max_items Maximum number of items to show at once (0 for auto)
 * @return true on success, false on error
 *
 * @note Display does not take ownership of the completion list
 * @note Use lle_completion_display_clear() to clean up
 */
bool lle_completion_display_init(
    lle_completion_display_t *display,
    lle_completion_list_t *completions,
    size_t max_items
) {
    if (!display || !completions) {
        return false;
    }
    
    // Initialize structure with defaults
    memset(display, 0, sizeof(lle_completion_display_t));
    
    display->completions = completions;
    display->display_start = 0;
    display->display_count = 0;
    display->max_display_items = max_items > 0 ? max_items : LLE_COMPLETION_DISPLAY_DEFAULT_MAX_ITEMS;
    display->show_descriptions = true;
    display->show_selection = true;
    
    // Set default indicators
    if (strlen(LLE_COMPLETION_DISPLAY_DEFAULT_SELECTION_INDICATOR) < sizeof(display->selection_indicator)) {
        memcpy(display->selection_indicator, LLE_COMPLETION_DISPLAY_DEFAULT_SELECTION_INDICATOR,
               strlen(LLE_COMPLETION_DISPLAY_DEFAULT_SELECTION_INDICATOR) + 1);
    }
    
    if (strlen(LLE_COMPLETION_DISPLAY_DEFAULT_SEPARATOR) < sizeof(display->item_separator)) {
        memcpy(display->item_separator, LLE_COMPLETION_DISPLAY_DEFAULT_SEPARATOR,
               strlen(LLE_COMPLETION_DISPLAY_DEFAULT_SEPARATOR) + 1);
    }
    
    // Configure default colors
    display->use_colors = true;
    display->selection_color = LLE_COMPLETION_COLOR_SELECTION;
    display->text_color = LLE_COMPLETION_COLOR_TEXT;
    display->desc_color = LLE_COMPLETION_COLOR_DESC;
    display->reset_color = LLE_COMPLETION_COLOR_RESET;
    
    // Calculate display metrics
    display->max_text_width = completions->max_text_len;
    display->max_desc_width = completions->max_desc_len;
    
    // Update viewport to show initial selection
    lle_completion_display_update_viewport(display);
    
    return true;
}

/**
 * @brief Clear completion display and reset state
 *
 * Clears the completion display structure and resets all state to
 * default values. Does not free the structure itself or the
 * completion list (unless display owns it).
 *
 * @param display Display structure to clear (can be NULL)
 */
void lle_completion_display_clear(lle_completion_display_t *display) {
    if (!display) {
        return;
    }
    
    // Clear all fields but don't free the completion list
    // (display doesn't own it in this case)
    memset(display, 0, sizeof(lle_completion_display_t));
}

/**
 * @brief Destroy completion display and free memory
 *
 * Destroys the completion display structure and frees all associated
 * memory. If the display owns the completion list, it will also be
 * destroyed.
 *
 * @param display Display structure to destroy (can be NULL)
 */
void lle_completion_display_destroy(lle_completion_display_t *display) {
    if (!display) {
        return;
    }
    
    // Note: We don't destroy the completion list here because
    // the display doesn't own it in the current design
    free(display);
}

/**
 * @brief Calculate visible item count for current viewport
 *
 * Internal helper function to calculate how many items should be
 * visible in the current display viewport based on list size
 * and display configuration.
 *
 * @param display Completion display (must not be NULL)
 * @return Number of items that should be visible
 */
static size_t lle_completion_display_calculate_visible_count(const lle_completion_display_t *display) {
    if (!display || !display->completions) {
        return 0;
    }
    
    size_t remaining = display->completions->count - display->display_start;
    size_t max_visible = display->max_display_items;
    
    return remaining < max_visible ? remaining : max_visible;
}

/**
 * @brief Format completion item line for display
 *
 * Internal helper function to format a single completion item
 * for display including selection indicator, text, and description
 * with proper alignment and color coding.
 *
 * @param display Display configuration (must not be NULL)
 * @param item Completion item to format (must not be NULL)
 * @param is_selected True if item is currently selected
 * @param buffer Output buffer for formatted line
 * @param buffer_size Size of output buffer
 * @return true on success, false on error
 */
static bool lle_completion_display_format_item(
    const lle_completion_display_t *display,
    const lle_completion_item_t *item,
    bool is_selected,
    char *buffer,
    size_t buffer_size
) {
    if (!display || !item || !buffer || buffer_size == 0) {
        return false;
    }
    
    size_t pos = 0;
    
    // Add color codes if enabled
    if (display->use_colors && is_selected && display->selection_color) {
        int written = snprintf(buffer + pos, buffer_size - pos, "%s", display->selection_color);
        if (written < 0 || pos + written >= buffer_size) return false;
        pos += written;
    }
    
    // Add selection indicator
    if (display->show_selection) {
        const char *indicator = is_selected ? display->selection_indicator : " ";
        int written = snprintf(buffer + pos, buffer_size - pos, "%s%s", 
                              indicator, display->item_separator);
        if (written < 0 || pos + written >= buffer_size) return false;
        pos += written;
    }
    
    // Add completion text with color
    if (display->use_colors && display->text_color && !is_selected) {
        int written = snprintf(buffer + pos, buffer_size - pos, "%s", display->text_color);
        if (written < 0 || pos + written >= buffer_size) return false;
        pos += written;
    }
    
    int written = snprintf(buffer + pos, buffer_size - pos, "%s", item->text);
    if (written < 0 || pos + written >= buffer_size) return false;
    pos += written;
    
    // Add description if enabled and available
    if (display->show_descriptions && item->description) {
        // Add padding between text and description
        size_t padding = display->max_text_width > item->text_len ? 
                        display->max_text_width - item->text_len + 2 : 2;
        
        for (size_t i = 0; i < padding && pos < buffer_size - 1; i++) {
            buffer[pos++] = ' ';
        }
        
        // Add description color
        if (display->use_colors && display->desc_color) {
            written = snprintf(buffer + pos, buffer_size - pos, "%s", display->desc_color);
            if (written < 0 || pos + written >= buffer_size) return false;
            pos += written;
        }
        
        written = snprintf(buffer + pos, buffer_size - pos, "%s", item->description);
        if (written < 0 || pos + written >= buffer_size) return false;
        pos += written;
    }
    
    // Reset colors
    if (display->use_colors && display->reset_color) {
        written = snprintf(buffer + pos, buffer_size - pos, "%s", display->reset_color);
        if (written < 0 || pos + written >= buffer_size) return false;
        pos += written;
    }
    
    // Ensure null termination
    if (pos < buffer_size) {
        buffer[pos] = '\0';
    } else {
        buffer[buffer_size - 1] = '\0';
    }
    
    return true;
}

/**
 * @brief Display completion list using absolute positioning system
 *
 * Renders the completion list to the terminal with proper formatting,
 * selection highlighting, and scrolling support. Integrates with the
 * Phase 2A absolute positioning system for multi-line compatibility.
 *
 * @param display_state Display state with absolute positioning support
 * @param completion_display Completion display configuration (must not be NULL)
 * @return true on success, false on error
 *
 * @note Uses Phase 2A absolute positioning for multi-line scenarios
 * @note Automatically handles terminal width constraints
 * @note Maintains cursor position tracking in display state
 */
bool lle_completion_display_show(
    lle_display_state_t *display_state,
    lle_completion_display_t *completion_display
) {
    if (!display_state || !completion_display || !completion_display->completions) {
        return false;
    }
    
    if (!display_state->position_tracking_valid) {
        return false; // Need valid position tracking for absolute positioning
    }
    
    // Update display count for current viewport
    completion_display->display_count = lle_completion_display_calculate_visible_count(completion_display);
    
    if (completion_display->display_count == 0) {
        return true; // Nothing to display, but not an error
    }
    
    // Calculate starting position for completion menu using Phase 2A coordinate system
    lle_cursor_position_t current_cursor = lle_calculate_cursor_position(
        display_state->buffer, &display_state->geometry, 
        lle_prompt_get_last_line_width(display_state->prompt));
    
    // Position completion menu one line below current cursor
    lle_terminal_coordinates_t menu_start_pos = lle_convert_to_terminal_coordinates(
        &current_cursor, display_state->content_start_row, display_state->content_start_col);
    
    if (!menu_start_pos.valid) {
        return false; // Invalid coordinate conversion
    }
    
    // Check if menu would exceed terminal height and position accordingly
    size_t menu_height = completion_display->display_count;
    size_t terminal_height = display_state->geometry.height;
    
    // Position menu below cursor if there's room, otherwise above
    bool position_below = true;
    if (menu_start_pos.terminal_row + 1 + menu_height >= terminal_height) {
        // Not enough room below - position above cursor instead
        if (current_cursor.absolute_row >= menu_height) {
            position_below = false;
            menu_start_pos.terminal_row = (current_cursor.absolute_row >= menu_height) ? 
                current_cursor.absolute_row - menu_height : 0;
        } else {
            // Not enough room above either - position below and let it scroll
            menu_start_pos.terminal_row += 1;
        }
    } else {
        // Room below - position one line down as normal
        menu_start_pos.terminal_row += 1;
    }
    
    // Ensure we don't go beyond terminal bounds
    if (menu_start_pos.terminal_row >= terminal_height) {
        menu_start_pos.terminal_row = terminal_height - 1;
    }
    
    if (!lle_terminal_move_cursor(display_state->terminal, 
                                 menu_start_pos.terminal_row, 1)) {
        return false;
    }
    
    // Display each visible item using absolute positioning
    for (size_t i = 0; i < completion_display->display_count; i++) {
        size_t item_index = completion_display->display_start + i;
        if (item_index >= completion_display->completions->count) {
            break;
        }
        
        const lle_completion_item_t *item = &completion_display->completions->items[item_index];
        bool is_selected = (item_index == completion_display->completions->selected);
        
        // Format the completion item with terminal width constraints
        char line_buffer[LLE_COMPLETION_DISPLAY_MAX_LINE_LENGTH];
        if (!lle_completion_display_format_item(completion_display, item, is_selected, 
                                               line_buffer, sizeof(line_buffer))) {
            return false;
        }
        
        // Truncate line if it exceeds terminal width
        size_t line_length = strlen(line_buffer);
        if (line_length >= display_state->geometry.width) {
            // Truncate with ellipsis if needed
            if (display_state->geometry.width > 3) {
                line_buffer[display_state->geometry.width - 4] = '.';
                line_buffer[display_state->geometry.width - 3] = '.';
                line_buffer[display_state->geometry.width - 2] = '.';
                line_buffer[display_state->geometry.width - 1] = '\0';
            } else {
                line_buffer[display_state->geometry.width - 1] = '\0';
            }
        }
        
        // Position cursor at start of line using absolute positioning
        if (!lle_terminal_move_cursor(display_state->terminal, 
                                     menu_start_pos.terminal_row + i, 1)) {
            return false;
        }
        
        // Clear line and write formatted completion item
        if (!lle_terminal_clear_to_eol(display_state->terminal)) {
            return false;
        }
        
        if (!lle_terminal_write(display_state->terminal, line_buffer, strlen(line_buffer))) {
            return false;
        }
    }
    
    // Restore cursor to original position using absolute positioning
    lle_terminal_coordinates_t restore_pos = lle_convert_to_terminal_coordinates(
        &current_cursor, display_state->content_start_row, display_state->content_start_col);
    
    if (restore_pos.valid) {
        lle_terminal_move_cursor(display_state->terminal, 
                                restore_pos.terminal_row, restore_pos.terminal_col);
    }
    
    return true;
}

/**
 * @brief Update display viewport for current selection
 *
 * Adjusts the display viewport (display_start and display_count) to
 * ensure the currently selected item is visible. Provides smooth
 * scrolling behavior for navigation through large lists.
 *
 * @param display Completion display to update (must not be NULL)
 * @return true if viewport changed, false if no change needed or error
 *
 * @note Called automatically by lle_completion_display_navigate()
 * @note Can be called manually after direct selection changes
 */
bool lle_completion_display_update_viewport(lle_completion_display_t *display) {
    if (!display || !display->completions) {
        return false;
    }
    
    size_t selected = display->completions->selected;
    size_t old_start = display->display_start;
    
    // If selected item is before the visible range, scroll up
    if (selected < display->display_start) {
        display->display_start = selected;
    }
    // If selected item is after the visible range, scroll down
    else if (selected >= display->display_start + display->max_display_items) {
        // Position selected item at bottom of display
        if (selected + 1 >= display->max_display_items) {
            display->display_start = selected + 1 - display->max_display_items;
        } else {
            display->display_start = 0;
        }
    }
    
    // Update display count
    display->display_count = lle_completion_display_calculate_visible_count(display);
    
    return display->display_start != old_start;
}

/**
 * @brief Navigate completion display selection
 *
 * Moves the selection within the completion display and updates
 * scrolling as needed. Provides smooth navigation through large
 * completion lists with automatic viewport adjustment.
 *
 * @param display Completion display to navigate (must not be NULL)
 * @param direction Navigation direction (-1 for up, 1 for down, 0 for no change)
 * @return true if selection changed, false if at boundary or error
 *
 * @note Wraps around at list boundaries for continuous navigation
 * @note Automatically adjusts display_start for scrolling
 * @note Updates completion list selection state
 */
bool lle_completion_display_navigate(
    lle_completion_display_t *display,
    int direction
) {
    if (!display || !display->completions || display->completions->count == 0) {
        return false;
    }
    
    bool selection_changed = false;
    
    if (direction > 0) {
        // Move down (next)
        selection_changed = lle_completion_list_select_next(display->completions);
    } else if (direction < 0) {
        // Move up (previous)
        selection_changed = lle_completion_list_select_prev(display->completions);
    }
    
    if (selection_changed) {
        // Update viewport to ensure selected item is visible
        lle_completion_display_update_viewport(display);
    }
    
    return selection_changed;
}

/**
 * @brief Configure display colors and formatting
 *
 * Sets the color scheme and formatting options for the completion
 * display. Allows customization of selection highlighting, text
 * colors, and visual indicators.
 *
 * @param display Display to configure (must not be NULL)
 * @param use_colors Enable color display
 * @param selection_color ANSI color code for selected item (can be NULL)
 * @param text_color ANSI color code for completion text (can be NULL)
 * @param desc_color ANSI color code for descriptions (can be NULL)
 * @return true on success, false on error
 *
 * @note NULL color parameters use default terminal colors
 * @note Colors are stored as references, not copied
 * @note Changes take effect on next display update
 */
bool lle_completion_display_configure_colors(
    lle_completion_display_t *display,
    bool use_colors,
    const char *selection_color,
    const char *text_color,
    const char *desc_color
) {
    if (!display) {
        return false;
    }
    
    display->use_colors = use_colors;
    
    if (selection_color) {
        display->selection_color = selection_color;
    }
    
    if (text_color) {
        display->text_color = text_color;
    }
    
    if (desc_color) {
        display->desc_color = desc_color;
    }
    
    return true;
}

/**
 * @brief Configure display layout and indicators
 *
 * Sets the visual layout options including selection indicators,
 * item separators, and display limits. Provides control over
 * the visual presentation of completion candidates.
 *
 * @param display Display to configure (must not be NULL)
 * @param show_descriptions Show item descriptions
 * @param show_selection Show selection indicator
 * @param selection_indicator Custom selection indicator (copied, can be NULL)
 * @param max_items Maximum items to display (0 for auto)
 * @return true on success, false on error
 *
 * @note Default selection indicator is ">" if not specified
 * @note max_items of 0 uses terminal height constraints
 * @note Changes take effect on next display update
 */
bool lle_completion_display_configure_layout(
    lle_completion_display_t *display,
    bool show_descriptions,
    bool show_selection,
    const char *selection_indicator,
    size_t max_items
) {
    if (!display) {
        return false;
    }
    
    display->show_descriptions = show_descriptions;
    display->show_selection = show_selection;
    
    if (selection_indicator) {
        size_t len = strlen(selection_indicator);
        if (len < sizeof(display->selection_indicator)) {
            memcpy(display->selection_indicator, selection_indicator, len + 1);
        }
    }
    
    if (max_items > 0) {
        display->max_display_items = max_items;
        // Update viewport with new max items
        lle_completion_display_update_viewport(display);
    }
    
    return true;
}

/**
 * @brief Get display statistics and metrics
 *
 * Retrieves current display state and metrics for debugging
 * and optimization purposes. Provides insight into display
 * performance and configuration.
 *
 * @param display Display to query (must not be NULL)
 * @param total_items Pointer to store total completion count (can be NULL)
 * @param visible_items Pointer to store visible item count (can be NULL)
 * @param selected_index Pointer to store selected item index (can be NULL)
 * @param display_start Pointer to store first visible index (can be NULL)
 * @return true on success, false on error
 *
 * @note All output parameters are optional (can be NULL)
 * @note Provides real-time display state information
 */
bool lle_completion_display_get_stats(
    const lle_completion_display_t *display,
    size_t *total_items,
    size_t *visible_items,
    size_t *selected_index,
    size_t *display_start
) {
    if (!display || !display->completions) {
        return false;
    }
    
    if (total_items) {
        *total_items = display->completions->count;
    }
    
    if (visible_items) {
        *visible_items = display->display_count;
    }
    
    if (selected_index) {
        *selected_index = display->completions->selected;
    }
    
    if (display_start) {
        *display_start = display->display_start;
    }
    
    return true;
}