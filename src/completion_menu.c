/*
 * Lusush - Interactive Completion Menu System Implementation
 * Copyright (C) 2021-2025  Michael Berry
 */

#include "../include/completion_menu.h"
#include "../include/termcap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// ANSI escape codes for display
#define ANSI_CLEAR_LINE "\033[2K"
#define ANSI_MOVE_UP "\033[1A"
#define ANSI_MOVE_DOWN "\033[1B"
#define ANSI_CURSOR_HIDE "\033[?25l"
#define ANSI_CURSOR_SHOW "\033[?25h"
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_REVERSE "\033[7m"
#define ANSI_CYAN "\033[36m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Get terminal dimensions
 */
static void get_terminal_size(int *width, int *height) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        *width = w.ws_col;
        *height = w.ws_row;
    } else {
        *width = 80;   // Default fallback
        *height = 24;
    }
}

/**
 * Count categories in a result
 */
static size_t count_categories(const completion_result_t *result) {
    if (!result) return 0;
    
    size_t count = 0;
    if (result->builtin_count > 0) count++;
    if (result->command_count > 0) count++;
    if (result->file_count > 0) count++;
    if (result->directory_count > 0) count++;
    if (result->variable_count > 0) count++;
    if (result->alias_count > 0) count++;
    if (result->history_count > 0) count++;
    
    return count;
}

/**
 * Find category positions in sorted result
 */
static void find_category_positions(completion_menu_t *menu) {
    if (!menu || !menu->result) return;
    
    menu->category_count = 0;
    menu->category_positions = malloc(sizeof(size_t) * 8); // Max 8 categories
    if (!menu->category_positions) return;
    
    completion_type_t last_type = COMPLETION_TYPE_UNKNOWN;
    
    for (size_t i = 0; i < menu->result->count; i++) {
        completion_type_t current_type = menu->result->items[i].type;
        
        if (current_type != last_type) {
            menu->category_positions[menu->category_count++] = i;
            last_type = current_type;
        }
    }
}

// ============================================================================
// MENU LIFECYCLE
// ============================================================================

completion_menu_config_t completion_menu_get_default_config(void) {
    completion_menu_config_t config = {
        .max_visible_items = 10,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = false,  // Too verbose for typical menu
        .enable_scrolling = true,
        .highlight_selection = true,
        .min_items_for_menu = 2
    };
    return config;
}

completion_menu_t* completion_menu_create(completion_result_t *result,
                                         completion_menu_config_t *config) {
    if (!result || result->count == 0) {
        return NULL;
    }
    
    completion_menu_t *menu = malloc(sizeof(completion_menu_t));
    if (!menu) {
        return NULL;
    }
    
    memset(menu, 0, sizeof(completion_menu_t));
    
    // Set result (menu doesn't own it)
    menu->result = result;
    
    // Set configuration
    if (config) {
        menu->config = *config;
    } else {
        menu->config = completion_menu_get_default_config();
    }
    
    // Validate and fix config
    if (menu->config.max_visible_items == 0) {
        menu->config.max_visible_items = 10;  // Default
    }
    
    // Initialize navigation state
    menu->selected_index = 0;
    menu->first_visible = 0;
    menu->visible_count = menu->config.max_visible_items;
    
    // Get terminal dimensions
    get_terminal_size(&menu->terminal_width, &menu->terminal_height);
    
    // Adjust visible count based on terminal height
    size_t max_for_height = (size_t)(menu->terminal_height - 5); // Leave room for prompt
    if (menu->visible_count > max_for_height) {
        menu->visible_count = max_for_height;
    }
    
    // Don't show more items than we have
    if (menu->visible_count > result->count) {
        menu->visible_count = result->count;
    }
    
    // Find category positions
    find_category_positions(menu);
    
    menu->menu_active = false;
    
    return menu;
}

