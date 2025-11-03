/*
 * Completion Menu Theme Integration Implementation
 * 
 * Integrates completion menu with Lusush theme system for consistent
 * visual appearance across all shell components.
 */

#include "../include/completion_menu_theme.h"
#include "../include/themes.h"
#include "../include/completion_types.h"
#include "../include/completion_menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ANSI codes for formatting
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_REVERSE "\033[7m"

// Global theme configuration
static completion_menu_theme_config_t g_theme_config;
static bool g_theme_initialized = false;

// ============================================================================
// DEFAULT THEME CONFIGURATION
// ============================================================================

static completion_menu_theme_config_t get_default_theme_config(void) {
    completion_menu_theme_config_t config = {
        .colors = {
            .category_header = "\033[1;36m",      // Bold cyan
            .category_border = "\033[2;37m",      // Dim white
            .item_selected = "\033[7m",           // Reverse video
            .item_normal = "\033[0m",             // Normal
            .item_dimmed = "\033[2m",             // Dim
            .type_builtin = "\033[1;33m",         // Bold yellow
            .type_command = "\033[1;32m",         // Bold green
            .type_file = "\033[0;37m",            // Normal white
            .type_directory = "\033[1;34m",       // Bold blue
            .type_variable = "\033[1;35m",        // Bold magenta
            .type_alias = "\033[1;36m",           // Bold cyan
            .type_history = "\033[2;37m",         // Dim white
            .scroll_indicator = "\033[2;33m"      // Dim yellow
        },
        .symbols = {
            .builtin_indicator = "⚙",
            .command_indicator = "⚡",
            .file_indicator = "📄",
            .directory_indicator = "📁",
            .variable_indicator = "$",
            .alias_indicator = "@",
            .history_indicator = "🕐",
            .header_left = "---",
            .header_right = "---",
            .scroll_up = "^",
            .scroll_down = "v",
            .selection_prefix = ">"
        },
        .use_colors = true,
        .use_unicode = true,
        .use_bold = true,
        .use_underline = false,
        .compact_mode = false
    };
    
    return config;
}

static completion_menu_theme_config_t get_ascii_fallback_config(void) {
    completion_menu_theme_config_t config = get_default_theme_config();
    
    // Replace Unicode symbols with ASCII
    config.symbols.builtin_indicator = "*";
    config.symbols.command_indicator = ">";
    config.symbols.file_indicator = "f";
    config.symbols.directory_indicator = "d";
    config.symbols.variable_indicator = "$";
    config.symbols.alias_indicator = "@";
    config.symbols.history_indicator = "h";
    config.symbols.header_left = "[";
    config.symbols.header_right = "]";
    config.symbols.scroll_up = "^";
    config.symbols.scroll_down = "v";
    config.symbols.selection_prefix = ">";
    config.use_unicode = false;
    
    return config;
}

// ============================================================================
// THEME INTEGRATION
// ============================================================================

bool completion_menu_theme_init(void) {
    if (g_theme_initialized) {
        return true;
    }
    
    // Try to get active theme
    theme_definition_t *active_theme = theme_get_active();
    
    if (active_theme && active_theme->colors.primary[0] != '\0') {
        // Map theme colors to menu colors
        g_theme_config = get_default_theme_config();
        
        // Use theme's semantic colors
        snprintf(g_theme_config.colors.category_header, COLOR_CODE_MAX, 
                "%s", active_theme->colors.primary);
        snprintf(g_theme_config.colors.item_selected, COLOR_CODE_MAX,
                "%s", active_theme->colors.highlight);
        snprintf(g_theme_config.colors.type_builtin, COLOR_CODE_MAX,
                "%s", active_theme->colors.warning);
        snprintf(g_theme_config.colors.type_command, COLOR_CODE_MAX,
                "%s", active_theme->colors.success);
        snprintf(g_theme_config.colors.type_directory, COLOR_CODE_MAX,
                "%s", active_theme->colors.info);
        snprintf(g_theme_config.colors.type_variable, COLOR_CODE_MAX,
                "%s", active_theme->colors.secondary);
        snprintf(g_theme_config.colors.type_alias, COLOR_CODE_MAX,
                "%s", active_theme->colors.primary);
        
        // Check symbol compatibility
        symbol_compatibility_t symbol_mode = symbol_get_compatibility_mode();
        if (symbol_mode == SYMBOL_MODE_ASCII) {
            g_theme_config.use_unicode = false;
            completion_menu_theme_config_t ascii_config = get_ascii_fallback_config();
            g_theme_config.symbols = ascii_config.symbols;
        }
    } else {
        // Use default configuration
        int color_support = theme_detect_color_support();
        if (color_support < 8) {
            // No color support - use plain ASCII
            g_theme_config = get_ascii_fallback_config();
            g_theme_config.use_colors = false;
        } else if (color_support < 256) {
            // Basic colors only
            g_theme_config = get_default_theme_config();
        } else {
            // Full color support
            g_theme_config = get_default_theme_config();
        }
    }
    
    g_theme_initialized = true;
    return true;
}

