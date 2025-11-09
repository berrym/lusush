/*
 * Lusush Shell - Layered Display Architecture
 * Display Controller Implementation - High-Level Display Management System
 * 
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
 *
 * ============================================================================
 * 
 * DISPLAY CONTROLLER IMPLEMENTATION
 * 
 * This file implements the high-level display controller that coordinates all
 * display layers of the Lusush layered display architecture. It provides
 * system-wide performance monitoring, intelligent caching, and optimization
 * for enterprise-grade shell display functionality.
 * 
 * Key Features Implemented:
 * - High-level coordination of composition engine and all display layers
 * - System-wide performance monitoring with adaptive optimization
 * - Display state caching with intelligent diff algorithms
 * - Enterprise-grade configuration management
 * - Integration preparation for existing shell functions
 * - Memory-safe resource management with comprehensive error handling
 * 
 * Architecture Integration:
 * This controller serves as the top-level coordination layer in the display
 * architecture, managing the composition engine, terminal control, and event
 * systems to provide a unified interface for shell integration.
 */

#include "display/display_controller.h"
#include "display/base_terminal.h"
#include "display/prompt_layer.h"
#include "display/command_layer.h"
#include "display/continuation_prompt_layer.h"
#include "display/screen_buffer.h"
#include "display_integration.h"
#include "lusush_memory_pool.h"
#include "lle/utf8_support.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>

// ============================================================================
// INTERNAL CONSTANTS AND MACROS
// ============================================================================

#define DC_HASH_SEED 0x811c9dc5
#define DC_HASH_PRIME 0x01000193
#define DC_MAX_STATE_HASH_LENGTH 65
#define DC_CACHE_CLEANUP_INTERVAL_MS 30000
#define DC_PERFORMANCE_UPDATE_INTERVAL_MS 100
#define DC_ADAPTIVE_OPTIMIZATION_THRESHOLD 5

// Debugging and logging macros
#if 0  // DC_DEBUG disabled - interferes with display
#define DC_DEBUG(fmt, ...) \
    fprintf(stderr, "[DC_DEBUG] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DC_DEBUG(fmt, ...) do { } while(0)
#endif

#define DC_ERROR(fmt, ...) \
    fprintf(stderr, "[DC_ERROR] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

// ============================================================================
// INTERNAL UTILITY FUNCTIONS
// ============================================================================

/**
 * Get current timestamp in microseconds.
 */
static uint64_t dc_get_timestamp_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

/**
 * Handle LAYER_EVENT_REDRAW_NEEDED from command_layer
 * 
 * This is the critical event handler that connects command_layer updates
 * to actual terminal rendering. When LLE updates the command_layer content,
 * command_layer publishes REDRAW_NEEDED, and this handler renders it.
 * 
 * @param event The redraw event
 * @param user_data Pointer to display_controller_t instance
 * @return LAYER_EVENTS_SUCCESS on success, error code on failure
 */
/* SCREEN BUFFER SYSTEM - The proper way to handle terminal updates
 * 
 * Instead of trying to move cursor back and redraw (which is unreliable),
 * we maintain virtual screen buffers representing what's on the terminal.
 * On each update, we diff old vs new and apply only the changes.
 * 
 * This is the proven approach used by ZLE, Fish, and Replxx.
 */
static screen_buffer_t current_screen;
static screen_buffer_t desired_screen;
static bool screen_buffer_initialized = false;
static bool prompt_rendered = false;

/**
 * Reset display state - called when starting new input session
 * 
 * Clears screen buffers so next render starts fresh.
 */
void dc_reset_prompt_display_state(void) {
    prompt_rendered = false;
    if (screen_buffer_initialized) {
        screen_buffer_clear(&current_screen);
        screen_buffer_clear(&desired_screen);
    }
}

/**
 * Finalize input and prepare for command output
 * 
 * Writes newline to terminal to move cursor below the input, then
 * resets display state for the next prompt.
 */
void dc_finalize_input(void) {
    /* Write newline to move cursor to next line for command output */
    write(STDOUT_FILENO, "\n", 1);
    
    /* Reset display state for next prompt */
    dc_reset_prompt_display_state();
}

/**
 * Mark prompt as needing redraw - called when prompt content changes
 * (No longer needed with always-redraw approach, kept for API compatibility)
 */
static void dc_invalidate_prompt_cache(void) {
    /* No-op: we always redraw everything now */
}

static layer_events_error_t dc_handle_redraw_needed(
    const layer_event_t *event,
    void *user_data) {
    
    (void)event;
    display_controller_t *controller = (display_controller_t *)user_data;
    
    if (!controller || !controller->is_initialized) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }
    
    command_layer_t *cmd_layer = controller->compositor->command_layer;
    if (!cmd_layer) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    int term_width = 80;
    if (controller->terminal_ctrl && controller->terminal_ctrl->capabilities.terminal_width > 0) {
        term_width = controller->terminal_ctrl->capabilities.terminal_width;
    }

    if (!screen_buffer_initialized) {
        screen_buffer_init(&current_screen, term_width);
        screen_buffer_init(&desired_screen, term_width);
        screen_buffer_initialized = true;
    } else {
        current_screen.terminal_width = term_width;
        desired_screen.terminal_width = term_width;
    }

    prompt_layer_t *prompt_layer = controller->compositor->prompt_layer;
    char prompt_buffer[PROMPT_LAYER_MAX_CONTENT_SIZE] = {0};
    if (prompt_layer) {
        prompt_layer_get_rendered_content(prompt_layer, prompt_buffer, sizeof(prompt_buffer));
    }

    char command_buffer[COMMAND_LAYER_MAX_HIGHLIGHTED_SIZE];
    command_layer_error_t cmd_result = command_layer_get_highlighted_text(
        cmd_layer, command_buffer, sizeof(command_buffer));
    
    if (cmd_result != COMMAND_LAYER_SUCCESS) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }

    size_t cursor_byte_offset = cmd_layer->cursor_position;
    screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);

    /* Apply continuation prompts to screen buffer lines (line 1+) */
    if (controller->continuation_layer && desired_screen.num_rows > 1) {
        /* Get the full command text from the command layer */
        const char *full_command = cmd_layer->command_text;
        
        /* Set prefix for each continuation line (skip line 0, which is the primary prompt) */
        for (int line = 1; line < desired_screen.num_rows; line++) {
            char prefix[CONTINUATION_PROMPT_MAX_LENGTH];
            continuation_prompt_error_t cont_result = continuation_prompt_layer_get_prompt_for_line(
                controller->continuation_layer,
                line,  /* line_number (1-based for continuation lines) */
                full_command,
                prefix,
                sizeof(prefix)
            );
            
            if (cont_result == CONTINUATION_PROMPT_SUCCESS) {
                screen_buffer_set_line_prefix(&desired_screen, line, prefix);
            }
        }
    }

    /* Use clear and redraw approach with cursor tracking
     * We'll output everything, track where cursor should be, then reposition */
    
    /* Move cursor to start of content for redraw
     * After previous redraw, cursor is at (current_screen.cursor_row, current_screen.cursor_col)
     * We need to move to row 0 (start of prompt) to begin redrawing */
    if (prompt_rendered) {
        /* Move up from current cursor row to row 0 */
        if (current_screen.cursor_row > 0) {
            char up_seq[16];
            int up_len = snprintf(up_seq, sizeof(up_seq), "\033[%dA", current_screen.cursor_row);
            if (up_len > 0) {
                write(STDOUT_FILENO, up_seq, up_len);
            }
        }
    }
    
    /* Move to beginning of line */
    write(STDOUT_FILENO, "\r", 1);
    
    /* Clear from cursor to end of screen (clears current line and any wrapped lines below) */
    write(STDOUT_FILENO, "\033[J", 3);
    
    /* For single-line or simple rendering, use original approach */
    if (desired_screen.num_rows == 1 || !controller->continuation_layer) {
        /* Draw prompt */
        if (prompt_buffer[0]) {
            write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
        }
        
        /* Draw command text completely */
        if (command_buffer[0]) {
            write(STDOUT_FILENO, command_buffer, strlen(command_buffer));
        }
    } else {
        /* Multi-line with continuation prompts: manually split and insert prefixes */
        /* First line: prompt + start of command up to first \n */
        if (prompt_buffer[0]) {
            write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
        }
        
        /* Split command_buffer by newlines and insert continuation prompts */
        const char *line_start = command_buffer;
        const char *line_end;
        int line_num = 0;
        
        while (*line_start) {
            line_end = strchr(line_start, '\n');
            
            if (line_end) {
                /* Write this line */
                write(STDOUT_FILENO, line_start, line_end - line_start);
                write(STDOUT_FILENO, "\n", 1);
                
                line_num++;
                line_start = line_end + 1;
                
                /* Insert continuation prompt for next line
                 * NOTE: We need to show the prompt even if the line is empty (*line_start == 0)
                 * because the user needs to see where to type on continuation lines */
                if (controller->continuation_layer && line_num < desired_screen.num_rows) {
                    const char *full_command = cmd_layer->command_text;
                    char prefix[CONTINUATION_PROMPT_MAX_LENGTH];
                    
                    continuation_prompt_error_t cont_result = continuation_prompt_layer_get_prompt_for_line(
                        controller->continuation_layer,
                        line_num,
                        full_command,
                        prefix,
                        sizeof(prefix)
                    );
                    
                    if (cont_result == CONTINUATION_PROMPT_SUCCESS) {
                        write(STDOUT_FILENO, prefix, strlen(prefix));
                    }
                }
            } else {
                /* Last line (no newline) */
                write(STDOUT_FILENO, line_start, strlen(line_start));
                break;
            }
        }
    }
    
    /* Position cursor using row/col from screen_buffer 
     * The screen_buffer_render() calculated the exact cursor position accounting
     * for line wrapping, prompt width, and UTF-8 character widths. */
    int cursor_row = desired_screen.cursor_row;
    int cursor_col = desired_screen.cursor_col;
    
    /* If cursor is on a continuation line (row > 0), account for continuation prompt width */
    if (cursor_row > 0 && controller->continuation_layer) {
        const char *full_command = cmd_layer->command_text;
        char prefix[CONTINUATION_PROMPT_MAX_LENGTH];
        
        continuation_prompt_error_t cont_result = continuation_prompt_layer_get_prompt_for_line(
            controller->continuation_layer,
            cursor_row,
            full_command,
            prefix,
            sizeof(prefix)
        );
        
        if (cont_result == CONTINUATION_PROMPT_SUCCESS) {
            /* Add the visual width of the continuation prompt 
             * Use screen_buffer_calculate_visual_width() to properly account for:
             * - ANSI escape codes (color/formatting)
             * - Wide Unicode characters
             * - Tab characters */
            size_t prefix_visual_width = screen_buffer_calculate_visual_width(prefix, 0);
            cursor_col += (int)prefix_visual_width;
        }
    }
    
    /* We just drew everything, so cursor is at the end
     * Calculate how to move cursor to the correct position */
    int final_row = desired_screen.num_rows - 1;  /* Last row of rendered content (0-based) */
    
    /* Move cursor to correct position:
     * 1. If cursor is on an earlier row, move UP
     * 2. Then move to column 0 with \r
     * 3. Then move RIGHT to correct column */
    
    int rows_to_move_up = final_row - cursor_row;
    if (rows_to_move_up > 0) {
        char up_seq[16];
        int up_len = snprintf(up_seq, sizeof(up_seq), "\033[%dA", rows_to_move_up);
        if (up_len > 0) {
            write(STDOUT_FILENO, up_seq, up_len);
        }
    }
    
    /* Move to beginning of line */
    write(STDOUT_FILENO, "\r", 1);
    
    /* Move right to cursor column if needed */
    if (cursor_col > 0) {
        char right_seq[16];
        int right_len = snprintf(right_seq, sizeof(right_seq), "\033[%dC", cursor_col);
        if (right_len > 0) {
            write(STDOUT_FILENO, right_seq, right_len);
        }
    }
    
    screen_buffer_copy(&current_screen, &desired_screen);
    prompt_rendered = true;

    /* NOTE: fsync() was causing input timeouts after cursor positioning - removed
     * stdout is line-buffered by default and terminal I/O doesn't need fsync */
    
    return LAYER_EVENTS_SUCCESS;
}

