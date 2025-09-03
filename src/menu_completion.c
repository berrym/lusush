/*
 * Lusush - Menu Completion System
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Menu Completion System - Fish-style TAB cycling with visual selection
 * 
 * This module implements Fish-style menu completion with:
 * - TAB cycling through completions (forward and backward)
 * - Multi-column display with descriptions
 * - Visual selection highlighting
 * - Theme integration for colors
 * - Performance optimization with caching
 * - Full integration with existing completion system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <readline/readline.h>

#include "../include/menu_completion.h"
#include "../include/config.h"
#include "../include/themes.h"
#include "../include/rich_completion.h"
#include "../include/completion.h"
#include "../include/autosuggestions.h"

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

static bool initialized = false;
static menu_completion_state_t menu_state = {0};
static menu_performance_stats_t perf_stats = {0};

// Default configuration values
static const char *DEFAULT_SELECTION_COLOR = "\033[48;5;237m\033[38;5;255m"; // Dark background, white text
static const char *DEFAULT_DESCRIPTION_COLOR = "\033[38;5;244m";           // Gray text
static const char *COLOR_RESET = "\033[0m";

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * Get current time in milliseconds for performance tracking
 */
static double get_time_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
    }
    return 0.0;
}

/**
 * Safe string duplication with NULL check
 */
static char *safe_strdup(const char *str) {
    if (!str) return NULL;
    char *dup = strdup(str);
    if (!dup) {
        fprintf(stderr, "lusush: menu_completion: memory allocation failed\n");
    }
    return dup;
}

/**
 * Free menu completion state resources
 */
static void free_menu_state(menu_completion_state_t *state) {
    if (!state) return;

    if (state->completions) {
        for (int i = 0; i < state->count; i++) {
            free(state->completions[i]);
        }
        free(state->completions);
        state->completions = NULL;
    }

    if (state->descriptions) {
        for (int i = 0; i < state->count; i++) {
            free(state->descriptions[i]);
        }
        free(state->descriptions);
        state->descriptions = NULL;
    }

    free(state->original_text);
    free(state->common_prefix);
    
    state->original_text = NULL;
    state->common_prefix = NULL;
    state->count = 0;
    state->selected_index = 0;
    state->active = false;
}

// ============================================================================
// CORE INITIALIZATION AND CLEANUP
// ============================================================================

bool lusush_menu_completion_init(void) {
    if (initialized) {
        return true;
    }

    // Initialize state
    memset(&menu_state, 0, sizeof(menu_state));
    memset(&perf_stats, 0, sizeof(perf_stats));

    // Set default configuration if not already set
    if (!config.menu_completion_selection_color) {
        config.menu_completion_selection_color = safe_strdup(DEFAULT_SELECTION_COLOR);
    }
    if (!config.menu_completion_description_color) {
        config.menu_completion_description_color = safe_strdup(DEFAULT_DESCRIPTION_COLOR);
    }

    // Set default values for uninitialized settings
    if (config.menu_completion_max_columns == 0) {
        config.menu_completion_max_columns = 0; // Auto-calculate
    }
    if (config.menu_completion_max_rows == 0) {
        config.menu_completion_max_rows = 10;
    }

    initialized = true;
    return true;
}

void lusush_menu_completion_cleanup(void) {
    if (!initialized) return;

    free_menu_state(&menu_state);
    initialized = false;
}

// ============================================================================
// TERMINAL AND LAYOUT FUNCTIONS
// ============================================================================

bool lusush_get_terminal_size(int *width, int *height) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (width) *width = ws.ws_col;
        if (height) *height = ws.ws_row;
        return true;
    }
    
    // Fallback to standard size
    if (width) *width = 80;
    if (height) *height = 24;
    return false;
}

