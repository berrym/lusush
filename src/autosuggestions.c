/*
 * Lusush - A modern shell with advanced autosuggestions
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fish-inspired Autosuggestions System for Lusush
 * 
 * This module provides real-time autosuggestions similar to Fish shell:
 * - History-based suggestions (most common)
 * - Completion-based suggestions  
 * - Alias expansion suggestions
 * - Smart contextual filtering
 * - Performance-optimized with caching
 * - Seamless integration with existing readline system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "../include/autosuggestions.h"
#include "../include/readline_integration.h"
#include "../include/completion.h"
#include "../include/config.h"
#include "../include/posix_history.h"
#include "../include/alias.h"
#include "../include/builtins.h"

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

static bool initialized = false;
static lusush_autosuggestion_t current_suggestion = {0};
static autosuggestion_config_t autosugg_config = {
    .enabled = true,
    .history_enabled = true,
    .completion_enabled = true,
    .alias_enabled = true,
    .max_suggestion_length = 80,
    .min_input_length = 2,
    .suggestion_color = "\033[90m",  // Gray
    .show_source_info = false
};

// Performance optimization cache
typedef struct {
    char *last_input;
    lusush_autosuggestion_t cached_suggestion;
    time_t cache_time;
    bool cache_valid;
} suggestion_cache_t;

static suggestion_cache_t cache = {0};

// Statistics for performance monitoring
typedef struct {
    int suggestions_generated;
    int suggestions_accepted;
    int cache_hits;
    int cache_misses;
    double avg_generation_time_ms;
} autosuggestion_internal_stats_t;

static autosuggestion_internal_stats_t stats = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get current time in milliseconds for performance measurement
 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/**
 * Clear suggestion data
 */
static void clear_suggestion(lusush_autosuggestion_t *suggestion) {
    if (!suggestion) return;
    
    free(suggestion->suggestion);
    free(suggestion->display_text);
    memset(suggestion, 0, sizeof(lusush_autosuggestion_t));
}

/**
 * Copy suggestion data
 */
static void copy_suggestion(lusush_autosuggestion_t *dest, const lusush_autosuggestion_t *src) {
    if (!dest || !src) return;
    
    clear_suggestion(dest);
    
    if (src->suggestion) {
        dest->suggestion = strdup(src->suggestion);
    }
    if (src->display_text) {
        dest->display_text = strdup(src->display_text);
    }
    
    dest->source_type = src->source_type;
    dest->confidence_score = src->confidence_score;
    dest->suggestion_start = src->suggestion_start;
    dest->is_valid = src->is_valid;
}

/**
 * Check if input is suitable for suggestions
 */
static bool should_suggest(const char *input, size_t cursor_pos) {
    if (!autosugg_config.enabled || !input) {
        return false;
    }
    
    size_t len = strlen(input);
    
    // Must meet minimum length requirement
    if (len < autosugg_config.min_input_length) {
        return false;
    }
    
    // Don't suggest if cursor is not at end (user is editing middle)
    if (cursor_pos != len) {
        return false;
    }
    
    // Don't suggest if input ends with space (command is complete)
    if (len > 0 && input[len - 1] == ' ') {
        return false;
    }
    
    // Don't suggest for very long inputs (performance)
    if (len > 200) {
        return false;
    }
    
    return true;
}

/**
 * Score similarity between two strings (0-100)
 */
static int calculate_similarity_score(const char *input, const char *candidate) {
    if (!input || !candidate) {
        return 0;
    }
    
    size_t input_len = strlen(input);
    size_t candidate_len = strlen(candidate);
    
    // Must be a prefix match for autosuggestions
    if (strncmp(input, candidate, input_len) != 0) {
        return 0;
    }
    
    // Perfect prefix match
    if (input_len == candidate_len) {
        return 100;
    }
    
    // Score based on how much additional content we're suggesting
    size_t suggestion_len = candidate_len - input_len;
    
    // Prefer shorter completions
    if (suggestion_len <= 10) return 90;
    if (suggestion_len <= 20) return 80;
    if (suggestion_len <= 40) return 70;
    
    return 60;
}

