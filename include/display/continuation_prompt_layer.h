/*
 * Lusush Shell - Continuation Prompt Layer
 *
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 *
 * CONTINUATION PROMPT LAYER
 *
 * Generates context-aware continuation prompts for multiline command input.
 * This layer analyzes command context and provides appropriate prompts for
 * each continuation line (e.g., "loop> ", "if> ", "> ").
 *
 * Architecture:
 * - Integrates with input_continuation.c for context analysis
 * - Supports two modes: SIMPLE (fixed "> ") and CONTEXT_AWARE
 * - Performance optimized with caching (<10μs simple, <100μs context-aware)
 * - Independent layer in the display system
 *
 * Design Principles:
 * - Separation of concerns: Prompt generation separate from rendering
 * - Stateless operation: No persistent command state stored
 * - Performance-first: Caching and fast paths for common cases
 * - Extensible: Easy to add new context types and prompts
 */

#ifndef CONTINUATION_PROMPT_LAYER_H
#define CONTINUATION_PROMPT_LAYER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define CONTINUATION_PROMPT_MAX_LENGTH 64
#define CONTINUATION_PROMPT_CACHE_SIZE 8

// Performance targets
#define CONTINUATION_PROMPT_TARGET_SIMPLE_TIME_US 10
#define CONTINUATION_PROMPT_TARGET_CONTEXT_TIME_US 100

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * Error codes for continuation prompt operations
 */
typedef enum {
    CONTINUATION_PROMPT_SUCCESS = 0,
    CONTINUATION_PROMPT_ERROR_INVALID_PARAM,
    CONTINUATION_PROMPT_ERROR_NULL_POINTER,
    CONTINUATION_PROMPT_ERROR_BUFFER_TOO_SMALL,
    CONTINUATION_PROMPT_ERROR_NOT_INITIALIZED,
    CONTINUATION_PROMPT_ERROR_ALLOCATION_FAILED
} continuation_prompt_error_t;

/**
 * Continuation prompt modes
 */
typedef enum {
    CONTINUATION_PROMPT_MODE_SIMPLE = 0,    // Fixed "> " prompt
    CONTINUATION_PROMPT_MODE_CONTEXT_AWARE, // Context-aware prompts
    CONTINUATION_PROMPT_MODE_COUNT
} continuation_prompt_mode_t;

/**
 * Performance metrics
 */
typedef struct {
    uint64_t generation_count;       // Number of prompts generated
    uint64_t cache_hits;             // Cache hit count
    uint64_t cache_misses;           // Cache miss count
    uint64_t avg_generation_time_ns; // Average generation time
    uint64_t max_generation_time_ns; // Maximum generation time
    double cache_hit_rate;           // Cache hit rate (0.0-1.0)
} continuation_prompt_performance_t;

/**
 * Opaque continuation prompt layer structure
 */
typedef struct continuation_prompt_layer_t continuation_prompt_layer_t;

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Create a new continuation prompt layer instance
 *
 * Allocates and initializes a new continuation prompt layer with default
 * configuration (SIMPLE mode).
 *
 * @return Pointer to new layer instance, or NULL on allocation failure
 */
continuation_prompt_layer_t *continuation_prompt_layer_create(void);

/**
 * Initialize the continuation prompt layer
 *
 * Initializes the layer with default settings. Must be called before
 * using the layer.
 *
 * @param layer The layer to initialize
 * @return SUCCESS on success, error code on failure
 */
continuation_prompt_error_t
continuation_prompt_layer_init(continuation_prompt_layer_t *layer);

/**
 * Cleanup the continuation prompt layer
 *
 * Cleans up internal resources and resets the layer. The layer can be
 * reused after calling init() again.
 *
 * @param layer The layer to cleanup
 * @return SUCCESS on success, error code on failure
 */
continuation_prompt_error_t
continuation_prompt_layer_cleanup(continuation_prompt_layer_t *layer);

/**
 * Destroy a continuation prompt layer instance
 *
 * Performs cleanup and frees all memory. The layer pointer becomes invalid
 * after this call. NULL-safe.
 *
 * @param layer The layer to destroy (can be NULL)
 */
