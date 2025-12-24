/*
 * Lusush Shell - Layered Display Architecture
 * Command Layer Implementation - Real-Time Syntax Highlighting System
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
 * COMMAND LAYER IMPLEMENTATION
 * 
 * This file implements the command layer of the Lusush Display System,
 * providing real-time syntax highlighting for command input that works
 * universally with ANY prompt structure.
 * 
 * Key Implementation Features:
 * - Real-time syntax highlighting using existing lusush functions
 * - Universal compatibility with any prompt structure
 * - High-performance command processing (<5ms updates)
 * - Intelligent caching system for syntax highlighting results
 * - Event-driven communication with prompt layer
 * - Memory-safe command text management
 * - Integration with existing readline and syntax highlighting systems
 * 
 * Architecture Integration:
 * - Leverages existing lusush_output_colored_line() for syntax highlighting
 * - Coordinates with prompt layer through event system
 * - Maintains independence from prompt structure
 * - Provides positioning information for universal composition
 * 
 * Performance Targets:
 * - Command update processing: <5ms
 * - Cache hit rate: >80% for repeated commands
 * - Memory usage: <2MB for command layer
 * - Zero memory leaks (valgrind clean)
 */

#include "display/command_layer.h"
#include "display_integration.h"
#include "display/display_controller.h"
#include "display/base_terminal.h"
#include "lle/adaptive_terminal_integration.h"
#include "lle/syntax_highlighting.h"

// Note: Completion menu support moved to display_controller (proper architecture)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <limits.h>

// ============================================================================
// CONSTANTS AND INTERNAL CONFIGURATION
// ============================================================================

#define COMMAND_LAYER_MAGIC 0x434D444C  // "CMDL" in hex
#define COMMAND_LAYER_VERSION_STRING "1.0.0"

// Performance monitoring constants
#define NANOSECONDS_PER_SECOND 1000000000ULL
#define NANOSECONDS_PER_MILLISECOND 1000000ULL

// Cache management constants
#define CACHE_HASH_MULTIPLIER 31
#define CACHE_CLEANUP_THRESHOLD 0.8

// ============================================================================
// INTERNAL STRUCTURES AND STATE
// ============================================================================

/**
 * Syntax highlighting statistics
 */
typedef struct {
    uint64_t tokens_parsed;
    uint64_t regions_created;
    uint64_t cache_lookups;
    uint64_t highlighting_time_ns;
} highlighting_stats_t;

// Global statistics for monitoring
static highlighting_stats_t g_highlighting_stats = {0};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// Core functionality
static command_layer_error_t perform_syntax_highlighting(command_layer_t *layer);
static command_layer_error_t update_command_metrics(command_layer_t *layer);
static uint64_t get_current_time_ns(void);
static void update_performance_stats(command_layer_t *layer, uint64_t operation_time_ns);

// Cache management
static uint32_t calculate_command_hash(const char *command);
static command_cache_entry_t *find_cache_entry(command_layer_t *layer, const char *command);
static command_layer_error_t add_to_cache(command_layer_t *layer, const char *command, 
                                         const char *highlighted, const command_metrics_t *metrics);
static void expire_old_cache_entries(command_layer_t *layer);

// Syntax highlighting implementation
static const char *get_token_color(command_layer_t *layer, command_token_type_t token_type);
static command_token_type_t map_spec_token_type(lle_syntax_token_type_t spec_type);

// Event handling
static layer_events_error_t handle_layer_event(const layer_event_t *event, void *user_data);
static command_layer_error_t publish_command_event(command_layer_t *layer, layer_event_type_t event_type);

// Validation and utilities
static bool validate_layer_state(command_layer_t *layer);
static void reset_layer_state(command_layer_t *layer);
static size_t safe_string_copy(char *dest, const char *src, size_t dest_size);

// ============================================================================
// CORE API IMPLEMENTATION
// ============================================================================

const char *command_layer_get_version(void) {
    return COMMAND_LAYER_VERSION_STRING;
}

command_layer_t *command_layer_create(void) {
    command_layer_t *layer = calloc(1, sizeof(command_layer_t));
    if (!layer) {
        return NULL;
    }
    
    // Initialize magic number for validation
    layer->magic = COMMAND_LAYER_MAGIC;
    layer->initialized = false;
    layer->needs_redraw = false;
    
    // Initialize command content
    layer->command_text[0] = '\0';
    layer->highlighted_text[0] = '\0';
    layer->cursor_position = 0;
    
    // Initialize syntax highlighting state
    layer->region_count = 0;
    layer->syntax_config.enabled = true;
    layer->syntax_config.use_colors = true;
    layer->syntax_config.highlight_errors = true;
    layer->syntax_config.cache_enabled = true;
    layer->syntax_config.cache_expiry_ms = COMMAND_LAYER_CACHE_EXPIRY_MS;
    layer->syntax_config.max_update_time_ms = COMMAND_LAYER_TARGET_UPDATE_TIME_MS;
    
    // Initialize performance statistics
    memset(&layer->performance, 0, sizeof(command_performance_t));
    layer->performance.min_update_time_ns = UINT64_MAX;
    
    // Initialize cache
    layer->cache_size = 0;
    layer->cache_access_count = 0;
    for (size_t i = 0; i < COMMAND_LAYER_CACHE_SIZE; i++) {
        layer->cache[i].is_valid = false;
    }
    
    // Initialize event system integration
    layer->event_system = NULL;
    layer->event_subscription_id = 0;
    
    // Initialize prompt layer integration
    layer->prompt_layer = NULL;
    layer->prompt_integration_enabled = false;
    
    // Initialize completion menu state
    layer->completion_menu_visible = false;
    layer->completion_menu_content = NULL;
    layer->completion_menu_content_size = 0;
    layer->completion_menu_lines = 0;
    layer->completion_menu_selected_index = 0;
    
    // Initialize performance monitoring
    clock_gettime(CLOCK_MONOTONIC, &layer->last_update_time);
    layer->update_sequence_number = 0;
    
    // Initialize spec-compliant syntax highlighter (Spec 11)
    layer->spec_highlighter = NULL;
    if (lle_syntax_highlighter_create(&layer->spec_highlighter) != 0) {
        // Highlighter creation failed - continue without it, fall back to inline
        layer->spec_highlighter = NULL;
    }
    
    return layer;
}

