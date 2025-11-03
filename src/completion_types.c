/*
 * Lusush - Completion Type Classification System Implementation
 * Copyright (C) 2021-2025  Michael Berry
 */

#include "../include/completion_types.h"
#include "../include/alias.h"
#include "../include/builtins.h"
#include "../include/readline_integration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// ============================================================================
// TYPE INFORMATION DATABASE
// ============================================================================

static const completion_type_info_t TYPE_INFO_DATABASE[] = {
    {
        .type = COMPLETION_TYPE_BUILTIN,
        .type_name = "Built-in",
        .category_name = "BUILT-INS",
        .indicator = "⚙",
        .default_priority = 900
    },
    {
        .type = COMPLETION_TYPE_COMMAND,
        .type_name = "Command",
        .category_name = "COMMANDS",
        .indicator = "⚡",
        .default_priority = 800
    },
    {
        .type = COMPLETION_TYPE_FILE,
        .type_name = "File",
        .category_name = "FILES",
        .indicator = "📄",
        .default_priority = 600
    },
    {
        .type = COMPLETION_TYPE_DIRECTORY,
        .type_name = "Directory",
        .category_name = "DIRECTORIES",
        .indicator = "📁",
        .default_priority = 700
    },
    {
        .type = COMPLETION_TYPE_VARIABLE,
        .type_name = "Variable",
        .category_name = "VARIABLES",
        .indicator = "$",
        .default_priority = 500
    },
    {
        .type = COMPLETION_TYPE_ALIAS,
        .type_name = "Alias",
        .category_name = "ALIASES",
        .indicator = "@",
        .default_priority = 950
    },
    {
        .type = COMPLETION_TYPE_HISTORY,
        .type_name = "History",
        .category_name = "HISTORY",
        .indicator = "🕐",
        .default_priority = 400
    },
    {
        .type = COMPLETION_TYPE_UNKNOWN,
        .type_name = "Other",
        .category_name = "OTHER",
        .indicator = "?",
        .default_priority = 100
    }
};

static const size_t TYPE_INFO_COUNT = sizeof(TYPE_INFO_DATABASE) / sizeof(TYPE_INFO_DATABASE[0]);

// ============================================================================
// TYPE INFORMATION QUERIES
// ============================================================================

const completion_type_info_t* completion_type_get_info(completion_type_t type) {
    if (type < 0 || type >= TYPE_INFO_COUNT) {
        return &TYPE_INFO_DATABASE[COMPLETION_TYPE_UNKNOWN];
    }
    return &TYPE_INFO_DATABASE[type];
}

const char* completion_type_get_category(completion_type_t type) {
    const completion_type_info_t *info = completion_type_get_info(type);
    return info->category_name;
}

const char* completion_type_get_indicator(completion_type_t type) {
    const completion_type_info_t *info = completion_type_get_info(type);
    return info->indicator;
}

// ============================================================================
// COMPLETION ITEM MANAGEMENT
// ============================================================================

completion_item_t* completion_item_create(const char *text,
                                         const char *suffix,
                                         completion_type_t type,
                                         int relevance_score) {
    return completion_item_create_with_description(text, suffix, type, 
                                                   relevance_score, NULL);
}

completion_item_t* completion_item_create_with_description(const char *text,
                                                          const char *suffix,
                                                          completion_type_t type,
                                                          int relevance_score,
                                                          const char *description) {
    if (!text) {
        return NULL;
    }
    
    completion_item_t *item = malloc(sizeof(completion_item_t));
    if (!item) {
        return NULL;
    }
    
    // Duplicate text
    item->text = strdup(text);
    if (!item->text) {
        free(item);
        return NULL;
    }
    item->owns_text = true;
    
    // Set suffix (not owned, points to static string or NULL)
    item->suffix = (char*)suffix;
    
    // Set type and get indicator
    item->type = type;
    item->type_indicator = completion_type_get_indicator(type);
    
    // Set relevance score (clamp to valid range)
    if (relevance_score < 0) relevance_score = 0;
    if (relevance_score > 1000) relevance_score = 1000;
    item->relevance_score = relevance_score;
    
    // Duplicate description if provided
    if (description) {
        item->description = strdup(description);
        item->owns_description = (item->description != NULL);
    } else {
        item->description = NULL;
        item->owns_description = false;
    }
    
    return item;
}