/**
 * Calculate time difference in nanoseconds.
 */
static uint64_t dc_time_diff_ns(const struct timeval *start, const struct timeval *end) {
    if (!start || !end) return 0;
    
    uint64_t start_ns = (uint64_t)start->tv_sec * 1000000000 + (uint64_t)start->tv_usec * 1000;
    uint64_t end_ns = (uint64_t)end->tv_sec * 1000000000 + (uint64_t)end->tv_usec * 1000;
    
    return (end_ns > start_ns) ? (end_ns - start_ns) : 0;
}

/**
 * Simple FNV-1a hash function for state comparison.
 */
static uint32_t dc_hash_string(const char *str) {
    if (!str) return 0;
    
    uint32_t hash = DC_HASH_SEED;
    const unsigned char *data = (const unsigned char *)str;
    
    while (*data) {
        hash ^= *data++;
        hash *= DC_HASH_PRIME;
    }
    
    return hash;
}

/**
 * Extract semantic components from prompt for intelligent caching.
 * Timestamp-aware normalization that ignores dynamic time elements.
 */
static uint32_t dc_hash_prompt_semantic(const char *prompt) {
    if (!prompt || *prompt == '\0') return DC_HASH_SEED;
    
    // For maximum cache hit rates, normalize prompt by removing time-sensitive elements
    char normalized[512] = {0};
    const char *p = prompt;
    int pos = 0;
    bool in_color_seq = false;
    bool skip_timestamp = false;
    
    while (*p && pos < sizeof(normalized) - 1) {
        if (*p == '\033' || *p == '\x1b') {
            in_color_seq = true;
            normalized[pos++] = 'C';  // Color marker only
            p++;
            continue;
        }
        
        if (in_color_seq) {
            if (*p == 'm' || *p == 'K' || *p == 'J' || *p == 'H') {
                in_color_seq = false;
            }
            p++;
            continue;
        }
        
        // Detect and skip timestamp patterns (common formats)
        // Skip sequences like "Mon Jan 27 15:30:42 2025" or "15:30:42" or "Jan 27"
        if ((*p >= '0' && *p <= '9') && (*(p+1) == ':' || 
            (isdigit(*(p+1)) && *(p+2) == ':'))) {
            // Skip time patterns like "15:30:42" or "3:45"
            skip_timestamp = true;
        } else if (skip_timestamp && (*p == ' ' || *p == '\t' || *p == '\n')) {
            skip_timestamp = false;
            p++;
            continue;
        } else if (skip_timestamp) {
            p++;
            continue;
        }
        
        // Skip common date patterns
        if ((strncmp(p, "Mon ", 4) == 0) || (strncmp(p, "Tue ", 4) == 0) ||
            (strncmp(p, "Wed ", 4) == 0) || (strncmp(p, "Thu ", 4) == 0) ||
            (strncmp(p, "Fri ", 4) == 0) || (strncmp(p, "Sat ", 4) == 0) ||
            (strncmp(p, "Sun ", 4) == 0) || (strncmp(p, "Jan ", 4) == 0) ||
            (strncmp(p, "Feb ", 4) == 0) || (strncmp(p, "Mar ", 4) == 0) ||
            (strncmp(p, "Apr ", 4) == 0) || (strncmp(p, "May ", 4) == 0) ||
            (strncmp(p, "Jun ", 4) == 0) || (strncmp(p, "Jul ", 4) == 0) ||
            (strncmp(p, "Aug ", 4) == 0) || (strncmp(p, "Sep ", 4) == 0) ||
            (strncmp(p, "Oct ", 4) == 0) || (strncmp(p, "Nov ", 4) == 0) ||
            (strncmp(p, "Dec ", 4) == 0)) {
            // Skip to end of timestamp section
            while (*p && *p != '\n' && *p != '$' && *p != '#' && *p != '>') {
                p++;
            }
            continue;
        }
        
        // Normalize structural elements
        if (*p == '$' || *p == '#' || *p == '>') {
            normalized[pos++] = 'P';  // Prompt end marker
        } else if (*p == '[' || *p == '(' || *p == '{') {
            normalized[pos++] = 'B';  // Begin bracket
        } else if (*p == ']' || *p == ')' || *p == '}') {
            normalized[pos++] = 'E';  // End bracket  
        } else if (*p == '/' || *p == '~') {
            normalized[pos++] = 'D';  // Directory marker
        } else if (*p == '@') {
            normalized[pos++] = 'A';  // User@host separator
        } else if (isalpha(*p)) {
            normalized[pos++] = tolower(*p);  // Normalize case
        } else if (*p != ' ' && *p != '\t' && *p != '\n') {
            normalized[pos++] = *p;  // Keep other significant chars
        }
        
        p++;
    }
    
    normalized[pos] = '\0';
    
    // Hash the normalized prompt structure
    uint32_t hash = DC_HASH_SEED;
    const unsigned char *data = (const unsigned char *)normalized;
    while (*data) {
        hash ^= *data++;
        hash *= DC_HASH_PRIME;
    }
    
    return hash;
}

/**
 * Extract semantic components from command for intelligent caching.
 * Deterministic command classification for consistent cache grouping.
 */
static uint32_t dc_hash_command_semantic(const char *command) {
    if (!command || *command == '\0') return DC_HASH_SEED;
    
    // Simplified but more predictable command classification
    uint32_t cmd_hash = DC_HASH_SEED;
    const char *p = command;
    
    // Skip leading whitespace
    while (*p && isspace(*p)) p++;
    
    if (!*p) return DC_HASH_SEED;
    
    // Extract base command for classification
    char base_cmd[32] = {0};
    int i = 0;
    while (*p && !isspace(*p) && i < sizeof(base_cmd) - 1) {
        base_cmd[i++] = tolower(*p++);
    }
    
    // Hash the base command
    for (i = 0; base_cmd[i]; i++) {
        cmd_hash ^= (unsigned char)base_cmd[i];
        cmd_hash *= DC_HASH_PRIME;
    }
    
    // Simple argument classification (not exact matching)
    while (*p && isspace(*p)) p++;
    int has_flags = 0, has_args = 0;
    
    while (*p) {
        if (*p == '-') {
            has_flags = 1;
            while (*p && !isspace(*p)) p++;
        } else if (!isspace(*p)) {
            has_args = 1;
            while (*p && !isspace(*p)) p++;
        } else {
            p++;
        }
    }
    
    // Include argument pattern in hash for differentiation
    cmd_hash ^= (has_flags << 8) | (has_args << 4);
    
    return cmd_hash;
}

/**
 * Generate intelligent state hash for caching and comparison.
 * Balanced approach for optimal cache hit rates with necessary differentiation.
 */
