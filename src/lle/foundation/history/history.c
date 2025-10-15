// src/lle/foundation/history/history.c
//
// LLE Command History System Implementation

#include "history.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Initialize history system
int lle_history_init(lle_history_t *history,
                     size_t capacity,
                     const char *history_file) {
    if (!history) {
        return LLE_HISTORY_ERR_NULL_PTR;
    }
    
    if (capacity == 0) {
        return LLE_HISTORY_ERR_INVALID_SIZE;
    }
    
    // Zero-initialize
    memset(history, 0, sizeof(*history));
    
    // Allocate entries array
    history->entries = calloc(capacity, sizeof(lle_history_entry_t));
    if (!history->entries) {
        return LLE_HISTORY_ERR_ALLOC_FAILED;
    }
    
    history->capacity = capacity;
    history->count = 0;
    history->head = 0;
    history->next_index = 0;
    
    // Set history file if provided
    if (history_file) {
        history->history_file = strdup(history_file);
        if (!history->history_file) {
            free(history->entries);
            return LLE_HISTORY_ERR_ALLOC_FAILED;
        }
    }
    
    // Default configuration
    history->auto_save = false;
    history->modified = false;
    history->ignore_duplicates = true;
    history->ignore_space = true;
    history->max_line_length = LLE_HISTORY_MAX_LINE;
    
    // Search state
    history->search_pos = 0;
    history->search_pattern = NULL;
    history->search_active = false;
    
    return LLE_HISTORY_OK;
}

// Add entry to history
int lle_history_add(lle_history_t *history, const char *line) {
    if (!history || !line) {
        return LLE_HISTORY_ERR_NULL_PTR;
    }
    
    if (!history->entries) {
        return LLE_HISTORY_ERR_NOT_INIT;
    }
    
    size_t line_len = strlen(line);
    
    // Check line length
    if (line_len == 0) {
        return LLE_HISTORY_OK;  // Ignore empty lines
    }
    
    if (line_len > history->max_line_length) {
        return LLE_HISTORY_ERR_LINE_TOO_LONG;
    }
    
    // Ignore lines starting with space if configured
    if (history->ignore_space && line[0] == ' ') {
        return LLE_HISTORY_OK;
    }
    
    // Check for duplicate of most recent entry
    if (history->ignore_duplicates && lle_history_is_duplicate(history, line)) {
        return LLE_HISTORY_OK;
    }
    
    // Calculate position for new entry
    size_t pos;
    if (history->count < history->capacity) {
        // Still have room, add at end
        pos = history->count;
        history->count++;
    } else {
        // Buffer full, overwrite oldest entry
        pos = history->head;
        history->head = (history->head + 1) % history->capacity;
        
        // Free old entry
        if (history->entries[pos].line) {
            free(history->entries[pos].line);
        }
    }
    
    // Allocate and copy line
    history->entries[pos].line = malloc(line_len + 1);
    if (!history->entries[pos].line) {
        return LLE_HISTORY_ERR_ALLOC_FAILED;
    }
    
    memcpy(history->entries[pos].line, line, line_len + 1);
    history->entries[pos].line_len = line_len;
    history->entries[pos].timestamp = time(NULL);
    history->entries[pos].index = history->next_index++;
    
    history->modified = true;
    
    // Auto-save if enabled
    if (history->auto_save && history->history_file) {
        lle_history_save(history);
    }
    
    return LLE_HISTORY_OK;
}

// Get entry by index (0 = most recent)
const lle_history_entry_t* lle_history_get(const lle_history_t *history,
                                           size_t index) {
    if (!history || !history->entries) {
        return NULL;
    }
    
    if (index >= history->count) {
        return NULL;
    }
    
    // Calculate actual position (counting back from most recent)
    size_t pos;
    if (history->count < history->capacity) {
        // Not yet wrapped
        pos = history->count - 1 - index;
    } else {
        // Wrapped, most recent is one before head
        size_t most_recent = (history->head + history->capacity - 1) % history->capacity;
        pos = (most_recent + history->capacity - index) % history->capacity;
    }
    
    return &history->entries[pos];
}