// ============================================================================
// SUGGESTION GENERATORS
// ============================================================================

/**
 * Generate suggestion from command history
 */
static lusush_autosuggestion_t* generate_history_suggestion(const char *input) {
    if (!config.history_enabled || !input) {
        return NULL;
    }
    
    // Use existing history system from readline integration
    int history_len = lusush_history_length();
    if (history_len == 0) {
        return NULL;
    }
    
    const char *best_match = NULL;
    int best_score = 0;
    
    // Search through history for best match
    // Search backwards (most recent first)
    for (int i = history_len - 1; i >= 0; i--) {
        const char *hist_entry = lusush_history_get(i);
        if (!hist_entry) continue;
        
        int score = calculate_similarity_score(input, hist_entry);
        if (score > best_score) {
            best_score = score;
            best_match = hist_entry;
        }
        
        // Stop if we found a perfect match
        if (score >= 90) break;
    }
    
    if (!best_match || best_score < 60) {
        return NULL;
    }
    
    lusush_autosuggestion_t *suggestion = calloc(1, sizeof(lusush_autosuggestion_t));
    if (!suggestion) return NULL;
    
    suggestion->suggestion = strdup(best_match);
    suggestion->source_type = SUGGESTION_HISTORY;
    suggestion->confidence_score = best_score;
    suggestion->suggestion_start = strlen(input);
    suggestion->is_valid = true;
    
    // Create display text (may be truncated)
    size_t remaining_len = strlen(best_match) - strlen(input);
    if (remaining_len <= autosugg_config.max_suggestion_length) {
        suggestion->display_text = strdup(best_match + strlen(input));
    } else {
        char *truncated = malloc(autosugg_config.max_suggestion_length + 4);
        if (truncated) {
            strncpy(truncated, best_match + strlen(input), autosugg_config.max_suggestion_length - 3);
            strcpy(truncated + autosugg_config.max_suggestion_length - 3, "...");
            suggestion->display_text = truncated;
        }
    }
    
    return suggestion;
}

/**
 * Generate suggestion from completion system
 */
static lusush_autosuggestion_t* generate_completion_suggestion(const char *input) {
    if (!config.completion_enabled || !input) {
        return NULL;
    }
    
    // Use existing completion system
    lusush_completions_t completions = {0};
    lusush_completion_callback(input, &completions);
    
    if (completions.len == 0) {
        return NULL;
    }
    
    // Find best completion match
    const char *best_match = NULL;
    int best_score = 0;
    
    for (size_t i = 0; i < completions.len; i++) {
        if (!completions.cvec[i]) continue;
        
        int score = calculate_similarity_score(input, completions.cvec[i]);
        if (score > best_score) {
            best_score = score;
            best_match = completions.cvec[i];
        }
    }
    
    lusush_autosuggestion_t *suggestion = NULL;
    
    if (best_match && best_score >= 70) {
        suggestion = calloc(1, sizeof(lusush_autosuggestion_t));
        if (suggestion) {
            suggestion->suggestion = strdup(best_match);
            suggestion->source_type = SUGGESTION_COMPLETION;
            suggestion->confidence_score = best_score;
            suggestion->suggestion_start = strlen(input);
            suggestion->is_valid = true;
            
            // Create display text
            size_t remaining_len = strlen(best_match) - strlen(input);
            if (remaining_len <= autosugg_config.max_suggestion_length) {
                suggestion->display_text = strdup(best_match + strlen(input));
            } else {
                char *truncated = malloc(autosugg_config.max_suggestion_length + 4);
                if (truncated) {
                    strncpy(truncated, best_match + strlen(input), autosugg_config.max_suggestion_length - 3);
                    strcpy(truncated + autosugg_config.max_suggestion_length - 3, "...");
                    suggestion->display_text = truncated;
                }
            }
        }
    }
    
    // Clean up completions
    lusush_free_completions(&completions);
    
    return suggestion;
}

