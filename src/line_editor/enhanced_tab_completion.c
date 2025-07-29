/*
 * Enhanced Tab Completion System for Lusush Line Editor (LLE)
 * 
 * This module provides a robust tab completion system that fixes cross-platform
 * issues and provides consistent behavior across macOS/iTerm2 and Linux/Konsole.
 * 
 * Key improvements over the original:
 * - Fixed completion cycling state management
 * - Proper word boundary detection
 * - Cross-platform key event handling
 * - Enhanced debugging and logging
 * - Multiple completion types (files, commands, variables)
 * - Intelligent completion context detection
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include "enhanced_tab_completion.h"
#include "completion.h"
#include "text_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Forward declaration for strncasecmp
int strncasecmp(const char *s1, const char *s2, size_t n);

// ============================================================================
// Enhanced Completion State Management
// ============================================================================

/**
 * @brief Enhanced completion session state
 */
typedef struct {
    bool active;                    /**< Whether completion session is active */
    char original_word[512];        /**< Original word being completed */
    char current_prefix[512];       /**< Current prefix for filtering */
    size_t word_start_pos;          /**< Start position of word in buffer */
    size_t word_end_pos;            /**< End position of word in buffer */
    size_t original_cursor_pos;     /**< Original cursor position */
    int completion_index;           /**< Current completion index */
    int total_completions;          /**< Total number of completions */
    lle_enhanced_completion_type_t completion_type; /**< Type of completion */
    uint32_t session_id;            /**< Unique session identifier */
    bool has_cycled;                /**< Whether user has cycled through completions */
    char last_applied_completion[512]; /**< Last completion that was applied */
} lle_enhanced_completion_state_t;

static lle_enhanced_completion_state_t g_completion_state = {0};
static uint32_t g_next_session_id = 1;
static bool g_debug_mode = false;

// ============================================================================
// Debug and Logging Functions
// ============================================================================

/**
 * @brief Initialize debug mode from environment
 */
static void init_debug_mode(void) {
    static bool initialized = false;
    if (initialized) return;
    
    const char *debug_env = getenv("LLE_DEBUG_COMPLETION");
    if (!debug_env) {
        debug_env = getenv("LLE_DEBUG");
    }
    g_debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    initialized = true;
}

/**
 * @brief Debug logging macro
 */