void completion_menu_free(completion_menu_t *menu) {
    if (!menu) {
        return;
    }
    
    if (menu->category_positions) {
        free(menu->category_positions);
    }
    
    // Note: We don't free result - caller owns it
    
    free(menu);
}

// ============================================================================
// MENU NAVIGATION
// ============================================================================

bool completion_menu_navigate(completion_menu_t *menu, 
                             menu_nav_direction_t direction) {
    if (!menu || !menu->result || menu->result->count == 0) {
        return false;
    }
    
    size_t old_index = menu->selected_index;
    
    switch (direction) {
        case MENU_NAV_UP:
            if (menu->selected_index > 0) {
                menu->selected_index--;
            } else {
                // Wrap to bottom
                menu->selected_index = menu->result->count - 1;
            }
            break;
            
        case MENU_NAV_DOWN:
            if (menu->selected_index < menu->result->count - 1) {
                menu->selected_index++;
            } else {
                // Wrap to top
                menu->selected_index = 0;
            }
            break;
            
        case MENU_NAV_LEFT:
            // Move to previous category (with wrapping)
            if (menu->category_count > 1) {
                if (menu->current_category > 0) {
                    menu->current_category--;
                } else {
                    // Wrap to last category
                    menu->current_category = menu->category_count - 1;
                }
                menu->selected_index = menu->category_positions[menu->current_category];
            }
            break;
            
        case MENU_NAV_RIGHT:
            // Move to next category (with wrapping)
            if (menu->category_count > 1) {
                if (menu->current_category < menu->category_count - 1) {
                    menu->current_category++;
                } else {
                    // Wrap to first category
                    menu->current_category = 0;
                }
                menu->selected_index = menu->category_positions[menu->current_category];
            }
            break;
            
        case MENU_NAV_PAGE_UP:
            if (menu->selected_index >= menu->visible_count) {
                menu->selected_index -= menu->visible_count;
            } else {
                menu->selected_index = 0;
            }
            break;
            
        case MENU_NAV_PAGE_DOWN:
            menu->selected_index += menu->visible_count;
            if (menu->selected_index >= menu->result->count) {
                menu->selected_index = menu->result->count - 1;
            }
            break;
            
        case MENU_NAV_HOME:
            menu->selected_index = 0;
            break;
            
        case MENU_NAV_END:
            menu->selected_index = menu->result->count - 1;
            break;
    }
    
    // Update current category based on selected index
    for (size_t i = 0; i < menu->category_count; i++) {
        if (i == menu->category_count - 1 || 
            menu->selected_index < menu->category_positions[i + 1]) {
            menu->current_category = i;
            break;
        }
    }
    
    // Adjust first_visible for scrolling
    if (menu->config.enable_scrolling) {
        if (menu->selected_index < menu->first_visible) {
            menu->first_visible = menu->selected_index;
        } else if (menu->selected_index >= menu->first_visible + menu->visible_count) {
            menu->first_visible = menu->selected_index - menu->visible_count + 1;
        }
    }
    
    return menu->selected_index != old_index;
}

bool completion_menu_goto(completion_menu_t *menu, size_t index) {
    if (!menu || !menu->result || index >= menu->result->count) {
        return false;
    }
    
    menu->selected_index = index;
    
    // Update category
    for (size_t i = 0; i < menu->category_count; i++) {
        if (i == menu->category_count - 1 || 
            index < menu->category_positions[i + 1]) {
            menu->current_category = i;
            break;
        }
    }
    
    // Adjust scrolling
    if (menu->config.enable_scrolling) {
        if (index < menu->first_visible) {
            menu->first_visible = index;
        } else if (index >= menu->first_visible + menu->visible_count) {
            menu->first_visible = index - menu->visible_count + 1;
        }
    }
    
    return true;
}

