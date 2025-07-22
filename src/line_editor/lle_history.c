/**
 * @file lle_history.c
 * @brief Command history implementation for Lusush Line Editor (LLE)
 * 
 * This module implements command history management for the line editor,
 * providing efficient storage, navigation, and retrieval of previously
 * entered commands with configurable size limits.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "lle_history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Get current timestamp
 * @return Current Unix timestamp
 */
static uint64_t lle_history_get_timestamp(void) {
    return (uint64_t)time(NULL);
}

/**
 * @brief Free a history entry
 * @param entry Entry to free
 */
static void lle_history_free_entry(lle_history_entry_t *entry) {
    if (entry && entry->command) {
        free(entry->command);
        entry->command = NULL;
        entry->length = 0;
        entry->timestamp = 0;
        entry->persistent = false;
    }
}

/**
 * @brief Calculate circular buffer index
 * @param history History structure
 * @param logical_index Logical index (0 = oldest)
 * @return Actual array index
 */
static size_t lle_history_get_actual_index(const lle_history_t *history, size_t logical_index) {
    if (!history || logical_index >= history->count) {
        return LLE_HISTORY_INVALID_POSITION;
    }
    
    if (!history->is_full) {
        return logical_index;
    }
    
    return (history->oldest_index + logical_index) % history->capacity;
}

/**
 * @brief Validate history structure
 * @param history History structure to validate
 * @return true if valid, false otherwise
 */
static bool lle_history_validate(const lle_history_t *history) {
    if (!history) {
        return false;
    }
    
    if (history->capacity == 0 || !history->entries) {
        return false;
    }
    
    if (history->count > history->capacity) {
        return false;
    }
    
    if (history->max_entries < LLE_HISTORY_MIN_ENTRIES || 
        history->max_entries > LLE_HISTORY_MAX_ENTRIES) {
        return false;
    }
    
    if (history->oldest_index >= history->capacity) {
        return false;
    }
    
    return true;
}

// Forward declarations for helper functions
static size_t lle_history_find_command(const lle_history_t *history, const char *command);
static bool lle_history_remove_at_index(lle_history_t *history, size_t logical_index);

// ============================================================================
// Public API Implementation
// ============================================================================

lle_history_t *lle_history_create(size_t max_entries, bool no_duplicates) {
    // Use default if max_entries is 0
    if (max_entries == 0) {
        max_entries = LLE_HISTORY_DEFAULT_MAX_ENTRIES;
    }
    
    // Validate max_entries range
    if (max_entries < LLE_HISTORY_MIN_ENTRIES || 
        max_entries > LLE_HISTORY_MAX_ENTRIES) {
        return NULL;
    }
    
    // Allocate history structure
    lle_history_t *history = malloc(sizeof(lle_history_t));
    if (!history) {
        return NULL;
    }
    
    // Initialize structure
    if (!lle_history_init(history, max_entries, no_duplicates)) {
        free(history);
        return NULL;
    }
    
    return history;
}

bool lle_history_init(lle_history_t *history, size_t max_entries, bool no_duplicates) {
    if (!history) {
        return false;
    }
    
    // Use default if max_entries is 0
    if (max_entries == 0) {
        max_entries = LLE_HISTORY_DEFAULT_MAX_ENTRIES;
    }
    
    // Validate max_entries range
    if (max_entries < LLE_HISTORY_MIN_ENTRIES || 
        max_entries > LLE_HISTORY_MAX_ENTRIES) {
        return false;
    }
    
    // Allocate entries array
    history->entries = calloc(max_entries, sizeof(lle_history_entry_t));
    if (!history->entries) {
        return false;
    }
    
    // Initialize structure fields
    history->count = 0;
    history->capacity = max_entries;
    history->current = LLE_HISTORY_INVALID_POSITION;
    history->max_entries = max_entries;
    history->oldest_index = 0;
    history->is_full = false;
    history->navigation_mode = false;
    history->no_duplicates = no_duplicates;
    history->temp_buffer = NULL;
    history->temp_length = 0;
    
    return true;
}

