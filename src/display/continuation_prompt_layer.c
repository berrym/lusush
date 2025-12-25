/*
 * Lusush Shell - Continuation Prompt Layer Implementation
 *
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This file implements the continuation prompt layer which generates
 * context-aware continuation prompts for multiline command input.
 */

#include "display/continuation_prompt_layer.h"
#include "input_continuation.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

/**
 * Cache entry for prompt results
 */
typedef struct {
    char command_hash[32]; // Simple hash of command
    size_t line_number;    // Line number
    char cached_prompt[CONTINUATION_PROMPT_MAX_LENGTH]; // Cached result
    bool valid;                                         // Entry is valid
} prompt_cache_entry_t;

/**
 * Continuation prompt layer implementation
 */
struct continuation_prompt_layer_t {
    // Configuration
    continuation_prompt_mode_t mode; // Current mode
    bool initialized;                // Layer is initialized

    // Caching
    prompt_cache_entry_t cache[CONTINUATION_PROMPT_CACHE_SIZE];
    size_t cache_next_slot; // Next slot for replacement

    // Performance metrics
    uint64_t generation_count;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t total_time_ns;
    uint64_t max_time_ns;
    uint64_t min_time_ns;
};

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * Simple hash function for command content
 */
static void calculate_command_hash(const char *command, char *hash_out,
                                   size_t hash_size) {
    if (!command || !hash_out || hash_size < 32) {
        if (hash_out && hash_size > 0)
            hash_out[0] = '\0';
        return;
    }

    // Simple FNV-1a hash (good enough for cache keys)
    uint64_t hash = 14695981039346656037ULL;
    while (*command) {
        hash ^= (uint64_t)(unsigned char)*command;
        hash *= 1099511628211ULL;
        command++;
    }

    snprintf(hash_out, hash_size, "%016" PRIx64, hash);
}

/**
 * Look up prompt in cache
 */
static bool cache_lookup(continuation_prompt_layer_t *layer,
                         const char *command_hash, size_t line_number,
                         char *out_prompt, size_t out_size) {
    if (!layer || !command_hash || !out_prompt)
        return false;

    for (size_t i = 0; i < CONTINUATION_PROMPT_CACHE_SIZE; i++) {
        prompt_cache_entry_t *entry = &layer->cache[i];

        if (entry->valid && entry->line_number == line_number &&
            strcmp(entry->command_hash, command_hash) == 0) {

            snprintf(out_prompt, out_size, "%s", entry->cached_prompt);
            return true;
        }
    }

    return false;
}

/**
 * Store prompt in cache
 */
static void cache_store(continuation_prompt_layer_t *layer,
                        const char *command_hash, size_t line_number,
                        const char *prompt) {
    if (!layer || !command_hash || !prompt)
        return;

    // Use simple round-robin replacement
    size_t slot = layer->cache_next_slot;
    prompt_cache_entry_t *entry = &layer->cache[slot];

    snprintf(entry->command_hash, sizeof(entry->command_hash), "%s",
             command_hash);
    entry->line_number = line_number;
    snprintf(entry->cached_prompt, sizeof(entry->cached_prompt), "%s", prompt);
    entry->valid = true;

    layer->cache_next_slot = (slot + 1) % CONTINUATION_PROMPT_CACHE_SIZE;
}

/**
 * Get current time in nanoseconds
 */
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

continuation_prompt_layer_t *continuation_prompt_layer_create(void) {
    continuation_prompt_layer_t *layer = (continuation_prompt_layer_t *)calloc(
        1, sizeof(continuation_prompt_layer_t));

    if (!layer) {
        return NULL;
    }

    // Initialize to safe defaults
    layer->mode = CONTINUATION_PROMPT_MODE_SIMPLE;
    layer->initialized = false;
    layer->cache_next_slot = 0;
    layer->min_time_ns = UINT64_MAX;

    return layer;
}

continuation_prompt_error_t
continuation_prompt_layer_init(continuation_prompt_layer_t *layer) {
    if (!layer) {
        return CONTINUATION_PROMPT_ERROR_NULL_POINTER;
    }

    // Clear cache
    memset(layer->cache, 0, sizeof(layer->cache));
    layer->cache_next_slot = 0;

    // Reset metrics
    layer->generation_count = 0;
    layer->cache_hits = 0;
    layer->cache_misses = 0;
    layer->total_time_ns = 0;
    layer->max_time_ns = 0;
    layer->min_time_ns = UINT64_MAX;

    layer->initialized = true;

    return CONTINUATION_PROMPT_SUCCESS;
}