static void dc_generate_state_hash(const char *prompt, const char *command,
                                   const char *theme_name, symbol_compatibility_t symbol_mode,
                                   char *hash_buffer, size_t buffer_size) {
    if (!hash_buffer || buffer_size < DC_MAX_STATE_HASH_LENGTH) return;
    
    // Get semantic hashes for grouping similar states
    uint32_t prompt_semantic = dc_hash_prompt_semantic(prompt);
    uint32_t command_semantic = dc_hash_command_semantic(command);
    
    // Add theme context to hash calculation
    uint32_t theme_hash = dc_hash_string(theme_name ? theme_name : "default");
    uint32_t symbol_hash = (uint32_t)symbol_mode;
    
    // Create primary hash from semantic components including theme context
    uint32_t primary_hash = prompt_semantic ^ (command_semantic << 1) ^ (theme_hash << 2) ^ (symbol_hash << 3);
    
    // For identical repeated commands, ensure exact cache hits
    uint32_t exact_prompt = dc_hash_string(prompt ? prompt : "");
    uint32_t exact_command = dc_hash_string(command ? command : "");
    
    // Balance semantic grouping with exact matching
    // Use semantic hash as primary, exact hash for differentiation
    uint32_t secondary_hash = (exact_prompt & 0xFFFF) ^ ((exact_command & 0xFFFF) << 16);
    
    // Final combined hash: semantic-driven but with exact differentiation and theme context
    uint32_t combined_hash = primary_hash ^ secondary_hash;
    
    snprintf(hash_buffer, buffer_size, "s%08x_e%08x_c%08x_t%08x_m%x", 
             primary_hash, secondary_hash, combined_hash, theme_hash, symbol_hash);
}

/**
 * Initialize default configuration.
 */
static void dc_init_default_config(display_controller_config_t *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(display_controller_config_t));
    
    // Performance configuration
    config->optimization_level = DISPLAY_OPTIMIZATION_STANDARD;
    config->cache_ttl_ms = DISPLAY_CONTROLLER_DEFAULT_CACHE_TTL_MS;
    config->performance_monitor_interval_ms = DISPLAY_CONTROLLER_DEFAULT_MONITORING_INTERVAL_MS;
    config->max_cache_entries = 256;
    
    // Feature toggles
    config->enable_caching = true;
    config->enable_diff_algorithms = true;
    config->enable_performance_monitoring = true;
    config->enable_adaptive_optimization = true;
    config->enable_integration_mode = false;
    
    // Threshold configuration
    config->performance_threshold_ms = DISPLAY_CONTROLLER_PERFORMANCE_THRESHOLD_MS;
    config->cache_hit_rate_threshold = DISPLAY_CONTROLLER_CACHE_HIT_RATE_THRESHOLD;
    config->memory_threshold_mb = DISPLAY_CONTROLLER_MEMORY_THRESHOLD_MB;
    
    // Debug and diagnostics
    config->enable_debug_logging = false;
    config->enable_performance_profiling = false;
    config->log_file_path = NULL;
}

/**
 * Update performance history.
 */
static void dc_update_performance_history(display_controller_t *controller, uint64_t operation_time_ns) {
    if (!controller) return;
    
    controller->performance_history[controller->performance_history_index] = operation_time_ns;
    controller->performance_history_index = 
        (controller->performance_history_index + 1) % DISPLAY_CONTROLLER_PERFORMANCE_HISTORY_SIZE;
}

/**
 * Calculate average from performance history.
 */
static uint64_t dc_calculate_average_performance(const display_controller_t *controller) {
    if (!controller) return 0;
    
    uint64_t total = 0;
    size_t count = 0;
    
    for (size_t i = 0; i < DISPLAY_CONTROLLER_PERFORMANCE_HISTORY_SIZE; i++) {
        if (controller->performance_history[i] > 0) {
            total += controller->performance_history[i];
            count++;
        }
    }
    
    return count > 0 ? total / count : 0;
}

/**
 * Clean expired cache entries.
 */
static void dc_cleanup_expired_cache_entries(display_controller_t *controller) {
    if (!controller || !controller->cache_entries) return;
    
    struct timeval current_time;
    if (gettimeofday(&current_time, NULL) != 0) return;
    
    for (size_t i = 0; i < controller->cache_count; ) {
        display_cache_entry_t *entry = &controller->cache_entries[i];
        
        if (!entry->is_valid) {
            // Remove invalid entry
            if (entry->display_content) {
                lusush_pool_free(entry->display_content);
                entry->display_content = NULL;
            }
            if (entry->state_hash) {
                lusush_pool_free(entry->state_hash);
                entry->state_hash = NULL;
            }
            
            // Move last entry to this position
            if (i < controller->cache_count - 1) {
                *entry = controller->cache_entries[controller->cache_count - 1];
            }
            controller->cache_count--;
            continue;
        }
        
        // Calculate age
        uint64_t age_ms = ((uint64_t)current_time.tv_sec - (uint64_t)entry->timestamp.tv_sec) * 1000 +
                         ((uint64_t)current_time.tv_usec - (uint64_t)entry->timestamp.tv_usec) / 1000;
        
        // Adaptive TTL based on access frequency
        uint32_t adaptive_ttl_ms = controller->config.cache_ttl_ms;
        
        if (entry->access_count > 5) {
            // High-frequency entries get 4x longer TTL
            adaptive_ttl_ms *= 4;
        } else if (entry->access_count > 2) {
            // Medium-frequency entries get 2x longer TTL
            adaptive_ttl_ms *= 2;
        } else if (entry->access_count == 1) {
            // Single-use entries get much shorter TTL
            adaptive_ttl_ms = adaptive_ttl_ms / 3;
        }
        
        if (age_ms > adaptive_ttl_ms) {
            // Entry has expired
            if (entry->display_content) {
                lusush_pool_free(entry->display_content);
                entry->display_content = NULL;
            }
            if (entry->state_hash) {
                lusush_pool_free(entry->state_hash);
                entry->state_hash = NULL;
            }
            entry->is_valid = false;
            
            // Move last entry to this position
            if (i < controller->cache_count - 1) {
                *entry = controller->cache_entries[controller->cache_count - 1];
            }
            controller->cache_count--;
            continue;
        }
        
        i++;
    }
}

/**
 * Find cache entry by state hash.
 */
static display_cache_entry_t *dc_find_cache_entry(display_controller_t *controller, 
                                                   const char *state_hash) {
    if (!controller || !controller->cache_entries || !state_hash) return NULL;
    
    for (size_t i = 0; i < controller->cache_count; i++) {
        display_cache_entry_t *entry = &controller->cache_entries[i];
        if (entry->is_valid && entry->state_hash && 
            strcmp(entry->state_hash, state_hash) == 0) {
            entry->access_count++;
            return entry;
        }
    }
    
    return NULL;
}

/**
 * Add new cache entry.
 */
