/*
 * Lusush Shell - LLE Completion Type Classification System Implementation
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
 * LLE COMPLETION TYPE CLASSIFICATION IMPLEMENTATION
 *
 * This module contains ONLY pure logic and data structures - NO terminal I/O.
 * All allocations use LLE memory pool for efficient memory management.
 */

#include "lle/completion/completion_types.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Forward declarations for shell integration
// These will be properly integrated via completion_sources module
extern bool lle_shell_is_builtin(const char *text);
extern bool lle_shell_is_alias(const char *text);

// ============================================================================
// TYPE INFORMATION DATABASE
// ============================================================================

static const lle_completion_type_info_t TYPE_INFO_DATABASE[] = {
    {.type = LLE_COMPLETION_TYPE_BUILTIN,
     .type_name = "Built-in",
     .category_name = "BUILT-INS",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 900},
    {.type = LLE_COMPLETION_TYPE_COMMAND,
     .type_name = "Command",
     .category_name = "COMMANDS",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 800},
    {.type = LLE_COMPLETION_TYPE_FILE,
     .type_name = "File",
     .category_name = "FILES",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 600},
    {.type = LLE_COMPLETION_TYPE_DIRECTORY,
     .type_name = "Directory",
     .category_name = "DIRECTORIES",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 700},
    {.type = LLE_COMPLETION_TYPE_VARIABLE,
     .type_name = "Variable",
     .category_name = "VARIABLES",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 500},
    {.type = LLE_COMPLETION_TYPE_ALIAS,
     .type_name = "Alias",
     .category_name = "ALIASES",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 950},
    {.type = LLE_COMPLETION_TYPE_HISTORY,
     .type_name = "History",
     .category_name = "HISTORY",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 400},
    {.type = LLE_COMPLETION_TYPE_UNKNOWN,
     .type_name = "Other",
     .category_name = "OTHER",
     .indicator = "", /* No symbols by default - user customizable via theme */
     .default_priority = 100}};

/* Note: TYPE_INFO_COUNT not currently used but kept for future validation */
#define TYPE_INFO_COUNT                                                        \
    (sizeof(TYPE_INFO_DATABASE) / sizeof(TYPE_INFO_DATABASE[0]))

// ============================================================================
// TYPE INFORMATION QUERIES
// ============================================================================

const lle_completion_type_info_t *
lle_completion_type_get_info(lle_completion_type_t type) {
    if (type < 0 || type >= LLE_COMPLETION_TYPE_COUNT) {
        return &TYPE_INFO_DATABASE[LLE_COMPLETION_TYPE_UNKNOWN];
    }
    return &TYPE_INFO_DATABASE[type];
}

const char *lle_completion_type_get_category(lle_completion_type_t type) {
    const lle_completion_type_info_t *info = lle_completion_type_get_info(type);
    return info->category_name;
}

const char *lle_completion_type_get_indicator(lle_completion_type_t type) {
    const lle_completion_type_info_t *info = lle_completion_type_get_info(type);
    return info->indicator;
}

// ============================================================================
// HELPER: STRING DUPLICATION WITH MEMORY POOL
// ============================================================================

static char *lle_strdup_pool(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = (char *)lle_pool_alloc(len);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, str, len);
    return copy;
}

// ============================================================================
// COMPLETION ITEM MANAGEMENT
// ============================================================================

lle_result_t lle_completion_item_create(lle_memory_pool_t *memory_pool,
                                        const char *text, const char *suffix,
                                        lle_completion_type_t type,
                                        int32_t relevance_score,
                                        lle_completion_item_t **item) {
    return lle_completion_item_create_with_description(
        memory_pool, text, suffix, type, relevance_score, NULL, item);
}