/**
 * Generate suggestion from alias expansion
 */
static lusush_autosuggestion_t* generate_alias_suggestion(const char *input) {
    if (!autosugg_config.alias_enabled || !input) {
        return NULL;
    }
    
    // Check if input could be start of an alias
    char *space_pos = strchr(input, ' ');
    if (space_pos) {
        // Input already contains arguments, not suitable for alias expansion
        return NULL;
    }
    
    // Try to find matching alias (this would need to integrate with alias system)
    // For now, return NULL - would need to implement alias lookup
    
    return NULL;
}

// ============================================================================
// MAIN SUGGESTION ENGINE
// ============================================================================

/**
 * Generate the best suggestion for current input
 */
lusush_autosuggestion_t* lusush_get_suggestion(const char *current_line, size_t cursor_pos) {
    if (!initialized || !should_suggest(current_line, cursor_pos)) {
        return NULL;
    }
    
    double start_time = get_time_ms();
    
    // Check cache first
    if (cache.cache_valid && cache.last_input && 
        strcmp(current_line, cache.last_input) == 0) {
        stats.cache_hits++;
        
        lusush_autosuggestion_t *suggestion = calloc(1, sizeof(lusush_autosuggestion_t));
        if (suggestion) {
            copy_suggestion(suggestion, &cache.cached_suggestion);
        }
        return suggestion;
    }
    
    stats.cache_misses++;
    
    // Try different suggestion sources in priority order
    lusush_autosuggestion_t *suggestions[] = {
        generate_history_suggestion(current_line),
        generate_completion_suggestion(current_line),
        generate_alias_suggestion(current_line)
    };
    
    // Find best suggestion
    lusush_autosuggestion_t *best = NULL;
    int best_score = 0;
    
    for (size_t i = 0; i < sizeof(suggestions) / sizeof(suggestions[0]); i++) {
        if (suggestions[i] && suggestions[i]->confidence_score > best_score) {
            if (best) {
                clear_suggestion(best);
                free(best);
            }
            best = suggestions[i];
            best_score = suggestions[i]->confidence_score;
        } else if (suggestions[i]) {
            clear_suggestion(suggestions[i]);
            free(suggestions[i]);
        }
    }
    
    // Update cache
    free(cache.last_input);
    cache.last_input = strdup(current_line);
    clear_suggestion(&cache.cached_suggestion);
    
    if (best) {
        copy_suggestion(&cache.cached_suggestion, best);
        cache.cache_valid = true;
        cache.cache_time = time(NULL);
    } else {
        cache.cache_valid = false;
    }
    
    // Update statistics
    stats.suggestions_generated++;
    double elapsed = get_time_ms() - start_time;
    stats.avg_generation_time_ms = (stats.avg_generation_time_ms * (stats.suggestions_generated - 1) + elapsed) / stats.suggestions_generated;
    
    return best;
}

/**
 * Accept the current suggestion (user pressed right arrow or end)
 */
void lusush_accept_suggestion(lusush_autosuggestion_t *suggestion) {
    if (!suggestion || !suggestion->is_valid) {
        return;
    }
    
    // This would integrate with readline to actually insert the text
    // Implementation depends on readline integration details
    
    stats.suggestions_accepted++;
    
    // Invalidate cache since line will change
    cache.cache_valid = false;
}

/**
 * Dismiss any current suggestion
 */
void lusush_dismiss_suggestion(void) {
    clear_suggestion(&current_suggestion);
    cache.cache_valid = false;
}

/**
 * Free autosuggestion structure
 */
void lusush_free_autosuggestion(lusush_autosuggestion_t *suggestion) {
    if (!suggestion) {
        return;
    }
    
    clear_suggestion(suggestion);
    free(suggestion);
}

// ============================================================================
// CONFIGURATION AND LIFECYCLE
// ============================================================================

/**
 * Initialize autosuggestion system
 */