// Get count of entries
size_t lle_history_count(const lle_history_t *history) {
    if (!history) {
        return 0;
    }
    return history->count;
}

// Clear all entries
int lle_history_clear(lle_history_t *history) {
    if (!history || !history->entries) {
        return LLE_HISTORY_ERR_NOT_INIT;
    }
    
    // Free all line allocations
    for (size_t i = 0; i < history->count; i++) {
        size_t pos;
        if (history->count < history->capacity) {
            pos = i;
        } else {
            pos = (history->head + i) % history->capacity;
        }
        
        if (history->entries[pos].line) {
            free(history->entries[pos].line);
            history->entries[pos].line = NULL;
        }
    }
    
    history->count = 0;
    history->head = 0;
    history->modified = true;
    
    return LLE_HISTORY_OK;
}

// Load history from file
int lle_history_load(lle_history_t *history) {
    if (!history || !history->entries) {
        return LLE_HISTORY_ERR_NOT_INIT;
    }
    
    if (!history->history_file) {
        return LLE_HISTORY_ERR_FILE_OPEN;
    }
    
    FILE *fp = fopen(history->history_file, "r");
    if (!fp) {
        // File doesn't exist yet, not an error
        return LLE_HISTORY_OK;
    }
    
    char line[LLE_HISTORY_MAX_LINE + 1];
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Add to history (respects ignore settings)
        lle_history_add(history, line);
    }
    
    fclose(fp);
    history->modified = false;
    
    return LLE_HISTORY_OK;
}

// Save history to file
int lle_history_save(const lle_history_t *history) {
    if (!history || !history->entries) {
        return LLE_HISTORY_ERR_NOT_INIT;
    }
    
    if (!history->history_file) {
        return LLE_HISTORY_ERR_FILE_OPEN;
    }
    
    FILE *fp = fopen(history->history_file, "w");
    if (!fp) {
        return LLE_HISTORY_ERR_FILE_WRITE;
    }
    
    // Write entries in chronological order (oldest to newest)
    for (size_t i = 0; i < history->count; i++) {
        size_t pos;
        if (history->count < history->capacity) {
            pos = i;
        } else {
            pos = (history->head + i) % history->capacity;
        }
        
        if (history->entries[pos].line) {
            fprintf(fp, "%s\n", history->entries[pos].line);
        }
    }
    
    fclose(fp);
    return LLE_HISTORY_OK;
}

// Start search
int lle_history_search_start(lle_history_t *history, const char *pattern) {
    if (!history || !pattern) {
        return LLE_HISTORY_ERR_NULL_PTR;
    }
    
    if (!history->entries) {
        return LLE_HISTORY_ERR_NOT_INIT;
    }
    
    // Free old pattern if exists
    if (history->search_pattern) {
        free(history->search_pattern);
    }
    
    // Copy new pattern
    history->search_pattern = strdup(pattern);
    if (!history->search_pattern) {
        return LLE_HISTORY_ERR_ALLOC_FAILED;
    }
    
    history->search_pos = 0;
    history->search_active = true;
    
    return LLE_HISTORY_OK;
}

// Search forward (towards newer entries)
const lle_history_entry_t* lle_history_search_next(lle_history_t *history) {
    if (!history || !history->search_active || !history->search_pattern) {
        return NULL;
    }
    
    // Search from current position towards newer entries
    for (size_t i = history->search_pos; i < history->count; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (entry && entry->line && strstr(entry->line, history->search_pattern)) {
            history->search_pos = i + 1;
            return entry;
        }
    }
    
    return NULL;
}

// Search backward (towards older entries)
const lle_history_entry_t* lle_history_search_prev(lle_history_t *history) {
    if (!history || !history->search_active || !history->search_pattern) {
        return NULL;
    }
    
    // Search from current position towards older entries
    if (history->search_pos == 0) {
        return NULL;  // Already at beginning
    }
    
    for (size_t i = history->search_pos - 1; i != (size_t)-1; i--) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (entry && entry->line && strstr(entry->line, history->search_pattern)) {
            history->search_pos = i;
            return entry;
        }
    }
    
    return NULL;
}