command_layer_error_t command_layer_init(command_layer_t *layer, layer_event_system_t *events) {
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (!events) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    // Store event system reference
    layer->event_system = events;
    
    // Create default color scheme
    command_layer_error_t result = command_layer_create_default_colors(&layer->syntax_config.color_scheme);
    if (result != COMMAND_LAYER_SUCCESS) {
        return result;
    }
    
    // Subscribe to relevant events
    // Temporarily disable theme subscription to isolate issue
    /*
    layer_events_error_t event_result = layer_events_subscribe(
        events,
        LAYER_EVENT_THEME_CHANGED,
        LAYER_ID_COMMAND_LAYER,
        handle_layer_event,
        layer,
        LAYER_EVENT_PRIORITY_HIGH
    );
    
    if (event_result != LAYER_EVENTS_SUCCESS) {
        return COMMAND_LAYER_ERROR_EVENT_SYSTEM;
    }
    */
    
    // Subscribe to prompt layer events if available
    layer_events_error_t event_result = layer_events_subscribe(
        events,
        LAYER_EVENT_CONTENT_CHANGED,
        LAYER_ID_COMMAND_LAYER,
        handle_layer_event,
        layer,
        LAYER_EVENT_PRIORITY_NORMAL
    );
    
    if (event_result != LAYER_EVENTS_SUCCESS) {
        return COMMAND_LAYER_ERROR_EVENT_SYSTEM;
    }
    
    layer->initialized = true;
    
    // Publish initialization complete event
    publish_command_event(layer, LAYER_EVENT_INITIALIZATION_COMPLETE);
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_set_command(command_layer_t *layer,
                                               const char *command_text,
                                               size_t cursor_pos) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (!command_text) {
        return COMMAND_LAYER_ERROR_NULL_POINTER;
    }
    
    size_t command_length = strlen(command_text);
    if (command_length >= COMMAND_LAYER_MAX_COMMAND_SIZE) {
        return COMMAND_LAYER_ERROR_COMMAND_TOO_LARGE;
    }
    
    if (cursor_pos > command_length) {
        return COMMAND_LAYER_ERROR_INVALID_CURSOR_POS;
    }
    
    uint64_t start_time = get_current_time_ns();
    
    // Check if command has changed
    bool command_changed = (strcmp(layer->command_text, command_text) != 0);
    bool cursor_changed = (layer->cursor_position != cursor_pos);
    
    /* Always need initial render even if buffer is empty */
    bool is_first_render = (layer->update_sequence_number == 0);
    
    /* Check if completion menu state changed (even if command/cursor didn't)
     * When menu is shown/hidden, we need redraw even if command text unchanged */
    bool menu_changed = false;
    display_controller_t *dc = display_integration_get_controller();
    if (dc) {
        menu_changed = display_controller_check_and_clear_menu_changed(dc);
    }
    
    if (!command_changed && !cursor_changed && !is_first_render && !menu_changed) {
        // No change, just update performance stats with minimal time
        update_performance_stats(layer, get_current_time_ns() - start_time);
        return COMMAND_LAYER_SUCCESS;
    }
    
    // Update command text and cursor position
    safe_string_copy(layer->command_text, command_text, sizeof(layer->command_text));
    layer->cursor_position = cursor_pos;
    layer->needs_redraw = true;
    layer->update_sequence_number++;
    
    // Check cache first for performance
    command_cache_entry_t *cached = NULL;
    if (layer->syntax_config.cache_enabled && command_changed) {
        cached = find_cache_entry(layer, command_text);
        layer->performance.cache_hits += (cached != NULL) ? 1 : 0;
        layer->performance.cache_misses += (cached == NULL) ? 1 : 0;
        
        // Record layer-specific cache operation
        display_integration_record_layer_cache_operation("command_layer", (cached != NULL));
    }
    
    if (cached && cached->is_valid) {
        // Use cached result
        safe_string_copy(layer->highlighted_text, cached->highlighted_text, 
                        sizeof(layer->highlighted_text));
        layer->metrics = cached->metrics;
        layer->region_count = 0; // Reset regions as they're embedded in highlighted text
    } else {
        // Perform syntax highlighting
        command_layer_error_t result = perform_syntax_highlighting(layer);
        if (result != COMMAND_LAYER_SUCCESS) {
            update_performance_stats(layer, get_current_time_ns() - start_time);
            return result;
        }
        
        // Update metrics
        result = update_command_metrics(layer);
        if (result != COMMAND_LAYER_SUCCESS) {
            update_performance_stats(layer, get_current_time_ns() - start_time);
            return result;
        }
        
        // Add to cache if enabled
        if (layer->syntax_config.cache_enabled && command_changed) {
            add_to_cache(layer, command_text, layer->highlighted_text, &layer->metrics);
        }
    }
    
    // Update cursor position in metrics
    layer->metrics.cursor_position = cursor_pos;
    
    uint64_t operation_time = get_current_time_ns() - start_time;
    update_performance_stats(layer, operation_time);
    
    // Publish content changed event if command changed
    if (command_changed) {
        publish_command_event(layer, LAYER_EVENT_CONTENT_CHANGED);
    }
    
    // Publish cursor moved event if cursor changed
    if (cursor_changed) {
        publish_command_event(layer, LAYER_EVENT_CURSOR_MOVED);
    }
    
    // Publish redraw needed event (triggers display_controller to render)
    // This was previously done by command_layer_update(), but that caused
    // redundant syntax highlighting. Now we do it here after highlighting.
    publish_command_event(layer, LAYER_EVENT_REDRAW_NEEDED);
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_get_highlighted_text(command_layer_t *layer,
                                                        char *output,
                                                        size_t output_size) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (!output || output_size == 0) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    size_t highlighted_length = strlen(layer->highlighted_text);
    if (highlighted_length >= output_size) {
        return COMMAND_LAYER_ERROR_BUFFER_TOO_SMALL;
    }
    
    safe_string_copy(output, layer->highlighted_text, output_size);
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_set_cursor_position(command_layer_t *layer,
                                                       size_t cursor_pos) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    size_t command_length = strlen(layer->command_text);
    if (cursor_pos > command_length) {
        return COMMAND_LAYER_ERROR_INVALID_CURSOR_POS;
    }
    
    if (layer->cursor_position != cursor_pos) {
        layer->cursor_position = cursor_pos;
        layer->metrics.cursor_position = cursor_pos;
        layer->needs_redraw = true;
        
        // Publish cursor moved event
        publish_command_event(layer, LAYER_EVENT_CURSOR_MOVED);
    }
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_get_metrics(command_layer_t *layer,
                                               command_metrics_t *metrics) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (!metrics) {
        return COMMAND_LAYER_ERROR_NULL_POINTER;
    }
    
    *metrics = layer->metrics;
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_update(command_layer_t *layer) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    uint64_t start_time = get_current_time_ns();
    
    // Force syntax highlighting update
    command_layer_error_t result = perform_syntax_highlighting(layer);
    if (result != COMMAND_LAYER_SUCCESS) {
        return result;
    }
    
    // Update metrics
    result = update_command_metrics(layer);
    if (result != COMMAND_LAYER_SUCCESS) {
        return result;
    }
    
    layer->needs_redraw = true;
    layer->update_sequence_number++;
    
    uint64_t operation_time = get_current_time_ns() - start_time;
    update_performance_stats(layer, operation_time);
    
    // Publish redraw needed event
    publish_command_event(layer, LAYER_EVENT_REDRAW_NEEDED);
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_clear(command_layer_t *layer) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    // Clear command content
    layer->command_text[0] = '\0';
    layer->highlighted_text[0] = '\0';
    layer->cursor_position = 0;
    layer->region_count = 0;
    layer->needs_redraw = true;
    
    // Reset metrics
    memset(&layer->metrics, 0, sizeof(command_metrics_t));
    
    // Publish content changed event
    publish_command_event(layer, LAYER_EVENT_CONTENT_CHANGED);
    
    return COMMAND_LAYER_SUCCESS;
}

