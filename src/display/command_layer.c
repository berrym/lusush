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
#include "display/base_terminal.h"
#include "display/terminal_control.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>

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

// Syntax highlighting constants
#define MAX_TOKEN_SIZE 256
#define MAX_WORD_SIZE 128

// ============================================================================
// INTERNAL STRUCTURES AND STATE
// ============================================================================

/**
 * Internal token parsing state
 */
typedef struct {
    const char *text;
    size_t length;
    size_t position;
    char current_char;
    bool in_string;
    char string_delimiter;
    bool in_comment;
    bool escaped;
} token_parser_t;

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
static void init_token_parser(token_parser_t *parser, const char *text);
static command_token_type_t get_next_token(token_parser_t *parser, size_t *token_start, size_t *token_length);
static command_token_type_t classify_token(const char *token, size_t length, bool is_first_token);
static bool is_shell_keyword(const char *token, size_t length);
static bool is_shell_builtin(const char *token, size_t length);
static const char *get_token_color(command_layer_t *layer, command_token_type_t token_type);

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
    
    // Initialize performance monitoring
    clock_gettime(CLOCK_MONOTONIC, &layer->last_update_time);
    layer->update_sequence_number = 0;
    
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
    
    if (!command_changed && !cursor_changed) {
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
    
    // Clear magic number
    layer->magic = 0;
    
    // Free memory
    free(layer);
}

// ============================================================================
// SYNTAX HIGHLIGHTING IMPLEMENTATION
// ============================================================================

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
    
    // Initialize token parser
    token_parser_t parser;
    init_token_parser(&parser, layer->command_text);
    
    size_t output_pos = 0;
    size_t token_start, token_length;
    command_token_type_t token_type;
    bool is_first_token = true;
    
    // Parse and highlight tokens
    size_t last_token_end = 0;  // Track position after last token
    
    while ((token_type = get_next_token(&parser, &token_start, &token_length)) != COMMAND_TOKEN_NONE) {
        // Skip if we've exceeded region limit
        if (layer->region_count >= COMMAND_LAYER_MAX_HIGHLIGHT_REGIONS) {
            break;
        }
        
        // Add any whitespace between previous token and this token
        if (!is_first_token && last_token_end < token_start) {
            size_t whitespace_len = token_start - last_token_end;
            if (output_pos + whitespace_len < sizeof(layer->highlighted_text) - 1) {
                strncpy(layer->highlighted_text + output_pos,
                       layer->command_text + last_token_end, whitespace_len);
                output_pos += whitespace_len;
                layer->highlighted_text[output_pos] = '\0';
            }
        }
        
        // Classify the token if it's not already classified
        if (token_type == COMMAND_TOKEN_ARGUMENT) {
            token_type = classify_token(layer->command_text + token_start, token_length, is_first_token);
        }
        
        // Get color for this token type
        const char *color = get_token_color(layer, token_type);
        
        // Add color code to output if colors are enabled
        if (layer->syntax_config.use_colors && color && color[0]) {
            size_t color_len = strlen(color);
            if (output_pos + color_len < sizeof(layer->highlighted_text) - 1) {
                strcpy(layer->highlighted_text + output_pos, color);
                output_pos += color_len;
            }
        }
        
        // Add the token text
        if (output_pos + token_length < sizeof(layer->highlighted_text) - 1) {
            strncpy(layer->highlighted_text + output_pos, 
                   layer->command_text + token_start, token_length);
            output_pos += token_length;
            layer->highlighted_text[output_pos] = '\0';
        }
        
        // Add reset color code if colors are enabled
        if (layer->syntax_config.use_colors && color && color[0]) {
            const char *reset = layer->syntax_config.color_scheme.reset_color;
            size_t reset_len = strlen(reset);
            if (output_pos + reset_len < sizeof(layer->highlighted_text) - 1) {
                strcpy(layer->highlighted_text + output_pos, reset);
                output_pos += reset_len;
            }
        }
        
        // Store highlight region information
        if (layer->region_count < COMMAND_LAYER_MAX_HIGHLIGHT_REGIONS) {
            command_highlight_region_t *region = &layer->highlight_regions[layer->region_count];
            region->start = token_start;
            region->length = token_length;
            region->token_type = token_type;
            if (color) {
                safe_string_copy(region->color_code, color, sizeof(region->color_code));
            } else {
                region->color_code[0] = '\0';
            }
            layer->region_count++;
        }
        
        // Update tracking for next iteration
        last_token_end = token_start + token_length;
        is_first_token = false;
        g_highlighting_stats.tokens_parsed++;
    }
    
    // Add any remaining text after the last token
    size_t command_len = strlen(layer->command_text);
    if (last_token_end < command_len) {
        size_t remaining_len = command_len - last_token_end;
        if (output_pos + remaining_len < sizeof(layer->highlighted_text) - 1) {
            strncpy(layer->highlighted_text + output_pos,
                   layer->command_text + last_token_end, remaining_len);
            output_pos += remaining_len;
            layer->highlighted_text[output_pos] = '\0';
        }
    }
    
    // Ensure null termination
    layer->highlighted_text[sizeof(layer->highlighted_text) - 1] = '\0';


    
    uint64_t highlighting_time = get_current_time_ns() - start_time;
    g_highlighting_stats.highlighting_time_ns += highlighting_time;
    g_highlighting_stats.regions_created += layer->region_count;
    
    return COMMAND_LAYER_SUCCESS;
}

