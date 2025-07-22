/*
 * Lusush Line Editor - Completion Framework Implementation
 * 
 * This file implements the completion system architecture for the Lusush Line Editor.
 * It provides a flexible framework for tab completion with support for multiple
 * completion providers, prioritized results, and extensible architecture.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "completion.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* Forward declaration for strncasecmp */
int strncasecmp(const char *s1, const char *s2, size_t n);

/* Default initial capacity for completion lists */
#define LLE_COMPLETION_DEFAULT_CAPACITY 16

/* Maximum number of registered providers */
#define LLE_COMPLETION_MAX_PROVIDERS 32

/* Provider registry */
static lle_completion_provider_registration_t g_providers[LLE_COMPLETION_MAX_PROVIDERS];
static size_t g_provider_count = 0;

/* ========================================================================
 * Internal Helper Functions
 * ======================================================================== */

/**
 * Resize completion list to accommodate more items
 */
static bool completion_list_resize(lle_completion_list_t *list, size_t new_capacity) {
    if (!list || new_capacity < list->count) {
        return false;
    }
    
    lle_completion_item_t *new_items = realloc(list->items, 
                                              new_capacity * sizeof(lle_completion_item_t));
    if (!new_items) {
        return false;
    }
    
    list->items = new_items;
    list->capacity = new_capacity;
    return true;
}

/**
 * Comparison function for sorting completion items
 */
static int completion_item_compare(const void *a, const void *b) {
    const lle_completion_item_t *item_a = (const lle_completion_item_t *)a;
    const lle_completion_item_t *item_b = (const lle_completion_item_t *)b;
    
    // First sort by priority (higher first)
    if (item_a->priority != item_b->priority) {
        return item_b->priority - item_a->priority;
    }
    
    // Then sort alphabetically by text
    if (item_a->text && item_b->text) {
        return strcmp(item_a->text, item_b->text);
    }
    
    // Handle NULL cases
    if (item_a->text && !item_b->text) return -1;
    if (!item_a->text && item_b->text) return 1;
    return 0;
}

/**
 * Safely duplicate a string
 */
static char *safe_strdup(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    
    memcpy(copy, str, len + 1);
    return copy;
}

/**
 * Safely duplicate a string with explicit length
 */
static char *safe_strndup(const char *str, size_t len) {
    if (!str || len == 0) return NULL;
    
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    
    memcpy(copy, str, len);
    copy[len] = '\0';
    return copy;
}

/**
 * Find word boundaries around cursor position
 */
static void find_word_boundaries(const char *input, size_t cursor_pos, 
                                size_t *word_start, size_t *word_end) {
    if (!input || !word_start || !word_end) return;
    
    size_t len = strlen(input);
    if (cursor_pos > len) cursor_pos = len;
    
    // Find start of word (move backward from cursor)
    *word_start = cursor_pos;
    while (*word_start > 0 && !isspace(input[*word_start - 1])) {
        (*word_start)--;
    }
    
    // Find end of word (move forward from cursor)
    *word_end = cursor_pos;
    while (*word_end < len && !isspace(input[*word_end])) {
        (*word_end)++;
    }
}

/* ========================================================================
 * Completion List Management
 * ======================================================================== */

lle_completion_list_t *lle_completion_list_create(size_t initial_capacity) {
    lle_completion_list_t *list = malloc(sizeof(lle_completion_list_t));
    if (!list) return NULL;
    
    if (!lle_completion_list_init(list, initial_capacity)) {
        free(list);
        return NULL;
    }
    
    list->owns_memory = true;
    return list;
}

bool lle_completion_list_init(lle_completion_list_t *list, size_t initial_capacity) {
    if (!list) return false;
    
    if (initial_capacity == 0) {
        initial_capacity = LLE_COMPLETION_DEFAULT_CAPACITY;
    }
    
    memset(list, 0, sizeof(lle_completion_list_t));
    
    list->items = malloc(initial_capacity * sizeof(lle_completion_item_t));
    if (!list->items) return false;
    
    list->capacity = initial_capacity;
    list->owns_memory = true;
    return true;
}

void lle_completion_list_clear(lle_completion_list_t *list) {
    if (!list || !list->items) return;
    
    // Clear all items if we own their memory
    if (list->owns_memory) {
        for (size_t i = 0; i < list->count; i++) {
            lle_completion_item_clear(&list->items[i]);
        }
    }
    
    list->count = 0;
    list->selected = 0;
    list->sorted = false;
    list->total_text_len = 0;
    list->max_text_len = 0;
    list->max_desc_len = 0;
}