void completion_item_free(completion_item_t *item) {
    if (!item) {
        return;
    }
    
    if (item->owns_text && item->text) {
        free(item->text);
    }
    
    if (item->owns_description && item->description) {
        free(item->description);
    }
    
    free(item);
}

// ============================================================================
// COMPLETION RESULT MANAGEMENT
// ============================================================================

completion_result_t* completion_result_create(size_t initial_capacity) {
    completion_result_t *result = malloc(sizeof(completion_result_t));
    if (!result) {
        return NULL;
    }
    
    if (initial_capacity == 0) {
        initial_capacity = 16; // Default capacity
    }
    
    result->items = malloc(sizeof(completion_item_t) * initial_capacity);
    if (!result->items) {
        free(result);
        return NULL;
    }
    
    result->count = 0;
    result->capacity = initial_capacity;
    
    // Initialize category counts
    result->builtin_count = 0;
    result->command_count = 0;
    result->file_count = 0;
    result->directory_count = 0;
    result->variable_count = 0;
    result->alias_count = 0;
    result->history_count = 0;
    
    return result;
}

bool completion_result_add_item(completion_result_t *result, 
                               completion_item_t *item) {
    if (!result || !item) {
        return false;
    }
    
    // Grow array if needed
    if (result->count >= result->capacity) {
        size_t new_capacity = result->capacity * 2;
        completion_item_t *new_items = realloc(result->items, 
                                              sizeof(completion_item_t) * new_capacity);
        if (!new_items) {
            return false;
        }
        result->items = new_items;
        result->capacity = new_capacity;
    }
    
    // Add item (transfer ownership)
    result->items[result->count] = *item;
    result->count++;
    
    // Update category count
    switch (item->type) {
        case COMPLETION_TYPE_BUILTIN:
            result->builtin_count++;
            break;
        case COMPLETION_TYPE_COMMAND:
            result->command_count++;
            break;
        case COMPLETION_TYPE_FILE:
            result->file_count++;
            break;
        case COMPLETION_TYPE_DIRECTORY:
            result->directory_count++;
            break;
        case COMPLETION_TYPE_VARIABLE:
            result->variable_count++;
            break;
        case COMPLETION_TYPE_ALIAS:
            result->alias_count++;
            break;
        case COMPLETION_TYPE_HISTORY:
            result->history_count++;
            break;
        default:
            break;
    }
    
    // Free the wrapper (contents now owned by result)
    free(item);
    
    return true;
}

bool completion_result_add(completion_result_t *result,
                          const char *text,
                          const char *suffix,
                          completion_type_t type,
                          int relevance_score) {
    completion_item_t *item = completion_item_create(text, suffix, type, relevance_score);
    if (!item) {
        return false;
    }
    
    return completion_result_add_item(result, item);
}