// ============================================================================
// NOTE: Completion menu integration moved to display_controller
// All menu-related functions removed - proper architecture now in place
// ============================================================================

command_layer_error_t command_layer_cleanup(command_layer_t *layer) {
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    // Unsubscribe from events if event system is available
    if (layer->event_system && layer->initialized) {
        layer_events_unsubscribe(layer->event_system, LAYER_EVENT_THEME_CHANGED, LAYER_ID_COMMAND_LAYER);
        layer_events_unsubscribe(layer->event_system, LAYER_EVENT_CONTENT_CHANGED, LAYER_ID_COMMAND_LAYER);
    }
    
    // Clear cache
    for (size_t i = 0; i < COMMAND_LAYER_CACHE_SIZE; i++) {
        layer->cache[i].is_valid = false;
    }
    layer->cache_size = 0;
    
    // Free completion menu content
    if (layer->completion_menu_content) {
        free(layer->completion_menu_content);
        layer->completion_menu_content = NULL;
        layer->completion_menu_content_size = 0;
    }
    layer->completion_menu_visible = false;
    layer->completion_menu_lines = 0;
    layer->completion_menu_selected_index = 0;
    
    // Reset state
    reset_layer_state(layer);
    layer->initialized = false;
    
    return COMMAND_LAYER_SUCCESS;
}

void command_layer_destroy(command_layer_t *layer) {
    if (!layer) {
        return;
    }
    
    // Cleanup if not already done
    if (layer->initialized) {
        command_layer_cleanup(layer);
    }
    
    // Destroy spec highlighter
    if (layer->spec_highlighter) {
        lle_syntax_highlighter_destroy(layer->spec_highlighter);
        layer->spec_highlighter = NULL;
    }
    
    // Clear magic number
    layer->magic = 0;
    
    // Free memory
    free(layer);
}

// ============================================================================
// SYNTAX HIGHLIGHTING IMPLEMENTATION
// ============================================================================

/**
 * Map spec token type to command layer token type
 */
static command_token_type_t map_spec_token_type(lle_syntax_token_type_t spec_type) {
    switch (spec_type) {
        case LLE_TOKEN_COMMAND_VALID:
        case LLE_TOKEN_COMMAND_BUILTIN:
        case LLE_TOKEN_COMMAND_ALIAS:
        case LLE_TOKEN_COMMAND_FUNCTION:
            return COMMAND_TOKEN_COMMAND;
        case LLE_TOKEN_COMMAND_INVALID:
            return COMMAND_TOKEN_ERROR;
        case LLE_TOKEN_KEYWORD:
            return COMMAND_TOKEN_KEYWORD;
        case LLE_TOKEN_STRING_SINGLE:
        case LLE_TOKEN_STRING_DOUBLE:
        case LLE_TOKEN_STRING_BACKTICK:
            return COMMAND_TOKEN_STRING;
        case LLE_TOKEN_VARIABLE:
        case LLE_TOKEN_VARIABLE_SPECIAL:
            return COMMAND_TOKEN_VARIABLE;
        case LLE_TOKEN_PATH_VALID:
        case LLE_TOKEN_PATH_INVALID:
            return COMMAND_TOKEN_PATH;
        case LLE_TOKEN_PIPE:
            return COMMAND_TOKEN_PIPE;
        case LLE_TOKEN_REDIRECT:
            return COMMAND_TOKEN_REDIRECT;
        case LLE_TOKEN_AND:
        case LLE_TOKEN_OR:
        case LLE_TOKEN_BACKGROUND:
        case LLE_TOKEN_SEMICOLON:
        case LLE_TOKEN_SUBSHELL_START:
        case LLE_TOKEN_SUBSHELL_END:
        case LLE_TOKEN_BRACE_START:
        case LLE_TOKEN_BRACE_END:
            return COMMAND_TOKEN_OPERATOR;
        case LLE_TOKEN_COMMENT:
            return COMMAND_TOKEN_COMMENT;
        case LLE_TOKEN_NUMBER:
            return COMMAND_TOKEN_NUMBER;
        case LLE_TOKEN_OPTION:
            return COMMAND_TOKEN_OPTION;
        case LLE_TOKEN_ARGUMENT:
        case LLE_TOKEN_GLOB:
            return COMMAND_TOKEN_ARGUMENT;
        case LLE_TOKEN_ERROR:
        case LLE_TOKEN_UNCLOSED_STRING:
        case LLE_TOKEN_UNCLOSED_SUBSHELL:
            return COMMAND_TOKEN_ERROR;
        default:
            return COMMAND_TOKEN_NONE;
    }
}

/**
 * Primary syntax highlighting using spec-compliant system (Spec 11)
 * Falls back to inline implementation if spec highlighter unavailable
 */