void lle_history_destroy(lle_history_t *history) {
    if (!history) {
        return;
    }
    
    // Free all entries
    if (history->entries) {
        for (size_t i = 0; i < history->count; i++) {
            size_t actual_index = lle_history_get_actual_index(history, i);
            if (actual_index != LLE_HISTORY_INVALID_POSITION) {
                lle_history_free_entry(&history->entries[actual_index]);
            }
        }
        free(history->entries);
        history->entries = NULL;
    }
    
    // Free temporary buffer
    if (history->temp_buffer) {
        free(history->temp_buffer);
        history->temp_buffer = NULL;
    }
    
    // Clear structure fields
    history->count = 0;
    history->capacity = 0;
    history->current = LLE_HISTORY_INVALID_POSITION;
    history->max_entries = 0;
    history->oldest_index = 0;
    history->is_full = false;
    history->navigation_mode = false;
    history->no_duplicates = false;
    history->temp_length = 0;
    
    // Free the structure itself (this function assumes heap allocation)
    free(history);
}

/**
 * @brief Clean up a stack-allocated history structure
 * 
 * Cleans up a stack-allocated history structure without freeing the
 * structure itself. Use this for histories initialized with lle_history_init().
 * 
 * @param history History structure to clean up
 */
void lle_history_cleanup(lle_history_t *history) {
    if (!history) {
        return;
    }
    
    // Free all entries
    if (history->entries) {
        for (size_t i = 0; i < history->count; i++) {
            size_t actual_index = lle_history_get_actual_index(history, i);
            if (actual_index != LLE_HISTORY_INVALID_POSITION) {
                lle_history_free_entry(&history->entries[actual_index]);
            }
        }
        free(history->entries);
        history->entries = NULL;
    }
    
    // Free temporary buffer
    if (history->temp_buffer) {
        free(history->temp_buffer);
        history->temp_buffer = NULL;
    }
    
    // Clear structure fields
    history->count = 0;
    history->capacity = 0;
    history->current = LLE_HISTORY_INVALID_POSITION;
    history->max_entries = 0;
    history->oldest_index = 0;
    history->is_full = false;
    history->navigation_mode = false;
    history->temp_length = 0;
}

bool lle_history_clear(lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return false;
    }
    
    // Free all entries
    for (size_t i = 0; i < history->count; i++) {
        size_t actual_index = lle_history_get_actual_index(history, i);
        if (actual_index != LLE_HISTORY_INVALID_POSITION) {
            lle_history_free_entry(&history->entries[actual_index]);
        }
    }
    
    // Reset structure state
    history->count = 0;
    history->current = LLE_HISTORY_INVALID_POSITION;
    history->oldest_index = 0;
    history->is_full = false;
    history->navigation_mode = false;
    
    // Clear temporary buffer
    if (history->temp_buffer) {
        free(history->temp_buffer);
        history->temp_buffer = NULL;
        history->temp_length = 0;
    }
    
    return true;
}

bool lle_history_add(lle_history_t *history, const char *command, bool force_add) {
    if (!lle_history_validate(history) || !command) {
        return false;
    }
    
    size_t command_len = strlen(command);
    if (command_len == 0) {
        return false; // Don't add empty commands
    }
    
    // Handle duplicate detection based on mode
    if (!force_add && history->count > 0) {
        if (history->no_duplicates) {
            // Check entire history for duplicates (hist_no_dups behavior)
            size_t duplicate_index = lle_history_find_command(history, command);
            if (duplicate_index != LLE_HISTORY_INVALID_POSITION) {
                // Found duplicate - remove it and add new entry at end
                if (!lle_history_remove_at_index(history, duplicate_index)) {
                    return false; // Failed to remove duplicate
                }
                // Continue to add the command at the end with updated timestamp
            }
        } else {
            // Only check consecutive duplicates (traditional behavior)
            const lle_history_entry_t *last = lle_history_get(history, history->count - 1);
            if (last && last->command && strcmp(last->command, command) == 0) {
                return true; // Skip consecutive duplicate
            }
        }
    }
    
    // Find insertion position
    size_t insert_index;
    lle_history_entry_t *entry;
    
    if (history->count < history->capacity) {
        // Not full yet, use next available slot
        insert_index = history->count;
        entry = &history->entries[insert_index];
        history->count++;
        
        // Check if we just reached capacity
        if (history->count == history->capacity) {
            history->is_full = true;
        }
    } else {
        // Full, reuse oldest entry
        insert_index = history->oldest_index;
        entry = &history->entries[insert_index];
        
        // Free existing entry
        lle_history_free_entry(entry);
        
        // Advance oldest index
        history->oldest_index = (history->oldest_index + 1) % history->capacity;
        history->is_full = true;
    }
    
    // Allocate and copy command
    entry->command = malloc(command_len + 1);
    if (!entry->command) {
        return false;
    }
    
    strcpy(entry->command, command);
    entry->length = command_len;
    entry->timestamp = lle_history_get_timestamp();
    entry->persistent = true; // Default to persistent
    
    // Reset navigation state
    lle_history_reset_navigation(history);
    
    return true;
}