// Comparison function for sorting
static int compare_completion_items(const void *a, const void *b) {
    const completion_item_t *item_a = (const completion_item_t*)a;
    const completion_item_t *item_b = (const completion_item_t*)b;
    
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

void completion_result_sort(completion_result_t *result) {
    if (!result || result->count <= 1) {
        return;
    }
    
    qsort(result->items, result->count, sizeof(completion_item_t), 
          compare_completion_items);
}

void completion_result_free(completion_result_t *result) {
    if (!result) {
        return;
    }
    
    // Free all items
    for (size_t i = 0; i < result->count; i++) {
        completion_item_t *item = &result->items[i];
        if (item->owns_text && item->text) {
            free(item->text);
        }
        if (item->owns_description && item->description) {
            free(item->description);
        }
    }
    
    free(result->items);
    free(result);
}

// ============================================================================
// CONVERSION UTILITIES
// ============================================================================

size_t completion_result_to_lusush_completions(completion_result_t *result,
                                              lusush_completions_t *lc) {
    if (!result || !lc) {
        return 0;
    }
    
    size_t added = 0;
    
    for (size_t i = 0; i < result->count; i++) {
        completion_item_t *item = &result->items[i];
        
        // Build completion string with suffix
        char completion[512];
        if (item->suffix) {
            snprintf(completion, sizeof(completion), "%s%s", 
                    item->text, item->suffix);
        } else {
            snprintf(completion, sizeof(completion), "%s", item->text);
        }
        
        lusush_add_completion(lc, completion);
        added++;
    }
    
    return added;
}

completion_result_t* completion_result_from_lusush_completions(
    lusush_completions_t *lc) {
    if (!lc || lc->len == 0) {
        return NULL;
    }
    
    completion_result_t *result = completion_result_create(lc->len);
    if (!result) {
        return NULL;
    }
    
    // Classify each completion
    for (size_t i = 0; i < lc->len; i++) {
        const char *text = lc->cvec[i];
        if (!text) {
            continue;
        }
        
        // Classify text (assume command position for now)
        completion_type_t type = completion_classify_text(text, true);
        
        // Add to result with default relevance
        completion_result_add(result, text, " ", type, 500);
    }
    
    return result;
}

// ============================================================================
// CLASSIFICATION HELPERS
// ============================================================================

bool completion_is_builtin(const char *text) {
    if (!text) {
        return false;
    }
    
    // Check against builtin command list
    extern builtin builtins[];
    extern const size_t builtins_count;
    
    for (size_t i = 0; i < builtins_count; i++) {
        if (strcmp(text, builtins[i].name) == 0) {
            return true;
        }
    }
    
    return false;
}

bool completion_is_alias(const char *text) {
    if (!text) {
        return false;
    }
    
    // Check if alias exists
    char *alias_value = lookup_alias(text);
    bool is_alias = (alias_value != NULL);
    free(alias_value);
    
    return is_alias;
}

bool completion_is_directory(const char *text) {
    if (!text) {
        return false;
    }
    
    struct stat st;
    if (stat(text, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    
    return false;
}

completion_type_t completion_classify_text(const char *text, 
                                          bool is_command_position) {
    if (!text) {
        return COMPLETION_TYPE_UNKNOWN;
    }
    
    // Check for variable
    if (text[0] == '$') {
        return COMPLETION_TYPE_VARIABLE;
    }
    
    // If not at command position, likely a file/directory
    if (!is_command_position) {
        if (completion_is_directory(text)) {
            return COMPLETION_TYPE_DIRECTORY;
        }
        return COMPLETION_TYPE_FILE;
    }
    
    // At command position - classify command type
    if (completion_is_alias(text)) {
        return COMPLETION_TYPE_ALIAS;
    }
    
    if (completion_is_builtin(text)) {
        return COMPLETION_TYPE_BUILTIN;
    }
    
    // Check if it's a path
    if (strchr(text, '/') != NULL) {
        if (completion_is_directory(text)) {
            return COMPLETION_TYPE_DIRECTORY;
        }
        return COMPLETION_TYPE_FILE;
    }
    
    // Assume it's a command from PATH
    return COMPLETION_TYPE_COMMAND;
}

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

void completion_result_print_stats(const completion_result_t *result) {
    if (!result) {
        printf("No completion result\n");
        return;
    }
    
    printf("Completion Statistics:\n");
    printf("  Total items: %zu\n", result->count);
    printf("  Built-ins:   %zu\n", result->builtin_count);
    printf("  Commands:    %zu\n", result->command_count);
    printf("  Files:       %zu\n", result->file_count);
    printf("  Directories: %zu\n", result->directory_count);
    printf("  Variables:   %zu\n", result->variable_count);
    printf("  Aliases:     %zu\n", result->alias_count);
    printf("  History:     %zu\n", result->history_count);
}

size_t completion_result_count_by_type(const completion_result_t *result,
                                      completion_type_t type) {
    if (!result) {
        return 0;
    }
    
    switch (type) {
        case COMPLETION_TYPE_BUILTIN:
            return result->builtin_count;
        case COMPLETION_TYPE_COMMAND:
            return result->command_count;
        case COMPLETION_TYPE_FILE:
            return result->file_count;
        case COMPLETION_TYPE_DIRECTORY:
            return result->directory_count;
        case COMPLETION_TYPE_VARIABLE:
            return result->variable_count;
        case COMPLETION_TYPE_ALIAS:
            return result->alias_count;
        case COMPLETION_TYPE_HISTORY:
            return result->history_count;
        default:
            return 0;
    }
}