void lle_completion_list_destroy(lle_completion_list_t *list) {
    if (!list) return;
    
    lle_completion_list_clear(list);
    
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    
    if (list->owns_memory) {
        free(list);
    }
}

bool lle_completion_list_add(lle_completion_list_t *list,
                            const char *text,
                            const char *description,
                            int priority) {
    if (!list || !text) return false;
    
    size_t text_len = strlen(text);
    size_t desc_len = description ? strlen(description) : 0;
    
    return lle_completion_list_add_len(list, text, text_len, 
                                      description, desc_len, priority);
}

bool lle_completion_list_add_len(lle_completion_list_t *list,
                                const char *text,
                                size_t text_len,
                                const char *description,
                                size_t desc_len,
                                int priority) {
    if (!list || !text || text_len == 0) return false;
    
    // Resize if needed
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        if (!completion_list_resize(list, new_capacity)) {
            return false;
        }
    }
    
    // Initialize new item
    lle_completion_item_t *item = &list->items[list->count];
    memset(item, 0, sizeof(lle_completion_item_t));
    
    // Copy text
    item->text = safe_strndup(text, text_len);
    if (!item->text) return false;
    
    item->text_len = text_len;
    item->priority = priority;
    item->allocated = true;
    
    // Copy description if provided
    if (description && desc_len > 0) {
        item->description = safe_strndup(description, desc_len);
        if (!item->description) {
            free(item->text);
            item->text = NULL;
            return false;
        }
        item->desc_len = desc_len;
    }
    
    // Update statistics
    list->total_text_len += text_len;
    if (text_len > list->max_text_len) {
        list->max_text_len = text_len;
    }
    if (desc_len > list->max_desc_len) {
        list->max_desc_len = desc_len;
    }
    
    list->count++;
    list->sorted = false;  // List is no longer sorted
    
    return true;
}

void lle_completion_list_sort(lle_completion_list_t *list) {
    if (!list || !list->items || list->count <= 1) return;
    
    qsort(list->items, list->count, sizeof(lle_completion_item_t), completion_item_compare);
    list->sorted = true;
    
    // Reset selection to first item
    list->selected = 0;
}

const lle_completion_item_t *lle_completion_list_get_selected(const lle_completion_list_t *list) {
    if (!list || !list->items || list->count == 0) return NULL;
    if (list->selected >= list->count) return NULL;
    
    return &list->items[list->selected];
}

bool lle_completion_list_set_selected(lle_completion_list_t *list, size_t index) {
    if (!list || index >= list->count) return false;
    
    list->selected = index;
    return true;
}

bool lle_completion_list_select_next(lle_completion_list_t *list) {
    if (!list || list->count == 0) return false;
    
    list->selected = (list->selected + 1) % list->count;
    return true;
}

bool lle_completion_list_select_prev(lle_completion_list_t *list) {
    if (!list || list->count == 0) return false;
    
    if (list->selected == 0) {
        list->selected = list->count - 1;
    } else {
        list->selected--;
    }
    return true;
}

/* ========================================================================
 * Completion Context Management
 * ======================================================================== */

lle_completion_context_t *lle_completion_context_create(const char *input, size_t cursor_pos) {
    lle_completion_context_t *context = malloc(sizeof(lle_completion_context_t));
    if (!context) return NULL;
    
    if (!lle_completion_context_init(context, input, cursor_pos)) {
        free(context);
        return NULL;
    }
    
    return context;
}

bool lle_completion_context_init(lle_completion_context_t *context, 
                                const char *input, 
                                size_t cursor_pos) {
    if (!context || !input) return false;
    
    memset(context, 0, sizeof(lle_completion_context_t));
    
    context->input = input;
    context->input_len = strlen(input);
    context->cursor_pos = cursor_pos > context->input_len ? context->input_len : cursor_pos;
    
    // Find word boundaries
    find_word_boundaries(input, cursor_pos, &context->word_start, &context->word_end);
    context->word = input + context->word_start;
    context->word_len = context->word_end - context->word_start;
    
    // Set defaults
    context->case_sensitive = false;
    context->include_hidden = false;
    context->max_results = 100;
    
    // Determine context type
    context->at_command_start = (context->word_start == 0);
    
    // Check if we're in quotes
    bool in_single_quote = false, in_double_quote = false;
    for (size_t i = 0; i < context->cursor_pos; i++) {
        if (input[i] == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            context->quote_char = '\'';
        } else if (input[i] == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            context->quote_char = '"';
        }
    }
    context->in_quotes = in_single_quote || in_double_quote;
    
    return true;
}