const completion_menu_theme_config_t* completion_menu_theme_get_config(void) {
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    return &g_theme_config;
}

bool completion_menu_theme_update(void) {
    g_theme_initialized = false;
    return completion_menu_theme_init();
}

const char* completion_menu_theme_get_type_color(completion_type_t type) {
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    
    if (!g_theme_config.use_colors) {
        return "";
    }
    
    switch (type) {
        case COMPLETION_TYPE_BUILTIN:
            return g_theme_config.colors.type_builtin;
        case COMPLETION_TYPE_COMMAND:
            return g_theme_config.colors.type_command;
        case COMPLETION_TYPE_FILE:
            return g_theme_config.colors.type_file;
        case COMPLETION_TYPE_DIRECTORY:
            return g_theme_config.colors.type_directory;
        case COMPLETION_TYPE_VARIABLE:
            return g_theme_config.colors.type_variable;
        case COMPLETION_TYPE_ALIAS:
            return g_theme_config.colors.type_alias;
        case COMPLETION_TYPE_HISTORY:
            return g_theme_config.colors.type_history;
        default:
            return g_theme_config.colors.item_normal;
    }
}

const char* completion_menu_theme_get_type_symbol(completion_type_t type) {
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    
    switch (type) {
        case COMPLETION_TYPE_BUILTIN:
            return g_theme_config.symbols.builtin_indicator;
        case COMPLETION_TYPE_COMMAND:
            return g_theme_config.symbols.command_indicator;
        case COMPLETION_TYPE_FILE:
            return g_theme_config.symbols.file_indicator;
        case COMPLETION_TYPE_DIRECTORY:
            return g_theme_config.symbols.directory_indicator;
        case COMPLETION_TYPE_VARIABLE:
            return g_theme_config.symbols.variable_indicator;
        case COMPLETION_TYPE_ALIAS:
            return g_theme_config.symbols.alias_indicator;
        case COMPLETION_TYPE_HISTORY:
            return g_theme_config.symbols.history_indicator;
        default:
            return "";
    }
}

const char* completion_menu_theme_get_header_color(void) {
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    return g_theme_config.use_colors ? g_theme_config.colors.category_header : "";
}

const char* completion_menu_theme_get_selection_color(void) {
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    return g_theme_config.use_colors ? g_theme_config.colors.item_selected : "";
}

// ============================================================================
// THEMED DISPLAY FUNCTIONS
// ============================================================================

size_t completion_menu_format_item_themed(const completion_item_t *item,
                                          bool is_selected,
                                          char *buffer,
                                          size_t buffer_size) {
    if (!item || !buffer || buffer_size == 0) {
        return 0;
    }
    
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    
    char formatted[512];
    size_t pos = 0;
    
    // Selection indicator
    if (is_selected) {
        if (g_theme_config.use_colors) {
            pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                          "%s", g_theme_config.colors.item_selected);
        }
        pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                      "%s ", g_theme_config.symbols.selection_prefix);
    } else {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos, "  ");
    }
    
    // Type indicator
    const char *symbol = completion_menu_theme_get_type_symbol(item->type);
    const char *color = completion_menu_theme_get_type_color(item->type);
    
    if (g_theme_config.use_colors && color[0] != '\0') {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                      "%s%s" ANSI_RESET, color, symbol);
    } else {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos, "%s", symbol);
    }
    
    // Item text
    pos += snprintf(formatted + pos, sizeof(formatted) - pos, " %s", item->text);
    
    // Reset formatting if selected
    if (is_selected && g_theme_config.use_colors) {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos, ANSI_RESET);
    }
    
    // Copy to output buffer
    strncpy(buffer, formatted, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return pos;
}