bool completion_menu_goto_category(completion_menu_t *menu,
                                  completion_type_t category) {
    if (!menu || !menu->result) {
        return false;
    }
    
    // Find first item of this category
    for (size_t i = 0; i < menu->result->count; i++) {
        if (menu->result->items[i].type == category) {
            return completion_menu_goto(menu, i);
        }
    }
    
    return false;
}

// ============================================================================
// MENU QUERIES
// ============================================================================

const completion_item_t* completion_menu_get_selected(const completion_menu_t *menu) {
    if (!menu || !menu->result || menu->selected_index >= menu->result->count) {
        return NULL;
    }
    
    return &menu->result->items[menu->selected_index];
}

const char* completion_menu_get_selected_text(const completion_menu_t *menu) {
    const completion_item_t *item = completion_menu_get_selected(menu);
    return item ? item->text : NULL;
}

size_t completion_menu_get_visible_count(const completion_menu_t *menu) {
    return menu ? menu->visible_count : 0;
}

bool completion_menu_has_categories(const completion_menu_t *menu) {
    return menu && menu->category_count > 1;
}

completion_type_t completion_menu_get_current_category(const completion_menu_t *menu) {
    if (!menu || !menu->result || menu->selected_index >= menu->result->count) {
        return COMPLETION_TYPE_UNKNOWN;
    }
    
    return menu->result->items[menu->selected_index].type;
}

// ============================================================================
// MENU FORMATTING
// ============================================================================

size_t completion_menu_format_item(const completion_item_t *item,
                                  bool is_selected,
                                  bool show_indicator,
                                  char *buffer,
                                  size_t buffer_size) {
    if (!item || !buffer || buffer_size == 0) {
        return 0;
    }
    
    char temp[512];
    size_t pos = 0;
    
    // Add selection highlight
    if (is_selected) {
        pos += snprintf(temp + pos, sizeof(temp) - pos, "%s", ANSI_REVERSE);
    }
    
    // Add type indicator
    if (show_indicator && item->type_indicator) {
        pos += snprintf(temp + pos, sizeof(temp) - pos, "%s ", item->type_indicator);
    }
    
    // Add item text
    pos += snprintf(temp + pos, sizeof(temp) - pos, "%s", item->text);
    
    // Add description if available
    if (item->description) {
        pos += snprintf(temp + pos, sizeof(temp) - pos, 
                       " %s(%s)%s", ANSI_DIM, item->description, ANSI_RESET);
    }
    
    // Reset formatting
    if (is_selected) {
        pos += snprintf(temp + pos, sizeof(temp) - pos, "%s", ANSI_RESET);
    }
    
    // Copy to output buffer
    size_t copy_len = pos < buffer_size - 1 ? pos : buffer_size - 1;
    memcpy(buffer, temp, copy_len);
    buffer[copy_len] = '\0';
    
    return copy_len;
}

size_t completion_menu_format_header(completion_type_t category,
                                    size_t count,
                                    char *buffer,
                                    size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return 0;
    }
    
    const char *category_name = completion_type_get_category(category);
    
    size_t len = snprintf(buffer, buffer_size, 
                         "%s%s--- %s (%zu) ---%s",
                         ANSI_BOLD, ANSI_CYAN,
                         category_name, count,
                         ANSI_RESET);
    
    return len < buffer_size ? len : buffer_size - 1;
}

// ============================================================================
// MENU DISPLAY
// ============================================================================