// End search
int lle_history_search_end(lle_history_t *history) {
    if (!history) {
        return LLE_HISTORY_ERR_NULL_PTR;
    }
    
    if (history->search_pattern) {
        free(history->search_pattern);
        history->search_pattern = NULL;
    }
    
    history->search_active = false;
    history->search_pos = 0;
    
    return LLE_HISTORY_OK;
}

// Get most recent entry
const lle_history_entry_t* lle_history_get_recent(const lle_history_t *history) {
    if (!history || history->count == 0) {
        return NULL;
    }
    
    return lle_history_get(history, 0);
}

// Get entry by global index
const lle_history_entry_t* lle_history_get_by_index(const lle_history_t *history,
                                                    uint32_t global_index) {
    if (!history || !history->entries) {
        return NULL;
    }
    
    // Search for entry with matching global index
    for (size_t i = 0; i < history->count; i++) {
        const lle_history_entry_t *entry = lle_history_get(history, i);
        if (entry && entry->index == global_index) {
            return entry;
        }
    }
    
    return NULL;
}

// Check if line is duplicate of most recent
bool lle_history_is_duplicate(const lle_history_t *history, const char *line) {
    if (!history || !line || history->count == 0) {
        return false;
    }
    
    const lle_history_entry_t *recent = lle_history_get_recent(history);
    if (!recent || !recent->line) {
        return false;
    }
    
    return strcmp(recent->line, line) == 0;
}

// Configuration setters
void lle_history_set_ignore_duplicates(lle_history_t *history, bool ignore) {
    if (history) {
        history->ignore_duplicates = ignore;
    }
}

void lle_history_set_ignore_space(lle_history_t *history, bool ignore) {
    if (history) {
        history->ignore_space = ignore;
    }
}

void lle_history_set_auto_save(lle_history_t *history, bool auto_save) {
    if (history) {
        history->auto_save = auto_save;
    }
}

void lle_history_set_max_line_length(lle_history_t *history, size_t max_len) {
    if (history) {
        history->max_line_length = max_len;
    }
}

// Cleanup
void lle_history_cleanup(lle_history_t *history) {
    if (!history) {
        return;
    }
    
    // Free all entries
    if (history->entries) {
        for (size_t i = 0; i < history->count; i++) {
            size_t pos;
            if (history->count < history->capacity) {
                pos = i;
            } else {
                pos = (history->head + i) % history->capacity;
            }
            
            if (history->entries[pos].line) {
                free(history->entries[pos].line);
            }
        }
        free(history->entries);
    }
    
    // Free history file path
    if (history->history_file) {
        free(history->history_file);
    }
    
    // Free search pattern
    if (history->search_pattern) {
        free(history->search_pattern);
    }
    
    memset(history, 0, sizeof(*history));
}

// Error string conversion
const char* lle_history_error_string(int error_code) {
    switch (error_code) {
        case LLE_HISTORY_OK:
            return "Success";
        case LLE_HISTORY_ERR_NULL_PTR:
            return "Null pointer argument";
        case LLE_HISTORY_ERR_INVALID_SIZE:
            return "Invalid size";
        case LLE_HISTORY_ERR_ALLOC_FAILED:
            return "Memory allocation failed";
        case LLE_HISTORY_ERR_FILE_OPEN:
            return "Failed to open file";
        case LLE_HISTORY_ERR_FILE_READ:
            return "Failed to read file";
        case LLE_HISTORY_ERR_FILE_WRITE:
            return "Failed to write file";
        case LLE_HISTORY_ERR_LINE_TOO_LONG:
            return "Line too long";
        case LLE_HISTORY_ERR_NOT_INIT:
            return "Not initialized";
        case LLE_HISTORY_ERR_EMPTY:
            return "History is empty";
        case LLE_HISTORY_ERR_NOT_FOUND:
            return "Entry not found";
        default:
            return "Unknown error";
    }
}