const lle_history_entry_t *lle_history_get(const lle_history_t *history, size_t index) {
    if (!lle_history_validate(history) || index >= history->count) {
        return NULL;
    }
    
    size_t actual_index = lle_history_get_actual_index(history, index);
    if (actual_index == LLE_HISTORY_INVALID_POSITION) {
        return NULL;
    }
    
    return &history->entries[actual_index];
}

const lle_history_entry_t *lle_history_current(const lle_history_t *history) {
    if (!lle_history_validate(history) || 
        !history->navigation_mode ||
        history->current == LLE_HISTORY_INVALID_POSITION) {
        return NULL;
    }
    
    return lle_history_get(history, history->current);
}

const lle_history_entry_t *lle_history_navigate(lle_history_t *history, 
                                                lle_history_direction_t direction) {
    if (!lle_history_validate(history) || history->count == 0) {
        return NULL;
    }
    
    // Enter navigation mode if not already
    if (!history->navigation_mode) {
        history->navigation_mode = true;
        history->current = history->count; // Start after newest entry
    }
    
    switch (direction) {
        case LLE_HISTORY_PREV:
            if (history->current > 0) {
                history->current--;
            } else {
                return NULL; // Already at oldest
            }
            break;
            
        case LLE_HISTORY_NEXT:
            if (history->current < history->count - 1) {
                history->current++;
            } else {
                // Past newest entry, exit navigation mode
                lle_history_reset_navigation(history);
                return NULL;
            }
            break;
            
        case LLE_HISTORY_FIRST:
            history->current = 0;
            break;
            
        case LLE_HISTORY_LAST:
            history->current = history->count - 1;
            break;
            
        default:
            return NULL;
    }
    
    return lle_history_current(history);
}

void lle_history_reset_navigation(lle_history_t *history) {
    if (!history) {
        return;
    }
    
    history->navigation_mode = false;
    history->current = LLE_HISTORY_INVALID_POSITION;
}

bool lle_history_set_temp_buffer(lle_history_t *history, 
                                 const char *buffer, 
                                 size_t length) {
    if (!lle_history_validate(history) || !buffer) {
        return false;
    }
    
    // Free existing temporary buffer
    if (history->temp_buffer) {
        free(history->temp_buffer);
        history->temp_buffer = NULL;
        history->temp_length = 0;
    }
    
    // Allocate new buffer
    history->temp_buffer = malloc(length + 1);
    if (!history->temp_buffer) {
        return false;
    }
    
    // Copy content
    memcpy(history->temp_buffer, buffer, length);
    history->temp_buffer[length] = '\0';
    history->temp_length = length;
    
    return true;
}

const char *lle_history_get_temp_buffer(const lle_history_t *history, 
                                        size_t *length) {
    if (!lle_history_validate(history) || !history->temp_buffer) {
        if (length) {
            *length = 0;
        }
        return NULL;
    }
    
    if (length) {
        *length = history->temp_length;
    }
    
    return history->temp_buffer;
}

bool lle_history_is_empty(const lle_history_t *history) {
    return !lle_history_validate(history) || history->count == 0;
}

bool lle_history_is_full(const lle_history_t *history) {
    return lle_history_validate(history) && history->is_full;
}

size_t lle_history_size(const lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return 0;
    }
    
    return history->count;
}

size_t lle_history_max_size(const lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return 0;
    }
    
    return history->max_entries;
}

