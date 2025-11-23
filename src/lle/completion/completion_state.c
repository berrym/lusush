/*
 * Lusush Shell - LLE Completion State Implementation
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 * 
 * COMPLETION STATE IMPLEMENTATION - Spec 12 Core
 * 
 * Tracks state of active completion session.
 * Used for inline TAB cycling and menu navigation.
 */

#include "lle/completion/completion_state.h"
#include <string.h>

// ============================================================================
// PUBLIC API
// ============================================================================

lle_result_t lle_completion_state_create(
    lle_memory_pool_t *pool,
    const char *buffer,
    size_t cursor_pos,
    lle_context_analyzer_t *context,
    lle_completion_result_t *results,
    lle_completion_state_t **out_state)
{
    if (!pool || !buffer || !context || !results || !out_state) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_completion_state_t *state = lle_pool_alloc(sizeof(*state));
    if (!state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy buffer snapshot */
    size_t buf_len = strlen(buffer);
    state->buffer_snapshot = lle_pool_alloc(buf_len + 1);
    if (!state->buffer_snapshot) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(state->buffer_snapshot, buffer, buf_len + 1);
    
    /* Copy original partial word */
    if (context->partial_word) {
        size_t word_len = strlen(context->partial_word);
        state->original_word = lle_pool_alloc(word_len + 1);
        if (!state->original_word) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(state->original_word, context->partial_word, word_len + 1);
    } else {
        state->original_word = lle_pool_alloc(1);
        if (!state->original_word) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        state->original_word[0] = '\0';
    }
    
    state->cursor_position = cursor_pos;
    state->context = context;
    state->results = results;
    state->current_index = -1;  /* No selection yet */
    state->generation_time_us = 0;  /* Timing tracking not implemented yet */
    state->active = true;
    state->menu_mode = false;  /* Determined by caller */
    state->pool = pool;
    
    *out_state = state;
    return LLE_SUCCESS;
}

void lle_completion_state_free(lle_completion_state_t *state) {
    if (!state) {
        return;
    }
    
    /* Memory is pool-allocated, will be freed with pool */
    /* Just mark as inactive */
    state->active = false;
}

const char *lle_completion_state_cycle_next(lle_completion_state_t *state) {
    if (!state || !state->active || !state->results) {
        return NULL;
    }
    
    if (state->results->count == 0) {
        return NULL;
    }
    
    /* Move to next index */
    state->current_index = (state->current_index + 1) % (int)state->results->count;
    
    return state->results->items[state->current_index].text;
}

const char *lle_completion_state_cycle_prev(lle_completion_state_t *state) {
    if (!state || !state->active || !state->results) {
        return NULL;
    }
    
    if (state->results->count == 0) {
        return NULL;
    }
    
    /* Move to previous index (with wrap-around) */
    if (state->current_index <= 0) {
        state->current_index = (int)state->results->count - 1;
    } else {
        state->current_index--;
    }
    
    return state->results->items[state->current_index].text;
}

const char *lle_completion_state_get_current(const lle_completion_state_t *state) {
    if (!state || !state->active || !state->results) {
        return NULL;
    }
    
    if (state->current_index < 0 || state->current_index >= (int)state->results->count) {
        return NULL;
    }
    
    return state->results->items[state->current_index].text;
}