continuation_prompt_error_t
continuation_prompt_layer_cleanup(continuation_prompt_layer_t *layer) {
    if (!layer) {
        return CONTINUATION_PROMPT_ERROR_NULL_POINTER;
    }

    // Clear cache
    memset(layer->cache, 0, sizeof(layer->cache));
    layer->cache_next_slot = 0;

    layer->initialized = false;

    return CONTINUATION_PROMPT_SUCCESS;
}

void continuation_prompt_layer_destroy(continuation_prompt_layer_t *layer) {
    if (!layer)
        return;

    continuation_prompt_layer_cleanup(layer);
    free(layer);
}

// ============================================================================
// MODE CONTROL FUNCTIONS
// ============================================================================

continuation_prompt_error_t
continuation_prompt_layer_set_mode(continuation_prompt_layer_t *layer,
                                   continuation_prompt_mode_t mode) {
    if (!layer) {
        return CONTINUATION_PROMPT_ERROR_NULL_POINTER;
    }

    if (mode >= CONTINUATION_PROMPT_MODE_COUNT) {
        return CONTINUATION_PROMPT_ERROR_INVALID_PARAM;
    }

    if (layer->mode != mode) {
        layer->mode = mode;

        // Invalidate cache on mode change
        continuation_prompt_layer_clear_cache(layer);
    }

    return CONTINUATION_PROMPT_SUCCESS;
}

continuation_prompt_mode_t
continuation_prompt_layer_get_mode(const continuation_prompt_layer_t *layer) {
    if (!layer) {
        return CONTINUATION_PROMPT_MODE_SIMPLE;
    }

    return layer->mode;
}

// ============================================================================
// PROMPT GENERATION FUNCTIONS
// ============================================================================

continuation_prompt_error_t continuation_prompt_layer_get_prompt_for_line(
    continuation_prompt_layer_t *layer, size_t line_number,
    const char *command_content, char *out_prompt, size_t out_prompt_size) {
    if (!layer) {
        return CONTINUATION_PROMPT_ERROR_NULL_POINTER;
    }

    if (!layer->initialized) {
        return CONTINUATION_PROMPT_ERROR_NOT_INITIALIZED;
    }

    if (!command_content || !out_prompt) {
        return CONTINUATION_PROMPT_ERROR_NULL_POINTER;
    }

    if (out_prompt_size < 4) { // Minimum: "> \0"
        return CONTINUATION_PROMPT_ERROR_BUFFER_TOO_SMALL;
    }

    uint64_t start_time = get_time_ns();

    // SIMPLE MODE: Fast path - always return "> "
    if (layer->mode == CONTINUATION_PROMPT_MODE_SIMPLE) {
        snprintf(out_prompt, out_prompt_size, "> ");

        uint64_t elapsed = get_time_ns() - start_time;
        layer->generation_count++;
        layer->total_time_ns += elapsed;
        if (elapsed > layer->max_time_ns)
            layer->max_time_ns = elapsed;
        if (elapsed < layer->min_time_ns)
            layer->min_time_ns = elapsed;

        return CONTINUATION_PROMPT_SUCCESS;
    }

    // CONTEXT-AWARE MODE: Check cache first
    char command_hash[32];
    calculate_command_hash(command_content, command_hash, sizeof(command_hash));

    if (cache_lookup(layer, command_hash, line_number, out_prompt,
                     out_prompt_size)) {
        layer->cache_hits++;
        layer->generation_count++;

        uint64_t elapsed = get_time_ns() - start_time;
        layer->total_time_ns += elapsed;
        if (elapsed > layer->max_time_ns)
            layer->max_time_ns = elapsed;
        if (elapsed < layer->min_time_ns)
            layer->min_time_ns = elapsed;

        return CONTINUATION_PROMPT_SUCCESS;
    }

    layer->cache_misses++;

    // Cache miss - analyze command using input_continuation.c
    continuation_state_t state;
    continuation_state_init(&state);

    // Analyze all lines up to the requested line
    const char *p = command_content;
    size_t current_line = 0;
    const char *line_start = p;

    while (*p && current_line < line_number) {
        if (*p == '\n') {
            // Process this line
            size_t line_len = p - line_start;
            char *line_buf = (char *)malloc(line_len + 1);
            if (line_buf) {
                memcpy(line_buf, line_start, line_len);
                line_buf[line_len] = '\0';
                continuation_analyze_line(line_buf, &state);
                free(line_buf);
            }

            current_line++;
            p++;
            line_start = p;
        } else {
            p++;
        }
    }

    // Get prompt for current state
    const char *prompt = continuation_get_prompt(&state);
    if (!prompt) {
        prompt = "> "; // Fallback
    }

    snprintf(out_prompt, out_prompt_size, "%s", prompt);

    // Store in cache
    cache_store(layer, command_hash, line_number, out_prompt);

    continuation_state_cleanup(&state);

    // Update metrics
    uint64_t elapsed = get_time_ns() - start_time;
    layer->generation_count++;
    layer->total_time_ns += elapsed;
    if (elapsed > layer->max_time_ns)
        layer->max_time_ns = elapsed;
    if (elapsed < layer->min_time_ns)
        layer->min_time_ns = elapsed;

    return CONTINUATION_PROMPT_SUCCESS;
}