void lle_completion_context_destroy(lle_completion_context_t *context) {
    if (context) {
        free(context);
    }
}

/* ========================================================================
 * Completion Item Utilities
 * ======================================================================== */

lle_completion_item_t *lle_completion_item_create(const char *text,
                                                 const char *description,
                                                 int priority) {
    lle_completion_item_t *item = malloc(sizeof(lle_completion_item_t));
    if (!item) return NULL;
    
    if (!lle_completion_item_init(item, text, description, priority)) {
        free(item);
        return NULL;
    }
    
    return item;
}

bool lle_completion_item_init(lle_completion_item_t *item,
                             const char *text,
                             const char *description,
                             int priority) {
    if (!item || !text) return false;
    
    memset(item, 0, sizeof(lle_completion_item_t));
    
    item->text = safe_strdup(text);
    if (!item->text) return false;
    
    item->text_len = strlen(text);
    item->priority = priority;
    item->allocated = true;
    
    if (description) {
        item->description = safe_strdup(description);
        if (!item->description) {
            free(item->text);
            item->text = NULL;
            return false;
        }
        item->desc_len = strlen(description);
    }
    
    return true;
}

void lle_completion_item_clear(lle_completion_item_t *item) {
    if (!item) return;
    
    if (item->allocated) {
        free(item->text);
        free(item->description);
    }
    
    memset(item, 0, sizeof(lle_completion_item_t));
}

void lle_completion_item_destroy(lle_completion_item_t *item) {
    if (!item) return;
    
    lle_completion_item_clear(item);
    free(item);
}

/* ========================================================================
 * Provider Management
 * ======================================================================== */

bool lle_completion_register_provider(const lle_completion_provider_registration_t *registration) {
    if (!registration || !registration->name || !registration->provider) {
        return false;
    }
    
    if (g_provider_count >= LLE_COMPLETION_MAX_PROVIDERS) {
        return false;
    }
    
    // Check for duplicate names
    for (size_t i = 0; i < g_provider_count; i++) {
        if (strcmp(g_providers[i].name, registration->name) == 0) {
            return false;  // Provider already registered
        }
    }
    
    // Copy registration
    g_providers[g_provider_count] = *registration;
    g_provider_count++;
    
    return true;
}

bool lle_completion_unregister_provider(const char *name) {
    if (!name) return false;
    
    for (size_t i = 0; i < g_provider_count; i++) {
        if (strcmp(g_providers[i].name, name) == 0) {
            // Shift remaining providers down
            memmove(&g_providers[i], &g_providers[i + 1], 
                    (g_provider_count - i - 1) * sizeof(lle_completion_provider_registration_t));
            g_provider_count--;
            return true;
        }
    }
    
    return false;
}

size_t lle_completion_get_provider_names(const char **names, size_t max_names) {
    if (!names || max_names == 0) return 0;
    
    size_t count = g_provider_count < max_names ? g_provider_count : max_names;
    for (size_t i = 0; i < count; i++) {
        names[i] = g_providers[i].name;
    }
    
    return count;
}

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

bool lle_completion_text_matches(const char *text, const char *prefix, bool case_sensitive) {
    if (!text || !prefix) return false;
    
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) return true;  // Empty prefix matches everything
    
    if (strlen(text) < prefix_len) return false;
    
    if (case_sensitive) {
        return strncmp(text, prefix, prefix_len) == 0;
    } else {
        return strncasecmp(text, prefix, prefix_len) == 0;
    }
}

size_t lle_completion_find_common_prefix(const lle_completion_list_t *list,
                                        char *common_prefix,
                                        size_t max_len) {
    if (!list || !common_prefix || max_len == 0 || list->count == 0) {
        if (common_prefix && max_len > 0) {
            common_prefix[0] = '\0';
        }
        return 0;
    }
    
    if (list->count == 1) {
        // Single item - return its text as common prefix
        size_t len = list->items[0].text_len;
        if (len >= max_len) len = max_len - 1;
        
        memcpy(common_prefix, list->items[0].text, len);
        common_prefix[len] = '\0';
        return len;
    }
    
    // Find common prefix among all items
    const char *first_text = list->items[0].text;
    if (!first_text) {
        common_prefix[0] = '\0';
        return 0;
    }
    
    size_t common_len = 0;
    size_t first_len = list->items[0].text_len;
    
    for (size_t pos = 0; pos < first_len && pos < max_len - 1; pos++) {
        char ch = first_text[pos];
        bool all_match = true;
        
        for (size_t i = 1; i < list->count; i++) {
            if (!list->items[i].text || 
                pos >= list->items[i].text_len || 
                list->items[i].text[pos] != ch) {
                all_match = false;
                break;
            }
        }
        
        if (!all_match) break;
        
        common_prefix[common_len++] = ch;
    }
    
    common_prefix[common_len] = '\0';
    return common_len;
}