bool lusush_autosuggestions_init(void) {
    if (initialized) {
        return true;
    }
    
    // Initialize cache
    memset(&cache, 0, sizeof(cache));
    memset(&stats, 0, sizeof(stats));
    
    // Set default colors if not configured
    if (!autosugg_config.suggestion_color) {
        autosugg_config.suggestion_color = strdup("\033[90m");  // Gray
    }
    
    initialized = true;
    return true;
}

/**
 * Cleanup autosuggestion system
 */
void lusush_autosuggestions_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    clear_suggestion(&current_suggestion);
    clear_suggestion(&cache.cached_suggestion);
    
    free(cache.last_input);
    free(autosugg_config.suggestion_color);
    
    memset(&cache, 0, sizeof(cache));
    memset(&autosugg_config, 0, sizeof(autosugg_config));
    memset(&stats, 0, sizeof(stats));
    
    initialized = false;
}

/**
 * Configure autosuggestion system
 */
void lusush_configure_autosuggestions(const autosuggestion_config_t *new_config) {
    if (!new_config) return;
    
    // Update configuration
    autosugg_config.enabled = new_config->enabled;
    autosugg_config.history_enabled = new_config->history_enabled;
    autosugg_config.completion_enabled = new_config->completion_enabled;
    autosugg_config.alias_enabled = new_config->alias_enabled;
    autosugg_config.max_suggestion_length = new_config->max_suggestion_length;
    autosugg_config.min_input_length = new_config->min_input_length;
    
    if (new_config->suggestion_color) {
        free(autosugg_config.suggestion_color);
        autosugg_config.suggestion_color = strdup(new_config->suggestion_color);
    }
    
    autosugg_config.show_source_info = new_config->show_source_info;
    // Invalidate cache when config changes
    cache.cache_valid = false;
}

// ============================================================================
// DISPLAY INTEGRATION
// ============================================================================

/**
 * Update the display with current autosuggestion
 * This would integrate with the readline display system
 */
void lusush_autosuggestion_update_display(void) {
    // This function would be called by the readline integration
    // to show suggestions in gray text after the cursor
    
    // Implementation would depend on terminal control integration
    // For now, this is a placeholder for the display logic
}

/**
 * Handle keypress events for autosuggestion interaction
 */
bool lusush_autosuggestion_handle_keypress(int key) {
    // Handle keys that interact with suggestions:
    // - Right arrow: accept suggestion
    // - Ctrl+F: accept suggestion  
    // - End: accept suggestion
    // - Any other key: dismiss suggestion if it invalidates it
    
    switch (key) {
        case '\033': // Escape sequence start - might be arrow key
            // Would need more sophisticated key parsing
            break;
            
        case '\004': // Ctrl+F
        case '\005': // Ctrl+E (End)
            if (current_suggestion.is_valid) {
                lusush_accept_suggestion(&current_suggestion);
                return true;
            }
            break;
            
        default:
            // Most keys invalidate current suggestion
            lusush_dismiss_suggestion();
            break;
    }
    return false;
}

// ============================================================================
// DEBUG AND STATISTICS
// ============================================================================

/**
 * Print autosuggestion statistics (for debugging)
 */
void lusush_autosuggestion_print_stats(void) {
    printf("Autosuggestion Statistics:\n");
    printf("  Generated: %d\n", stats.suggestions_generated);
    printf("  Accepted: %d\n", stats.suggestions_accepted);
    printf("  Cache hits: %d\n", stats.cache_hits);
    printf("  Cache misses: %d\n", stats.cache_misses);
    printf("  Avg generation time: %.2f ms\n", stats.avg_generation_time_ms);
    
    if (stats.suggestions_generated > 0) {
        printf("  Accept rate: %.1f%%\n", 
               (100.0 * stats.suggestions_accepted) / stats.suggestions_generated);
    }
    
    if (stats.cache_hits + stats.cache_misses > 0) {
        printf("  Cache hit rate: %.1f%%\n",
               (100.0 * stats.cache_hits) / (stats.cache_hits + stats.cache_misses));
    }
}