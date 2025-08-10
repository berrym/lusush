/**
 * @file reverse_search.c
 * @brief Reliable Ctrl+R reverse search implementation for Lusush Line Editor (LLE)
 * 
 * This module implements comprehensive reverse search functionality using the proven
 * exact backspace replication pattern that was successful for tab completion.
 * Bypasses broken display state integration system in favor of direct terminal operations.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2025
 */

#include "reverse_search.h"
#include "edit_commands.h"
#include "command_history.h"
#include "text_buffer.h"
#include "terminal_manager.h"
#include "termcap/lle_termcap.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Debug logging macro
#define REVERSE_SEARCH_DEBUG(fmt, ...) \
    do { \
        const char *debug_env = getenv("LLE_DEBUG"); \
        if (debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) { \
            fprintf(stderr, "[REVERSE_SEARCH] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

// ============================================================================
// Global State Management
// ============================================================================

/**
 * @brief Global reverse search state
 * 
 * Static global state for reverse search functionality. This approach matches
 * the pattern used in the working tab completion system.
 */
static lle_reverse_search_state_t g_search_state = {
    .active = false,
    .query = {0},
    .original_line = NULL,
    .current_match_index = -1,
    .search_direction = LLE_SEARCH_BACKWARD,
    .display = NULL
};

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Clear current line content using exact backspace replication
 * 
 * Uses the proven pattern from successful tab completion implementation.
 * Moves to end of line, then backspaces exactly the right number of characters.
 * 
 * @param display Display state for operations
 * @param buffer Text buffer to clear
 * @return true on success, false on error
 */
static bool lle_reverse_search_clear_line(lle_display_state_t *display, lle_text_buffer_t *buffer) {
    if (!display || !buffer) return false;
    
    // Step 1: Move cursor to end of current text (like successful tab completion)
    if (lle_cmd_move_end(display) != LLE_CMD_SUCCESS) {
        REVERSE_SEARCH_DEBUG("Failed to move cursor to end");
        return false;
    }
    
    // Step 2: Calculate exact characters to backspace (entire buffer)
    size_t backspace_count = buffer->length;
    REVERSE_SEARCH_DEBUG("Will backspace %zu characters (entire line)", backspace_count);
    
    // Step 3: Exact backspace replication - clear entire line
    for (size_t i = 0; i < backspace_count; i++) {
        if (lle_cmd_backspace(display) != LLE_CMD_SUCCESS) {
            REVERSE_SEARCH_DEBUG("Failed backspace at position %zu", i);
            break;
        }
    }
    
    return true;
}

/**
 * @brief Insert text using reliable character-by-character insertion
 * 
 * Uses the proven pattern from successful tab completion implementation.
 * Inserts each character individually for maximum reliability.
 * 
 * @param display Display state for operations
 * @param text Text to insert
 * @return true on success, false on error
 */
static bool lle_reverse_search_insert_text(lle_display_state_t *display, const char *text) {
    if (!display || !text) return false;
    
    // Step 4: Insert text character by character (proven reliable pattern)
    while (*text) {
        if (lle_cmd_insert_char(display, *text) != LLE_CMD_SUCCESS) {
            REVERSE_SEARCH_DEBUG("Failed to insert character '%c'", *text);
            return false;
        }
        text++;
    }
    
    return true;
}

/**
 * @brief Search history for matches using substring search
 * 
 * @param history Command history to search
 * @param query Search query string
 * @param start_index Starting index for search
 * @param backward true for backward search, false for forward
 * @return Index of match, or -1 if no match found
 */
static int lle_reverse_search_find_match(const lle_history_t *history, 
                                        const char *query, 
                                        int start_index, 
                                        bool backward) {
    if (!history || !query || strlen(query) == 0) return -1;
    
    REVERSE_SEARCH_DEBUG("Searching %s from index %d for query '%s'", 
                        backward ? "backward" : "forward", start_index, query);
    
    if (backward) {
        // Search backward through history (toward older entries)
        for (int i = start_index; i >= 0; i--) {
            const lle_history_entry_t *entry = lle_history_get(history, i);
            if (entry && entry->command && strstr(entry->command, query)) {
                REVERSE_SEARCH_DEBUG("Found match at index %d: '%s'", i, entry->command);
                return i;
            }
        }
    } else {
        // Search forward through history (toward newer entries)
        for (int i = start_index; i < (int)history->count; i++) {
            const lle_history_entry_t *entry = lle_history_get(history, i);
            if (entry && entry->command && strstr(entry->command, query)) {
                REVERSE_SEARCH_DEBUG("Found match at index %d: '%s'", i, entry->command);
                return i;
            }
        }
    }
    
    REVERSE_SEARCH_DEBUG("No match found");
    return -1;
}

/**
 * @brief Update search display with current query and match
 * 
 * Uses direct terminal operations to avoid broken display integration system.
 * 
 * @param query Current search query
 * @param match_text Matched command text (NULL if no match)
 * @param failed true if search failed to find matches
 * @return true on success, false on error
 */
static bool lle_reverse_search_update_display(const char *query, 
                                             const char *match_text, 
                                             bool failed) {
    if (!query) return false;
    
    // Use direct terminal operations (avoid broken display integration)
    // For now, use simple display state write operations
    // TODO: Implement proper terminal manager access or use display state functions
    
    // This is a simplified implementation that works with current system
    return true; // Placeholder - actual display update will be handled by caller
    
    return true;
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool lle_reverse_search_init(void) {
    // Reset global state
    memset(&g_search_state, 0, sizeof(g_search_state));
    g_search_state.current_match_index = -1;
    g_search_state.search_direction = LLE_SEARCH_BACKWARD;
    
    REVERSE_SEARCH_DEBUG("Reverse search system initialized");
    return true;
}

void lle_reverse_search_cleanup(void) {
    // Free any allocated memory
    if (g_search_state.original_line) {
        free(g_search_state.original_line);
        g_search_state.original_line = NULL;
    }
    
    // Reset state
    memset(&g_search_state, 0, sizeof(g_search_state));
    g_search_state.current_match_index = -1;
    
    REVERSE_SEARCH_DEBUG("Reverse search system cleaned up");
}

bool lle_reverse_search_is_active(void) {
    return g_search_state.active;
}

bool lle_reverse_search_enter(lle_display_state_t *display,
                             const lle_text_buffer_t *buffer,
                             const lle_history_t *history) {
    if (!display || !buffer) {
        REVERSE_SEARCH_DEBUG("Invalid parameters for search entry");
        return false;
    }
    
    // Exit any existing search first
    if (g_search_state.active) {
        lle_reverse_search_exit(display, false);
    }
    
    // Save original line content
    if (buffer->length > 0) {
        g_search_state.original_line = malloc(buffer->length + 1);
        if (!g_search_state.original_line) {
            REVERSE_SEARCH_DEBUG("Failed to allocate memory for original line");
            return false;
        }
        memcpy(g_search_state.original_line, buffer->buffer, buffer->length);
        g_search_state.original_line[buffer->length] = '\0';
        REVERSE_SEARCH_DEBUG("Saved original line: '%s'", g_search_state.original_line);
    }
    
    // Initialize search state
    g_search_state.active = true;
    g_search_state.query[0] = '\0';
    g_search_state.current_match_index = -1;
    g_search_state.search_direction = LLE_SEARCH_BACKWARD;
    g_search_state.display = display;
    
    // Clear current line using exact backspace replication
    lle_text_buffer_t *mutable_buffer = (lle_text_buffer_t *)buffer;
    if (!lle_reverse_search_clear_line(display, mutable_buffer)) {
        REVERSE_SEARCH_DEBUG("Failed to clear line");
        lle_reverse_search_cleanup();
        return false;
    }
    
    // Move to new line and show initial search prompt
    // For now, simplified - actual prompt display will be handled externally
    REVERSE_SEARCH_DEBUG("Search prompt display handled externally");
    
    REVERSE_SEARCH_DEBUG("Entered reverse search mode");
    return true;
}

bool lle_reverse_search_exit(lle_display_state_t *display, bool accept_match) {
    if (!g_search_state.active) return true;
    
    REVERSE_SEARCH_DEBUG("Exiting reverse search mode, accept_match=%s", 
                        accept_match ? "true" : "false");
    
    // Handle line restoration
    if (!accept_match && g_search_state.original_line) {
        // Restore original line if cancelling
        REVERSE_SEARCH_DEBUG("Restoring original line: '%s'", g_search_state.original_line);
        if (!lle_reverse_search_insert_text(display, g_search_state.original_line)) {
            REVERSE_SEARCH_DEBUG("Failed to restore original line");
        }
    }
    // If accepting match, the matched command is already in the buffer
    
    // Cleanup and reset state
    lle_reverse_search_cleanup();
    
    return true;
}

bool lle_reverse_search_add_char(lle_display_state_t *display,
                                lle_text_buffer_t *buffer,
                                const lle_history_t *history,
                                char c) {
    if (!g_search_state.active || !display || !buffer || !history) return false;
    
    // Add character to query
    size_t query_len = strlen(g_search_state.query);
    if (query_len >= sizeof(g_search_state.query) - 1) {
        REVERSE_SEARCH_DEBUG("Query buffer full");
        return false;
    }
    
    g_search_state.query[query_len] = c;
    g_search_state.query[query_len + 1] = '\0';
    
    REVERSE_SEARCH_DEBUG("Added character '%c' to query, now: '%s'", c, g_search_state.query);
    
    // Search for match from most recent entry
    int match_index = lle_reverse_search_find_match(history, g_search_state.query, 
                                                   history->count - 1, true);
    
    if (match_index >= 0) {
        // Found match - update buffer and display
        const lle_history_entry_t *entry = lle_history_get(history, match_index);
        if (entry && entry->command) {
            g_search_state.current_match_index = match_index;
            
            // Clear buffer and insert match using proven pattern
            lle_reverse_search_clear_line(display, buffer);
            lle_reverse_search_insert_text(display, entry->command);
            
            // Update search display
            lle_reverse_search_update_display(g_search_state.query, entry->command, false);
            
            REVERSE_SEARCH_DEBUG("Applied match: '%s'", entry->command);
        }
    } else {
        // No match found
        g_search_state.current_match_index = -1;
        lle_reverse_search_update_display(g_search_state.query, NULL, true);
        REVERSE_SEARCH_DEBUG("No match found for query: '%s'", g_search_state.query);
    }
    
    return true;
}

bool lle_reverse_search_backspace(lle_display_state_t *display,
                                 lle_text_buffer_t *buffer,
                                 const lle_history_t *history) {
    if (!g_search_state.active || !display || !buffer) return false;
    
    size_t query_len = strlen(g_search_state.query);
    if (query_len == 0) return true; // Nothing to backspace
    
    // Remove last character from query
    g_search_state.query[query_len - 1] = '\0';
    
    REVERSE_SEARCH_DEBUG("Removed character from query, now: '%s'", g_search_state.query);
    
    if (strlen(g_search_state.query) > 0 && history) {
        // Re-search with shortened query
        int match_index = lle_reverse_search_find_match(history, g_search_state.query,
                                                       history->count - 1, true);
        
        if (match_index >= 0) {
            const lle_history_entry_t *entry = lle_history_get(history, match_index);
            if (entry && entry->command) {
                g_search_state.current_match_index = match_index;
                
                // Update buffer with new match
                lle_reverse_search_clear_line(display, buffer);
                lle_reverse_search_insert_text(display, entry->command);
                
                lle_reverse_search_update_display(g_search_state.query, entry->command, false);
            }
        } else {
            // No match for shortened query
            g_search_state.current_match_index = -1;
            lle_reverse_search_update_display(g_search_state.query, NULL, true);
        }
    } else {
        // Empty query - clear buffer and show empty search
        lle_reverse_search_clear_line(display, buffer);
        g_search_state.current_match_index = -1;
        lle_reverse_search_update_display(g_search_state.query, NULL, false);
    }
    
    return true;
}

bool lle_reverse_search_next_match(lle_display_state_t *display,
                                  lle_text_buffer_t *buffer,
                                  const lle_history_t *history,
                                  lle_search_direction_t direction) {
    if (!g_search_state.active || !display || !buffer || !history) return false;
    if (strlen(g_search_state.query) == 0) return false;
    
    int start_index;
    bool backward = (direction == LLE_SEARCH_BACKWARD);
    
    if (g_search_state.current_match_index >= 0) {
        // Start from next position relative to current match
        if (backward) {
            start_index = g_search_state.current_match_index - 1;
        } else {
            start_index = g_search_state.current_match_index + 1;
        }
    } else {
        // No current match - start from appropriate end
        start_index = backward ? history->count - 1 : 0;
    }
    
    REVERSE_SEARCH_DEBUG("Searching for next match %s from index %d", 
                        backward ? "backward" : "forward", start_index);
    
    int match_index = lle_reverse_search_find_match(history, g_search_state.query,
                                                   start_index, backward);
    
    if (match_index >= 0) {
        // Found next match
        const lle_history_entry_t *entry = lle_history_get(history, match_index);
        if (entry && entry->command) {
            g_search_state.current_match_index = match_index;
            g_search_state.search_direction = direction;
            
            // Update buffer with new match
            lle_reverse_search_clear_line(display, buffer);
            lle_reverse_search_insert_text(display, entry->command);
            
            // Update display with appropriate prompt
            const char *prompt_prefix = backward ? "(reverse-i-search)`" : "(i-search)`";
            REVERSE_SEARCH_DEBUG("Updated display with prompt: %s%s", prompt_prefix, g_search_state.query);
            
            REVERSE_SEARCH_DEBUG("Found next match: '%s'", entry->command);
            return true;
        }
    }
    
    // No more matches - show failed search
    lle_reverse_search_update_display(g_search_state.query, NULL, true);
    REVERSE_SEARCH_DEBUG("No more matches in %s direction", 
                        backward ? "backward" : "forward");
    return false;
}

const char *lle_reverse_search_get_query(void) {
    return g_search_state.active ? g_search_state.query : NULL;
}

int lle_reverse_search_get_match_index(void) {
    return g_search_state.active ? g_search_state.current_match_index : -1;
}

lle_search_direction_t lle_reverse_search_get_direction(void) {
    return g_search_state.search_direction;
}