static void init_token_parser(token_parser_t *parser, const char *text) {
    parser->text = text;
    parser->length = strlen(text);
    parser->position = 0;
    parser->current_char = (parser->length > 0) ? text[0] : '\0';
    parser->in_string = false;
    parser->string_delimiter = '\0';
    parser->in_comment = false;
    parser->escaped = false;
}

static command_token_type_t get_next_token(token_parser_t *parser, size_t *token_start, size_t *token_length) {
    // Skip whitespace
    while (parser->position < parser->length && isspace(parser->current_char)) {
        parser->position++;
        parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
    }
    
    if (parser->position >= parser->length) {
        return COMMAND_TOKEN_NONE;
    }
    
    *token_start = parser->position;
    
    // Handle comments
    if (parser->current_char == '#') {
        // Comment extends to end of line or end of string
        while (parser->position < parser->length && parser->current_char != '\n') {
            parser->position++;
            parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        }
        *token_length = parser->position - *token_start;
        return COMMAND_TOKEN_COMMENT;
    }
    
    // Handle quoted strings
    if (parser->current_char == '"' || parser->current_char == '\'' || parser->current_char == '`') {
        char quote_char = parser->current_char;
        parser->position++; // Skip opening quote
        parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        
        while (parser->position < parser->length && parser->current_char != quote_char) {
            if (parser->current_char == '\\' && quote_char != '\'') {
                parser->position++; // Skip escape character
                if (parser->position < parser->length) {
                    parser->position++; // Skip escaped character
                }
            } else {
                parser->position++;
            }
            parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        }
        
        if (parser->position < parser->length && parser->current_char == quote_char) {
            parser->position++; // Skip closing quote
            parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        }
        
        *token_length = parser->position - *token_start;
        return COMMAND_TOKEN_STRING;
    }
    
    // Handle variables
    if (parser->current_char == '$') {
        parser->position++;
        parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        
        // Handle ${var} syntax
        if (parser->current_char == '{') {
            parser->position++;
            while (parser->position < parser->length && parser->current_char != '}') {
                parser->position++;
                parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
            }
            if (parser->position < parser->length && parser->current_char == '}') {
                parser->position++;
                parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
            }
        } else {
            // Handle $var syntax
            while (parser->position < parser->length && 
                   (isalnum(parser->current_char) || parser->current_char == '_')) {
                parser->position++;
                parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
            }
        }
        
        *token_length = parser->position - *token_start;
        return COMMAND_TOKEN_VARIABLE;
    }
    
    // Handle operators and special characters
    if (strchr("|&;<>(){}[]", parser->current_char)) {
        char first_char = parser->current_char;
        parser->position++;
        parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        
        // Handle multi-character operators
        if ((first_char == '|' && parser->current_char == '|') ||
            (first_char == '&' && parser->current_char == '&') ||
            (first_char == '>' && parser->current_char == '>') ||
            (first_char == '<' && parser->current_char == '<')) {
            parser->position++;
            parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
        }
        
        *token_length = parser->position - *token_start;
        
        if (first_char == '|') {
            return COMMAND_TOKEN_PIPE;
        } else if (strchr("<>", first_char)) {
            return COMMAND_TOKEN_REDIRECT;
        } else {
            return COMMAND_TOKEN_OPERATOR;
        }
    }
    
    // Handle regular words/tokens
    while (parser->position < parser->length && 
           !isspace(parser->current_char) && 
           !strchr("|&;<>(){}[]\"'`$#", parser->current_char)) {
        parser->position++;
        parser->current_char = (parser->position < parser->length) ? parser->text[parser->position] : '\0';
    }
    
    *token_length = parser->position - *token_start;
    return COMMAND_TOKEN_ARGUMENT; // Will be reclassified later
}