static command_layer_error_t perform_syntax_highlighting(command_layer_t *layer) {
    if (!layer->syntax_config.enabled) {
        // If highlighting is disabled, just copy the command text
        safe_string_copy(layer->highlighted_text, layer->command_text, 
                        sizeof(layer->highlighted_text));
        layer->region_count = 0;
        return COMMAND_LAYER_SUCCESS;
    }
    
    uint64_t start_time = get_current_time_ns();
    
    // Initialize highlighting output
    layer->highlighted_text[0] = '\0';
    layer->region_count = 0;
    
    if (!layer->command_text[0]) {
        // Empty command, nothing to highlight
        return COMMAND_LAYER_SUCCESS;
    }
    
    // Use spec-compliant highlighter if available
    if (layer->spec_highlighter) {
        size_t command_len = strlen(layer->command_text);
        
        // Tokenize using spec highlighter
        int token_count = lle_syntax_highlight(layer->spec_highlighter, 
                                                layer->command_text, 
                                                command_len);
        
        if (token_count >= 0) {
            // Get tokens from spec highlighter
            size_t spec_token_count;
            const lle_syntax_token_t *spec_tokens = lle_syntax_get_tokens(
                layer->spec_highlighter, &spec_token_count);
            
            // Build highlighted output using command layer's color scheme
            // This preserves the command layer's theme integration
            size_t output_pos = 0;
            size_t last_end = 0;
            
            for (size_t i = 0; i < spec_token_count && 
                 layer->region_count < COMMAND_LAYER_MAX_HIGHLIGHT_REGIONS; i++) {
                const lle_syntax_token_t *tok = &spec_tokens[i];
                
                // Add any whitespace/text between tokens
                if (last_end < tok->start) {
                    size_t gap_len = tok->start - last_end;
                    if (output_pos + gap_len < sizeof(layer->highlighted_text) - 1) {
                        memcpy(layer->highlighted_text + output_pos,
                               layer->command_text + last_end, gap_len);
                        output_pos += gap_len;
                    }
                }
                
                // Map spec token type to command layer type
                command_token_type_t cmd_type = map_spec_token_type(tok->type);
                
                // Skip whitespace tokens for coloring
                if (tok->type == LLE_TOKEN_WHITESPACE) {
                    size_t tok_len = tok->end - tok->start;
                    if (output_pos + tok_len < sizeof(layer->highlighted_text) - 1) {
                        memcpy(layer->highlighted_text + output_pos,
                               layer->command_text + tok->start, tok_len);
                        output_pos += tok_len;
                    }
                    last_end = tok->end;
                    continue;
                }
                
                // Get color from command layer's color scheme
                const char *color = get_token_color(layer, cmd_type);
                bool has_color = layer->syntax_config.use_colors && color && color[0];
                
                // Add color code
                if (has_color) {
                    size_t color_len = strlen(color);
                    if (output_pos + color_len < sizeof(layer->highlighted_text) - 1) {
                        memcpy(layer->highlighted_text + output_pos, color, color_len);
                        output_pos += color_len;
                    }
                }
                
                // Add token text, re-applying color after newlines for multiline tokens
                size_t tok_len = tok->end - tok->start;
                const char *tok_text = layer->command_text + tok->start;
                for (size_t j = 0; j < tok_len && 
                     output_pos < sizeof(layer->highlighted_text) - 1; j++) {
                    layer->highlighted_text[output_pos++] = tok_text[j];
                    
                    // Re-apply color after newlines within tokens
                    if (tok_text[j] == '\n' && has_color && j + 1 < tok_len) {
                        size_t color_len = strlen(color);
                        if (output_pos + color_len < sizeof(layer->highlighted_text) - 1) {
                            memcpy(layer->highlighted_text + output_pos, color, color_len);
                            output_pos += color_len;
                        }
                    }
                }
                
                // Add reset code
                if (has_color) {
                    const char *reset = layer->syntax_config.color_scheme.reset_color;
                    size_t reset_len = strlen(reset);
                    if (output_pos + reset_len < sizeof(layer->highlighted_text) - 1) {
                        memcpy(layer->highlighted_text + output_pos, reset, reset_len);
                        output_pos += reset_len;
                    }
                }
                
                // Store region info
                command_highlight_region_t *region = &layer->highlight_regions[layer->region_count];
                region->start = tok->start;
                region->length = tok->end - tok->start;
                region->token_type = cmd_type;
                if (color) {
                    safe_string_copy(region->color_code, color, sizeof(region->color_code));
                } else {
                    region->color_code[0] = '\0';
                }
                layer->region_count++;
                
                last_end = tok->end;
                g_highlighting_stats.tokens_parsed++;
            }
            
            // Add any remaining text
            if (last_end < command_len) {
                size_t remaining = command_len - last_end;
                if (output_pos + remaining < sizeof(layer->highlighted_text) - 1) {
                    memcpy(layer->highlighted_text + output_pos,
                           layer->command_text + last_end, remaining);
                    output_pos += remaining;
                }
            }
            
            layer->highlighted_text[output_pos] = '\0';
            
            uint64_t highlighting_time = get_current_time_ns() - start_time;
            g_highlighting_stats.highlighting_time_ns += highlighting_time;
            g_highlighting_stats.regions_created += layer->region_count;
            
            return COMMAND_LAYER_SUCCESS;
        }
        // Spec highlighting returned error - fall through to plain text
    }
    
    // No spec highlighter available or it failed - just copy plain text
    // This is a graceful degradation: syntax highlighting is cosmetic,
    // so we show unhighlighted text rather than failing
    safe_string_copy(layer->highlighted_text, layer->command_text,
                     sizeof(layer->highlighted_text));
    return COMMAND_LAYER_SUCCESS;
}

static const char *get_token_color(command_layer_t *layer, command_token_type_t token_type) {
    if (!layer->syntax_config.use_colors) {
        return "";
    }
    
    switch (token_type) {
        case COMMAND_TOKEN_COMMAND:
            return layer->syntax_config.color_scheme.command_color;
        case COMMAND_TOKEN_ARGUMENT:
            return layer->syntax_config.color_scheme.argument_color;
        case COMMAND_TOKEN_OPTION:
            return layer->syntax_config.color_scheme.option_color;
        case COMMAND_TOKEN_STRING:
            return layer->syntax_config.color_scheme.string_color;
        case COMMAND_TOKEN_VARIABLE:
            return layer->syntax_config.color_scheme.variable_color;
        case COMMAND_TOKEN_REDIRECT:
            return layer->syntax_config.color_scheme.redirect_color;
        case COMMAND_TOKEN_PIPE:
            return layer->syntax_config.color_scheme.pipe_color;
        case COMMAND_TOKEN_KEYWORD:
            return layer->syntax_config.color_scheme.keyword_color;
        case COMMAND_TOKEN_OPERATOR:
            return layer->syntax_config.color_scheme.operator_color;
        case COMMAND_TOKEN_PATH:
            return layer->syntax_config.color_scheme.path_color;
        case COMMAND_TOKEN_NUMBER:
            return layer->syntax_config.color_scheme.number_color;
        case COMMAND_TOKEN_COMMENT:
            return layer->syntax_config.color_scheme.comment_color;
        case COMMAND_TOKEN_ERROR:
            return layer->syntax_config.color_scheme.error_color;
        default:
            return "";
    }
}