size_t lle_completion_filter_by_prefix(lle_completion_list_t *list,
                                      const char *prefix,
                                      bool case_sensitive) {
    if (!list || !prefix) return 0;
    
    size_t write_pos = 0;
    
    for (size_t read_pos = 0; read_pos < list->count; read_pos++) {
        if (lle_completion_text_matches(list->items[read_pos].text, prefix, case_sensitive)) {
            if (write_pos != read_pos) {
                list->items[write_pos] = list->items[read_pos];
            }
            write_pos++;
        } else {
            // Clear filtered out item if we own its memory
            if (list->owns_memory) {
                lle_completion_item_clear(&list->items[read_pos]);
            }
        }
    }
    
    list->count = write_pos;
    
    // Reset selection if it's now out of bounds
    if (list->selected >= list->count) {
        list->selected = list->count > 0 ? list->count - 1 : 0;
    }
    
    return write_pos;
}

/* ========================================================================
 * File Completion Implementation (LLE-030)
 * ======================================================================== */

/**
 * @brief Extract word being completed from input at cursor position
 *
 * Analyzes the input string at the given cursor position to extract the
 * word currently being completed. Uses shell-aware word boundary detection
 * that recognizes command separators, redirections, and other shell syntax.
 * Handles both absolute and relative paths correctly.
 *
 * @param input Input string to analyze
 * @param cursor_pos Current cursor position in input (clamped to input length)
 * @param word Buffer to store extracted word (null-terminated)
 * @param word_size Size of word buffer including space for null terminator
 * @param word_start Pointer to store start position of extracted word
 * @return true on success, false on error or invalid parameters
 *
 * @note Word boundaries include: space, tab, pipe, redirections, semicolon
 * @note Handles quoted strings and escaped characters appropriately
 * @note word_start will point to first character of the extracted word
 */
bool lle_completion_extract_word(
    const char *input,
    size_t cursor_pos,
    char *word,
    size_t word_size,
    size_t *word_start
) {
    if (!input || !word || word_size == 0 || !word_start) {
        return false;
    }
    
    size_t input_len = strlen(input);
    if (cursor_pos > input_len) {
        cursor_pos = input_len;
    }
    
    // Find start of word (work backwards from cursor)
    size_t start = cursor_pos;
    while (start > 0) {
        char prev_char = input[start - 1];
        // Word boundary characters: space, tab, pipe, redirect, etc.
        if (prev_char == ' ' || prev_char == '\t' || prev_char == '|' || 
            prev_char == '>' || prev_char == '<' || prev_char == '&' ||
            prev_char == ';' || prev_char == '(' || prev_char == ')') {
            break;
        }
        start--;
    }
    
    // Find end of word (work forwards from cursor)
    size_t end = cursor_pos;
    while (end < input_len) {
        char curr_char = input[end];
        // Stop at word boundary characters
        if (curr_char == ' ' || curr_char == '\t' || curr_char == '|' || 
            curr_char == '>' || curr_char == '<' || curr_char == '&' ||
            curr_char == ';' || curr_char == '(' || curr_char == ')') {
            break;
        }
        end++;
    }
    
    // Extract the word
    size_t word_len = end - start;
    if (word_len >= word_size) {
        word_len = word_size - 1; // Leave space for null terminator
    }
    
    if (word_len > 0) {
        memcpy(word, input + start, word_len);
    }
    word[word_len] = '\0';
    
    *word_start = start;
    return true;
}

