/*
 * Lusush Shell - LLE Completion System Implementation
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ============================================================================
 *
 * COMPLETION SYSTEM - Spec 12 Core Implementation
 *
 * Enhanced completion system with proper architecture.
 * This FIXES the duplicate and categorization bugs.
 */

#include "lle/completion/completion_system.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LIFECYCLE FUNCTIONS
// ============================================================================

lle_result_t
lle_completion_system_create(lle_memory_pool_t *pool,
                             lle_completion_system_t **out_system) {
    if (!pool || !out_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_completion_system_t *system = lle_pool_alloc(sizeof(*system));
    if (!system) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Create source manager */
    lle_result_t res = lle_source_manager_create(pool, &system->source_manager);
    if (res != LLE_SUCCESS) {
        return res;
    }

    system->current_state = NULL;
    system->menu = NULL;
    system->pool = pool;
    system->enable_history_source = true;
    system->enable_fuzzy_matching = false; /* Future feature */
    system->max_completions = 100;

    *out_system = system;
    return LLE_SUCCESS;
}

void lle_completion_system_destroy(lle_completion_system_t *system) {
    if (!system) {
        return;
    }

    /* Free source manager */
    if (system->source_manager) {
        lle_source_manager_free(system->source_manager);
    }

    /* Free current state */
    if (system->current_state) {
        lle_completion_state_free(system->current_state);
    }

    /* Memory is pool-allocated */
}

void lle_completion_system_clear(lle_completion_system_t *system) {
    if (!system) {
        return;
    }

    if (system->current_state) {
        lle_completion_state_free(system->current_state);
        system->current_state = NULL;
    }

    /* Just set to NULL - menu is pool-allocated and will be freed with pool.
     * NOTE: display_controller also holds a pointer to this menu, so we must
     * not free it here. The display_controller will clear its reference via
     * display_controller_clear_completion_menu() called after this function. */
    system->menu = NULL;
}

// ============================================================================
// HELPER FUNCTIONS FOR PHASE 4
// ============================================================================

/**
 * Deduplicate completion results
 * CRITICAL: Fixes the "echo appears twice" bug
 */
static lle_result_t deduplicate_results(lle_completion_result_t *result) {
    if (!result || result->count <= 1) {
        return LLE_SUCCESS;
    }

    size_t write_pos = 0;

    for (size_t read_pos = 0; read_pos < result->count; read_pos++) {
        const char *text = result->items[read_pos].text;

        /* Check if we've seen this text before */
        bool duplicate = false;
        for (size_t check = 0; check < write_pos; check++) {
            if (strcmp(result->items[check].text, text) == 0) {
                duplicate = true;
                break;
            }
        }

        /* Keep only unique items */
        if (!duplicate) {
            if (write_pos != read_pos) {
                result->items[write_pos] = result->items[read_pos];
            }
            write_pos++;
        }
    }

    result->count = write_pos;
    return LLE_SUCCESS;
}

/**
 * Sort completion results
 */
static int completion_compare(const void *a, const void *b) {
    const lle_completion_item_t *item_a = (const lle_completion_item_t *)a;
    const lle_completion_item_t *item_b = (const lle_completion_item_t *)b;

    /* Sort by relevance_score first (higher score = earlier in list) */
    if (item_a->relevance_score != item_b->relevance_score) {
        return item_b->relevance_score -
               item_a->relevance_score; /* Descending order */
    }

    /* Then by type for items with same score */
    if (item_a->type != item_b->type) {
        return item_a->type - item_b->type;
    }

    /* Finally alphabetically within same type */
    return strcmp(item_a->text, item_b->text);
}

static lle_result_t sort_results(lle_completion_result_t *result) {
    if (!result || result->count <= 1) {
        return LLE_SUCCESS;
    }

    qsort(result->items, result->count, sizeof(lle_completion_item_t),
          completion_compare);

    return LLE_SUCCESS;
}

// ============================================================================
// COMPLETION GENERATION (Spec 12 Core)
// ============================================================================

lle_result_t
lle_completion_system_generate(lle_completion_system_t *system,
                               const char *buffer, size_t cursor_pos,
                               lle_completion_result_t **out_result) {
    if (!system || !buffer || !out_result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Step 1: Analyze context */
    lle_context_analyzer_t *context = NULL;
    lle_result_t res =
        lle_context_analyze(buffer, cursor_pos, system->pool, &context);
    if (res != LLE_SUCCESS) {
        return res;
    }

    /* Step 2: Create result structure */
    lle_completion_result_t *result = NULL;
    res = lle_completion_result_create(system->pool, 64, &result);
    if (res != LLE_SUCCESS) {
        lle_context_analyzer_free(context);
        return res;
    }

    /* Step 3: Query all applicable sources */
    const char *prefix = context->partial_word ? context->partial_word : "";
    res = lle_source_manager_query(system->source_manager, context, prefix,
                                   result);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        lle_context_analyzer_free(context);
        return res;
    }

    /* Step 4: Deduplicate results - FIXES THE DUPLICATE BUG */
    res = deduplicate_results(result);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        lle_context_analyzer_free(context);
        return res;
    }

    /* Step 5: Sort results */
    res = sort_results(result);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        lle_context_analyzer_free(context);
        return res;
    }

    /* Step 6: Create and store completion state */
    lle_completion_state_t *state = NULL;
    res = lle_completion_state_create(system->pool, buffer, cursor_pos, context,
                                      result, &state);
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        lle_context_analyzer_free(context);
        return res;
    }

    /* Step 7: Create menu if multiple completions (for display system) */
    lle_completion_menu_state_t *menu = NULL;
    if (result->count > 1) {
        /* Create menu with default config */
        lle_completion_menu_config_t menu_config = {
            .max_visible_items = 20,
            .show_category_headers = true,
            .show_type_indicators = false,
            .show_descriptions = false,
            .enable_scrolling = true,
            .min_items_for_menu = 2};

        res = lle_completion_menu_state_create(system->pool, result,
                                               &menu_config, &menu);
        if (res != LLE_SUCCESS) {
            lle_completion_state_free(state);
            lle_completion_result_free(result);
            lle_context_analyzer_free(context);
            return res;
        }
    }

    /* Clear old state and menu */
    if (system->current_state) {
        lle_completion_state_free(system->current_state);
    }
    if (system->menu) {
        lle_completion_menu_state_free(system->menu);
    }

    system->current_state = state;
    system->menu = menu; /* NULL if single completion or no completions */
    *out_result = result;

    return LLE_SUCCESS;
}

// ============================================================================
// STATE QUERIES
// ============================================================================

bool lle_completion_system_is_active(const lle_completion_system_t *system) {
    return system && system->current_state && system->current_state->active;
}

bool lle_completion_system_is_menu_visible(
    const lle_completion_system_t *system) {
    return system && system->menu != NULL;
}

lle_completion_state_t *
lle_completion_system_get_state(lle_completion_system_t *system) {
    return system ? system->current_state : NULL;
}

lle_completion_menu_state_t *
lle_completion_system_get_menu(lle_completion_system_t *system) {
    return system ? system->menu : NULL;
}
