/**
 * @file completion_state.c
 * @brief LLE Completion State Implementation
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 *
 * Tracks state of active completion session.
 * Used for inline TAB cycling and menu navigation.
 */

#include "lle/completion/completion_state.h"
#include "lle/completion/completion_types.h"
#include "lle/completion/context_analyzer.h"
#include <string.h>

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Create a new completion state
 * @param pool Memory pool for allocations
 * @param buffer Current input buffer
 * @param cursor_pos Cursor position in buffer
 * @param context Analyzed context
 * @param results Completion results
 * @param out_state Output for created state
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_completion_state_create(lle_memory_pool_t *pool,
                                         const char *buffer, size_t cursor_pos,
                                         lle_context_analyzer_t *context,
                                         lle_completion_result_t *results,
                                         lle_completion_state_t **out_state) {
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
    state->current_index = -1;     /* No selection yet */
    state->generation_time_us = 0; /* Timing tracking not implemented yet */
    state->active = true;
    state->menu_mode = false; /* Determined by caller */
    state->pool = pool;

    *out_state = state;
    return LLE_SUCCESS;
}

/**
 * @brief Free completion state resources
 * @param state State to free
 */
void lle_completion_state_free(lle_completion_state_t *state) {
    if (!state) {
        return;
    }

    /* Free the completion results - the state owns them */
    if (state->results) {
        lle_completion_result_free(state->results);
        state->results = NULL;
    }

    /* Free the context analyzer */
    if (state->context) {
        lle_context_analyzer_free(state->context);
        state->context = NULL;
    }

    /* Mark as inactive */
    state->active = false;

    /* Note: The state structure itself and string fields (buffer_snapshot,
     * original_word) are pool-allocated and will be freed with the pool */
}

/**
 * @brief Cycle to next completion in TAB cycling mode
 * @param state Completion state
 * @return Text of next completion or NULL
 */
const char *lle_completion_state_cycle_next(lle_completion_state_t *state) {
    if (!state || !state->active || !state->results) {
        return NULL;
    }

    if (state->results->count == 0) {
        return NULL;
    }

    /* Move to next index */
    state->current_index =
        (state->current_index + 1) % (int)state->results->count;

    return state->results->items[state->current_index].text;
}

/**
 * @brief Cycle to previous completion in TAB cycling mode
 * @param state Completion state
 * @return Text of previous completion or NULL
 */
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

/**
 * @brief Get currently selected completion text
 * @param state Completion state
 * @return Current completion text or NULL
 */
const char *
lle_completion_state_get_current(const lle_completion_state_t *state) {
    if (!state || !state->active || !state->results) {
        return NULL;
    }

    if (state->current_index < 0 ||
        state->current_index >= (int)state->results->count) {
        return NULL;
    }

    return state->results->items[state->current_index].text;
}