// ============================================================================
// CACHE MANAGEMENT
// ============================================================================

static uint32_t calculate_command_hash(const char *command) {
    uint32_t hash = 0;
    for (const char *p = command; *p; p++) {
        hash = hash * CACHE_HASH_MULTIPLIER + (unsigned char)*p;
    }
    return hash;
}

static command_cache_entry_t *find_cache_entry(command_layer_t *layer, const char *command) {
    if (!layer->syntax_config.cache_enabled || !command) {
        return NULL;
    }
    
    uint32_t hash = calculate_command_hash(command);
    uint64_t current_time = get_current_time_ns();
    
    for (size_t i = 0; i < layer->cache_size; i++) {
        command_cache_entry_t *entry = &layer->cache[i];
        
        if (!entry->is_valid) {
            continue;
        }
        
        // Check if cache entry has expired
        uint64_t age_ns = current_time - entry->timestamp_ns;
        if (age_ns > (layer->syntax_config.cache_expiry_ms * NANOSECONDS_PER_MILLISECOND)) {
            entry->is_valid = false;
            continue;
        }
        
        // Check if hash and command match
        if (entry->hash == hash && strcmp(entry->command_text, command) == 0) {
            return entry;
        }
    }
    
    return NULL;
}

static command_layer_error_t add_to_cache(command_layer_t *layer, const char *command, 
                                         const char *highlighted, const command_metrics_t *metrics) {
    if (!layer->syntax_config.cache_enabled || !command || !highlighted || !metrics) {
        return COMMAND_LAYER_SUCCESS; // Not an error, just disabled
    }
    
    // Find empty slot or oldest entry
    command_cache_entry_t *entry = NULL;
    uint64_t oldest_time = UINT64_MAX;
    size_t oldest_index = 0;
    
    for (size_t i = 0; i < COMMAND_LAYER_CACHE_SIZE; i++) {
        if (!layer->cache[i].is_valid) {
            entry = &layer->cache[i];
            break;
        }
        
        if (layer->cache[i].timestamp_ns < oldest_time) {
            oldest_time = layer->cache[i].timestamp_ns;
            oldest_index = i;
        }
    }
    
    // If no empty slot found, use oldest entry
    if (!entry) {
        entry = &layer->cache[oldest_index];
    } else if (layer->cache_size < COMMAND_LAYER_CACHE_SIZE) {
        layer->cache_size++;
    }
    
    // Store cache entry
    safe_string_copy(entry->command_text, command, sizeof(entry->command_text));
    safe_string_copy(entry->highlighted_text, highlighted, sizeof(entry->highlighted_text));
    entry->metrics = *metrics;
    entry->timestamp_ns = get_current_time_ns();
    entry->hash = calculate_command_hash(command);
    entry->is_valid = true;
    
    return COMMAND_LAYER_SUCCESS;
}

MAYBE_UNUSED
static void expire_old_cache_entries(command_layer_t *layer) {
    uint64_t current_time = get_current_time_ns();
    uint64_t expiry_time_ns = layer->syntax_config.cache_expiry_ms * NANOSECONDS_PER_MILLISECOND;
    
    for (size_t i = 0; i < layer->cache_size; i++) {
        command_cache_entry_t *entry = &layer->cache[i];
        
        if (entry->is_valid) {
            uint64_t age_ns = current_time - entry->timestamp_ns;
            if (age_ns > expiry_time_ns) {
                entry->is_valid = false;
            }
        }
    }
}

// ============================================================================
// METRICS AND PERFORMANCE
// ============================================================================

static command_layer_error_t update_command_metrics(command_layer_t *layer) {
    command_metrics_t *metrics = &layer->metrics;
    
    // Basic metrics
    metrics->command_length = strlen(layer->command_text);
    metrics->visual_length = strlen(layer->highlighted_text);
    metrics->token_count = layer->region_count;
    metrics->cursor_position = layer->cursor_position;
    
    // Error detection
    metrics->error_count = 0;
    metrics->has_syntax_errors = false;
    for (size_t i = 0; i < layer->region_count; i++) {
        if (layer->highlight_regions[i].token_type == COMMAND_TOKEN_ERROR) {
            metrics->error_count++;
            metrics->has_syntax_errors = true;
        }
    }
    
    // Multiline detection
    metrics->is_multiline_command = (strchr(layer->command_text, '\n') != NULL);
    
    // Display positioning (will be updated by composition engine)
    metrics->estimated_display_column = 0;
    metrics->estimated_display_row = 0;
    
    // If prompt layer integration is available, get positioning info
    if (layer->prompt_integration_enabled && layer->prompt_layer) {
        prompt_metrics_t prompt_metrics;
        if (prompt_layer_get_metrics(layer->prompt_layer, &prompt_metrics) == PROMPT_LAYER_SUCCESS) {
            metrics->estimated_display_column = prompt_metrics.estimated_command_column;
            metrics->estimated_display_row = prompt_metrics.estimated_command_row;
        }
    }
    
    return COMMAND_LAYER_SUCCESS;
}

static uint64_t get_current_time_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * NANOSECONDS_PER_SECOND + (uint64_t)ts.tv_nsec;
}

static void update_performance_stats(command_layer_t *layer, uint64_t operation_time_ns) {
    command_performance_t *perf = &layer->performance;
    
    perf->update_count++;
    perf->total_processing_time_ns += operation_time_ns;
    
    // Update min/max times
    if (operation_time_ns < perf->min_update_time_ns) {
        perf->min_update_time_ns = operation_time_ns;
    }
    if (operation_time_ns > perf->max_update_time_ns) {
        perf->max_update_time_ns = operation_time_ns;
    }
    
    // Calculate average
    perf->avg_update_time_ns = perf->total_processing_time_ns / perf->update_count;
    
    // Update last update time
    clock_gettime(CLOCK_MONOTONIC, &layer->last_update_time);
}

// ============================================================================
// EVENT HANDLING
// ============================================================================