bool completion_menu_display(completion_menu_t *menu) {
    if (!menu || !menu->result || menu->result->count == 0) {
        return false;
    }
    
    // Hide cursor during display
    printf("%s", ANSI_CURSOR_HIDE);
    
    size_t display_count = 0;
    size_t last_visible = menu->first_visible + menu->visible_count;
    if (last_visible > menu->result->count) {
        last_visible = menu->result->count;
    }
    
    completion_type_t last_category = COMPLETION_TYPE_UNKNOWN;
    
    for (size_t i = menu->first_visible; i < last_visible; i++) {
        completion_item_t *item = &menu->result->items[i];
        
        // Show category header if type changed
        if (menu->config.show_category_headers && item->type != last_category) {
            char header[256];
            size_t category_count = completion_result_count_by_type(
                menu->result, item->type);
            completion_menu_format_header(item->type, category_count,
                                        header, sizeof(header));
            printf("\n%s", header);
            display_count++;
            last_category = item->type;
        }
        
        // Format and display item
        char line[512];
        bool is_selected = (i == menu->selected_index);
        completion_menu_format_item(item, is_selected,
                                   menu->config.show_type_indicators,
                                   line, sizeof(line));
        
        printf("\n%s", line);
        display_count++;
    }
    
    // Show scrolling indicators
    if (menu->config.enable_scrolling) {
        if (menu->first_visible > 0) {
            printf("\n%s^ More above ^%s", ANSI_DIM, ANSI_RESET);
            display_count++;
        }
        if (last_visible < menu->result->count) {
            printf("\n%sv More below v%s", ANSI_DIM, ANSI_RESET);
            display_count++;
        }
    }
    
    printf("\n");
    
    // Show cursor
    printf("%s", ANSI_CURSOR_SHOW);
    fflush(stdout);
    
    menu->menu_active = true;
    
    return true;
}

void completion_menu_clear(completion_menu_t *menu) {
    if (!menu || !menu->menu_active) {
        return;
    }
    
    // Count lines displayed
    size_t lines = 0;
    size_t last_visible = menu->first_visible + menu->visible_count;
    if (last_visible > menu->result->count) {
        last_visible = menu->result->count;
    }
    
    // Count items + headers + scroll indicators
    completion_type_t last_category = COMPLETION_TYPE_UNKNOWN;
    for (size_t i = menu->first_visible; i < last_visible; i++) {
        if (menu->config.show_category_headers && 
            menu->result->items[i].type != last_category) {
            lines++;
            last_category = menu->result->items[i].type;
        }
        lines++;
    }
    
    if (menu->config.enable_scrolling) {
        if (menu->first_visible > 0) lines++;
        if (last_visible < menu->result->count) lines++;
    }
    
    lines++; // Extra newline at end
    
    // Move up and clear each line
    for (size_t i = 0; i < lines; i++) {
        printf("%s%s", ANSI_MOVE_UP, ANSI_CLEAR_LINE);
    }
    
    fflush(stdout);
    menu->menu_active = false;
}

bool completion_menu_refresh(completion_menu_t *menu) {
    if (!menu) {
        return false;
    }
    
    if (menu->menu_active) {
        completion_menu_clear(menu);
    }
    
    return completion_menu_display(menu);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void completion_menu_update_dimensions(completion_menu_t *menu) {
    if (!menu) {
        return;
    }
    
    get_terminal_size(&menu->terminal_width, &menu->terminal_height);
    
    // Adjust visible count
    size_t max_for_height = (size_t)(menu->terminal_height - 5);
    menu->visible_count = menu->config.max_visible_items;
    if (menu->visible_count > max_for_height) {
        menu->visible_count = max_for_height;
    }
    if (menu->result && menu->visible_count > menu->result->count) {
        menu->visible_count = menu->result->count;
    }
}

bool completion_menu_should_show(const completion_result_t *result,
                                int min_items) {
    if (!result) {
        return false;
    }
    
    return (int)result->count >= min_items;
}

// ============================================================================
// Query Functions
// ============================================================================

bool completion_menu_is_active(const completion_menu_t *menu) {
    if (!menu) {
        return false;
    }
    return menu->menu_active;
}

size_t completion_menu_get_item_count(const completion_menu_t *menu) {
    if (!menu || !menu->result) {
        return 0;
    }
    return menu->result->count;
}

size_t completion_menu_get_selected_index(const completion_menu_t *menu) {
    if (!menu) {
        return 0;
    }
    return menu->selected_index;
}