size_t completion_menu_format_header_themed(completion_type_t category,
                                            size_t count,
                                            char *buffer,
                                            size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return 0;
    }
    
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    
    const completion_type_info_t *info = completion_type_get_info(category);
    const char *category_name = info ? info->category_name : "UNKNOWN";
    const char *header_color = completion_menu_theme_get_header_color();
    
    char formatted[256];
    size_t pos = 0;
    
    // Apply header color
    if (g_theme_config.use_colors && header_color[0] != '\0') {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                      "%s", header_color);
    }
    
    // Format header
    if (g_theme_config.use_bold && g_theme_config.use_colors) {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos, ANSI_BOLD);
    }
    
    pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                  "%s %s (%zu) %s",
                  g_theme_config.symbols.header_left,
                  category_name,
                  count,
                  g_theme_config.symbols.header_right);
    
    // Reset formatting
    if (g_theme_config.use_colors) {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos, ANSI_RESET);
    }
    
    // Copy to output buffer
    strncpy(buffer, formatted, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return pos;
}

size_t completion_menu_format_scroll_indicator_themed(int direction,
                                                      char *buffer,
                                                      size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return 0;
    }
    
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    
    const char *symbol = (direction > 0) ? 
                         g_theme_config.symbols.scroll_down :
                         g_theme_config.symbols.scroll_up;
    
    char formatted[128];
    size_t pos = 0;
    
    // Apply scroll indicator color
    if (g_theme_config.use_colors) {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                      "%s", g_theme_config.colors.scroll_indicator);
    }
    
    // Format indicator
    const char *text = (direction > 0) ? "More below" : "More above";
    pos += snprintf(formatted + pos, sizeof(formatted) - pos,
                  "%s %s %s", symbol, text, symbol);
    
    // Reset formatting
    if (g_theme_config.use_colors) {
        pos += snprintf(formatted + pos, sizeof(formatted) - pos, ANSI_RESET);
    }
    
    // Copy to output buffer
    strncpy(buffer, formatted, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return pos;
}

bool completion_menu_display_themed(completion_menu_t *menu) {
    if (!menu || !menu->result) {
        return false;
    }
    
    if (!g_theme_initialized) {
        completion_menu_theme_init();
    }
    
    // Calculate visible range
    size_t last_visible = menu->first_visible + menu->visible_count;
    if (last_visible > menu->result->count) {
        last_visible = menu->result->count;
    }
    
    // Show scroll up indicator if needed
    if (menu->config.enable_scrolling && menu->first_visible > 0) {
        char scroll_indicator[128];
        completion_menu_format_scroll_indicator_themed(-1, scroll_indicator, 
                                                      sizeof(scroll_indicator));
        printf("\n%s", scroll_indicator);
    }
    
    // Display items with themed formatting
    completion_type_t last_category = COMPLETION_TYPE_UNKNOWN;
    for (size_t i = menu->first_visible; i < last_visible; i++) {
        completion_item_t *item = &menu->result->items[i];
        
        // Show category header when type changes
        if (menu->config.show_category_headers && item->type != last_category) {
            // Count items in this category
            size_t category_count = 0;
            for (size_t j = 0; j < menu->result->count; j++) {
                if (menu->result->items[j].type == item->type) {
                    category_count++;
                }
            }
            
            char header[256];
            completion_menu_format_header_themed(item->type, category_count,
                                                header, sizeof(header));
            printf("\n%s", header);
            last_category = item->type;
        }
        
        // Format and display item
        char line[512];
        bool is_selected = (i == menu->selected_index);
        completion_menu_format_item_themed(item, is_selected, line, sizeof(line));
        printf("\n%s", line);
    }
    
    // Show scroll down indicator if needed
    if (menu->config.enable_scrolling && last_visible < menu->result->count) {
        char scroll_indicator[128];
        completion_menu_format_scroll_indicator_themed(1, scroll_indicator,
                                                      sizeof(scroll_indicator));
        printf("\n%s", scroll_indicator);
    }
    
    printf("\n");
    fflush(stdout);
    
    return true;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool completion_menu_theme_is_supported(void) {
    int color_support = theme_detect_color_support();
    return (color_support >= 8);
}

completion_menu_theme_config_t completion_menu_theme_get_fallback(void) {
    return get_ascii_fallback_config();
}

void completion_menu_theme_cleanup(void) {
    g_theme_initialized = false;
}