/**
 * @brief Complete file names and directories for shell command line
 *
 * Provides comprehensive file and directory completion by analyzing the
 * current word being typed and searching the appropriate directory for
 * matching entries. Handles both relative and absolute paths, prioritizes
 * directories over files, and provides exact match detection.
 *
 * @param input Full input string containing the command line
 * @param cursor_pos Current cursor position in input string
 * @param completions Completion list to populate with file/directory matches
 * @return true on success (even if no matches found), false on error
 *
 * @note Automatically extracts the word being completed from input
 * @note Hidden files are only shown if prefix starts with '.'
 * @note Directories are marked with trailing '/' and given higher priority
 * @note Exact matches receive highest priority for selection
 * @note Completions are automatically sorted by priority and name
 * @note Returns true even when no completions found (not an error condition)
 */
bool lle_complete_files(
    const char *input,
    size_t cursor_pos,
    lle_completion_list_t *completions
) {
    if (!input || !completions) {
        return false;
    }
    
    // Extract the word being completed
    char word[512];
    size_t word_start;
    if (!lle_completion_extract_word(input, cursor_pos, word, sizeof(word), &word_start)) {
        return false;
    }
    
    // Determine directory and filename prefix
    char dir_path[512];
    char filename_prefix[256];
    const char *last_slash = strrchr(word, '/');
    
    if (last_slash) {
        // Word contains a path separator
        size_t dir_len = last_slash - word;
        if (dir_len >= sizeof(dir_path)) {
            return false; // Path too long
        }
        
        memcpy(dir_path, word, dir_len);
        dir_path[dir_len] = '\0';
        
        // Handle empty directory (relative to root)
        if (dir_len == 0) {
            memcpy(dir_path, "/", 2);  // Copy "/" + null terminator
        }
        
        // Copy filename prefix
        const char *filename_start = last_slash + 1;
        size_t prefix_len = strlen(filename_start);
        if (prefix_len >= sizeof(filename_prefix)) {
            prefix_len = sizeof(filename_prefix) - 1;
        }
        memcpy(filename_prefix, filename_start, prefix_len);
        filename_prefix[prefix_len] = '\0';
    } else {
        // No path separator, complete in current directory
        memcpy(dir_path, ".", 2);  // Copy "." + null terminator
        size_t prefix_len = strlen(word);
        if (prefix_len >= sizeof(filename_prefix)) {
            prefix_len = sizeof(filename_prefix) - 1;
        }
        memcpy(filename_prefix, word, prefix_len);
        filename_prefix[prefix_len] = '\0';
    }
    
    // Open directory
    DIR *dir = opendir(dir_path);
    if (!dir) {
        // Directory doesn't exist or can't be opened
        return true; // Not an error, just no completions
    }
    
    // Read directory entries
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files unless prefix starts with '.'
        if (entry->d_name[0] == '.' && filename_prefix[0] != '.') {
            // Skip . and .. and other hidden files
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            continue;
        }
        
        // Check if filename matches prefix
        if (strncmp(entry->d_name, filename_prefix, strlen(filename_prefix)) != 0) {
            continue;
        }
        
        // Build full path for stat
        char full_path[1024];
        int ret = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (ret >= (int)sizeof(full_path) || ret < 0) {
            continue; // Path too long
        }
        
        // Get file information
        struct stat file_stat;
        bool is_dir = false;
        if (stat(full_path, &file_stat) == 0) {
            is_dir = S_ISDIR(file_stat.st_mode);
        }
        
        // Create completion text
        char completion_text[512];
        if (last_slash) {
            // Include the directory path in completion
            ret = snprintf(completion_text, sizeof(completion_text), "%.*s/%s%s",
                          (int)(last_slash - word), word, entry->d_name, is_dir ? "/" : "");
        } else {
            // Just the filename
            ret = snprintf(completion_text, sizeof(completion_text), "%s%s",
                          entry->d_name, is_dir ? "/" : "");
        }
        
        if (ret >= (int)sizeof(completion_text) || ret < 0) {
            continue; // Completion text too long
        }
        
        // Determine priority and description
        int priority = LLE_COMPLETION_PRIORITY_NORMAL;
        const char *description = NULL;
        
        if (is_dir) {
            priority = LLE_COMPLETION_PRIORITY_HIGH; // Prefer directories
            description = "directory";
        } else {
            description = "file";
        }
        
        // Check for exact match
        if (strcmp(entry->d_name, filename_prefix) == 0) {
            priority = LLE_COMPLETION_PRIORITY_EXACT;
        }
        
        // Add to completion list
        if (!lle_completion_list_add(completions, completion_text, description, priority)) {
            // Failed to add completion, continue with others
            continue;
        }
    }
    
    closedir(dir);
    
    // Sort completions by priority
    lle_completion_list_sort(completions);
    
    return true;
}