#define COMPLETION_DEBUG(fmt, ...) do { \
    if (g_debug_mode) { \
        fprintf(stderr, "[ENHANCED_TAB_COMPLETION] " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

// ============================================================================
// Word Extraction and Boundary Detection
// ============================================================================

/**
 * @brief Check if character is a word separator
 */
static bool is_word_separator(char c) {
    return isspace(c) || c == '\0' || c == '|' || c == '&' || c == ';' || 
           c == '(' || c == ')' || c == '<' || c == '>' || c == '"' || 
           c == '\'' || c == '`' || c == '$' || c == '#';
}

/**
 * @brief Check if character is a path separator
 */
static bool is_path_separator(char c) {
    return c == '/' || c == '\\';
}

/**
 * @brief Enhanced word extraction with proper boundary detection
 */
static bool extract_completion_word(const char *buffer, size_t cursor_pos,
                                   char *word_out, size_t word_size,
                                   size_t *word_start_out, size_t *word_end_out) {
    if (!buffer || !word_out || !word_start_out || !word_end_out) {
        return false;
    }
    
    size_t buffer_len = strlen(buffer);
    if (cursor_pos > buffer_len) {
        cursor_pos = buffer_len;
    }
    
    // Find start of word (go backward from cursor)
    size_t word_start = cursor_pos;
    while (word_start > 0 && !is_word_separator(buffer[word_start - 1])) {
        word_start--;
    }
    
    // Find end of word (go forward from cursor)
    size_t word_end = cursor_pos;
    while (word_end < buffer_len && !is_word_separator(buffer[word_end])) {
        word_end++;
    }
    
    // Extract the word
    size_t word_len = word_end - word_start;
    if (word_len >= word_size) {
        word_len = word_size - 1;
    }
    
    memcpy(word_out, buffer + word_start, word_len);
    word_out[word_len] = '\0';
    
    *word_start_out = word_start;
    *word_end_out = word_end;
    
    COMPLETION_DEBUG("Extracted word: '%s' at pos %zu-%zu (cursor at %zu)", 
                    word_out, word_start, word_end, cursor_pos);
    
    return true;
}

// ============================================================================
// Completion Type Detection
// ============================================================================

/**
 * @brief Determine the type of completion needed based on context
 */
static lle_enhanced_completion_type_t detect_completion_type(const char *buffer, 
                                                           size_t word_start_pos) {
    if (!buffer) {
        return LLE_ENHANCED_COMPLETION_FILE;
    }
    
    // Check if we're at the beginning of the line (command completion)
    size_t pos = 0;
    while (pos < word_start_pos && isspace(buffer[pos])) {
        pos++;
    }
    
    if (pos == word_start_pos) {
        return LLE_ENHANCED_COMPLETION_COMMAND;
    }
    
    // Check for variable completion
    if (word_start_pos > 0 && buffer[word_start_pos - 1] == '$') {
        return LLE_ENHANCED_COMPLETION_VARIABLE;
    }
    
    // Check for path completion (contains / or starts with ~ or .)
    if (word_start_pos < strlen(buffer)) {
        const char *word_start = buffer + word_start_pos;
        if (*word_start == '~' || *word_start == '.' || strchr(word_start, '/')) {
            return LLE_ENHANCED_COMPLETION_PATH;
        }
    }
    
    // Default to file completion
    return LLE_ENHANCED_COMPLETION_FILE;
}

// ============================================================================
// File and Directory Completion
// ============================================================================

/**
 * @brief Add file completions to the completion list
 */
static bool add_file_completions(lle_completion_list_t *completions, 
                                const char *prefix, const char *directory) {
    if (!completions || !prefix) {
        return false;
    }
    
    const char *dir_path = directory ? directory : ".";
    DIR *dir = opendir(dir_path);
    if (!dir) {
        COMPLETION_DEBUG("Failed to open directory: %s", dir_path);
        return false;
    }
    
    struct dirent *entry;
    int added_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip hidden files unless prefix starts with '.'
        if (entry->d_name[0] == '.' && prefix[0] != '.') {
            continue;
        }
        
        // Check if filename matches prefix (case-insensitive)
        if (strncasecmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            // Determine if it's a directory
            struct stat file_stat;
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            
            bool is_directory = false;
            if (stat(full_path, &file_stat) == 0) {
                is_directory = S_ISDIR(file_stat.st_mode);
            }
            
            // Add appropriate suffix for directories
            char completion_text[512];
            if (is_directory) {
                snprintf(completion_text, sizeof(completion_text), "%s/", entry->d_name);
            } else {
                strncpy(completion_text, entry->d_name, sizeof(completion_text) - 1);
                completion_text[sizeof(completion_text) - 1] = '\0';
            }
            
            int priority = is_directory ? LLE_COMPLETION_PRIORITY_HIGH : LLE_COMPLETION_PRIORITY_NORMAL;
            const char *description = is_directory ? "directory" : "file";
            
            if (lle_completion_list_add(completions, completion_text, description, priority)) {
                added_count++;
                COMPLETION_DEBUG("Added %s completion: %s", description, completion_text);
            }
        }
    }
    
    closedir(dir);
    COMPLETION_DEBUG("Added %d file completions for prefix '%s'", added_count, prefix);
    return added_count > 0;
}

/**
 * @brief Add command completions to the completion list
 */
static bool add_command_completions(lle_completion_list_t *completions, const char *prefix) {
    if (!completions || !prefix) {
        return false;
    }
    
    // Common shell commands
    static const char *common_commands[] = {
        "ls", "cd", "pwd", "echo", "cat", "grep", "find", "which", "man",
        "cp", "mv", "rm", "mkdir", "rmdir", "touch", "chmod", "chown",
        "ps", "kill", "jobs", "bg", "fg", "nohup", "top", "htop",
        "git", "vim", "nano", "emacs", "less", "more", "head", "tail",
        "sort", "uniq", "wc", "cut", "awk", "sed", "tar", "gzip", "zip",
        "ssh", "scp", "rsync", "curl", "wget", "ping", "netstat",
        "history", "alias", "export", "source", "exec", "eval", "test"
    };
    
    int added_count = 0;
    size_t prefix_len = strlen(prefix);
    
    for (size_t i = 0; i < sizeof(common_commands) / sizeof(common_commands[0]); i++) {
        if (strncmp(common_commands[i], prefix, prefix_len) == 0) {
            if (lle_completion_list_add(completions, common_commands[i], 
                                      "command", LLE_COMPLETION_PRIORITY_HIGH)) {
                added_count++;
                COMPLETION_DEBUG("Added command completion: %s", common_commands[i]);
            }
        }
    }
    
    // Also add file completions for potential scripts
    add_file_completions(completions, prefix, NULL);
    
    COMPLETION_DEBUG("Added %d command completions for prefix '%s'", added_count, prefix);
    return added_count > 0;
}

/**
 * @brief Add variable completions to the completion list
 */
static bool add_variable_completions(lle_completion_list_t *completions, const char *prefix) {
    if (!completions || !prefix) {
        return false;
    }
    
    // Common environment variables
    static const char *common_vars[] = {
        "HOME", "PATH", "USER", "SHELL", "PWD", "OLDPWD", "TERM", "EDITOR",
        "DISPLAY", "LANG", "LC_ALL", "PS1", "PS2", "IFS", "HISTFILE",
        "HISTSIZE", "HISTCONTROL", "HOSTNAME", "LOGNAME", "MAIL"
    };
    
    int added_count = 0;
    size_t prefix_len = strlen(prefix);
    
    for (size_t i = 0; i < sizeof(common_vars) / sizeof(common_vars[0]); i++) {
        if (strncmp(common_vars[i], prefix, prefix_len) == 0) {
            if (lle_completion_list_add(completions, common_vars[i], 
                                      "variable", LLE_COMPLETION_PRIORITY_HIGH)) {
                added_count++;
                COMPLETION_DEBUG("Added variable completion: %s", common_vars[i]);
            }
        }
    }
    
    COMPLETION_DEBUG("Added %d variable completions for prefix '%s'", added_count, prefix);
    return added_count > 0;
}

// ============================================================================
// Enhanced Completion Session Management
// ============================================================================

/**
 * @brief Start a new completion session
 */
static bool start_completion_session(const char *buffer, size_t cursor_pos,
                                   lle_completion_list_t *completions) {
    if (!buffer || !completions) {
        return false;
    }
    
    // Extract the word at cursor
    char word[512];
    size_t word_start, word_end;
    if (!extract_completion_word(buffer, cursor_pos, word, sizeof(word), 
                                &word_start, &word_end)) {
        COMPLETION_DEBUG("Failed to extract completion word");
        return false;
    }
    
    // Initialize session state
    memset(&g_completion_state, 0, sizeof(g_completion_state));
    g_completion_state.active = true;
    g_completion_state.session_id = g_next_session_id++;
    g_completion_state.word_start_pos = word_start;
    g_completion_state.word_end_pos = word_end;
    g_completion_state.original_cursor_pos = cursor_pos;
    g_completion_state.completion_index = 0;
    g_completion_state.has_cycled = false;
    
    strncpy(g_completion_state.original_word, word, sizeof(g_completion_state.original_word) - 1);
    strncpy(g_completion_state.current_prefix, word, sizeof(g_completion_state.current_prefix) - 1);
    
    // Detect completion type
    g_completion_state.completion_type = detect_completion_type(buffer, word_start);
    
    COMPLETION_DEBUG("Started completion session %u: word='%s', type=%d, pos=%zu-%zu",
                    g_completion_state.session_id, word, g_completion_state.completion_type,
                    word_start, word_end);
    
    // Clear existing completions
    lle_completion_list_clear(completions);
    
    // Generate completions based on type
    bool success = false;
    switch (g_completion_state.completion_type) {
        case LLE_ENHANCED_COMPLETION_FILE:
            success = add_file_completions(completions, word, NULL);
            break;
        case LLE_ENHANCED_COMPLETION_COMMAND:
            success = add_command_completions(completions, word);
            break;
        case LLE_ENHANCED_COMPLETION_VARIABLE:
            success = add_variable_completions(completions, word);
            break;
        case LLE_ENHANCED_COMPLETION_PATH:
            // TODO: Implement path-specific completion with directory parsing
            success = add_file_completions(completions, word, NULL);
            break;
    }
    
    if (success && completions->count > 0) {
        // Sort completions for consistent order
        lle_completion_list_sort(completions);
        g_completion_state.total_completions = completions->count;
        
        COMPLETION_DEBUG("Generated %d completions", g_completion_state.total_completions);
        return true;
    } else {
        // No completions found - end session
        g_completion_state.active = false;
        COMPLETION_DEBUG("No completions found - session ended");
        return false;
    }
}

/**
 * @brief Continue an existing completion session (cycle to next)
 */
static bool continue_completion_session(lle_completion_list_t *completions) {
    if (!g_completion_state.active || !completions || completions->count == 0) {
        COMPLETION_DEBUG("Cannot continue session: not active or no completions");
        return false;
    }
    
    // Cycle to next completion
    g_completion_state.completion_index = (g_completion_state.completion_index + 1) % completions->count;
    g_completion_state.has_cycled = true;
    
    // Update list selection
    lle_completion_list_set_selected(completions, g_completion_state.completion_index);
    
    COMPLETION_DEBUG("Cycled to completion %d/%d", 
                    g_completion_state.completion_index + 1, g_completion_state.total_completions);
    
    return true;
}

/**
 * @brief End the current completion session
 */
static void end_completion_session(void) {
    if (g_completion_state.active) {
        COMPLETION_DEBUG("Ended completion session %u", g_completion_state.session_id);
        memset(&g_completion_state, 0, sizeof(g_completion_state));
    }
}

/**
 * @brief Check if this tab press should continue the current session
 */
static bool should_continue_session(const char *buffer, size_t cursor_pos) {
    if (!g_completion_state.active || !buffer) {
        return false;
    }
    
    // Extract current word
    char current_word[512];
    size_t word_start, word_end;
    if (!extract_completion_word(buffer, cursor_pos, current_word, sizeof(current_word),
                                &word_start, &word_end)) {
        return false;
    }
    
    // Check if cursor is still in the same completion region
    // Allow for the completion to have been applied (cursor moved)
    bool cursor_in_region = (cursor_pos >= g_completion_state.word_start_pos &&
                            cursor_pos <= word_end);
    
    // Check if we're still working on a completion that was applied
    bool working_on_completion = g_completion_state.has_cycled &&
                                (word_start == g_completion_state.word_start_pos);
    
    bool should_continue = cursor_in_region || working_on_completion;
    
    COMPLETION_DEBUG("Should continue session: cursor_in_region=%s, working_on_completion=%s, result=%s",
                    cursor_in_region ? "yes" : "no",
                    working_on_completion ? "yes" : "no", 
                    should_continue ? "yes" : "no");
    
    return should_continue;
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool lle_enhanced_tab_completion_init(void) {
    init_debug_mode();
    memset(&g_completion_state, 0, sizeof(g_completion_state));
    g_next_session_id = 1;
    
    COMPLETION_DEBUG("Enhanced tab completion initialized");
    return true;
}

void lle_enhanced_tab_completion_cleanup(void) {
    end_completion_session();
    COMPLETION_DEBUG("Enhanced tab completion cleaned up");
}

bool lle_enhanced_tab_completion_handle(lle_text_buffer_t *buffer,
                                      lle_completion_list_t *completions) {
    if (!buffer || !completions) {
        return false;
    }
    
    init_debug_mode();
    
    COMPLETION_DEBUG("Handling tab completion: cursor at %zu, buffer length %zu",
                    buffer->cursor_pos, buffer->length);
    
    // Check if we should continue an existing session
    if (should_continue_session(buffer->buffer, buffer->cursor_pos)) {
        if (continue_completion_session(completions)) {
            // Apply the next completion
            const lle_completion_item_t *item = lle_completion_list_get_selected(completions);
            if (item) {
                // Replace the current word with the selected completion
                size_t replace_start = g_completion_state.word_start_pos;
                size_t replace_end = buffer->cursor_pos;
                
                // If we've cycled, we need to replace the entire applied completion
                if (g_completion_state.has_cycled) {
                    // Find the end of the current word
                    while (replace_end < buffer->length && 
                           !is_word_separator(buffer->buffer[replace_end])) {
                        replace_end++;
                    }
                }
                
                // Delete current text
                if (replace_end > replace_start) {
                    if (!lle_text_delete_range(buffer, replace_start, replace_end)) {
                        COMPLETION_DEBUG("Failed to delete range for replacement");
                        return false;
                    }
                }
                
                // Insert new completion
                if (!lle_text_insert_at(buffer, replace_start, item->text)) {
                    COMPLETION_DEBUG("Failed to insert completion text");
                    return false;
                }
                
                // Update cursor position
                buffer->cursor_pos = replace_start + item->text_len;
                
                // Update session state
                strncpy(g_completion_state.last_applied_completion, item->text,
                       sizeof(g_completion_state.last_applied_completion) - 1);
                
                COMPLETION_DEBUG("Applied completion: '%s' (index %d/%d)",
                               item->text, g_completion_state.completion_index + 1,
                               g_completion_state.total_completions);
                
                return true;
            }
        }
    } else {
        // Start new completion session
        end_completion_session(); // End any previous session
        
        if (start_completion_session(buffer->buffer, buffer->cursor_pos, completions)) {
            // Apply the first completion
            const lle_completion_item_t *item = lle_completion_list_get_selected(completions);
            if (item) {
                // Replace the current word with the first completion
                size_t replace_start = g_completion_state.word_start_pos;
                size_t replace_end = g_completion_state.word_end_pos;
                
                // Delete current word
                if (replace_end > replace_start) {
                    if (!lle_text_delete_range(buffer, replace_start, replace_end)) {
                        COMPLETION_DEBUG("Failed to delete word for completion");
                        return false;
                    }
                }
                
                // Insert completion
                if (!lle_text_insert_at(buffer, replace_start, item->text)) {
                    COMPLETION_DEBUG("Failed to insert completion");
                    return false;
                }
                
                // Update cursor position
                buffer->cursor_pos = replace_start + item->text_len;
                
                // Update session state
                strncpy(g_completion_state.last_applied_completion, item->text,
                       sizeof(g_completion_state.last_applied_completion) - 1);
                
                COMPLETION_DEBUG("Applied first completion: '%s' (%d available)",
                               item->text, g_completion_state.total_completions);
                
                return true;
            }
        } else {
            COMPLETION_DEBUG("No completions available");
        }
    }
    
    return false;
}

void lle_enhanced_tab_completion_reset(void) {
    end_completion_session();
    COMPLETION_DEBUG("Completion session reset");
}

bool lle_enhanced_tab_completion_is_active(void) {
    return g_completion_state.active;
}

const lle_enhanced_completion_info_t *lle_enhanced_tab_completion_get_info(void) {
    static lle_enhanced_completion_info_t info;
    
    if (!g_completion_state.active) {
        memset(&info, 0, sizeof(info));
        return &info;
    }
    
    info.active = g_completion_state.active;
    info.completion_type = g_completion_state.completion_type;
    info.current_index = g_completion_state.completion_index;
    info.total_count = g_completion_state.total_completions;
    info.has_cycled = g_completion_state.has_cycled;
    
    strncpy(info.original_word, g_completion_state.original_word, sizeof(info.original_word) - 1);
    strncpy(info.current_completion, g_completion_state.last_applied_completion, 
           sizeof(info.current_completion) - 1);
    
    return &info;
}

const char *lle_enhanced_tab_completion_get_debug_info(void) {
    static char debug_buffer[1024];
    
    if (!g_completion_state.active) {
        strcpy(debug_buffer, "No active completion session");
        return debug_buffer;
    }
    
    const char *type_names[] = {
        "file", "command", "variable", "path"
    };
    
    snprintf(debug_buffer, sizeof(debug_buffer),
            "Enhanced Tab Completion Debug Info:\n"
            "  Session ID: %u\n"
            "  Active: %s\n"
            "  Type: %s\n"
            "  Original word: '%s'\n"
            "  Current completion: '%s'\n"
            "  Index: %d/%d\n"
            "  Has cycled: %s\n"
            "  Word position: %zu-%zu\n"
            "  Cursor position: %zu",
            g_completion_state.session_id,
            g_completion_state.active ? "yes" : "no",
            (g_completion_state.completion_type < 4) ? 
                type_names[g_completion_state.completion_type] : "unknown",
            g_completion_state.original_word,
            g_completion_state.last_applied_completion,
            g_completion_state.completion_index + 1,
            g_completion_state.total_completions,
            g_completion_state.has_cycled ? "yes" : "no",
            g_completion_state.word_start_pos,
            g_completion_state.word_end_pos,
            g_completion_state.original_cursor_pos);
    
    return debug_buffer;
}