static layer_events_error_t handle_layer_event(const layer_event_t *event, void *user_data) {
    command_layer_t *layer = (command_layer_t *)user_data;
    
    if (!validate_layer_state(layer) || !event) {
        return LAYER_EVENTS_ERROR_INVALID_PARAM;
    }
    
    switch (event->type) {
        case LAYER_EVENT_THEME_CHANGED:
            // Update color scheme when theme changes
            command_layer_create_default_colors(&layer->syntax_config.color_scheme);
            layer->needs_redraw = true;
            publish_command_event(layer, LAYER_EVENT_REDRAW_NEEDED);
            break;
            
        case LAYER_EVENT_CONTENT_CHANGED:
            // Prompt layer content changed, update positioning
            if (layer->prompt_integration_enabled) {
                update_command_metrics(layer);
                layer->needs_redraw = true;
            }
            break;
            
        case LAYER_EVENT_SIZE_CHANGED:
            // Terminal size changed, may affect display
            layer->needs_redraw = true;
            publish_command_event(layer, LAYER_EVENT_REDRAW_NEEDED);
            break;
            
        default:
            // Ignore other events
            break;
    }
    
    return LAYER_EVENTS_SUCCESS;
}

static command_layer_error_t publish_command_event(command_layer_t *layer, layer_event_type_t event_type) {
    if (!layer->event_system) {
        return COMMAND_LAYER_SUCCESS; // Not an error if no event system
    }
    
    /* Use HIGH priority for REDRAW_NEEDED events to match display_controller subscription */
    layer_event_priority_t priority = (event_type == LAYER_EVENT_REDRAW_NEEDED) ? 
                                      LAYER_EVENT_PRIORITY_HIGH : 
                                      LAYER_EVENT_PRIORITY_NORMAL;
    
    layer_events_error_t result = layer_events_publish_simple(
        layer->event_system,
        event_type,
        LAYER_ID_COMMAND_LAYER,
        0,
        priority
    );
    
    return (result == LAYER_EVENTS_SUCCESS) ? COMMAND_LAYER_SUCCESS : COMMAND_LAYER_ERROR_EVENT_SYSTEM;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static bool validate_layer_state(command_layer_t *layer) {
    return (layer != NULL && 
            layer->magic == COMMAND_LAYER_MAGIC && 
            layer->initialized);
}

static void reset_layer_state(command_layer_t *layer) {
    if (!layer) {
        return;
    }
    
    layer->command_text[0] = '\0';
    layer->highlighted_text[0] = '\0';
    layer->cursor_position = 0;
    layer->region_count = 0;
    layer->needs_redraw = false;
    
    memset(&layer->metrics, 0, sizeof(command_metrics_t));
    memset(&layer->performance, 0, sizeof(command_performance_t));
    layer->performance.min_update_time_ns = UINT64_MAX;
    
    layer->cache_size = 0;
    layer->cache_access_count = 0;
    
    layer->update_sequence_number = 0;
}

static size_t safe_string_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return 0;
    }
    
    size_t src_len = strlen(src);
    size_t copy_len = (src_len < dest_size - 1) ? src_len : dest_size - 1;
    
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    
    return copy_len;
}

// ============================================================================
// CONFIGURATION AND SYNTAX HIGHLIGHTING API
// ============================================================================

command_layer_error_t command_layer_set_syntax_enabled(command_layer_t *layer, bool enabled) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (layer->syntax_config.enabled != enabled) {
        layer->syntax_config.enabled = enabled;
        layer->needs_redraw = true;
        
        // Clear cache when enabling/disabling
        command_layer_clear_cache(layer);
        
        publish_command_event(layer, LAYER_EVENT_STYLE_UPDATED);
    }
    
    return COMMAND_LAYER_SUCCESS;
}

bool command_layer_is_syntax_enabled(command_layer_t *layer) {
    if (!validate_layer_state(layer)) {
        return false;
    }
    
    return layer->syntax_config.enabled;
}