// ============================================================================
// PERFORMANCE AND MONITORING
// ============================================================================

continuation_prompt_error_t continuation_prompt_layer_get_performance(
    const continuation_prompt_layer_t *layer,
    continuation_prompt_performance_t *performance) {
    if (!layer || !performance) {
        return CONTINUATION_PROMPT_ERROR_NULL_POINTER;
    }

    performance->generation_count = layer->generation_count;
    performance->cache_hits = layer->cache_hits;
    performance->cache_misses = layer->cache_misses;
    performance->max_generation_time_ns = layer->max_time_ns;

    if (layer->generation_count > 0) {
        performance->avg_generation_time_ns =
            layer->total_time_ns / layer->generation_count;

        uint64_t total_lookups = layer->cache_hits + layer->cache_misses;
        if (total_lookups > 0) {
            performance->cache_hit_rate =
                (double)layer->cache_hits / (double)total_lookups;
        } else {
            performance->cache_hit_rate = 0.0;
        }
    } else {
        performance->avg_generation_time_ns = 0;
        performance->cache_hit_rate = 0.0;
    }

    return CONTINUATION_PROMPT_SUCCESS;
}

void continuation_prompt_layer_clear_performance(
    continuation_prompt_layer_t *layer) {
    if (!layer)
        return;

    layer->generation_count = 0;
    layer->cache_hits = 0;
    layer->cache_misses = 0;
    layer->total_time_ns = 0;
    layer->max_time_ns = 0;
    layer->min_time_ns = UINT64_MAX;
}

void continuation_prompt_layer_clear_cache(continuation_prompt_layer_t *layer) {
    if (!layer)
        return;

    memset(layer->cache, 0, sizeof(layer->cache));
    layer->cache_next_slot = 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char *
continuation_prompt_error_string(continuation_prompt_error_t error) {
    switch (error) {
    case CONTINUATION_PROMPT_SUCCESS:
        return "Success";
    case CONTINUATION_PROMPT_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case CONTINUATION_PROMPT_ERROR_NULL_POINTER:
        return "NULL pointer";
    case CONTINUATION_PROMPT_ERROR_BUFFER_TOO_SMALL:
        return "Buffer too small";
    case CONTINUATION_PROMPT_ERROR_NOT_INITIALIZED:
        return "Not initialized";
    case CONTINUATION_PROMPT_ERROR_ALLOCATION_FAILED:
        return "Allocation failed";
    default:
        return "Unknown error";
    }
}

const char *continuation_prompt_mode_string(continuation_prompt_mode_t mode) {
    switch (mode) {
    case CONTINUATION_PROMPT_MODE_SIMPLE:
        return "Simple";
    case CONTINUATION_PROMPT_MODE_CONTEXT_AWARE:
        return "Context-aware";
    default:
        return "Unknown";
    }
}

bool continuation_prompt_layer_is_initialized(
    const continuation_prompt_layer_t *layer) {
    return layer && layer->initialized;
}
