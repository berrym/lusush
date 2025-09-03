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
 * 
 * IMPORTANT: All hash table implementations in this module MUST use lusush's
 * existing src/libhashtable library for consistency with the rest of the 
 * codebase. Do not implement custom hash tables or use external libraries.
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
#include <readline/history.h>

#include "../include/menu_completion.h"
#include "../include/config.h"
#include "../include/themes.h"
#include "../include/rich_completion.h"
#include "../include/completion.h"
#include "../include/autosuggestions.h"
#include "../include/libhashtable/ht.h"

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

// Hash table for fast description lookups (MUST use src/libhashtable)
static ht_strstr_t *description_cache = NULL;

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

// Forward declarations for hash table functions
static void init_description_cache(void);
static char *get_fast_description(const char *command);
static void cleanup_description_cache(void);

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
            if (state->completions[i]) {
                free(state->completions[i]);
                state->completions[i] = NULL;
            }
        }
        free(state->completions);
        state->completions = NULL;
    }

    if (state->descriptions) {
        for (int i = 0; i < state->count; i++) {
            if (state->descriptions[i]) {
                free(state->descriptions[i]);
                state->descriptions[i] = NULL;
            }
        }
        free(state->descriptions);
        state->descriptions = NULL;
    }

    if (state->original_text) {
        free(state->original_text);
        state->original_text = NULL;
    }
    
    if (state->common_prefix) {
        free(state->common_prefix);
        state->common_prefix = NULL;
    }
    
    state->count = 0;
    state->selected_index = 0;
    state->active = false;
}

/**
 * Initialize description cache using src/libhashtable
 * IMPORTANT: Uses lusush's existing libhashtable for consistency
 */
static void init_description_cache(void) {
    if (description_cache) return;
    
    description_cache = ht_strstr_create(128); // Initial size for common commands
    if (!description_cache) {
        fprintf(stderr, "lusush: menu_completion: failed to create description cache\n");
        return;
    }
    
    // Pre-populate with common commands for O(1) lookup performance
    ht_strstr_insert(description_cache, "ls", "list directory contents");
    ht_strstr_insert(description_cache, "cd", "change directory");
    ht_strstr_insert(description_cache, "pwd", "print working directory");
    ht_strstr_insert(description_cache, "echo", "display text");
    ht_strstr_insert(description_cache, "cat", "display file contents");
    ht_strstr_insert(description_cache, "cp", "copy files");
    ht_strstr_insert(description_cache, "mv", "move/rename files");
    ht_strstr_insert(description_cache, "rm", "remove files");
    ht_strstr_insert(description_cache, "mkdir", "create directory");
    ht_strstr_insert(description_cache, "rmdir", "remove directory");
    ht_strstr_insert(description_cache, "grep", "search text patterns");
    ht_strstr_insert(description_cache, "find", "find files and directories");
    ht_strstr_insert(description_cache, "ps", "show running processes");
    ht_strstr_insert(description_cache, "kill", "terminate processes");
    ht_strstr_insert(description_cache, "chmod", "change file permissions");
    ht_strstr_insert(description_cache, "chown", "change file ownership");
    ht_strstr_insert(description_cache, "tar", "archive files");
    ht_strstr_insert(description_cache, "zip", "compress files");
    ht_strstr_insert(description_cache, "unzip", "extract compressed files");
    ht_strstr_insert(description_cache, "git", "version control system");
    ht_strstr_insert(description_cache, "ssh", "secure shell connection");
    ht_strstr_insert(description_cache, "curl", "transfer data from servers");
    ht_strstr_insert(description_cache, "wget", "download files");
    ht_strstr_insert(description_cache, "vi", "text editor");
    ht_strstr_insert(description_cache, "vim", "enhanced text editor");
    ht_strstr_insert(description_cache, "nano", "simple text editor");
    ht_strstr_insert(description_cache, "less", "view file contents");
    ht_strstr_insert(description_cache, "more", "view file contents page by page");
    ht_strstr_insert(description_cache, "head", "show first lines of file");
    ht_strstr_insert(description_cache, "tail", "show last lines of file");
    ht_strstr_insert(description_cache, "sort", "sort lines of text");
    ht_strstr_insert(description_cache, "uniq", "report unique lines");
    ht_strstr_insert(description_cache, "wc", "count lines, words, characters");
    ht_strstr_insert(description_cache, "diff", "compare files");
    ht_strstr_insert(description_cache, "which", "locate command");
    ht_strstr_insert(description_cache, "whereis", "locate binary, source, manual");
    ht_strstr_insert(description_cache, "man", "display manual pages");
    ht_strstr_insert(description_cache, "history", "command history");
    ht_strstr_insert(description_cache, "alias", "create command aliases");
    ht_strstr_insert(description_cache, "export", "set environment variables");
    ht_strstr_insert(description_cache, "env", "display environment variables");
    ht_strstr_insert(description_cache, "date", "display or set date");
    ht_strstr_insert(description_cache, "cal", "display calendar");
    ht_strstr_insert(description_cache, "df", "display filesystem disk space");
    ht_strstr_insert(description_cache, "du", "display directory space usage");
    ht_strstr_insert(description_cache, "free", "display memory usage");
    ht_strstr_insert(description_cache, "top", "display running processes");
    ht_strstr_insert(description_cache, "htop", "interactive process viewer");
    ht_strstr_insert(description_cache, "uptime", "show system uptime");
    ht_strstr_insert(description_cache, "whoami", "display current username");
    ht_strstr_insert(description_cache, "id", "display user and group IDs");
    ht_strstr_insert(description_cache, "su", "switch user");
    ht_strstr_insert(description_cache, "sudo", "execute as another user");
    ht_strstr_insert(description_cache, "mount", "mount filesystem");
    ht_strstr_insert(description_cache, "umount", "unmount filesystem");
    ht_strstr_insert(description_cache, "lsof", "list open files");
    ht_strstr_insert(description_cache, "netstat", "display network connections");
    ht_strstr_insert(description_cache, "ping", "test network connectivity");
    ht_strstr_insert(description_cache, "nslookup", "query DNS records");
    ht_strstr_insert(description_cache, "banner", "display large text banner");
    ht_strstr_insert(description_cache, "base32", "base32 encode/decode");
    ht_strstr_insert(description_cache, "base64", "base64 encode/decode");
    ht_strstr_insert(description_cache, "basename", "extract filename from path");
    ht_strstr_insert(description_cache, "basenc", "encode/decode data");
    ht_strstr_insert(description_cache, "bash", "Bourne Again Shell");
    ht_strstr_insert(description_cache, "bashbug", "report bash bugs");
    ht_strstr_insert(description_cache, "batch", "execute commands at specified time");
}

