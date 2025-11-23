/*
 * Lusush Shell - LLE Completion State
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 * 
 * COMPLETION STATE - Spec 12 Core Component
 * 
 * Tracks state of current completion session.
 * Used for inline TAB cycling and menu navigation.
 */

#ifndef LLE_COMPLETION_STATE_H
#define LLE_COMPLETION_STATE_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/completion/context_analyzer.h"
#include "lle/completion/completion_types.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tracks state of current completion session
 */
typedef struct lle_completion_state {
    /* Input state */
    char *buffer_snapshot;            /* Buffer at completion start */
    size_t cursor_position;           /* Cursor at completion start */
    
    /* Context */
    lle_context_analyzer_t *context;  /* Analyzed context */
    
    /* Results */
    lle_completion_result_t *results; /* Generated completions */
    
    /* Cycling state (for inline completion) */
    int current_index;                /* Current selection (for TAB cycling) */
    char *original_word;              /* Original partial word */
    
    /* Timing */
    uint64_t generation_time_us;      /* Time to generate completions */
    
    /* Flags */
    bool active;                      /* Completion session active? */
    bool menu_mode;                   /* Menu shown or inline cycling? */
    
    /* Memory pool reference */
    lle_memory_pool_t *pool;          /* For allocations */
} lle_completion_state_t;

/**
 * Create completion state
 * 
 * @param pool Memory pool
 * @param buffer Buffer snapshot
 * @param cursor_pos Cursor position
 * @param context Analyzed context
 * @param results Completion results
 * @param out_state Output state
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_completion_state_create(
    lle_memory_pool_t *pool,
    const char *buffer,
    size_t cursor_pos,
    lle_context_analyzer_t *context,
    lle_completion_result_t *results,
    lle_completion_state_t **out_state
);

/**
 * Free completion state
 * 
 * @param state State to free
 */
void lle_completion_state_free(lle_completion_state_t *state);

/**
 * Cycle to next completion (for inline TAB cycling)
 * 
 * @param state Completion state
 * @return Next completion text or NULL
 */
const char *lle_completion_state_cycle_next(lle_completion_state_t *state);

/**
 * Cycle to previous completion (for Shift+TAB)
 * 
 * @param state Completion state
 * @return Previous completion text or NULL
 */
const char *lle_completion_state_cycle_prev(lle_completion_state_t *state);

/**
 * Get current selected completion
 * 
 * @param state Completion state
 * @return Current completion text or NULL
 */
const char *lle_completion_state_get_current(const lle_completion_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* LLE_COMPLETION_STATE_H */
