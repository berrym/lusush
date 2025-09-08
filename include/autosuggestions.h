/*
 * Lusush - A modern shell with advanced autosuggestions
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Autosuggestions System Header - Fish-inspired autosuggestions
 * 
 * This module provides real-time autosuggestions similar to Fish shell:
 * - History-based suggestions (most common)
 * - Completion-based suggestions  
 * - Alias expansion suggestions
 * - Smart contextual filtering
 * - Performance-optimized with caching
 * - Seamless integration with existing readline system
 */

#ifndef AUTOSUGGESTIONS_H
#define AUTOSUGGESTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPES AND STRUCTURES
// ============================================================================

/**
 * Sources of autosuggestions, in priority order
 */
typedef enum {
    SUGGESTION_HISTORY = 1,       // From command history (highest priority)
    SUGGESTION_COMPLETION = 2,    // From completion system
    SUGGESTION_ALIAS = 3,         // From alias expansion
    SUGGESTION_BUILTIN = 4,       // From builtin commands
    SUGGESTION_PATH_COMMAND = 5   // From PATH commands
} suggestion_source_t;

/**
 * Core autosuggestion data structure
 */
typedef struct {
    char *suggestion;           // The complete suggested command
    char *display_text;        // How to display it (may be truncated)
    suggestion_source_t source_type;  // Where suggestion came from
    int confidence_score;      // 0-100, how confident we are
    size_t suggestion_start;   // Position in buffer where suggestion starts
    bool is_valid;            // If suggestion is still valid for current input
    time_t created_time;      // When suggestion was generated
} lusush_autosuggestion_t;

/**
 * Configuration for autosuggestion system
 */
typedef struct {
    bool enabled;                    // Master enable/disable
    bool history_enabled;           // Enable history-based suggestions
    bool completion_enabled;        // Enable completion-based suggestions
    bool alias_enabled;            // Enable alias expansion suggestions
    int max_suggestion_length;     // Maximum length of displayed suggestion
    int min_input_length;          // Minimum input before suggesting
    int cache_timeout_seconds;     // How long to cache suggestions
    char *suggestion_color;        // ANSI color for displaying suggestions
    char *accepted_color;          // Color when accepting suggestions
    bool show_source_info;         // Show [history], [completion], etc.
    bool smart_case_matching;      // Case-insensitive matching when input is lowercase
    int history_search_limit;      // Max history entries to search
} autosuggestion_config_t;

/**
 * Statistics structure for monitoring performance
 */
typedef struct {
    int suggestions_generated;     // Total suggestions generated
    int suggestions_accepted;      // How many were accepted by user
    int suggestions_dismissed;     // How many were dismissed
    int cache_hits;               // Cache hit count
    int cache_misses;             // Cache miss count
    double avg_generation_time_ms; // Average time to generate suggestion
    int history_suggestions;       // Count by source type
    int completion_suggestions;
    int alias_suggestions;
} lusush_autosuggestion_stats_t;

// ============================================================================
// CORE API FUNCTIONS
// ============================================================================

/**
 * Initialize the autosuggestion system
 * Must be called before any other autosuggestion functions
 * 
 * @return true on success, false on failure
 */
bool lusush_autosuggestions_init(void);

/**
 * Cleanup the autosuggestion system and free all resources
 */
void lusush_autosuggestions_cleanup(void);

/**
 * Get autosuggestion for current input
 * This is the main function called during interactive editing
 * 
 * @param current_line The current command line input
 * @param cursor_pos Current cursor position in the line
 * @return Autosuggestion structure, or NULL if no suggestion
 *         Caller is responsible for freeing with lusush_free_autosuggestion()
 */
lusush_autosuggestion_t* lusush_get_suggestion(const char *current_line, size_t cursor_pos);

/**
 * Accept the current suggestion
 * Called when user presses right arrow, end key, or ctrl+f
 * 
 * @param suggestion The suggestion to accept
 */
void lusush_accept_suggestion(lusush_autosuggestion_t *suggestion);

/**
 * Accept only part of the current suggestion (word-by-word acceptance)
 * Called when user presses ctrl+right arrow or similar
 * 
 * @param suggestion The suggestion to partially accept
 * @param accept_length How many characters to accept
 * @return Updated suggestion with remaining text, or NULL if fully accepted
 */
lusush_autosuggestion_t* lusush_accept_partial_suggestion(lusush_autosuggestion_t *suggestion, size_t accept_length);

/**
 * Dismiss current suggestion
 * Called when user continues typing in a way that invalidates the suggestion
 */
void lusush_dismiss_suggestion(void);

/**
 * Dismiss any current suggestion and clear display artifacts
 * Enhanced version that handles both state and visual clearing
 */
void lusush_dismiss_suggestion_with_display(void);

/**
 * Free autosuggestion structure
 * 
 * @param suggestion The suggestion to free
 */
void lusush_free_autosuggestion(lusush_autosuggestion_t *suggestion);

// ============================================================================
// CONFIGURATION AND MANAGEMENT
// ============================================================================

/**
 * Configure the autosuggestion system
 * 
 * @param config New configuration settings
 */
void lusush_configure_autosuggestions(const autosuggestion_config_t *config);

/**
 * Get current autosuggestion configuration
 * 
 * @return Pointer to current configuration (read-only)
 */
const autosuggestion_config_t* lusush_get_autosuggestion_config(void);

/**
 * Get default autosuggestion configuration
 * 
 * @return Default configuration structure
 */
autosuggestion_config_t lusush_get_default_autosuggestion_config(void);