bool lle_history_set_max_size(lle_history_t *history, size_t max_entries) {
    if (!lle_history_validate(history) || 
        max_entries < LLE_HISTORY_MIN_ENTRIES || 
        max_entries > LLE_HISTORY_MAX_ENTRIES) {
        return false;
    }
    
    // If reducing size, remove oldest entries
    if (max_entries < history->count) {
        size_t entries_to_remove = history->count - max_entries;
        
        if (history->is_full) {
            // Circular buffer case: remove oldest entries
            for (size_t i = 0; i < entries_to_remove; i++) {
                size_t actual_index = lle_history_get_actual_index(history, i);
                if (actual_index != LLE_HISTORY_INVALID_POSITION) {
                    lle_history_free_entry(&history->entries[actual_index]);
                }
            }
            
            // Update oldest index for circular buffer
            history->oldest_index = (history->oldest_index + entries_to_remove) % history->capacity;
        } else {
            // Sequential case: remove entries from beginning
            for (size_t i = 0; i < entries_to_remove; i++) {
                lle_history_free_entry(&history->entries[i]);
            }
            
            // Shift remaining entries to front
            for (size_t i = 0; i < max_entries; i++) {
                history->entries[i] = history->entries[i + entries_to_remove];
            }
            
            // Clear the moved entries
            for (size_t i = max_entries; i < history->count; i++) {
                memset(&history->entries[i], 0, sizeof(lle_history_entry_t));
            }
        }
        
        // Update count
        history->count = max_entries;
        
        // Reset navigation if current position is now invalid
        if (history->navigation_mode && history->current >= history->count) {
            lle_history_reset_navigation(history);
        }
    }
    
    // Update maximum size
    history->max_entries = max_entries;
    
    // If new max is larger than current capacity, we'd need to reallocate
    // For now, just update the limit
    if (max_entries > history->capacity) {
        // TODO: Implement reallocation if needed
        // For LLE-024, we'll keep the current capacity
    }
    
    return true;
}

bool lle_history_get_stats(const lle_history_t *history, lle_history_stats_t *stats) {
    if (!lle_history_validate(history) || !stats) {
        return false;
    }
    
    // Initialize stats
    memset(stats, 0, sizeof(lle_history_stats_t));
    
    stats->current_entries = history->count;
    stats->total_entries = history->count; // For now, same as current
    
    if (history->count == 0) {
        return true;
    }
    
    // Calculate statistics
    size_t total_length = 0;
    size_t longest = 0;
    uint64_t oldest_ts = UINT64_MAX;
    uint64_t newest_ts = 0;
    
    for (size_t i = 0; i < history->count; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (entry && entry->command) {
            total_length += entry->length;
            if (entry->length > longest) {
                longest = entry->length;
            }
            if (entry->timestamp < oldest_ts) {
                oldest_ts = entry->timestamp;
            }
            if (entry->timestamp > newest_ts) {
                newest_ts = entry->timestamp;
            }
            stats->memory_usage += entry->length + 1; // +1 for null terminator
        }
    }
    
    stats->average_length = history->count > 0 ? total_length / history->count : 0;
    stats->longest_command = longest;
    stats->oldest_timestamp = oldest_ts != UINT64_MAX ? oldest_ts : 0;
    stats->newest_timestamp = newest_ts;
    stats->memory_usage += sizeof(lle_history_t) + 
                          (history->capacity * sizeof(lle_history_entry_t));
    
    return true;
}

// ============================================================================
// File I/O Functions (LLE-025)
// ============================================================================

/**
 * @brief Save history to file
 * 
 * Saves all history entries to the specified file. Each entry is written
 * as a separate line with timestamp metadata if available.
 * 
 * @param history History structure
 * @param filename Path to file where history should be saved
 * @return true on success, false on failure
 */
bool lle_history_save(const lle_history_t *history, const char *filename) {
    if (!lle_history_validate(history) || !filename) {
        return false;
    }
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        return false;
    }
    
    // Write each history entry
    for (size_t i = 0; i < history->count; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (!entry || !entry->command) {
            continue;
        }
        
        // Write entry with timestamp metadata
        if (fprintf(file, "#%lu:%s\n", (unsigned long)entry->timestamp, entry->command) < 0) {
            fclose(file);
            return false;
        }
    }
    
    if (fclose(file) != 0) {
        return false;
    }
    
    return true;
}

/**
 * @brief Load history from file
 * 
 * Loads history entries from the specified file. Existing history entries
 * are preserved unless clear_existing is true.
 * 
 * @param history History structure
 * @param filename Path to file containing history data
 * @param clear_existing Whether to clear existing entries before loading
 * @return true on success, false on failure
 */