static command_token_type_t classify_token(const char *token, size_t length, bool is_first_token) {
    if (length == 0) {
        return COMMAND_TOKEN_NONE;
    }
    
    // Check for options (start with -)
    if (token[0] == '-') {
        return COMMAND_TOKEN_OPTION;
    }
    
    // Check for numbers
    bool is_number = true;
    for (size_t i = 0; i < length; i++) {
        if (!isdigit(token[i]) && token[i] != '.' && token[i] != '-' && token[i] != '+') {
            is_number = false;
            break;
        }
    }
    if (is_number) {
        return COMMAND_TOKEN_NUMBER;
    }
    
    // Check for shell keywords
    if (is_shell_keyword(token, length)) {
        return COMMAND_TOKEN_KEYWORD;
    }
    
    // If it's the first token, it's likely a command
    if (is_first_token) {
        return COMMAND_TOKEN_COMMAND;
    }
    
    // Check if it looks like a path
    if (strchr(token, '/') || (length > 1 && token[0] == '.' && token[1] == '/') ||
        (length > 2 && token[0] == '.' && token[1] == '.' && token[2] == '/')) {
        return COMMAND_TOKEN_PATH;
    }
    
    return COMMAND_TOKEN_ARGUMENT;
}

static bool is_shell_keyword(const char *token, size_t length) {
    static const char *keywords[] = {
        "if", "then", "else", "elif", "fi",
        "for", "while", "until", "do", "done",
        "case", "esac", "function", "in",
        "select", "time", "coproc", "{", "}"
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i]) == length && strncmp(token, keywords[i], length) == 0) {
            return true;
        }
    }
    
    return false;
}

static bool is_shell_builtin(const char *token, size_t length) {
    static const char *builtins[] = {
        "cd", "pwd", "echo", "printf", "read", "exit",
        "export", "unset", "source", ".", "eval",
        "exec", "set", "unset", "alias", "unalias",
        "history", "jobs", "fg", "bg", "kill",
        "wait", "suspend", "disown", "help", "type",
        "which", "command", "builtin", "enable"
    };
    
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strlen(builtins[i]) == length && strncmp(token, builtins[i], length) == 0) {
            return true;
        }
    }
    
    return false;
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
    
    layer_events_error_t result = layer_events_publish_simple(
        layer->event_system,
        event_type,
        LAYER_ID_COMMAND_LAYER,
        0,
        LAYER_EVENT_PRIORITY_NORMAL
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
    
    strncpy(dest, src, copy_len);
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
    
    // Default color scheme (similar to existing lusush colors)
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
    safe_string_copy(color_scheme->error_color, "\033[1;41m", sizeof(color_scheme->error_color));         // Red background
    safe_string_copy(color_scheme->reset_color, "\033[0m", sizeof(color_scheme->reset_color));            // Reset
    
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