command_layer_error_t command_layer_set_color_scheme(command_layer_t *layer,
                                                     const command_color_scheme_t *color_scheme) {
    if (!validate_layer_state(layer) || !color_scheme) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    layer->syntax_config.color_scheme = *color_scheme;
    layer->needs_redraw = true;
    
    // Clear cache since colors changed
    command_layer_clear_cache(layer);
    
    publish_command_event(layer, LAYER_EVENT_STYLE_UPDATED);
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_get_color_scheme(command_layer_t *layer,
                                                     command_color_scheme_t *color_scheme) {
    if (!validate_layer_state(layer) || !color_scheme) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    *color_scheme = layer->syntax_config.color_scheme;
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_set_syntax_config(command_layer_t *layer,
                                                      const command_syntax_config_t *config) {
    if (!validate_layer_state(layer) || !config) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    layer->syntax_config = *config;
    layer->needs_redraw = true;
    
    // Clear cache since configuration changed
    command_layer_clear_cache(layer);
    
    publish_command_event(layer, LAYER_EVENT_STYLE_UPDATED);
    
    return COMMAND_LAYER_SUCCESS;
}

// ============================================================================
// PROMPT LAYER INTEGRATION
// ============================================================================

command_layer_error_t command_layer_set_prompt_layer(command_layer_t *layer,
                                                     prompt_layer_t *prompt_layer) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    layer->prompt_layer = prompt_layer;
    
    if (prompt_layer && layer->prompt_integration_enabled) {
        // Update metrics with new prompt information
        update_command_metrics(layer);
        layer->needs_redraw = true;
    }
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_set_prompt_integration(command_layer_t *layer, bool enabled) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    layer->prompt_integration_enabled = enabled;
    
    if (enabled && layer->prompt_layer) {
        // Update metrics when enabling integration
        update_command_metrics(layer);
        layer->needs_redraw = true;
    }
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_get_display_position(command_layer_t *layer,
                                                        int *column, int *row) {
    if (!validate_layer_state(layer) || !column || !row) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    *column = layer->metrics.estimated_display_column;
    *row = layer->metrics.estimated_display_row;
    
    return COMMAND_LAYER_SUCCESS;
}

// ============================================================================
// PERFORMANCE AND MONITORING API
// ============================================================================

command_layer_error_t command_layer_get_performance(command_layer_t *layer,
                                                   command_performance_t *performance) {
    if (!validate_layer_state(layer) || !performance) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    *performance = layer->performance;
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_reset_performance(command_layer_t *layer) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    memset(&layer->performance, 0, sizeof(command_performance_t));
    layer->performance.min_update_time_ns = UINT64_MAX;
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_set_cache_enabled(command_layer_t *layer, bool enabled) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    layer->syntax_config.cache_enabled = enabled;
    
    if (!enabled) {
        // Clear cache when disabling
        command_layer_clear_cache(layer);
    }
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_clear_cache(command_layer_t *layer) {
    if (!validate_layer_state(layer)) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    for (size_t i = 0; i < COMMAND_LAYER_CACHE_SIZE; i++) {
        layer->cache[i].is_valid = false;
    }
    layer->cache_size = 0;
    layer->performance.cache_hits = 0;
    layer->performance.cache_misses = 0;
    
    return COMMAND_LAYER_SUCCESS;
}

// ============================================================================
// VALIDATION AND DEBUGGING
// ============================================================================

bool command_layer_validate(command_layer_t *layer) {
    return validate_layer_state(layer);
}

command_layer_error_t command_layer_get_debug_info(command_layer_t *layer,
                                                   char *status_buffer,
                                                   size_t buffer_size) {
    if (!validate_layer_state(layer) || !status_buffer || buffer_size == 0) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    snprintf(status_buffer, buffer_size,
        "Command Layer Debug Info:\n"
        "Version: %s\n"
        "Initialized: %s\n"
        "Syntax Enabled: %s\n"
        "Cache Enabled: %s\n"
        "Command Length: %zu\n"
        "Cursor Position: %zu\n"
        "Highlight Regions: %zu\n"
        "Cache Size: %zu\n"
        "Update Count: %lu\n"
        "Cache Hit Rate: %.1f%%\n"
        "Avg Update Time: %lu ns\n"
        "Needs Redraw: %s\n",
        COMMAND_LAYER_VERSION_STRING,
        layer->initialized ? "Yes" : "No",
        layer->syntax_config.enabled ? "Yes" : "No",
        layer->syntax_config.cache_enabled ? "Yes" : "No",
        layer->metrics.command_length,
        layer->cursor_position,
        layer->region_count,
        layer->cache_size,
        (unsigned long)layer->performance.update_count,
        (layer->performance.cache_hits + layer->performance.cache_misses > 0) ?
            (100.0 * layer->performance.cache_hits / (layer->performance.cache_hits + layer->performance.cache_misses)) : 0.0,
        (unsigned long)layer->performance.avg_update_time_ns,
        layer->needs_redraw ? "Yes" : "No"
    );
    
    return COMMAND_LAYER_SUCCESS;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

command_layer_error_t command_layer_create_default_config(command_syntax_config_t *config) {
    if (!config) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    config->enabled = true;
    config->use_colors = true;
    config->highlight_errors = true;
    config->cache_enabled = true;
    config->cache_expiry_ms = COMMAND_LAYER_CACHE_EXPIRY_MS;
    config->max_update_time_ms = COMMAND_LAYER_TARGET_UPDATE_TIME_MS;
    
    return command_layer_create_default_colors(&config->color_scheme);
}

command_layer_error_t command_layer_create_default_colors(command_color_scheme_t *color_scheme) {
    if (!color_scheme) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }

    // Detect terminal capabilities for adaptive color support via LLE
    lle_terminal_detection_result_t *detection = NULL;
    lle_detect_terminal_capabilities_optimized(&detection);

    // Determine color mode based on terminal capabilities
    bool has_colors = detection && detection->supports_colors;
    bool has_256_colors = detection && detection->supports_256_colors;
    bool has_truecolor = detection && detection->supports_truecolor;

    // Reset color is always the same when colors are supported
    if (has_colors) {
        safe_string_copy(color_scheme->reset_color, "\033[0m", sizeof(color_scheme->reset_color));
    } else {
        // No colors - use empty string
        color_scheme->reset_color[0] = '\0';
    }

    if (!has_colors) {
        // Terminal doesn't support colors - use empty strings for all colors
        // This allows the shell to work on dumb terminals or when piping
        color_scheme->command_color[0] = '\0';
        color_scheme->argument_color[0] = '\0';
        color_scheme->option_color[0] = '\0';
        color_scheme->string_color[0] = '\0';
        color_scheme->variable_color[0] = '\0';
        color_scheme->redirect_color[0] = '\0';
        color_scheme->pipe_color[0] = '\0';
        color_scheme->keyword_color[0] = '\0';
        color_scheme->operator_color[0] = '\0';
        color_scheme->path_color[0] = '\0';
        color_scheme->number_color[0] = '\0';
        color_scheme->comment_color[0] = '\0';
        color_scheme->error_color[0] = '\0';
    } else if (has_256_colors || has_truecolor) {
        // 256-color or truecolor terminal - use enhanced color palette
        // These colors provide better visual distinction on modern terminals
        safe_string_copy(color_scheme->command_color, "\033[38;5;82m", sizeof(color_scheme->command_color));    // Bright lime green (82)
        safe_string_copy(color_scheme->argument_color, "\033[38;5;252m", sizeof(color_scheme->argument_color)); // Light gray (252)
        safe_string_copy(color_scheme->option_color, "\033[38;5;51m", sizeof(color_scheme->option_color));      // Cyan (51)
        safe_string_copy(color_scheme->string_color, "\033[38;5;214m", sizeof(color_scheme->string_color));     // Orange (214)
        safe_string_copy(color_scheme->variable_color, "\033[38;5;177m", sizeof(color_scheme->variable_color)); // Light purple (177)
        safe_string_copy(color_scheme->redirect_color, "\033[38;5;203m", sizeof(color_scheme->redirect_color)); // Salmon red (203)
        safe_string_copy(color_scheme->pipe_color, "\033[38;5;69m", sizeof(color_scheme->pipe_color));          // Steel blue (69)
        safe_string_copy(color_scheme->keyword_color, "\033[38;5;75m", sizeof(color_scheme->keyword_color));    // Sky blue (75)
        safe_string_copy(color_scheme->operator_color, "\033[38;5;167m", sizeof(color_scheme->operator_color)); // Indian red (167)
        safe_string_copy(color_scheme->path_color, "\033[38;5;114m", sizeof(color_scheme->path_color));         // Pale green (114)
        safe_string_copy(color_scheme->number_color, "\033[38;5;229m", sizeof(color_scheme->number_color));     // Light yellow (229)
        safe_string_copy(color_scheme->comment_color, "\033[38;5;244m", sizeof(color_scheme->comment_color));   // Gray (244)
        safe_string_copy(color_scheme->error_color, "\033[38;5;196m", sizeof(color_scheme->error_color));       // Bright red (196)
    } else {
        // Basic 16-color terminal - use standard ANSI colors with bold for bright
        safe_string_copy(color_scheme->command_color, "\033[1;32m", sizeof(color_scheme->command_color));     // Bright green
        safe_string_copy(color_scheme->argument_color, "\033[0;37m", sizeof(color_scheme->argument_color));   // White
        safe_string_copy(color_scheme->option_color, "\033[1;36m", sizeof(color_scheme->option_color));       // Bright cyan
        safe_string_copy(color_scheme->string_color, "\033[1;33m", sizeof(color_scheme->string_color));       // Bright yellow
        safe_string_copy(color_scheme->variable_color, "\033[1;35m", sizeof(color_scheme->variable_color));   // Bright magenta
        safe_string_copy(color_scheme->redirect_color, "\033[1;31m", sizeof(color_scheme->redirect_color));   // Bright red
        safe_string_copy(color_scheme->pipe_color, "\033[1;34m", sizeof(color_scheme->pipe_color));           // Bright blue
        safe_string_copy(color_scheme->keyword_color, "\033[1;34m", sizeof(color_scheme->keyword_color));     // Bright blue
        safe_string_copy(color_scheme->operator_color, "\033[1;31m", sizeof(color_scheme->operator_color));   // Bright red
        safe_string_copy(color_scheme->path_color, "\033[0;32m", sizeof(color_scheme->path_color));           // Green
        safe_string_copy(color_scheme->number_color, "\033[1;37m", sizeof(color_scheme->number_color));       // Bright white
        safe_string_copy(color_scheme->comment_color, "\033[0;90m", sizeof(color_scheme->comment_color));     // Dark gray
        safe_string_copy(color_scheme->error_color, "\033[1;31m", sizeof(color_scheme->error_color));         // Bright red
    }

    return COMMAND_LAYER_SUCCESS;
}

const char *command_layer_get_error_message(command_layer_error_t error) {
    switch (error) {
        case COMMAND_LAYER_SUCCESS:
            return "Success";
        case COMMAND_LAYER_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case COMMAND_LAYER_ERROR_NULL_POINTER:
            return "NULL pointer";
        case COMMAND_LAYER_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case COMMAND_LAYER_ERROR_BUFFER_TOO_SMALL:
            return "Buffer too small";
        case COMMAND_LAYER_ERROR_COMMAND_TOO_LARGE:
            return "Command too large";
        case COMMAND_LAYER_ERROR_INVALID_CURSOR_POS:
            return "Invalid cursor position";
        case COMMAND_LAYER_ERROR_CACHE_FULL:
            return "Cache is full";
        case COMMAND_LAYER_ERROR_SYNTAX_ERROR:
            return "Syntax highlighting error";
        case COMMAND_LAYER_ERROR_NOT_INITIALIZED:
            return "Layer not initialized";
        case COMMAND_LAYER_ERROR_EVENT_SYSTEM:
            return "Event system error";
        case COMMAND_LAYER_ERROR_PERFORMANCE_LIMIT:
            return "Performance limit exceeded";
        default:
            return "Unknown error";
    }
}

// ============================================================================
// COMPLETION MENU INTEGRATION
// ============================================================================

command_layer_error_t command_layer_set_completion_menu(
    command_layer_t *layer,
    const char *menu_content,
    int num_lines,
    int selected_index)
{
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (!menu_content || num_lines <= 0) {
        // Clear menu if invalid input
        return command_layer_clear_completion_menu(layer);
    }
    
    size_t content_len = strlen(menu_content);
    size_t needed_size = content_len + 1;
    
    // Allocate or reallocate buffer if needed
    if (layer->completion_menu_content_size < needed_size) {
        char *new_buffer = realloc(layer->completion_menu_content, needed_size);
        if (!new_buffer) {
            return COMMAND_LAYER_ERROR_MEMORY_ALLOCATION;
        }
        layer->completion_menu_content = new_buffer;
        layer->completion_menu_content_size = needed_size;
    }
    
    // Copy menu content
    memcpy(layer->completion_menu_content, menu_content, content_len + 1);
    
    // Update state
    layer->completion_menu_visible = true;
    layer->completion_menu_lines = num_lines;
    layer->completion_menu_selected_index = selected_index;
    layer->needs_redraw = true;
    
    // Publish redraw event
    if (layer->event_system) {
        layer_event_t event = {
            .type = LAYER_EVENT_REDRAW_NEEDED,
            .source_layer = LAYER_ID_COMMAND_LAYER,
            .timestamp = 0
        };
        layer_events_publish(layer->event_system, &event);
    }
    
    return COMMAND_LAYER_SUCCESS;
}

command_layer_error_t command_layer_clear_completion_menu(command_layer_t *layer)
{
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    bool was_visible = layer->completion_menu_visible;
    
    layer->completion_menu_visible = false;
    layer->completion_menu_lines = 0;
    layer->completion_menu_selected_index = 0;
    
    // Don't free the buffer - keep it for reuse
    // Just mark as not visible
    
    if (was_visible) {
        layer->needs_redraw = true;
        
        // Publish redraw event
        if (layer->event_system) {
            layer_event_t event = {
                .type = LAYER_EVENT_REDRAW_NEEDED,
                .source_layer = LAYER_ID_COMMAND_LAYER,
                .timestamp = 0
            };
            layer_events_publish(layer->event_system, &event);
        }
    }
    
    return COMMAND_LAYER_SUCCESS;
}

bool command_layer_is_menu_visible(const command_layer_t *layer)
{
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return false;
    }
    return layer->completion_menu_visible;
}

const char *command_layer_get_menu_content(const command_layer_t *layer)
{
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return NULL;
    }
    if (!layer->completion_menu_visible) {
        return NULL;
    }
    return layer->completion_menu_content;
}

int command_layer_get_menu_lines(const command_layer_t *layer)
{
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return 0;
    }
    if (!layer->completion_menu_visible) {
        return 0;
    }
    return layer->completion_menu_lines;
}

command_layer_error_t command_layer_set_menu_selection(
    command_layer_t *layer,
    int selected_index)
{
    if (!layer || layer->magic != COMMAND_LAYER_MAGIC) {
        return COMMAND_LAYER_ERROR_INVALID_PARAM;
    }
    
    if (!layer->completion_menu_visible) {
        return COMMAND_LAYER_ERROR_NOT_INITIALIZED;
    }
    
    layer->completion_menu_selected_index = selected_index;
    layer->needs_redraw = true;
    
    return COMMAND_LAYER_SUCCESS;
}