static display_controller_error_t dc_add_cache_entry(display_controller_t *controller,
                                                     const char *state_hash,
                                                     const char *display_content,
                                                     size_t content_length) {
    if (!controller || !state_hash || !display_content) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    // Check if cache is full
    if (controller->cache_count >= controller->cache_capacity) {
        // Find least recently used entry with improved protection for frequent entries
        size_t lru_index = 0;
        uint32_t min_score = UINT32_MAX;
        
        for (size_t i = 0; i < controller->cache_count; i++) {
            display_cache_entry_t *entry = &controller->cache_entries[i];
            
            // Calculate eviction score: lower score = more likely to evict
            // Protect frequently accessed entries by heavily weighting access count
            uint32_t score = entry->access_count * 1000;
            
            // Also consider recency (entries accessed recently get protection)
            struct timeval current_time;
            gettimeofday(&current_time, NULL);
            uint64_t age_ms = ((uint64_t)current_time.tv_sec - (uint64_t)entry->timestamp.tv_sec) * 1000 +
                             ((uint64_t)current_time.tv_usec - (uint64_t)entry->timestamp.tv_usec) / 1000;
            
            // Reduce score based on age (older entries have lower scores)
            if (age_ms > 5000) {
                score = score / 2;  // Halve score for entries older than 5 seconds
            }
            
            if (score < min_score) {
                min_score = score;
                lru_index = i;
            }
        }
        
        // Remove LRU entry
        display_cache_entry_t *lru_entry = &controller->cache_entries[lru_index];
        if (lru_entry->display_content) lusush_pool_free(lru_entry->display_content);
        if (lru_entry->state_hash) lusush_pool_free(lru_entry->state_hash);
        
        // Use this slot for new entry
        display_cache_entry_t *new_entry = lru_entry;
        memset(new_entry, 0, sizeof(display_cache_entry_t));
        
        new_entry->display_content = lusush_pool_alloc(content_length + 1);
        if (!new_entry->display_content) {
            return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
        }
        
        new_entry->state_hash = lusush_pool_alloc(strlen(state_hash) + 1);
        if (!new_entry->state_hash) {
            lusush_pool_free(new_entry->display_content);
            return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
        }
        
        memcpy(new_entry->display_content, display_content, content_length);
        new_entry->display_content[content_length] = '\0';
        strcpy(new_entry->state_hash, state_hash);
        new_entry->content_length = content_length;
        new_entry->access_count = 1;
        new_entry->is_valid = true;
        gettimeofday(&new_entry->timestamp, NULL);
    } else {
        // Add new entry
        display_cache_entry_t *new_entry = &controller->cache_entries[controller->cache_count];
        memset(new_entry, 0, sizeof(display_cache_entry_t));
        
        new_entry->display_content = lusush_pool_alloc(content_length + 1);
        if (!new_entry->display_content) {
            return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
        }
        
        new_entry->state_hash = lusush_pool_alloc(strlen(state_hash) + 1);
        if (!new_entry->state_hash) {
            lusush_pool_free(new_entry->display_content);
            return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
        }
        
        memcpy(new_entry->display_content, display_content, content_length);
        new_entry->display_content[content_length] = '\0';
        strcpy(new_entry->state_hash, state_hash);
        new_entry->content_length = content_length;
        new_entry->access_count = 1;
        new_entry->is_valid = true;
        gettimeofday(&new_entry->timestamp, NULL);
        
        controller->cache_count++;
    }
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

// ============================================================================
// CORE API IMPLEMENTATION
// ============================================================================

display_controller_t *display_controller_create(void) {
    display_controller_t *controller = malloc(sizeof(display_controller_t));
    if (!controller) {
        DC_ERROR("Failed to allocate memory for display controller");
        return NULL;
    }
    
    memset(controller, 0, sizeof(display_controller_t));
    
    // Initialize configuration with defaults
    dc_init_default_config(&controller->config);
    
    DC_DEBUG("Display controller created successfully");
    return controller;
}

display_controller_error_t display_controller_init(
    display_controller_t *controller,
    const display_controller_config_t *config,
    layer_event_system_t *event_system) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    DC_DEBUG("Initializing display controller");
    
    // Apply configuration
    if (config) {
        controller->config = *config;
    } else {
        dc_init_default_config(&controller->config);
    }
    
    // Initialize event system
    controller->event_system = event_system;
    
    // Create composition engine
    controller->compositor = composition_engine_create();
    if (!controller->compositor) {
        DC_ERROR("Failed to create composition engine");
        return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
    }
    
    // Create base terminal first (required for terminal control)
    base_terminal_t *base_terminal = base_terminal_create();
    if (!base_terminal) {
        DC_ERROR("Failed to create base terminal");
        composition_engine_destroy(controller->compositor);
        controller->compositor = NULL;
        return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
    }
    
    // CRITICAL: Initialize base_terminal BEFORE creating terminal_control
    // This sets up FDs and detects terminal capabilities
    // Only initialize if stdout is a TTY (won't work with pipes/redirects)
    if (isatty(STDOUT_FILENO)) {
        base_terminal_error_t bt_result = base_terminal_init(base_terminal);
        if (bt_result != BASE_TERMINAL_SUCCESS) {
            DC_ERROR("Failed to initialize base terminal (error %d) - using defaults", bt_result);
            // Non-fatal - will use default 80x24
        } else {
            DC_DEBUG("Base terminal initialized successfully");
        }
    } else {
        DC_DEBUG("stdout is not a TTY, skipping base_terminal initialization");
        // Will use default 80x24 when output is redirected
    }

    // Create terminal control context
    controller->terminal_ctrl = terminal_control_create(base_terminal);
    if (!controller->terminal_ctrl) {
        DC_ERROR("Failed to create terminal control context");
        base_terminal_destroy(base_terminal);
        composition_engine_destroy(controller->compositor);
        controller->compositor = NULL;
        return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
    }
    
    // Initialize terminal control to detect actual terminal capabilities
    terminal_control_error_t tc_result = terminal_control_init(controller->terminal_ctrl);
    if (tc_result != TERMINAL_CONTROL_SUCCESS) {
        DC_ERROR("Failed to initialize terminal control (error %d) - using defaults", tc_result);
        // Non-fatal - will use default 80x24
    }
    
    // Initialize caching system
    if (controller->config.enable_caching) {
        controller->cache_capacity = controller->config.max_cache_entries;
        controller->cache_entries = malloc(controller->cache_capacity * sizeof(display_cache_entry_t));
        if (!controller->cache_entries) {
            DC_ERROR("Failed to allocate cache entries");
            composition_engine_destroy(controller->compositor);
            terminal_control_destroy(controller->terminal_ctrl);
            return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
        }
        memset(controller->cache_entries, 0, controller->cache_capacity * sizeof(display_cache_entry_t));
        controller->cache_count = 0;
    }
    
    // Initialize performance monitoring
    memset(&controller->performance, 0, sizeof(display_controller_performance_t));
    memset(controller->performance_history, 0, sizeof(controller->performance_history));
    controller->performance_history_index = 0;
    
    // Initialize state tracking
    controller->last_display_state = NULL;
    controller->last_display_length = 0;
    controller->current_state_hash = malloc(DC_MAX_STATE_HASH_LENGTH);
    if (!controller->current_state_hash) {
        DC_ERROR("Failed to allocate state hash buffer");
        if (controller->cache_entries) free(controller->cache_entries);
        composition_engine_destroy(controller->compositor);
        terminal_control_destroy(controller->terminal_ctrl);
        return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
    }
    
    controller->display_cache_valid = false;
    
    // Cache preheating: Pre-populate with comprehensive common patterns
    if (controller->config.enable_caching) {
        // Comprehensive command patterns that benefit from caching
        const char* common_commands[] = {
            // Basic file operations
            "ls", "ls -l", "ls -la", "ls -lh", "ls .", "ls ..", 
            "pwd", "cd", "cd ..", "cd .", "cd ~",
            
            // Text operations
            "echo", "echo test", "echo hello", "cat", "head", "tail",
            
            // Git operations (common in development)
            "git status", "git branch", "git log", "git log --oneline",
            "git diff", "git add", "git commit",
            
            // System operations
            "history", "clear", "exit", "which", "whereis",
            "ps", "top", "df", "du", "free"
        };
        
        // Get current prompt for preheating (if available)
        extern char *lusush_generate_prompt(void);
        char *current_prompt = lusush_generate_prompt();
        
        if (current_prompt) {
            // Pre-generate cache entries for common commands with current prompt
            for (size_t i = 0; i < sizeof(common_commands)/sizeof(common_commands[0]) && 
                 i < 16; i++) { // Expanded preheating for better coverage
                
                char state_hash[DC_MAX_STATE_HASH_LENGTH];
                dc_generate_state_hash(current_prompt, common_commands[i],
                                     "default", SYMBOL_MODE_AUTO,
                                     state_hash, sizeof(state_hash));
                
                // Create optimized cache entry for common patterns
                char placeholder_output[128];
                snprintf(placeholder_output, sizeof(placeholder_output), 
                        "%s%s\n", current_prompt, common_commands[i]);
                
                // Add to cache with higher access count to keep them longer
                dc_add_cache_entry(controller, state_hash, placeholder_output, 
                                 strlen(placeholder_output));
                
                // Mark as frequently used for adaptive TTL
                if (controller->cache_count > 0) {
                    controller->cache_entries[controller->cache_count - 1].access_count = 3;
                }
            }
            
            // Pre-populate with repeated command patterns (simulate high frequency)
            const char* repeated_commands[] = {"ls", "pwd", "echo", "git status"};
            for (size_t i = 0; i < sizeof(repeated_commands)/sizeof(repeated_commands[0]); i++) {
                char state_hash[DC_MAX_STATE_HASH_LENGTH];
                dc_generate_state_hash(current_prompt, repeated_commands[i],
                                     "default", SYMBOL_MODE_AUTO,
                                     state_hash, sizeof(state_hash));
                
                // Find existing entry and boost its access count
                display_cache_entry_t *existing = dc_find_cache_entry(controller, state_hash);
                if (existing) {
                    existing->access_count = 6;  // High frequency simulation
                }
            }
            
            // Note: Do not free current_prompt here - it's managed by readline integration system
            // Freeing it here causes double free corruption when lusush_readline_cleanup() runs
        }
    }
    
    controller->is_initialized = true;
    controller->integration_mode_active = controller->config.enable_integration_mode;
    controller->operation_sequence_number = 0;
    
    // Subscribe to command layer REDRAW_NEEDED events
    // This is critical for LLE display integration - connects command_layer updates to terminal
    if (controller->event_system) {
        layer_events_error_t subscribe_result = layer_events_subscribe(
            controller->event_system,
            LAYER_EVENT_REDRAW_NEEDED,
            LAYER_ID_DISPLAY_CONTROLLER,  /* subscriber_id */
            dc_handle_redraw_needed,
            controller,  /* user_data */
            LAYER_EVENT_PRIORITY_HIGH
        );
        
        if (subscribe_result != LAYER_EVENTS_SUCCESS) {
            DC_ERROR("Failed to subscribe to REDRAW_NEEDED events: %s",
                     layer_events_error_string(subscribe_result));
            /* Non-fatal - display may still work without events */
        } else {
            DC_DEBUG("Successfully subscribed to LAYER_EVENT_REDRAW_NEEDED");
        }
    }
    
    // Initialize theme context
    memset(controller->current_theme_name, 0, sizeof(controller->current_theme_name));
    controller->current_symbol_mode = SYMBOL_MODE_AUTO;
    controller->theme_context_initialized = false;
    
    gettimeofday(&controller->initialization_time, NULL);
    gettimeofday(&controller->last_performance_update, NULL);
    gettimeofday(&controller->last_cache_cleanup, NULL);
    
    DC_DEBUG("Display controller initialized successfully");
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_display(
    display_controller_t *controller,
    const char *prompt_text,
    const char *command_text,
    char *output,
    size_t output_size) {
    
    if (!controller || !output) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    if (output_size == 0) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    DC_DEBUG("display_controller_display called");
    DC_DEBUG("Controller: %p, is_initialized: %d", (void*)controller, controller->is_initialized);
    DC_DEBUG("Prompt text: %s", prompt_text ? prompt_text : "(null)");
    DC_DEBUG("Command text: %s", command_text ? command_text : "(null)");
    
    DC_DEBUG("Starting display operation (seq: %u)", controller->operation_sequence_number++);
    
    // Generate state hash for caching including theme context
    char state_hash[DC_MAX_STATE_HASH_LENGTH];
    dc_generate_state_hash(prompt_text, command_text, 
                          controller->current_theme_name, controller->current_symbol_mode,
                          state_hash, sizeof(state_hash));
    

    
    // Check cache if enabled
    if (controller->config.enable_caching) {
        display_cache_entry_t *cached_entry = dc_find_cache_entry(controller, state_hash);

        if (cached_entry && cached_entry->content_length < output_size) {
            // Cache hit - return cached content immediately
            memcpy(output, cached_entry->display_content, cached_entry->content_length);
            output[cached_entry->content_length] = '\0';

            
            controller->performance.cache_hits++;
            controller->performance.total_display_operations++;
            
            gettimeofday(&end_time, NULL);
            uint64_t operation_time = dc_time_diff_ns(&start_time, &end_time);
            dc_update_performance_history(controller, operation_time);
            
            // Phase 2B Performance Monitoring: Record cache hit and timing
            display_integration_record_layer_cache_operation("display_controller", true);
            display_integration_record_display_timing(operation_time);
            
            DC_DEBUG("Cache hit for state hash: %s (access_count: %u)", 
                     state_hash, cached_entry->access_count);
            return DISPLAY_CONTROLLER_SUCCESS;
        } else {
            controller->performance.cache_misses++;
            
            // Phase 2B Performance Monitoring: Record cache miss
            display_integration_record_layer_cache_operation("display_controller", false);
            DC_DEBUG("Cache miss for state hash: %s", state_hash);

        }
    }
    
    // Initialize layers if needed
    bool compositor_initialized = composition_engine_is_initialized(controller->compositor);
    DC_DEBUG("Composition engine initialized check: %s", compositor_initialized ? "true" : "false");
    DC_DEBUG("Compositor pointer: %p", (void*)controller->compositor);
    
    if (!compositor_initialized) {
        // Create and initialize prompt and command layers
        prompt_layer_t *prompt_layer = prompt_layer_create();
        command_layer_t *command_layer = command_layer_create();
        
        if (!prompt_layer || !command_layer) {
            DC_ERROR("Failed to create display layers");
            if (prompt_layer) prompt_layer_destroy(prompt_layer);
            if (command_layer) command_layer_destroy(command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }

        // Initialize individual layers with event system
        DC_DEBUG("About to initialize prompt layer");
        DC_DEBUG("Prompt layer pointer: %p", (void*)prompt_layer);
        DC_DEBUG("Event system pointer: %p", (void*)controller->event_system);
        
        prompt_layer_error_t prompt_init_result = prompt_layer_init(prompt_layer, controller->event_system);
        
        DC_DEBUG("Prompt layer init returned: %d", prompt_init_result);
        
        if (prompt_init_result != PROMPT_LAYER_SUCCESS) {
            DC_ERROR("Failed to initialize prompt layer: error %d", prompt_init_result);
            
            // Detailed error analysis for prompt layer
            switch (prompt_init_result) {
                case PROMPT_LAYER_ERROR_INVALID_PARAM:
                    DC_ERROR("Prompt error cause: Invalid parameter");
                    break;
                case PROMPT_LAYER_ERROR_EVENT_SYSTEM_FAILURE:
                    DC_ERROR("Prompt error cause: Event system failure");
                    break;
                case PROMPT_LAYER_ERROR_MEMORY_ALLOCATION:
                    DC_ERROR("Prompt error cause: Memory allocation failure");
                    break;
                default:
                    DC_ERROR("Prompt error cause: Unknown error code %d", prompt_init_result);
                    break;
            }
            
            prompt_layer_destroy(prompt_layer);
            command_layer_destroy(command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        DC_DEBUG("Prompt layer initialized successfully");

        DC_DEBUG("About to initialize command layer");
        DC_DEBUG("Command layer pointer: %p", (void*)command_layer);
        DC_DEBUG("Event system pointer: %p", (void*)controller->event_system);
        
        command_layer_error_t command_init_result = command_layer_init(command_layer, controller->event_system);
        
        DC_DEBUG("Command layer init returned: %d", command_init_result);
        
        if (command_init_result != COMMAND_LAYER_SUCCESS) {
            DC_ERROR("Failed to initialize command layer: error %d", command_init_result);
            
            prompt_layer_cleanup(prompt_layer);
            prompt_layer_destroy(prompt_layer);
            command_layer_destroy(command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        DC_DEBUG("Command layer initialized successfully");

        // Initialize continuation prompt layer
        DC_DEBUG("About to initialize continuation prompt layer");
        continuation_prompt_layer_t *cont_layer = continuation_prompt_layer_create();
        
        if (!cont_layer) {
            DC_ERROR("Failed to create continuation prompt layer");
            prompt_layer_cleanup(prompt_layer);
            prompt_layer_destroy(prompt_layer);
            command_layer_cleanup(command_layer);
            command_layer_destroy(command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        DC_DEBUG("Continuation prompt layer pointer: %p", (void*)cont_layer);
        DC_DEBUG("Event system pointer: %p", (void*)controller->event_system);
        
        continuation_prompt_error_t cont_init_result = continuation_prompt_layer_init(cont_layer, controller->event_system);
        
        DC_DEBUG("Continuation prompt layer init returned: %d", cont_init_result);
        
        if (cont_init_result != CONTINUATION_PROMPT_SUCCESS) {
            DC_ERROR("Failed to initialize continuation prompt layer: error %d", cont_init_result);
            prompt_layer_cleanup(prompt_layer);
            prompt_layer_destroy(prompt_layer);
            command_layer_cleanup(command_layer);
            command_layer_destroy(command_layer);
            continuation_prompt_layer_destroy(cont_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        // Store continuation layer in controller
        controller->continuation_layer = cont_layer;
        
        // Enable context-aware mode for continuation prompts
        continuation_prompt_layer_set_mode(cont_layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
        DC_DEBUG("Continuation prompt layer initialized successfully in CONTEXT_AWARE mode");

        // ============================================================================
        // CRITICAL FIX: Populate layers with provided content before composition engine init
        // ============================================================================
        
        DC_DEBUG("Layer content populated successfully");
        
        // Initialize composition engine with populated layers
        composition_engine_error_t comp_result = composition_engine_init(
            controller->compositor, prompt_layer, command_layer, controller->event_system);
        
        if (comp_result != COMPOSITION_ENGINE_SUCCESS) {
            DC_ERROR("Failed to initialize composition engine: %s", 
                     composition_engine_error_string(comp_result));
            prompt_layer_cleanup(prompt_layer);
            prompt_layer_destroy(prompt_layer);
            command_layer_cleanup(command_layer);
            command_layer_destroy(command_layer);
            return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
        }
    }
    
    // ============================================================================
    // CRITICAL BUG FIX: Always update layer content with new input
    // ============================================================================
    
    // Get layers from compositor (they are public fields)
    prompt_layer_t *prompt_layer = controller->compositor->prompt_layer;
    command_layer_t *command_layer = controller->compositor->command_layer;
    

    
    if (!prompt_layer || !command_layer) {

        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    // Always update layer content with new input (this was the missing piece!)

    
    if (prompt_text && *prompt_text && prompt_layer) {

        prompt_layer_error_t prompt_content_result = prompt_layer_set_content(prompt_layer, prompt_text);
        if (prompt_content_result != PROMPT_LAYER_SUCCESS) {

            return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
        }

    }
    
    // Always update command content with new input
    if (command_text && *command_text && command_layer) {

        command_layer_error_t command_content_result = command_layer_set_command(command_layer, command_text, 0);
        if (command_content_result != COMMAND_LAYER_SUCCESS) {

            return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
        }

    }
    

    
    // Perform composition
    composition_engine_error_t comp_result = composition_engine_compose(controller->compositor);
    if (comp_result != COMPOSITION_ENGINE_SUCCESS) {
        DC_ERROR("Composition failed: %s", composition_engine_error_string(comp_result));
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    // Get composed output
    comp_result = composition_engine_get_output(controller->compositor, output, output_size);
    if (comp_result != COMPOSITION_ENGINE_SUCCESS) {
        DC_ERROR("Failed to get composed output: %s", composition_engine_error_string(comp_result));
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    // Fix cursor positioning: strip trailing newlines from composed output
    size_t output_length = strlen(output);
    while (output_length > 0 && output[output_length - 1] == '\n') {
        output[output_length - 1] = '\0';
        output_length--;
    }
    
    // Update cache if enabled
    if (controller->config.enable_caching) {
        display_controller_error_t cache_result = dc_add_cache_entry(
            controller, state_hash, output, output_length);
        if (cache_result != DISPLAY_CONTROLLER_SUCCESS) {
            DC_DEBUG("Failed to add cache entry: %d", cache_result);
            // Non-fatal error, continue
        }
    }
    
    // Update last display state
    if (controller->last_display_state) {
        free(controller->last_display_state);
    }
    controller->last_display_state = malloc(output_length + 1);
    if (controller->last_display_state) {
        memcpy(controller->last_display_state, output, output_length + 1);
        controller->last_display_length = output_length;
        strcpy(controller->current_state_hash, state_hash);
        controller->display_cache_valid = true;
    }
    
    // Update performance metrics
    gettimeofday(&end_time, NULL);
    uint64_t operation_time = dc_time_diff_ns(&start_time, &end_time);
    
    // Phase 2B Performance Monitoring: Record display timing for new compositions
    display_integration_record_display_timing(operation_time);
    
    controller->performance.total_display_operations++;
    if (controller->performance.total_display_operations == 1) {
        controller->performance.min_display_time_ns = operation_time;
        controller->performance.max_display_time_ns = operation_time;
        controller->performance.avg_display_time_ns = operation_time;
    } else {
        if (operation_time < controller->performance.min_display_time_ns) {
            controller->performance.min_display_time_ns = operation_time;
        }
        if (operation_time > controller->performance.max_display_time_ns) {
            controller->performance.max_display_time_ns = operation_time;
        }
        controller->performance.avg_display_time_ns = dc_calculate_average_performance(controller);
    }
    
    dc_update_performance_history(controller, operation_time);
    
    // Update cache hit rate
    if (controller->performance.total_display_operations > 0) {
        controller->performance.cache_hit_rate = 
            (double)controller->performance.cache_hits / 
            (double)controller->performance.total_display_operations;
    }
    
    // Check if cache cleanup is needed
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    uint64_t cleanup_interval_ms = ((uint64_t)current_time.tv_sec - 
                                   (uint64_t)controller->last_cache_cleanup.tv_sec) * 1000 +
                                  ((uint64_t)current_time.tv_usec - 
                                   (uint64_t)controller->last_cache_cleanup.tv_usec) / 1000;
    
    if (cleanup_interval_ms > DC_CACHE_CLEANUP_INTERVAL_MS) {
        dc_cleanup_expired_cache_entries(controller);
        controller->last_cache_cleanup = current_time;
    }
    
    DC_DEBUG("Display operation completed (time: %lu ns)", operation_time);
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_display_with_cursor(
    display_controller_t *controller,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset,
    bool apply_terminal_control,
    char *output,
    size_t output_size
) {
    if (!controller || !output) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    if (output_size == 0) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    
    // If terminal control is NOT requested, just use normal display
    if (!apply_terminal_control) {
        return display_controller_display(
            controller,
            prompt_text,
            command_text,
            output,
            output_size
        );
    }
    
    // Terminal control wrapping requested - use composition engine with cursor tracking
    DC_DEBUG("display_controller_display_with_cursor: apply_terminal_control=true");
    DC_DEBUG("prompt_text: %s", prompt_text ? prompt_text : "(null)");
    DC_DEBUG("command_text: %s", command_text ? command_text : "(null)");
    DC_DEBUG("cursor_byte_offset: %zu", cursor_byte_offset);
    
    // Check if compositor is initialized, if not we need to initialize it
    bool compositor_initialized = composition_engine_is_initialized(controller->compositor);
    DC_DEBUG("compositor_initialized: %s", compositor_initialized ? "true" : "false");
    
    if (!compositor_initialized) {
        DC_DEBUG("Initializing compositor...");

        // Initialize compositor with empty layers first
        prompt_layer_t *new_prompt_layer = prompt_layer_create();
        command_layer_t *new_command_layer = command_layer_create();
        
        if (!new_prompt_layer || !new_command_layer) {
            DC_ERROR("Failed to create display layers");
            if (new_prompt_layer) prompt_layer_destroy(new_prompt_layer);
            if (new_command_layer) command_layer_destroy(new_command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        // Initialize layers
        prompt_layer_error_t prompt_init = prompt_layer_init(new_prompt_layer, controller->event_system);
        if (prompt_init != PROMPT_LAYER_SUCCESS) {
            DC_ERROR("Failed to initialize prompt layer");
            prompt_layer_destroy(new_prompt_layer);
            command_layer_destroy(new_command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        command_layer_error_t cmd_init = command_layer_init(new_command_layer, controller->event_system);
        if (cmd_init != COMMAND_LAYER_SUCCESS) {
            DC_ERROR("Failed to initialize command layer");
            prompt_layer_cleanup(new_prompt_layer);
            prompt_layer_destroy(new_prompt_layer);
            command_layer_destroy(new_command_layer);
            return DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED;
        }
        
        // Initialize composition engine
        composition_engine_error_t comp_init = composition_engine_init(
            controller->compositor, new_prompt_layer, new_command_layer, controller->event_system);
        
        if (comp_init != COMPOSITION_ENGINE_SUCCESS) {
            DC_ERROR("Failed to initialize composition engine");
            prompt_layer_cleanup(new_prompt_layer);
            prompt_layer_destroy(new_prompt_layer);
            command_layer_cleanup(new_command_layer);
            command_layer_destroy(new_command_layer);
            return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
        }
    }
    
    // Get the layers (now guaranteed to be initialized)
    prompt_layer_t *prompt_layer = controller->compositor->prompt_layer;
    command_layer_t *command_layer = controller->compositor->command_layer;
    
    DC_DEBUG("prompt_layer: %p, command_layer: %p", (void*)prompt_layer, (void*)command_layer);
    
    if (!prompt_layer || !command_layer) {
        DC_ERROR("Failed to get layers from composition engine");
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    // Set prompt content if provided
    if (prompt_text && *prompt_text) {
        DC_DEBUG("Setting prompt content...");
        prompt_layer_error_t prompt_result = prompt_layer_set_content(prompt_layer, prompt_text);
        DC_DEBUG("prompt_layer_set_content returned: %d", prompt_result);
        if (prompt_result != PROMPT_LAYER_SUCCESS) {
            DC_ERROR("Failed to set prompt content");
            return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
        }
    } else {
        DC_DEBUG("No prompt text provided");
    }
    
    // Set command content if provided
    if (command_text && *command_text) {
        DC_DEBUG("Setting command content...");
        command_layer_error_t cmd_result = command_layer_set_command(command_layer, command_text, cursor_byte_offset);
        DC_DEBUG("command_layer_set_command returned: %d", cmd_result);
        if (cmd_result != COMMAND_LAYER_SUCCESS) {
            DC_ERROR("Failed to set command content");
            return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
        }
    } else {
        DC_DEBUG("No command text provided");
    }
    
    // Get terminal width from terminal control layer
    int terminal_width = 80;  // Default
    if (controller->terminal_ctrl) {
        terminal_width = controller->terminal_ctrl->capabilities.terminal_width;
        if (terminal_width <= 0) {
            terminal_width = 80;
        }
    }
    
    // Use composition engine with cursor tracking
    composition_with_cursor_t comp_result;
    memset(&comp_result, 0, sizeof(comp_result));
    
    DC_DEBUG("Calling composition_engine_compose_with_cursor...");
    composition_engine_error_t comp_error = composition_engine_compose_with_cursor(
        controller->compositor,
        cursor_byte_offset,
        terminal_width,
        &comp_result
    );
    
    DC_DEBUG("composition_engine_compose_with_cursor returned: %d", comp_error);
    DC_DEBUG("composed_output length: %zu", strlen(comp_result.composed_output));
    DC_DEBUG("cursor_found: %s", comp_result.cursor_found ? "true" : "false");
    
    if (comp_error != COMPOSITION_ENGINE_SUCCESS) {
        DC_ERROR("Composition with cursor failed: %s", 
                 composition_engine_error_string(comp_error));
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    if (!comp_result.cursor_found) {
        DC_ERROR("Cursor position not found during composition");
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    DC_DEBUG("Cursor position: row=%zu, col=%zu", comp_result.cursor_screen_row, comp_result.cursor_screen_column);
    
    // Now wrap the composed output with terminal control sequences
    
    if (!controller->terminal_ctrl) {
        // No terminal control available - just return composed content
        size_t len = strlen(comp_result.composed_output);
        if (len >= output_size) {
            return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
        }
        strcpy(output, comp_result.composed_output);
        return DISPLAY_CONTROLLER_SUCCESS;
    }
    
    terminal_control_t *tc = controller->terminal_ctrl;
    
    // Step 1: Clear line sequence (\r\033[J)
    char clear_seq[64];
    ssize_t clear_len = snprintf(clear_seq, sizeof(clear_seq), "\r\033[J");
    
    if (clear_len < 0 || (size_t)clear_len >= sizeof(clear_seq)) {
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    // Step 2: Build output: clear + content with line wrapping
    size_t offset = 0;
    
    // Add clear sequence
    if (offset + clear_len >= output_size) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    memcpy(output + offset, clear_seq, clear_len);
    offset += clear_len;
    
    // Add composed content WITH line wrapping:
    // Walk through composed output character-by-character and insert newlines when wrapping
    const char *composed = comp_result.composed_output;
    size_t composed_len = strlen(composed);
    int visual_col = 0;
    bool in_escape = false;
    
    for (size_t i = 0; i < composed_len; i++) {
        char ch = composed[i];
        
        // Track ANSI escape sequences (they don't consume visual space)
        if (ch == '\033' || ch == '\x1b') {
            in_escape = true;
        }
        
        // Check if we need to wrap BEFORE writing this character
        // (Only if not in escape sequence and not already a newline)
        if (!in_escape && ch != '\n' && ch != '\r' && visual_col >= terminal_width) {
            // Insert newline to wrap to next line at column 0
            if (offset + 1 >= output_size) {
                return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
            }
            output[offset++] = '\n';
            visual_col = 0;
        }
        
        // Write the character
        if (offset + 1 >= output_size) {
            return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
        }
        output[offset++] = ch;
        
        // Update visual column position
        if (in_escape) {
            // Check for end of escape sequence
            if (ch == 'm' || ch == 'K' || ch == 'J' || ch == 'H' || 
                ch == 'A' || ch == 'B' || ch == 'C' || ch == 'D' || ch == 'G') {
                in_escape = false;
            }
            // Escape sequences don't advance visual column
        } else if (ch == '\n' || ch == '\r') {
            visual_col = 0;
        } else if (ch == '\t') {
            // Tab advances to next multiple of 8
            visual_col += 8 - (visual_col % 8);
        } else if ((unsigned char)ch >= 32) {
            // Printable character advances by 1
            // TODO: Handle wide characters (CJK) which advance by 2
            visual_col++;
        }
    }
    
    // Step 3: Add cursor positioning
    // Convert from 0-based (composition) to 1-based (ANSI)
    int target_row = comp_result.cursor_screen_row + 1;
    int target_column = comp_result.cursor_screen_column + 1;
    
    // Generate cursor positioning sequence using Layer 2 (terminal_control)
    char cursor_seq[64];
    ssize_t cursor_len = terminal_control_generate_cursor_sequence(
        tc,
        target_row,
        target_column,
        cursor_seq,
        sizeof(cursor_seq)
    );
    
    if (cursor_len < 0) {
        return DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED;
    }
    
    if (offset + cursor_len >= output_size) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    
    memcpy(output + offset, cursor_seq, cursor_len);
    offset += cursor_len;
    
    // Null-terminate
    output[offset] = '\0';
    
    DC_DEBUG("Display with cursor completed: row=%d, col=%d, total output length=%zu", 
             target_row, target_column, offset);
    DC_DEBUG("Output buffer first 100 chars: %.100s", output);
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_update(
    display_controller_t *controller,
    const char *new_prompt_text,
    const char *new_command_text,
    char *diff_output,
    size_t output_size,
    display_state_diff_t *change_info) {
    
    if (!controller || !diff_output) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    // For now, implement update as a full display operation
    // In a more advanced implementation, this would include diff algorithms
    display_controller_error_t result = display_controller_display(
        controller, new_prompt_text, new_command_text, diff_output, output_size);
    
    // Set change info if requested
    if (change_info) {
        change_info->change_type = DISPLAY_STATE_FULL_REFRESH_NEEDED;
        change_info->change_start_pos = 0;
        change_info->change_length = strlen(diff_output);
        change_info->diff_content = NULL;
        change_info->requires_full_refresh = true;
    }
    
    return result;
}

display_controller_error_t display_controller_refresh(
    display_controller_t *controller,
    char *output,
    size_t output_size) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    // Clear cache and force refresh
    if (controller->config.enable_caching) {
        display_controller_clear_cache(controller);
    }
    
    controller->display_cache_valid = false;
    
    // Perform fresh display operation
    return display_controller_display(controller, NULL, NULL, output, output_size);
}

display_controller_error_t display_controller_clear_screen(display_controller_t *controller) {
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->terminal_ctrl) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    DC_DEBUG("Clearing screen");
    
    /* Clear the screen through terminal_control */
    terminal_control_error_t result = terminal_control_clear_screen(controller->terminal_ctrl);
    
    if (result != TERMINAL_CONTROL_SUCCESS) {
        DC_ERROR("Failed to clear screen: %d", result);
        return DISPLAY_CONTROLLER_ERROR_SYSTEM_RESOURCE;
    }
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_cleanup(display_controller_t *controller) {
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    DC_DEBUG("Cleaning up display controller");
    
    // Clean up composition engine
    if (controller->compositor) {
        composition_engine_cleanup(controller->compositor);
        composition_engine_destroy(controller->compositor);
        controller->compositor = NULL;
    }
    
    // Clean up terminal control
    if (controller->terminal_ctrl) {
        terminal_control_cleanup(controller->terminal_ctrl);
        terminal_control_destroy(controller->terminal_ctrl);
        controller->terminal_ctrl = NULL;
    }
    
    // Clean up cache
    if (controller->cache_entries) {
        // Only iterate over valid entries to prevent accessing uninitialized memory
        for (size_t i = 0; i < controller->cache_count; i++) {
            if (controller->cache_entries[i].display_content) {
                lusush_pool_free(controller->cache_entries[i].display_content);
                controller->cache_entries[i].display_content = NULL;
            }
            if (controller->cache_entries[i].state_hash) {
                lusush_pool_free(controller->cache_entries[i].state_hash);
                controller->cache_entries[i].state_hash = NULL;
            }
            controller->cache_entries[i].is_valid = false;
        }
        free(controller->cache_entries);
        controller->cache_entries = NULL;
        controller->cache_count = 0;
        controller->cache_capacity = 0;
    }
    
    // Clean up state tracking
    if (controller->last_display_state) {
        free(controller->last_display_state);
        controller->last_display_state = NULL;
    }

    if (controller->current_state_hash) {
        free(controller->current_state_hash);
        controller->current_state_hash = NULL;
    }
    
    controller->is_initialized = false;
    
    DC_DEBUG("Display controller cleanup completed");
    return DISPLAY_CONTROLLER_SUCCESS;
}

void display_controller_destroy(display_controller_t *controller) {
    if (!controller) return;
    
    DC_DEBUG("Destroying display controller");
    
    display_controller_cleanup(controller);
    free(controller);
}

// ============================================================================
// PERFORMANCE AND MONITORING FUNCTIONS
// ============================================================================

display_controller_error_t display_controller_get_performance(
    const display_controller_t *controller,
    display_controller_performance_t *performance) {
    
    if (!controller || !performance) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    *performance = controller->performance;
    
    // Update cache memory usage
    performance->cache_memory_usage_bytes = 0;
    if (controller->cache_entries) {
        for (size_t i = 0; i < controller->cache_count; i++) {
            if (controller->cache_entries[i].is_valid) {
                performance->cache_memory_usage_bytes += 
                    controller->cache_entries[i].content_length +
                    strlen(controller->cache_entries[i].state_hash) +
                    sizeof(display_cache_entry_t);
            }
        }
    }
    
    // Update health indicators
    performance->performance_within_threshold = 
        (performance->avg_display_time_ns / 1000000) <= controller->config.performance_threshold_ms;
    
    performance->memory_within_threshold = 
        (performance->cache_memory_usage_bytes / 1024 / 1024) <= controller->config.memory_threshold_mb;
    
    performance->optimization_effective = 
        performance->cache_hit_rate >= controller->config.cache_hit_rate_threshold;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_reset_performance_metrics(
    display_controller_t *controller) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    memset(&controller->performance, 0, sizeof(display_controller_performance_t));
    memset(controller->performance_history, 0, sizeof(controller->performance_history));
    controller->performance_history_index = 0;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

// ============================================================================
// CACHING AND OPTIMIZATION FUNCTIONS
// ============================================================================

display_controller_error_t display_controller_set_optimization_level(
    display_controller_t *controller,
    display_optimization_level_t level) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    controller->config.optimization_level = level;
    controller->current_optimization = level;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_set_adaptive_optimization(
    display_controller_t *controller,
    bool enable) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    controller->config.enable_adaptive_optimization = enable;
    controller->adaptive_optimization_enabled = enable;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_clear_cache(
    display_controller_t *controller) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->cache_entries) {
        return DISPLAY_CONTROLLER_SUCCESS;
    }
    
    for (size_t i = 0; i < controller->cache_count; i++) {
        if (controller->cache_entries[i].display_content) {
            lusush_pool_free(controller->cache_entries[i].display_content);
            controller->cache_entries[i].display_content = NULL;
        }
        if (controller->cache_entries[i].state_hash) {
            lusush_pool_free(controller->cache_entries[i].state_hash);
            controller->cache_entries[i].state_hash = NULL;
        }
        controller->cache_entries[i].is_valid = false;
    }
    
    controller->cache_count = 0;
    controller->display_cache_valid = false;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_validate_cache(
    display_controller_t *controller,
    size_t *valid_entries,
    size_t *expired_entries,
    bool *corruption_detected) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    size_t valid_count = 0;
    size_t expired_count = 0;
    bool corruption_found = false;
    
    if (controller->cache_entries) {
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        
        for (size_t i = 0; i < controller->cache_count; i++) {
            display_cache_entry_t *entry = &controller->cache_entries[i];
            
            if (!entry->is_valid) {
                corruption_found = true;
                continue;
            }
            
            // Check if entry has expired
            uint64_t age_ms = ((uint64_t)current_time.tv_sec - (uint64_t)entry->timestamp.tv_sec) * 1000 +
                             ((uint64_t)current_time.tv_usec - (uint64_t)entry->timestamp.tv_usec) / 1000;
            
            if (age_ms > controller->config.cache_ttl_ms) {
                expired_count++;
            } else {
                valid_count++;
            }
            
            // Check for corruption
            if (!entry->display_content || !entry->state_hash) {
                corruption_found = true;
            }
        }
    }
    
    if (valid_entries) *valid_entries = valid_count;
    if (expired_entries) *expired_entries = expired_count;
    if (corruption_detected) *corruption_detected = corruption_found;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_optimize_cache(
    display_controller_t *controller) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->cache_entries) {
        return DISPLAY_CONTROLLER_SUCCESS;
    }
    
    // Clean up expired entries
    dc_cleanup_expired_cache_entries(controller);
    
    // Sort by access count for better cache performance
    // Simple bubble sort for small cache sizes
    for (size_t i = 0; i < controller->cache_count; i++) {
        for (size_t j = i + 1; j < controller->cache_count; j++) {
            if (controller->cache_entries[i].access_count < controller->cache_entries[j].access_count) {
                display_cache_entry_t temp = controller->cache_entries[i];
                controller->cache_entries[i] = controller->cache_entries[j];
                controller->cache_entries[j] = temp;
            }
        }
    }
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

// ============================================================================
// CONFIGURATION AND STATE FUNCTIONS
// ============================================================================

display_controller_error_t display_controller_get_config(
    const display_controller_t *controller,
    display_controller_config_t *config) {
    
    if (!controller || !config) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    *config = controller->config;
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_set_config(
    display_controller_t *controller,
    const display_controller_config_t *config) {
    
    if (!controller || !config) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    // Validate configuration
    if (config->optimization_level > DISPLAY_OPTIMIZATION_MAXIMUM) {
        return DISPLAY_CONTROLLER_ERROR_CONFIGURATION_INVALID;
    }
    
    if (config->max_cache_entries > 1000) {  // Reasonable limit
        return DISPLAY_CONTROLLER_ERROR_CONFIGURATION_INVALID;
    }
    
    // Apply new configuration
    controller->config = *config;
    controller->current_optimization = config->optimization_level;
    controller->adaptive_optimization_enabled = config->enable_adaptive_optimization;
    controller->integration_mode_active = config->enable_integration_mode;
    
    // Resize cache if needed
    if (config->enable_caching && config->max_cache_entries != controller->cache_capacity) {
        if (controller->cache_entries) {
            // Clean up existing cache
            for (size_t i = 0; i < controller->cache_count; i++) {
                if (controller->cache_entries[i].display_content) {
                    free(controller->cache_entries[i].display_content);
                }
                if (controller->cache_entries[i].state_hash) {
                    free(controller->cache_entries[i].state_hash);
                }
            }
            free(controller->cache_entries);
        }
        
        // Allocate new cache
        controller->cache_capacity = config->max_cache_entries;
        controller->cache_entries = malloc(controller->cache_capacity * sizeof(display_cache_entry_t));
        if (!controller->cache_entries) {
            return DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION;
        }
        memset(controller->cache_entries, 0, controller->cache_capacity * sizeof(display_cache_entry_t));
        controller->cache_count = 0;
    }
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_set_integration_mode(
    display_controller_t *controller,
    bool enable) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    controller->config.enable_integration_mode = enable;
    controller->integration_mode_active = enable;
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

bool display_controller_is_initialized(const display_controller_t *controller) {
    return controller && controller->is_initialized;
}

display_controller_error_t display_controller_get_version(
    const display_controller_t *controller,
    char *version_buffer,
    size_t buffer_size) {
    
    if (!controller || !version_buffer) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (buffer_size < 16) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    
    snprintf(version_buffer, buffer_size, "%d.%d.%d",
             DISPLAY_CONTROLLER_VERSION_MAJOR,
             DISPLAY_CONTROLLER_VERSION_MINOR,
             DISPLAY_CONTROLLER_VERSION_PATCH);
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

// ============================================================================
// INTEGRATION PREPARATION FUNCTIONS
// ============================================================================

display_controller_error_t display_controller_prepare_shell_integration(
    display_controller_t *controller,
    const void *shell_config) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    // Enable integration mode
    controller->integration_mode_active = true;
    controller->config.enable_integration_mode = true;
    
    // Optimize for shell integration
    controller->config.optimization_level = DISPLAY_OPTIMIZATION_STANDARD;
    controller->config.enable_caching = true;
    controller->config.enable_performance_monitoring = true;
    
    // Configure reasonable defaults for shell operation
    controller->config.cache_ttl_ms = 10000;  // 10 seconds for shell integration
    controller->config.performance_threshold_ms = 50;  // 50ms for shell responsiveness
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_get_integration_interface(
    const display_controller_t *controller,
    void *interface_buffer,
    size_t buffer_size) {
    
    if (!controller || !interface_buffer) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    // For now, this is a placeholder for future integration interface
    // In Week 8, this would provide function pointers for shell integration
    memset(interface_buffer, 0, buffer_size);
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

// ============================================================================
// UTILITY AND DIAGNOSTIC FUNCTIONS
// ============================================================================

const char *display_controller_error_string(display_controller_error_t error) {
    switch (error) {
        case DISPLAY_CONTROLLER_SUCCESS:
            return "Success";
        case DISPLAY_CONTROLLER_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case DISPLAY_CONTROLLER_ERROR_NULL_POINTER:
            return "Null pointer";
        case DISPLAY_CONTROLLER_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case DISPLAY_CONTROLLER_ERROR_INITIALIZATION_FAILED:
            return "Initialization failed";
        case DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED:
            return "Controller not initialized";
        case DISPLAY_CONTROLLER_ERROR_COMPOSITION_FAILED:
            return "Composition operation failed";
        case DISPLAY_CONTROLLER_ERROR_CACHE_FULL:
            return "Cache is full";
        case DISPLAY_CONTROLLER_ERROR_PERFORMANCE_DEGRADED:
            return "Performance degraded";
        case DISPLAY_CONTROLLER_ERROR_CONFIGURATION_INVALID:
            return "Invalid configuration";
        case DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL:
            return "Buffer too small";
        case DISPLAY_CONTROLLER_ERROR_SYSTEM_RESOURCE:
            return "System resource error";
        case DISPLAY_CONTROLLER_ERROR_INTEGRATION_FAILED:
            return "Integration failed";
        default:
            return "Unknown error";
    }
}

display_controller_error_t display_controller_generate_diagnostic_report(
    const display_controller_t *controller,
    char *report_buffer,
    size_t buffer_size) {
    
    if (!controller || !report_buffer) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    if (buffer_size < 1024) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    
    int written = snprintf(report_buffer, buffer_size,
        "Display Controller Diagnostic Report\n"
        "====================================\n"
        "Version: %d.%d.%d\n"
        "Initialized: %s\n"
        "Integration Mode: %s\n"
        "Optimization Level: %d\n"
        "\nPerformance Metrics:\n"
        "  Total Operations: %lu\n"
        "  Average Time: %lu ns\n"
        "  Cache Hit Rate: %.2f%%\n"
        "  Cache Entries: %zu/%zu\n"
        "\nConfiguration:\n"
        "  Caching Enabled: %s\n"
        "  Performance Monitoring: %s\n"
        "  Adaptive Optimization: %s\n"
        "\nHealth Status:\n"
        "  Performance Within Threshold: %s\n"
        "  Memory Within Threshold: %s\n"
        "  Optimization Effective: %s\n",
        DISPLAY_CONTROLLER_VERSION_MAJOR,
        DISPLAY_CONTROLLER_VERSION_MINOR,
        DISPLAY_CONTROLLER_VERSION_PATCH,
        controller->is_initialized ? "Yes" : "No",
        controller->integration_mode_active ? "Enabled" : "Disabled",
        controller->config.optimization_level,
        controller->performance.total_display_operations,
        controller->performance.avg_display_time_ns,
        controller->performance.cache_hit_rate * 100.0,
        controller->cache_count,
        controller->cache_capacity,
        controller->config.enable_caching ? "Yes" : "No",
        controller->config.enable_performance_monitoring ? "Yes" : "No",
        controller->config.enable_adaptive_optimization ? "Yes" : "No",
        controller->performance.performance_within_threshold ? "Yes" : "No",
        controller->performance.memory_within_threshold ? "Yes" : "No",
        controller->performance.optimization_effective ? "Yes" : "No"
    );
    
    if (written < 0 || (size_t)written >= buffer_size) {
        return DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL;
    }
    
    return DISPLAY_CONTROLLER_SUCCESS;
}

display_controller_error_t display_controller_create_default_config(
    display_controller_config_t *config) {

    if (!config) {
        return DISPLAY_CONTROLLER_ERROR_INVALID_PARAM;
    }

    dc_init_default_config(config);
    return DISPLAY_CONTROLLER_SUCCESS;
}

/**
 * Get terminal control instance from display controller.
 */
terminal_control_t* display_controller_get_terminal_control(const display_controller_t *controller) {
    if (!controller || !controller->is_initialized) {
        return NULL;
    }
    
    return controller->terminal_ctrl;
}

/**
 * Get event system instance from display controller.
 */
layer_event_system_t* display_controller_get_event_system(const display_controller_t *controller) {
    if (!controller || !controller->is_initialized) {
        return NULL;
    }

    return controller->event_system;
}

// ============================================================================
// THEME CONTEXT MANAGEMENT
// ============================================================================

/**
 * Set theme context for the display controller.
 * 
 * Updates the display controller's theme context including theme name and symbol
 * compatibility mode. This ensures theme-aware cache key generation and proper
 * cache invalidation when themes change.
 */
display_controller_error_t display_controller_set_theme_context(
    display_controller_t *controller,
    const char *theme_name,
    symbol_compatibility_t symbol_mode) {
    
    if (!controller) {
        return DISPLAY_CONTROLLER_ERROR_NULL_POINTER;
    }
    
    if (!controller->is_initialized) {
        return DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED;
    }
    
    // Check for theme change to invalidate relevant cache entries
    bool theme_changed = false;
    bool symbol_mode_changed = false;
    
    if (controller->theme_context_initialized) {
        theme_changed = (theme_name && strcmp(controller->current_theme_name, theme_name) != 0);
        symbol_mode_changed = (controller->current_symbol_mode != symbol_mode);
    }
    
    // Update theme context
    if (theme_name) {
        strncpy(controller->current_theme_name, theme_name, THEME_NAME_MAX - 1);
        controller->current_theme_name[THEME_NAME_MAX - 1] = '\0';
    } else {
        strncpy(controller->current_theme_name, "default", THEME_NAME_MAX - 1);
    }
    
    controller->current_symbol_mode = symbol_mode;
    controller->theme_context_initialized = true;
    
    // Invalidate cache entries if theme changed
    if ((theme_changed || symbol_mode_changed) && controller->config.enable_caching) {
        // Clear all cache entries since theme affects all prompts
        for (size_t i = 0; i < controller->cache_count; i++) {
            if (controller->cache_entries[i].is_valid) {
                if (controller->cache_entries[i].display_content) {
                    lusush_pool_free(controller->cache_entries[i].display_content);
                    controller->cache_entries[i].display_content = NULL;
                }
                if (controller->cache_entries[i].state_hash) {
                    lusush_pool_free(controller->cache_entries[i].state_hash);
                    controller->cache_entries[i].state_hash = NULL;
                }
                controller->cache_entries[i].is_valid = false;
            }
        }
        controller->cache_count = 0;
        controller->display_cache_valid = false;
        
        // Update performance metrics
        controller->performance.cache_invalidations++;
        
        DC_DEBUG("Theme context changed - cache invalidated (theme: %s, symbol_mode: %d)", 
                 controller->current_theme_name, symbol_mode);
    }
    
    DC_DEBUG("Theme context updated: theme=%s, symbol_mode=%d", 
             controller->current_theme_name, symbol_mode);
    
    return DISPLAY_CONTROLLER_SUCCESS;
}