void continuation_prompt_layer_destroy(continuation_prompt_layer_t *layer);

// ============================================================================
// MODE CONTROL FUNCTIONS
// ============================================================================

/**
 * Set the continuation prompt mode
 *
 * Changes the prompt generation mode. Invalidates cache.
 *
 * @param layer The layer
 * @param mode The mode to set (SIMPLE or CONTEXT_AWARE)
 * @return SUCCESS on success, error code on failure
 */
continuation_prompt_error_t
continuation_prompt_layer_set_mode(continuation_prompt_layer_t *layer,
                                   continuation_prompt_mode_t mode);

/**
 * Get the current continuation prompt mode
 *
 * @param layer The layer
 * @return Current mode, or SIMPLE on error
 */
continuation_prompt_mode_t
continuation_prompt_layer_get_mode(const continuation_prompt_layer_t *layer);

// ============================================================================
// PROMPT GENERATION FUNCTIONS
// ============================================================================

/**
 * Get continuation prompt for a specific line
 *
 * Generates the appropriate continuation prompt for the given line number
 * based on command content and current mode.
 *
 * Line numbering:
 * - Line 0: Primary prompt (not a continuation, don't call this)
 * - Line 1+: Continuation lines (call this function)
 *
 * Simple mode:
 * - Always returns "> "
 *
 * Context-aware mode:
 * - Analyzes command content using input_continuation.c parser
 * - Returns context-appropriate prompt:
 *   - "if> " for if statements
 *   - "loop> " for for/while loops
 *   - "func> " for function definitions
 *   - "sh> " for subshells
 *   - "> " for quotes or unknown contexts
 *
 * Performance:
 * - Simple mode: <10 microseconds
 * - Context-aware mode: <100 microseconds (with caching)
 *
 * @param layer The layer
 * @param line_number Line number (1-based, 0 is primary prompt)
 * @param command_content Full command text (may contain \n)
 * @param out_prompt Output buffer for prompt text
 * @param out_prompt_size Size of output buffer
 * @return SUCCESS on success, error code on failure
 */
continuation_prompt_error_t continuation_prompt_layer_get_prompt_for_line(
    continuation_prompt_layer_t *layer, size_t line_number,
    const char *command_content, char *out_prompt, size_t out_prompt_size);

// ============================================================================
// PERFORMANCE AND MONITORING
// ============================================================================

/**
 * Get performance metrics
 *
 * Returns detailed performance metrics including cache statistics and
 * timing information.
 *
 * @param layer The layer
 * @param performance Output structure for metrics
 * @return SUCCESS on success, error code on failure
 */
continuation_prompt_error_t continuation_prompt_layer_get_performance(
    const continuation_prompt_layer_t *layer,
    continuation_prompt_performance_t *performance);

/**
 * Clear performance metrics
 *
 * Resets all performance counters to zero.
 *
 * @param layer The layer
 */
void continuation_prompt_layer_clear_performance(
    continuation_prompt_layer_t *layer);

/**
 * Clear cache
 *
 * Invalidates all cached prompt results. Use when command content changes
 * or mode changes (mode change automatically clears cache).
 *
 * @param layer The layer
 */
void continuation_prompt_layer_clear_cache(continuation_prompt_layer_t *layer);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Convert error code to string
 *
 * @param error The error code
 * @return Human-readable string (never NULL)
 */
const char *continuation_prompt_error_string(continuation_prompt_error_t error);

/**
 * Convert mode to string
 *
 * @param mode The mode
 * @return Human-readable string (never NULL)
 */
const char *continuation_prompt_mode_string(continuation_prompt_mode_t mode);

/**
 * Check if layer is initialized
 *
 * @param layer The layer to check
 * @return true if initialized and ready, false otherwise
 */
bool continuation_prompt_layer_is_initialized(
    const continuation_prompt_layer_t *layer);

#ifdef __cplusplus
}
#endif

#endif /* CONTINUATION_PROMPT_LAYER_H */