bool lle_history_load(lle_history_t *history, const char *filename, bool clear_existing) {
    if (!lle_history_validate(history) || !filename) {
        return false;
    }
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }
    
    if (clear_existing) {
        if (!lle_history_clear(history)) {
            fclose(file);
            return false;
        }
    }
    
    char line[4096];
    bool success = true;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }
        
        if (len == 0) {
            continue; // Skip empty lines
        }
        
        const char *command = line;
        
        // Parse timestamp metadata if present
        if (line[0] == '#') {
            char *colon = strchr(line, ':');
            if (colon && colon[1] != '\0') {
                command = colon + 1;
            } else {
                continue; // Invalid format, skip line
            }
        }
        
        // Add command to history (don't force duplicates)
        if (!lle_history_add(history, command, false)) {
            success = false;
            break;
        }
    }
    
    if (fclose(file) != 0) {
        success = false;
    }
    
    return success;
}

// ============================================================================
// Navigation Convenience Functions (LLE-026)
// ============================================================================

/**
 * @brief Navigate to previous (older) history entry
 * 
 * Convenience function that moves to the previous entry in history.
 * Automatically enters navigation mode if not already active.
 * 
 * @param history History structure
 * @return Command string of previous entry, or NULL if at beginning
 */
const char *lle_history_prev(lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return NULL;
    }
    
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_PREV);
    return entry ? entry->command : NULL;
}

/**
 * @brief Navigate to next (newer) history entry
 * 
 * Convenience function that moves to the next entry in history.
 * Returns NULL and exits navigation mode if moving past the newest entry.
 * 
 * @param history History structure
 * @return Command string of next entry, or NULL if at end
 */
const char *lle_history_next(lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return NULL;
    }
    
    const lle_history_entry_t *entry = lle_history_navigate(history, LLE_HISTORY_NEXT);
    return entry ? entry->command : NULL;
}

/**
 * @brief Get current navigation position
 * 
 * Returns the current position in history navigation. Position 0 is the
 * oldest entry, and count-1 is the newest entry.
 * 
 * @param history History structure
 * @return Current position, or LLE_HISTORY_INVALID_POSITION if not navigating
 */
size_t lle_history_get_position(const lle_history_t *history) {
    if (!lle_history_validate(history) || !history->navigation_mode) {
        return LLE_HISTORY_INVALID_POSITION;
    }
    
    return history->current;
}

/**
 * @brief Set navigation position directly
 * 
 * Sets the current navigation position to the specified index.
 * Automatically enters navigation mode if not already active.
 * 
 * @param history History structure
 * @param position Position to navigate to (0-based index)
 * @return true on success, false if position is invalid
 */
bool lle_history_set_position(lle_history_t *history, size_t position) {
    if (!lle_history_validate(history) || position >= history->count) {
        return false;
    }
    
    // Enter navigation mode if not already
    if (!history->navigation_mode) {
        history->navigation_mode = true;
    }
    
    history->current = position;
    return true;
}

/**
 * @brief Reset navigation position
 * 
 * Convenience function that exits navigation mode and resets position.
 * Equivalent to lle_history_reset_navigation().
 * 
 * @param history History structure
 * @return true on success, false on failure
 */
bool lle_history_reset_position(lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return false;
    }
    
    lle_history_reset_navigation(history);
    return true;
}

// ============================================================================
// Internal Helper Functions for Duplicate Management
// ============================================================================

/**
 * @brief Find index of command in history
 * @param history History structure
 * @param command Command to search for
 * @return Index of command, or LLE_HISTORY_INVALID_POSITION if not found
 */
static size_t lle_history_find_command(const lle_history_t *history, const char *command) {
    if (!history || !command) {
        return LLE_HISTORY_INVALID_POSITION;
    }
    
    for (size_t i = 0; i < history->count; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (entry && entry->command && strcmp(entry->command, command) == 0) {
            return i;
        }
    }
    
    return LLE_HISTORY_INVALID_POSITION;
}

/**
 * @brief Remove entry at logical index and compact array
 * @param history History structure
 * @param logical_index Logical index (0 = oldest)
 * @return true on success, false on failure
 */