/**
 * Get fast description using src/libhashtable cache
 * IMPORTANT: Uses lusush's existing libhashtable for O(1) performance
 */
static char *get_fast_description(const char *command) {
    if (!command) return NULL;
    
    // Initialize cache on first use
    if (!description_cache) {
        init_description_cache();
    }
    
    if (!description_cache) return NULL;
    
    // O(1) hash table lookup using lusush's libhashtable
    const char *desc = ht_strstr_get(description_cache, command);
    return desc ? safe_strdup(desc) : NULL;
}

/**
 * Generate simple description for common commands when rich completion unavailable
 * 
 * NOTE: Now uses src/libhashtable for optimal performance and consistency
 * with the rest of lusush codebase.
 */
static char *lusush_generate_simple_description(const char *command) {
    if (!command) return NULL;
    
    // Use fast hash table lookup instead of linear search
    // This provides O(1) performance using src/libhashtable
    return get_fast_description(command);
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

    // Initialize description cache for fast lookups
    init_description_cache();
    
    initialized = true;
    return true;
}

void lusush_menu_completion_cleanup(void) {
    if (!initialized) return;
    
    free_menu_state(&menu_state);
    cleanup_description_cache();
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
    
    // Calculate column layout with responsive width handling
    int separator_width = 2; // "  " between columns
    int description_separator = layout.has_descriptions ? 3 : 0; // " - " before description
    
    layout.column_width = layout.max_completion_width;
    if (layout.has_descriptions && layout.terminal_width > 80) {
        // Only show descriptions on wider terminals
        int max_desc_width = layout.max_description_width;
        
        // Limit description width based on terminal size
        if (layout.terminal_width < 120) {
            max_desc_width = (max_desc_width > 20) ? 20 : max_desc_width;
        } else if (layout.terminal_width < 100) {
            max_desc_width = (max_desc_width > 15) ? 15 : max_desc_width;
        }
        
        layout.column_width += description_separator + max_desc_width;
    } else if (layout.has_descriptions && layout.terminal_width <= 80) {
        // Disable descriptions on narrow terminals for better fit
        layout.has_descriptions = false;
    }
    
    // Calculate number of columns with better responsive design
    if (config.menu_completion_max_columns > 0) {
        layout.columns = config.menu_completion_max_columns;
    } else {
        // Auto-calculate based on terminal width with responsive breakpoints
        int available_width = layout.terminal_width - 4; // Leave more margin
        int min_column_width = layout.column_width + separator_width;
        
        // Responsive breakpoints for different terminal sizes
        if (layout.terminal_width < 60) {
            // Very narrow terminals - single column
            layout.columns = 1;
        } else if (layout.terminal_width < 100) {
            // Narrow terminals - max 2-3 columns
            layout.columns = available_width / min_column_width;
            if (layout.columns > 3) layout.columns = 3;
        } else {
            // Wide terminals - normal calculation
            layout.columns = available_width / min_column_width;
            if (layout.columns > 6) layout.columns = 6; // Max 6 columns for readability
        }
        
        // Ensure minimum and maximum bounds
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
    
    // Clear current line and move to menu display area
    printf("\033[2K\r"); // Clear entire line and return to start
    printf("\n"); // Move to next line for menu
    
    // Get theme-aware colors
    const char *selection_color = NULL;
    const char *description_color = NULL;

    
    // Use theme colors if available, otherwise fall back to config or defaults
    if (config.menu_completion_selection_color && 
        strcmp(config.menu_completion_selection_color, "auto") != 0) {
        selection_color = config.menu_completion_selection_color;
    } else {
        // Get theme selection color (highlight color works well for selections)
        selection_color = theme_get_color("highlight");
        if (!selection_color || strlen(selection_color) == 0) {
            selection_color = DEFAULT_SELECTION_COLOR;
        }
    }
    
    if (config.menu_completion_description_color && 
        strcmp(config.menu_completion_description_color, "auto") != 0) {
        description_color = config.menu_completion_description_color;
    } else {
        // Get theme description color (text_dim works well for descriptions)
        description_color = theme_get_color("text_dim");
        if (!description_color || strlen(description_color) == 0) {
            description_color = DEFAULT_DESCRIPTION_COLOR;
        }
    }
    
    for (int row = 0; row < layout.rows; row++) {
        for (int col = 0; col < layout.columns; col++) {
            int index = row * layout.columns + col;
            if (index >= state->count) break;
            
            bool is_selected = (index == state->selected_index);
            
            // Enhanced display with theme-aware colors and rich descriptions
            if (is_selected && config.menu_completion_show_selection_highlight) {
                printf("%s", selection_color);
            }
            
            // Print completion name
            printf("%s", state->completions[index]);
            
            // Add description if available with better formatting
            if (layout.has_descriptions && state->descriptions[index]) {
                // Use description color
                printf("%s - %s%s", description_color, state->descriptions[index], COLOR_RESET);
            }
            
            // Calculate padding for column alignment (simplified for performance)
            int content_length = strlen(state->completions[index]);
            if (layout.has_descriptions && state->descriptions[index]) {
                content_length += 3 + strlen(state->descriptions[index]); // " - " + description
            }
            
            // Add padding to maintain column width
            int padding = layout.column_width - content_length;
            if (padding > 0 && padding < 50) { // Reasonable padding limit
                printf("%*s", padding, "");
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
    
    // Don't reposition cursor - let readline handle it naturally
    
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
        return rl_complete(count, key);
    }
    
    double start_time = get_time_ms();
    
    // Clear any autosuggestion remnants before completion
    lusush_dismiss_suggestion();
    printf("\033[K");  // Clear to end of line
    fflush(stdout);
    
    // Update menu state selection if cycling through existing menu
    if (menu_state.active && menu_state.count > 0) {
        // Move to next completion
        menu_state.selected_index = (menu_state.selected_index + 1) % menu_state.count;
        
        // Replace the current input with selected completion
        if (menu_state.completions[menu_state.selected_index]) {
            rl_delete_text(0, rl_end);
            rl_insert_text(menu_state.completions[menu_state.selected_index]);
            rl_point = rl_end;
        }
        
        // Re-render menu with updated selection
        lusush_render_completion_menu(&menu_state);
        
        // Update performance stats
        double elapsed = get_time_ms() - start_time;
        perf_stats.avg_cycling_time_ms = 
            (perf_stats.avg_cycling_time_ms * perf_stats.total_completions + elapsed)
            / (perf_stats.total_completions + 1);
        perf_stats.total_completions++;
        
        return 0; // Success, handled internally
    }
    
    // First time completion - use standard completion to generate matches
    int result = rl_complete(count, key);
    
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
    
    // Update menu state selection if cycling through existing menu
    if (menu_state.active && menu_state.count > 0) {
        // Move to previous completion (wrap around)
        menu_state.selected_index = (menu_state.selected_index - 1 + menu_state.count) % menu_state.count;
        
        // Replace the current input with selected completion
        if (menu_state.completions[menu_state.selected_index]) {
            rl_delete_text(0, rl_end);
            rl_insert_text(menu_state.completions[menu_state.selected_index]);
            rl_point = rl_end;
        }
        
        // Re-render menu with updated selection
        lusush_render_completion_menu(&menu_state);
        
        // Update performance stats
        double elapsed = get_time_ms() - start_time;
        perf_stats.avg_cycling_time_ms = 
            (perf_stats.avg_cycling_time_ms * perf_stats.total_completions + elapsed)
            / (perf_stats.total_completions + 1);
        perf_stats.total_completions++;
        
        return 0; // Success, handled internally
    }
    
    // First time completion - use standard completion to generate matches
    int result = rl_complete(count, key);
    
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
    
    // Don't display menu immediately - let the completion happen first
    // The menu will be shown during cycling
    if (!menu_state.active) {
        // Store matches for future cycling but don't display menu yet
        lusush_prepare_menu_state(matches, len);
        
        // Use standard readline display for first completion
        rl_display_match_list(matches, len, max);
        return;
    }
    
    // Skip first element (common prefix) and create our state
    int actual_count = len - 1;
    if (actual_count <= 0) {
        return;
    }
    
    // Free previous state safely
    if (menu_state.active) {
        free_menu_state(&menu_state);
    }
    
    // Allocate new state with zero initialization
    menu_state.completions = calloc(actual_count, sizeof(char*));
    menu_state.descriptions = calloc(actual_count, sizeof(char*));
    
    if (!menu_state.completions || !menu_state.descriptions) {
        fprintf(stderr, "lusush: menu_completion: memory allocation failed\n");
        rl_display_match_list(matches, len, max);
        return;
    }
    
    // Copy completions and get rich descriptions efficiently (skip first element which is common prefix)
    menu_state.count = actual_count;
    
    // Skip expensive rich completion lookup for initial display performance
    rich_completion_list_t* rich_list = NULL;
    
    for (int i = 0; i < actual_count; i++) {
        menu_state.completions[i] = safe_strdup(matches[i + 1]);
        menu_state.descriptions[i] = NULL; // Default to no description
        
        // Use fast hash table lookup for optimal performance
        menu_state.descriptions[i] = get_fast_description(matches[i + 1]);
    }
    
    // No rich list cleanup needed for performance optimization
    
    menu_state.selected_index = 0;
    menu_state.active = true;
    menu_state.common_prefix = safe_strdup(matches[0]);
    
    // Don't render immediately - will render during cycling
}

// Helper function to prepare menu state without displaying
void lusush_prepare_menu_state(char **matches, int len) {
    // Skip first element (common prefix) and create our state
    int actual_count = len - 1;
    if (actual_count <= 0) {
        return;
    }
    
    // Free previous state safely
    if (menu_state.active) {
        free_menu_state(&menu_state);
    }
    
    // Allocate new state with zero initialization
    menu_state.completions = calloc(actual_count, sizeof(char*));
    menu_state.descriptions = calloc(actual_count, sizeof(char*));
    
    if (!menu_state.completions || !menu_state.descriptions) {
        fprintf(stderr, "lusush: menu_completion: memory allocation failed\n");
        return;
    }
    
    // Copy completions and get rich descriptions efficiently
    menu_state.count = actual_count;
    
    // Use fast simple descriptions for initial performance
    rich_completion_list_t* rich_list = NULL;
    
    for (int i = 0; i < actual_count; i++) {
        menu_state.completions[i] = safe_strdup(matches[i + 1]);
        menu_state.descriptions[i] = NULL;
        
        // Fast hash table descriptions only
        menu_state.descriptions[i] = get_fast_description(matches[i + 1]);
    }
    
    // No rich list cleanup needed for performance optimization
    
    menu_state.selected_index = 0;
    menu_state.active = true;
    menu_state.common_prefix = safe_strdup(matches[0]);
}

void lusush_menu_completion_reset(void) {
    free_menu_state(&menu_state);
}

/**
 * Cleanup description cache using src/libhashtable
 * IMPORTANT: Properly cleanup libhashtable resources
 */
static void cleanup_description_cache(void) {
    if (description_cache) {
        ht_strstr_destroy(description_cache);
        description_cache = NULL;
    }
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