/**
 * Enable or disable autosuggestions globally
 * 
 * @param enabled true to enable, false to disable
 */
void lusush_set_autosuggestions_enabled(bool enabled);

/**
 * Check if autosuggestions are currently enabled
 * 
 * @return true if enabled, false otherwise
 */
bool lusush_are_autosuggestions_enabled(void);

// ============================================================================
// DISPLAY INTEGRATION
// ============================================================================

/**
 * Update the terminal display with current autosuggestion
 * This integrates with the readline display system to show suggestions
 * in gray text after the cursor position
 */
void lusush_autosuggestion_update_display(void);

/**
 * Clear autosuggestion from display
 * Called when suggestion is dismissed or accepted
 */
void lusush_autosuggestion_clear_display(void);

/**
 * Handle keypress events for autosuggestion interaction
 * Called by the main input system to handle suggestion-related keys
 * 
 * @param key The key that was pressed
 * @return true if key was handled by autosuggestion system, false otherwise
 */
bool lusush_autosuggestion_handle_keypress(int key);

/**
 * Get the display string for current autosuggestion
 * Returns the gray text that should be displayed after cursor
 * 
 * @param suggestion Current suggestion
 * @return Formatted display string (caller must free), or NULL
 */
char* lusush_get_autosuggestion_display_string(const lusush_autosuggestion_t *suggestion);

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

/**
 * Force regeneration of suggestions (clears cache)
 * Useful when context has changed significantly
 */
void lusush_autosuggestion_invalidate_cache(void);

/**
 * Add a custom suggestion source
 * Allows plugins or extensions to contribute suggestions
 * 
 * @param generator Function that generates suggestions
 * @param priority Priority level (higher = checked first)
 * @return true on success, false on failure
 */
typedef lusush_autosuggestion_t* (*suggestion_generator_t)(const char *input, size_t cursor_pos);
bool lusush_register_suggestion_generator(suggestion_generator_t generator, int priority);

/**
 * Remove a custom suggestion source
 * 
 * @param generator The generator function to remove
 * @return true on success, false if not found
 */
bool lusush_unregister_suggestion_generator(suggestion_generator_t generator);

/**
 * Preload suggestions for better performance
 * Generates and caches suggestions for common patterns
 * 
 * @param input_patterns Array of input patterns to preload
 * @param pattern_count Number of patterns
 */
void lusush_preload_suggestions(const char **input_patterns, size_t pattern_count);

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

/**
 * Get autosuggestion statistics
 * 
 * @return Pointer to current statistics (read-only)
 */
const lusush_autosuggestion_stats_t* lusush_get_autosuggestion_stats(void);

/**
 * Reset autosuggestion statistics
 */
void lusush_reset_autosuggestion_stats(void);

/**
 * Print detailed autosuggestion statistics to stdout
 * Useful for debugging and performance analysis
 */
void lusush_print_autosuggestion_stats(void);

/**
 * Enable or disable debug output for autosuggestions
 * 
 * @param enabled true to enable debug output
 */
void lusush_set_autosuggestion_debug(bool enabled);

/**
 * Check if debug output is enabled
 * 
 * @return true if debug is enabled
 */
bool lusush_is_autosuggestion_debug_enabled(void);

/**
 * Get detailed information about last suggestion generation
 * Useful for debugging why suggestions appear or don't appear
 * 
 * @return Debug information string (caller must free), or NULL
 */
char* lusush_get_autosuggestion_debug_info(void);

// ============================================================================
// INTEGRATION HOOKS
// ============================================================================

/**
 * Hook called when a suggestion is generated
 * Allows monitoring and logging of suggestion activity
 */
typedef void (*suggestion_generated_hook_t)(const lusush_autosuggestion_t *suggestion);

/**
 * Hook called when a suggestion is accepted
 * Allows learning from user behavior
 */
typedef void (*suggestion_accepted_hook_t)(const lusush_autosuggestion_t *suggestion);

/**
 * Hook called when a suggestion is dismissed
 * Allows tracking of rejected suggestions
 */
typedef void (*suggestion_dismissed_hook_t)(const lusush_autosuggestion_t *suggestion);

/**
 * Set hook for suggestion generation events
 * 
 * @param hook Function to call when suggestion is generated
 */
void lusush_set_suggestion_generated_hook(suggestion_generated_hook_t hook);

/**
 * Set hook for suggestion acceptance events
 * 
 * @param hook Function to call when suggestion is accepted
 */
void lusush_set_suggestion_accepted_hook(suggestion_accepted_hook_t hook);

/**
 * Set hook for suggestion dismissal events
 * 
 * @param hook Function to call when suggestion is dismissed
 */
void lusush_set_suggestion_dismissed_hook(suggestion_dismissed_hook_t hook);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Check if a string is a valid suggestion for given input
 * 
 * @param input The current input
 * @param candidate Potential suggestion
 * @return true if candidate is a valid suggestion for input
 */
bool lusush_is_valid_suggestion(const char *input, const char *candidate);

/**
 * Calculate confidence score for a suggestion
 * 
 * @param input The current input
 * @param candidate The candidate suggestion
 * @param source_type Where the suggestion came from
 * @return Confidence score (0-100)
 */
int lusush_calculate_suggestion_confidence(const char *input, const char *candidate, suggestion_source_t source_type);

/**
 * Get human-readable name for suggestion source
 * 
 * @param source_type The source type
 * @return String name of the source (do not free)
 */
const char* lusush_get_suggestion_source_name(suggestion_source_t source_type);

#ifdef __cplusplus
}
#endif

#endif /* AUTOSUGGESTIONS_H */