static bool lle_history_remove_at_index(lle_history_t *history, size_t logical_index) {
    if (!lle_history_validate(history) || logical_index >= history->count) {
        return false;
    }
    
    size_t actual_index = lle_history_get_actual_index(history, logical_index);
    if (actual_index == LLE_HISTORY_INVALID_POSITION) {
        return false;
    }
    
    // Free the entry being removed
    lle_history_free_entry(&history->entries[actual_index]);
    
    // If this is a circular buffer, we need to handle removal carefully
    if (history->is_full) {
        // For full circular buffer, we need to shift elements
        // This is complex, so for now we'll use a simpler approach:
        // Convert to linear arrangement, remove, then update
        
        // Create temporary array for entries in logical order
        lle_history_entry_t *temp_entries = malloc(history->count * sizeof(lle_history_entry_t));
        if (!temp_entries) {
            return false;
        }
        
        // Copy entries in logical order, skipping the one to remove
        size_t new_count = 0;
        for (size_t i = 0; i < history->count; i++) {
            if (i != logical_index) {
                size_t src_actual = lle_history_get_actual_index(history, i);
                if (src_actual != LLE_HISTORY_INVALID_POSITION) {
                    temp_entries[new_count] = history->entries[src_actual];
                    new_count++;
                }
            }
        }
        
        // Clear original array
        memset(history->entries, 0, history->capacity * sizeof(lle_history_entry_t));
        
        // Copy back in linear order
        for (size_t i = 0; i < new_count; i++) {
            history->entries[i] = temp_entries[i];
        }
        
        free(temp_entries);
        
        // Update structure state
        history->count = new_count;
        history->oldest_index = 0;
        history->is_full = (new_count == history->capacity);
        
    } else {
        // Linear array case - simple shift
        for (size_t i = logical_index; i < history->count - 1; i++) {
            history->entries[i] = history->entries[i + 1];
        }
        
        // Clear the last entry
        memset(&history->entries[history->count - 1], 0, sizeof(lle_history_entry_t));
        history->count--;
    }
    
    return true;
}

// ============================================================================
// New Public API Functions
// ============================================================================

bool lle_history_set_no_duplicates(lle_history_t *history, bool no_duplicates) {
    if (!lle_history_validate(history)) {
        return false;
    }
    
    bool old_setting = history->no_duplicates;
    history->no_duplicates = no_duplicates;
    
    // If enabling no_duplicates, clean existing history
    if (no_duplicates && !old_setting) {
        lle_history_remove_duplicates(history);
    }
    
    return true;
}

bool lle_history_get_no_duplicates(const lle_history_t *history) {
    if (!lle_history_validate(history)) {
        return false;
    }
    
    return history->no_duplicates;
}

size_t lle_history_remove_duplicates(lle_history_t *history) {
    if (!history) {
        return SIZE_MAX; // Error - NULL parameter
    }
    
    if (!lle_history_validate(history) || history->count == 0) {
        return 0;
    }
    
    size_t removed_count = 0;
    
    // Create a temporary array to store unique entries in chronological order
    lle_history_entry_t *unique_entries = malloc(history->count * sizeof(lle_history_entry_t));
    if (!unique_entries) {
        return SIZE_MAX; // Error
    }
    
    size_t unique_count = 0;
    
    // Walk through history from oldest to newest
    for (size_t i = 0; i < history->count; i++) {
        const lle_history_entry_t *current_entry = lle_history_get(history, i);
        
        if (!current_entry || !current_entry->command) {
            continue;
        }
        
        // Check if this command appears later in history
        bool found_later = false;
        for (size_t j = i + 1; j < history->count; j++) {
            const lle_history_entry_t *later_entry = lle_history_get(history, j);
            if (later_entry && later_entry->command && 
                strcmp(later_entry->command, current_entry->command) == 0) {
                found_later = true;
                break;
            }
        }
        
        // If not found later, this is the latest occurrence - keep it
        if (!found_later) {
            unique_entries[unique_count] = *current_entry;
            // Duplicate the command string
            unique_entries[unique_count].command = malloc(strlen(current_entry->command) + 1);
            if (unique_entries[unique_count].command) {
                strcpy(unique_entries[unique_count].command, current_entry->command);
                unique_count++;
            }
        } else {
            removed_count++;
        }
    }
    
    // Clear original entries
    for (size_t i = 0; i < history->count; i++) {
        size_t actual_index = lle_history_get_actual_index(history, i);
        if (actual_index != LLE_HISTORY_INVALID_POSITION) {
            lle_history_free_entry(&history->entries[actual_index]);
        }
    }
    
    // Clear entire array
    memset(history->entries, 0, history->capacity * sizeof(lle_history_entry_t));
    
    // Copy unique entries back in linear order
    for (size_t i = 0; i < unique_count; i++) {
        history->entries[i] = unique_entries[i];
    }
    
    // Update history state
    history->count = unique_count;
    history->oldest_index = 0;
    history->is_full = (unique_count == history->capacity);
    
    free(unique_entries);
    return removed_count;
}