lle_result_t lle_completion_item_create_with_description(
    lle_memory_pool_t *memory_pool, const char *text, const char *suffix,
    lle_completion_type_t type, int32_t relevance_score,
    const char *description, lle_completion_item_t **item) {
    if (!memory_pool || !text || !item) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Allocate item structure
    lle_completion_item_t *new_item =
        (lle_completion_item_t *)lle_pool_alloc(sizeof(lle_completion_item_t));
    if (!new_item) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    // Duplicate text
    new_item->text = lle_strdup_pool(text);
    if (!new_item->text) {
        lle_pool_free(new_item);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    new_item->owns_text = true;

    // Duplicate suffix if provided
    if (suffix) {
        new_item->suffix = lle_strdup_pool(suffix);
        if (!new_item->suffix) {
            lle_pool_free(new_item->text);
            lle_pool_free(new_item);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        new_item->owns_suffix = true;
    } else {
        new_item->suffix = NULL;
        new_item->owns_suffix = false;
    }

    // Set type and get indicator
    new_item->type = type;
    new_item->type_indicator = lle_completion_type_get_indicator(type);

    // Set relevance score (clamp to valid range)
    if (relevance_score < 0)
        relevance_score = 0;
    if (relevance_score > 1000)
        relevance_score = 1000;
    new_item->relevance_score = relevance_score;

    // Duplicate description if provided
    if (description) {
        new_item->description = lle_strdup_pool(description);
        new_item->owns_description = (new_item->description != NULL);
    } else {
        new_item->description = NULL;
        new_item->owns_description = false;
    }

    *item = new_item;
    return LLE_SUCCESS;
}

lle_result_t lle_completion_item_free(lle_memory_pool_t *memory_pool,
                                      lle_completion_item_t *item) {
    if (!memory_pool || !item) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (item->owns_text && item->text) {
        lle_pool_free(item->text);
    }

    if (item->owns_suffix && item->suffix) {
        lle_pool_free(item->suffix);
    }

    if (item->owns_description && item->description) {
        lle_pool_free(item->description);
    }

    lle_pool_free(item);
    return LLE_SUCCESS;
}

// ============================================================================
// COMPLETION RESULT MANAGEMENT
// ============================================================================

lle_result_t lle_completion_result_create(lle_memory_pool_t *memory_pool,
                                          size_t initial_capacity,
                                          lle_completion_result_t **result) {
    if (!memory_pool || !result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (initial_capacity == 0) {
        initial_capacity = 16; // Default capacity
    }

    // Allocate result structure
    lle_completion_result_t *new_result =
        (lle_completion_result_t *)lle_pool_alloc(
            sizeof(lle_completion_result_t));
    if (!new_result) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    // Allocate items array
    new_result->items = (lle_completion_item_t *)lle_pool_alloc(
        sizeof(lle_completion_item_t) * initial_capacity);
    if (!new_result->items) {
        lle_pool_free(new_result);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    new_result->count = 0;
    new_result->capacity = initial_capacity;
    new_result->memory_pool = memory_pool;

    // Initialize category counts
    new_result->builtin_count = 0;
    new_result->command_count = 0;
    new_result->file_count = 0;
    new_result->directory_count = 0;
    new_result->variable_count = 0;
    new_result->alias_count = 0;
    new_result->history_count = 0;

    *result = new_result;
    return LLE_SUCCESS;
}

lle_result_t lle_completion_result_add_item(lle_completion_result_t *result,
                                            lle_completion_item_t *item) {
    if (!result || !item) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Grow array if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = result->capacity * 2;
        lle_completion_item_t *new_items =
            (lle_completion_item_t *)lle_pool_alloc(
                sizeof(lle_completion_item_t) * new_capacity);
        if (!new_items) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }

        // Copy existing items
        memcpy(new_items, result->items,
               sizeof(lle_completion_item_t) * result->count);

        // Free old array
        lle_pool_free(result->items);

        result->items = new_items;
        result->capacity = new_capacity;
    }

    // Add item (transfer ownership)
    result->items[result->count] = *item;
    result->count++;

    // Update category count
    switch (item->type) {
    case LLE_COMPLETION_TYPE_BUILTIN:
        result->builtin_count++;
        break;
    case LLE_COMPLETION_TYPE_COMMAND:
        result->command_count++;
        break;
    case LLE_COMPLETION_TYPE_FILE:
        result->file_count++;
        break;
    case LLE_COMPLETION_TYPE_DIRECTORY:
        result->directory_count++;
        break;
    case LLE_COMPLETION_TYPE_VARIABLE:
        result->variable_count++;
        break;
    case LLE_COMPLETION_TYPE_ALIAS:
        result->alias_count++;
        break;
    case LLE_COMPLETION_TYPE_HISTORY:
        result->history_count++;
        break;
    default:
        break;
    }

    // Free the item wrapper (contents now owned by result)
    lle_pool_free(item);

    return LLE_SUCCESS;
}

lle_result_t lle_completion_result_add(lle_completion_result_t *result,
                                       const char *text, const char *suffix,
                                       lle_completion_type_t type,
                                       int32_t relevance_score) {
    if (!result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_completion_item_t *item = NULL;
    lle_result_t res = lle_completion_item_create(
        result->memory_pool, text, suffix, type, relevance_score, &item);

    if (res != LLE_SUCCESS) {
        return res;
    }

    return lle_completion_result_add_item(result, item);
}

// Comparison function for sorting
static int compare_completion_items(const void *a, const void *b) {
    const lle_completion_item_t *item_a = (const lle_completion_item_t *)a;
    const lle_completion_item_t *item_b = (const lle_completion_item_t *)b;

    // First sort by type (category)
    if (item_a->type != item_b->type) {
        return item_a->type - item_b->type;
    }

    // Within same type, sort by relevance score (descending)
    if (item_a->relevance_score != item_b->relevance_score) {
        return item_b->relevance_score - item_a->relevance_score;
    }

    // Finally, alphabetical order
    return strcmp(item_a->text, item_b->text);
}

lle_result_t lle_completion_result_sort(lle_completion_result_t *result) {
    if (!result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (result->count <= 1) {
        return LLE_SUCCESS;
    }

    qsort(result->items, result->count, sizeof(lle_completion_item_t),
          compare_completion_items);

    return LLE_SUCCESS;
}

lle_result_t lle_completion_result_free(lle_completion_result_t *result) {
    if (!result) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    // Free all items
    for (size_t i = 0; i < result->count; i++) {
        lle_completion_item_t *item = &result->items[i];
        if (item->owns_text && item->text) {
            lle_pool_free(item->text);
        }
        if (item->owns_suffix && item->suffix) {
            lle_pool_free(item->suffix);
        }
        if (item->owns_description && item->description) {
            lle_pool_free(item->description);
        }
    }

    lle_pool_free(result->items);
    lle_pool_free(result);

    return LLE_SUCCESS;
}

// ============================================================================
// CLASSIFICATION HELPERS
// ============================================================================

bool lle_completion_is_builtin(const char *text) {
    if (!text) {
        return false;
    }

    // This will be properly implemented in completion_sources module
    // For now, use weak symbol that can be overridden
    return lle_shell_is_builtin(text);
}

bool lle_completion_is_alias(const char *text) {
    if (!text) {
        return false;
    }

    // This will be properly implemented in completion_sources module
    // For now, use weak symbol that can be overridden
    return lle_shell_is_alias(text);
}

bool lle_completion_is_directory(const char *text) {
    if (!text) {
        return false;
    }

    struct stat st;
    if (stat(text, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    return false;
}

lle_completion_type_t lle_completion_classify_text(const char *text,
                                                   bool is_command_position) {
    if (!text) {
        return LLE_COMPLETION_TYPE_UNKNOWN;
    }

    // Check for variable
    if (text[0] == '$') {
        return LLE_COMPLETION_TYPE_VARIABLE;
    }

    // If not at command position, likely a file/directory
    if (!is_command_position) {
        if (lle_completion_is_directory(text)) {
            return LLE_COMPLETION_TYPE_DIRECTORY;
        }
        return LLE_COMPLETION_TYPE_FILE;
    }

    // At command position - classify command type
    if (lle_completion_is_alias(text)) {
        return LLE_COMPLETION_TYPE_ALIAS;
    }

    if (lle_completion_is_builtin(text)) {
        return LLE_COMPLETION_TYPE_BUILTIN;
    }

    // Check if it's a path
    if (strchr(text, '/') != NULL) {
        if (lle_completion_is_directory(text)) {
            return LLE_COMPLETION_TYPE_DIRECTORY;
        }
        return LLE_COMPLETION_TYPE_FILE;
    }

    // Assume it's a command from PATH
    return LLE_COMPLETION_TYPE_COMMAND;
}

// ============================================================================
// STATISTICS AND QUERIES
// ============================================================================

size_t
lle_completion_result_count_by_type(const lle_completion_result_t *result,
                                    lle_completion_type_t type) {
    if (!result) {
        return 0;
    }

    switch (type) {
    case LLE_COMPLETION_TYPE_BUILTIN:
        return result->builtin_count;
    case LLE_COMPLETION_TYPE_COMMAND:
        return result->command_count;
    case LLE_COMPLETION_TYPE_FILE:
        return result->file_count;
    case LLE_COMPLETION_TYPE_DIRECTORY:
        return result->directory_count;
    case LLE_COMPLETION_TYPE_VARIABLE:
        return result->variable_count;
    case LLE_COMPLETION_TYPE_ALIAS:
        return result->alias_count;
    case LLE_COMPLETION_TYPE_HISTORY:
        return result->history_count;
    default:
        return 0;
    }
}

const lle_completion_item_t *
lle_completion_result_get_item(const lle_completion_result_t *result,
                               size_t index) {
    if (!result || index >= result->count) {
        return NULL;
    }

    return &result->items[index];
}

// ============================================================================
// WEAK SYMBOLS FOR SHELL INTEGRATION
// ============================================================================

// These weak symbols can be overridden by the actual shell integration
// in the completion_sources module

__attribute__((weak)) bool lle_shell_is_builtin(const char *text) {
    (void)text;
    return false;
}

__attribute__((weak)) bool lle_shell_is_alias(const char *text) {
    (void)text;
    return false;
}