menu_layout_info_t lusush_calculate_menu_layout(char **completions, 
                                               char **descriptions, 
                                               int count) {
    menu_layout_info_t layout = {0};
    
    // Get terminal dimensions
    lusush_get_terminal_size(&layout.terminal_width, NULL);
    
    if (count == 0) {
        return layout;
    }
    
    layout.has_descriptions = descriptions && config.menu_completion_show_descriptions;
    
    // Find maximum widths
    layout.max_completion_width = 0;
    layout.max_description_width = 0;
    
    for (int i = 0; i < count; i++) {
        if (completions[i]) {
            int comp_len = strlen(completions[i]);
            if (comp_len > layout.max_completion_width) {
                layout.max_completion_width = comp_len;
            }
        }
        
        if (layout.has_descriptions && descriptions[i]) {
            int desc_len = strlen(descriptions[i]);
            if (desc_len > layout.max_description_width) {
                layout.max_description_width = desc_len;
            }
        }
    }
    
    // Calculate column layout
    int separator_width = 2; // "  " between columns
    int description_separator = layout.has_descriptions ? 3 : 0; // " - " before description
    
    layout.column_width = layout.max_completion_width;
    if (layout.has_descriptions) {
        layout.column_width += description_separator + layout.max_description_width;
    }
    
    // Calculate number of columns
    if (config.menu_completion_max_columns > 0) {
        layout.columns = config.menu_completion_max_columns;
    } else {
        // Auto-calculate based on terminal width
        int available_width = layout.terminal_width - 2; // Leave margin
        layout.columns = available_width / (layout.column_width + separator_width);
        if (layout.columns < 1) layout.columns = 1;
        if (layout.columns > count) layout.columns = count;
    }
    
    // Calculate number of rows
    layout.rows = (count + layout.columns - 1) / layout.columns;
    
    return layout;
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

char *lusush_format_completion_entry(const char *completion,
                                    const char *description,
                                    int max_width) {
    if (!completion) return NULL;
    
    int comp_len = strlen(completion);
    bool has_desc = description && config.menu_completion_show_descriptions;
    
    if (!has_desc) {
        return safe_strdup(completion);
    }
    
    // Format: "completion - description"
    int desc_len = strlen(description);
    int total_len = comp_len + 3 + desc_len + 1; // " - " + null terminator
    
    if (max_width > 0 && total_len > max_width) {
        // Truncate description if needed
        int available_desc_len = max_width - comp_len - 3 - 3; // "..." at end
        if (available_desc_len > 0) {
            char *formatted = malloc(max_width + 1);
            if (formatted) {
                snprintf(formatted, max_width + 1, "%s - %.*s...", 
                        completion, available_desc_len, description);
            }
            return formatted;
        } else {
            // Just return completion if no room for description
            return safe_strdup(completion);
        }
    }
    
    char *formatted = malloc(total_len);
    if (formatted) {
        snprintf(formatted, total_len, "%s - %s", completion, description);
    }
    return formatted;
}

void lusush_render_completion_menu(menu_completion_state_t *state) {
    if (!state || !lusush_validate_menu_state(state)) {
        return;
    }
    
    double start_time = get_time_ms();
    
    menu_layout_info_t layout = lusush_calculate_menu_layout(
        state->completions, state->descriptions, state->count);
    
    if (layout.columns == 0 || layout.rows == 0) {
        return;
    }
    
    printf("\n"); // Start menu on new line
    
    // Get colors
    const char *selection_color = config.menu_completion_selection_color 
                                 ? config.menu_completion_selection_color 
                                 : DEFAULT_SELECTION_COLOR;
    // Note: desc_color will be used in future enhancement phases
    // const char *desc_color = config.menu_completion_description_color
    //                        ? config.menu_completion_description_color
    //                        : DEFAULT_DESCRIPTION_COLOR;
    
    for (int row = 0; row < layout.rows; row++) {
        for (int col = 0; col < layout.columns; col++) {
            int index = row * layout.columns + col;
            if (index >= state->count) break;
            
            bool is_selected = (index == state->selected_index);
            
            // Apply selection highlighting
            if (is_selected && config.menu_completion_show_selection_highlight) {
                printf("%s", selection_color);
            }
            
            // Format and print completion entry
            char *formatted = lusush_format_completion_entry(
                state->completions[index],
                layout.has_descriptions ? state->descriptions[index] : NULL,
                layout.column_width);
            
            if (formatted) {
                printf("%-*s", layout.column_width, formatted);
                free(formatted);
            } else {
                printf("%-*s", layout.column_width, state->completions[index]);
            }
            
            // Reset color after selection
            if (is_selected && config.menu_completion_show_selection_highlight) {
                printf("%s", COLOR_RESET);
            }
            
            // Add column separator (except for last column)
            if (col < layout.columns - 1) {
                printf("  ");
            }
        }
        printf("\n");
    }
    
    fflush(stdout);
    
    // Update performance stats
    double elapsed = get_time_ms() - start_time;
    perf_stats.menu_displays++;
    perf_stats.avg_display_time_ms = 
        (perf_stats.avg_display_time_ms * (perf_stats.menu_displays - 1) + elapsed) 
        / perf_stats.menu_displays;
}

// ============================================================================
// READLINE INTEGRATION FUNCTIONS  
// ============================================================================

int lusush_menu_complete_handler(int count, int key) {
    if (!initialized || !config.menu_completion_enabled) {
        // Fallback to standard completion
        return rl_complete(count, key);
    }
    
    double start_time = get_time_ms();
    
    // Clear any autosuggestion remnants before completion
    lusush_dismiss_suggestion();
    printf("\033[K");  // Clear to end of line
    fflush(stdout);
    
    // Use readline's built-in menu completion for now
    // This provides basic TAB cycling functionality
    int result = rl_menu_complete(count, key);
    
    // Update performance stats
    double elapsed = get_time_ms() - start_time;
    perf_stats.avg_cycling_time_ms = 
        (perf_stats.avg_cycling_time_ms * perf_stats.total_completions + elapsed)
        / (perf_stats.total_completions + 1);
    perf_stats.total_completions++;
    
    return result;
}

int lusush_menu_complete_backward_handler(int count, int key) {
    if (!initialized || !config.menu_completion_enabled) {
        return rl_complete(count, key);
    }
    
    double start_time = get_time_ms();
    
    // Clear any autosuggestion remnants before completion
    lusush_dismiss_suggestion();
    printf("\033[K");  // Clear to end of line
    fflush(stdout);
    
    // Use readline's built-in backward menu completion
    int result = rl_backward_menu_complete(count, key);
    
    // Update performance stats  
    double elapsed = get_time_ms() - start_time;
    perf_stats.avg_cycling_time_ms = 
        (perf_stats.avg_cycling_time_ms * perf_stats.total_completions + elapsed)
        / (perf_stats.total_completions + 1);
    perf_stats.total_completions++;
    
    return result;
}

void lusush_display_completion_menu(char **matches, int len, int max) {
    if (!initialized || !config.menu_completion_enabled || !matches || len <= 1) {
        // Use standard readline display
        rl_display_match_list(matches, len, max);
        return;
    }
    
    // Clear any autosuggestion remnants before displaying menu
    lusush_dismiss_suggestion();
    printf("\033[K");  // Clear to end of line
    fflush(stdout);
    
    // Skip first element (common prefix) and create our state
    int actual_count = len - 1;
    if (actual_count <= 0) {
        return;
    }
    
    // Free previous state
    free_menu_state(&menu_state);
    
    // Allocate new state
    menu_state.completions = malloc(actual_count * sizeof(char*));
    menu_state.descriptions = malloc(actual_count * sizeof(char*));
    
    if (!menu_state.completions || !menu_state.descriptions) {
        fprintf(stderr, "lusush: menu_completion: memory allocation failed\n");
        rl_display_match_list(matches, len, max);
        return;
    }
    
    // Copy completions (skip first element which is common prefix)
    menu_state.count = actual_count;
    for (int i = 0; i < actual_count; i++) {
        menu_state.completions[i] = safe_strdup(matches[i + 1]);
        menu_state.descriptions[i] = NULL; // TODO: integrate with rich_completion
    }
    
    menu_state.selected_index = 0;
    menu_state.active = true;
    menu_state.common_prefix = safe_strdup(matches[0]);
    
    // Render the menu
    lusush_render_completion_menu(&menu_state);
}

void lusush_menu_completion_reset(void) {
    free_menu_state(&menu_state);
}

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

bool lusush_setup_menu_completion(void) {
    if (!initialized) {
        if (!lusush_menu_completion_init()) {
            return false;
        }
    }
    
    // This will be called from readline_integration.c to set up key bindings
    return true;
}

bool lusush_update_menu_completion_config(void) {
    if (!initialized) return false;
    
    // Update colors from theme if set to "auto"
    if (config.menu_completion_selection_color && 
        strcmp(config.menu_completion_selection_color, "auto") == 0) {
        free(config.menu_completion_selection_color);
        config.menu_completion_selection_color = safe_strdup(DEFAULT_SELECTION_COLOR);
    }
    
    if (config.menu_completion_description_color && 
        strcmp(config.menu_completion_description_color, "auto") == 0) {
        free(config.menu_completion_description_color);
        config.menu_completion_description_color = safe_strdup(DEFAULT_DESCRIPTION_COLOR);
    }
    
    return true;
}

bool lusush_is_menu_completion_active(void) {
    return initialized && config.menu_completion_enabled;
}

// ============================================================================
// VALIDATION AND UTILITY FUNCTIONS
// ============================================================================

bool lusush_validate_menu_state(menu_completion_state_t *state) {
    return state && 
           state->completions && 
           state->count > 0 && 
           state->selected_index >= 0 && 
           state->selected_index < state->count;
}

// ============================================================================
// PERFORMANCE AND DEBUG FUNCTIONS
// ============================================================================

menu_performance_stats_t lusush_get_menu_performance_stats(void) {
    return perf_stats;
}

void lusush_reset_menu_performance_stats(void) {
    memset(&perf_stats, 0, sizeof(perf_stats));
}

void lusush_debug_menu_state(const menu_completion_state_t *state) {
    if (!state) {
        printf("Menu completion state: NULL\n");
        return;
    }
    
    printf("Menu completion state:\n");
    printf("  Active: %s\n", state->active ? "yes" : "no");
    printf("  Count: %d\n", state->count);
    printf("  Selected: %d\n", state->selected_index);
    printf("  Columns: %d\n", state->display_columns);
    printf("  Rows: %d\n", state->display_rows);
    printf("  Original text: %s\n", state->original_text ? state->original_text : "(null)");
    printf("  Common prefix: %s\n", state->common_prefix ? state->common_prefix : "(null)");
    
    if (state->completions) {
        printf("  Completions:\n");
        for (int i = 0; i < state->count && i < 5; i++) { // Show first 5
            printf("    [%d] %s\n", i, state->completions[i] ? state->completions[i] : "(null)");
        }
        if (state->count > 5) {
            printf("    ... and %d more\n", state->count - 5);
        }
    }
}

// ============================================================================
// GLOBAL STATE ACCESS
// ============================================================================

const menu_completion_state_t *lusush_get_menu_completion_state(void) {
    return initialized ? &menu_state : NULL;
}

bool lusush_is_menu_completion_in_progress(void) {
    return initialized && menu_state.active;
}