// src/lle/foundation/buffer/buffer_manager.c
//
// LLE Buffer Manager Implementation

#include "buffer_manager.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get current time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Find buffer by ID
static lle_managed_buffer_t* find_buffer_by_id(lle_buffer_manager_t *manager,
                                                uint32_t buffer_id) {
    lle_managed_buffer_t *buf = manager->first_buffer;
    while (buf) {
        if (buf->buffer_id == buffer_id) {
            return buf;
        }
        buf = buf->next;
    }
    return NULL;
}

// Find buffer by name
static lle_managed_buffer_t* find_buffer_by_name(lle_buffer_manager_t *manager,
                                                  const char *name) {
    if (!name) return NULL;
    
    lle_managed_buffer_t *buf = manager->first_buffer;
    while (buf) {
        if (buf->name && strcmp(buf->name, name) == 0) {
            return buf;
        }
        buf = buf->next;
    }
    return NULL;
}

// Free a managed buffer
static void free_managed_buffer(lle_managed_buffer_t *buf) {
    if (!buf) return;
    
    lle_buffer_cleanup(&buf->buffer);
    lle_change_tracker_cleanup(&buf->tracker);
    free(buf->name);
    free(buf);
}

// Unlink buffer from list
static void unlink_buffer(lle_buffer_manager_t *manager, lle_managed_buffer_t *buf) {
    if (buf->prev) {
        buf->prev->next = buf->next;
    } else {
        manager->first_buffer = buf->next;
    }
    
    if (buf->next) {
        buf->next->prev = buf->prev;
    } else {
        manager->last_buffer = buf->prev;
    }
}

// Initialize buffer manager
int lle_buffer_manager_init(lle_buffer_manager_t *manager,
                            size_t max_buffers,
                            size_t default_buffer_capacity) {
    if (!manager) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    memset(manager, 0, sizeof(lle_buffer_manager_t));
    
    manager->max_buffers = (max_buffers > 0) ? max_buffers : LLE_BUFFER_MGR_DEFAULT_MAX_BUFFERS;
    manager->max_scratch_buffers = LLE_BUFFER_MGR_DEFAULT_MAX_SCRATCH;
    manager->default_buffer_capacity = (default_buffer_capacity > 0) ? 
                                        default_buffer_capacity : 
                                        LLE_BUFFER_MGR_DEFAULT_CAPACITY;
    manager->undo_max_sequences = LLE_UNDO_DEFAULT_MAX_SEQUENCES;
    manager->undo_max_memory = LLE_UNDO_DEFAULT_MAX_MEMORY;
    manager->next_buffer_id = 1;
    
    return LLE_BUFFER_MGR_OK;
}

// Cleanup buffer manager
void lle_buffer_manager_cleanup(lle_buffer_manager_t *manager) {
    if (!manager) return;
    
    lle_managed_buffer_t *buf = manager->first_buffer;
    while (buf) {
        lle_managed_buffer_t *next = buf->next;
        free_managed_buffer(buf);
        buf = next;
    }
    
    memset(manager, 0, sizeof(lle_buffer_manager_t));
}

// Create named buffer
int lle_buffer_manager_create_buffer(lle_buffer_manager_t *manager,
                                     const char *name,
                                     uint32_t *buffer_id) {
    if (!manager || !name || !buffer_id) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    // Validate name
    if (strlen(name) == 0) {
        return LLE_BUFFER_MGR_ERR_INVALID_NAME;
    }
    
    // Check if name already exists
    if (find_buffer_by_name(manager, name)) {
        return LLE_BUFFER_MGR_ERR_BUFFER_EXISTS;
    }
    
    // Check buffer limit
    if (manager->buffer_count >= manager->max_buffers) {
        return LLE_BUFFER_MGR_ERR_MAX_BUFFERS;
    }
    
    // Allocate managed buffer
    lle_managed_buffer_t *buf = calloc(1, sizeof(lle_managed_buffer_t));
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Set up buffer
    buf->buffer_id = manager->next_buffer_id++;
    buf->name = strdup(name);
    if (!buf->name) {
        free(buf);
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Initialize gap buffer
    int result = lle_buffer_init(&buf->buffer, manager->default_buffer_capacity);
    if (result != LLE_BUFFER_OK) {
        free(buf->name);
        free(buf);
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Initialize undo tracker
    result = lle_change_tracker_init(&buf->tracker, 
                                     manager->undo_max_sequences,
                                     manager->undo_max_memory);
    if (result != LLE_UNDO_OK) {
        lle_buffer_cleanup(&buf->buffer);
        free(buf->name);
        free(buf);
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Set metadata
    buf->flags = LLE_BUFFER_FLAG_PERSISTENT;
    buf->creation_time_ns = get_time_ns();
    buf->last_access_time_ns = buf->creation_time_ns;
    buf->access_count = 0;
    
    // Add to list
    if (manager->last_buffer) {
        manager->last_buffer->next = buf;
        buf->prev = manager->last_buffer;
        manager->last_buffer = buf;
    } else {
        manager->first_buffer = buf;
        manager->last_buffer = buf;
    }
    
    manager->buffer_count++;
    manager->named_buffer_count++;
    
    // Set as current if no current buffer
    if (!manager->current_buffer) {
        manager->current_buffer = buf;
    }
    
    *buffer_id = buf->buffer_id;
    return LLE_BUFFER_MGR_OK;
}

// Create scratch buffer
int lle_buffer_manager_create_scratch(lle_buffer_manager_t *manager,
                                      uint32_t *buffer_id) {
    if (!manager || !buffer_id) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    // Check buffer limits
    if (manager->buffer_count >= manager->max_buffers) {
        return LLE_BUFFER_MGR_ERR_MAX_BUFFERS;
    }
    
    if (manager->scratch_buffer_count >= manager->max_scratch_buffers) {
        return LLE_BUFFER_MGR_ERR_MAX_BUFFERS;
    }
    
    // Allocate managed buffer
    lle_managed_buffer_t *buf = calloc(1, sizeof(lle_managed_buffer_t));
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Set up buffer (no name for scratch)
    buf->buffer_id = manager->next_buffer_id++;
    buf->name = NULL;
    
    // Initialize gap buffer
    int result = lle_buffer_init(&buf->buffer, manager->default_buffer_capacity);
    if (result != LLE_BUFFER_OK) {
        free(buf);
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Initialize undo tracker
    result = lle_change_tracker_init(&buf->tracker,
                                     manager->undo_max_sequences,
                                     manager->undo_max_memory);
    if (result != LLE_UNDO_OK) {
        lle_buffer_cleanup(&buf->buffer);
        free(buf);
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    // Set metadata
    buf->flags = LLE_BUFFER_FLAG_SCRATCH;
    buf->creation_time_ns = get_time_ns();
    buf->last_access_time_ns = buf->creation_time_ns;
    buf->access_count = 0;
    
    // Add to list
    if (manager->last_buffer) {
        manager->last_buffer->next = buf;
        buf->prev = manager->last_buffer;
        manager->last_buffer = buf;
    } else {
        manager->first_buffer = buf;
        manager->last_buffer = buf;
    }
    
    manager->buffer_count++;
    manager->scratch_buffer_count++;
    
    *buffer_id = buf->buffer_id;
    return LLE_BUFFER_MGR_OK;
}

// Delete buffer by ID
int lle_buffer_manager_delete_buffer(lle_buffer_manager_t *manager,
                                     uint32_t buffer_id) {
    if (!manager) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    lle_managed_buffer_t *buf = find_buffer_by_id(manager, buffer_id);
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND;
    }
    
    // If this is current buffer, switch to another
    if (manager->current_buffer == buf) {
        manager->current_buffer = buf->next ? buf->next : buf->prev;
    }
    
    // Update counts
    manager->buffer_count--;
    if (buf->flags & LLE_BUFFER_FLAG_SCRATCH) {
        manager->scratch_buffer_count--;
    } else if (buf->flags & LLE_BUFFER_FLAG_PERSISTENT) {
        manager->named_buffer_count--;
    }
    
    // Unlink and free
    unlink_buffer(manager, buf);
    free_managed_buffer(buf);
    
    return LLE_BUFFER_MGR_OK;
}

// Delete buffer by name
int lle_buffer_manager_delete_buffer_by_name(lle_buffer_manager_t *manager,
                                             const char *name) {
    if (!manager || !name) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    lle_managed_buffer_t *buf = find_buffer_by_name(manager, name);
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND;
    }
    
    return lle_buffer_manager_delete_buffer(manager, buf->buffer_id);
}

// Switch to buffer by ID
int lle_buffer_manager_switch_to_buffer(lle_buffer_manager_t *manager,
                                        uint32_t buffer_id) {
    if (!manager) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    lle_managed_buffer_t *buf = find_buffer_by_id(manager, buffer_id);
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND;
    }
    
    manager->current_buffer = buf;
    buf->last_access_time_ns = get_time_ns();
    buf->access_count++;
    
    return LLE_BUFFER_MGR_OK;
}

// Switch to buffer by name
int lle_buffer_manager_switch_to_buffer_by_name(lle_buffer_manager_t *manager,
                                                const char *name) {
    if (!manager || !name) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    lle_managed_buffer_t *buf = find_buffer_by_name(manager, name);
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND;
    }
    
    return lle_buffer_manager_switch_to_buffer(manager, buf->buffer_id);
}

// Get current buffer
lle_managed_buffer_t* lle_buffer_manager_get_current(lle_buffer_manager_t *manager) {
    return manager ? manager->current_buffer : NULL;
}

// Get buffer by ID
lle_managed_buffer_t* lle_buffer_manager_get_buffer(lle_buffer_manager_t *manager,
                                                    uint32_t buffer_id) {
    return manager ? find_buffer_by_id(manager, buffer_id) : NULL;
}

// Get buffer by name
lle_managed_buffer_t* lle_buffer_manager_get_buffer_by_name(lle_buffer_manager_t *manager,
                                                            const char *name) {
    return manager ? find_buffer_by_name(manager, name) : NULL;
}

// Rename buffer
int lle_buffer_manager_rename_buffer(lle_buffer_manager_t *manager,
                                     uint32_t buffer_id,
                                     const char *new_name) {
    if (!manager || !new_name) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    // Validate new name
    if (strlen(new_name) == 0) {
        return LLE_BUFFER_MGR_ERR_INVALID_NAME;
    }
    
    // Check if new name already exists
    if (find_buffer_by_name(manager, new_name)) {
        return LLE_BUFFER_MGR_ERR_BUFFER_EXISTS;
    }
    
    lle_managed_buffer_t *buf = find_buffer_by_id(manager, buffer_id);
    if (!buf) {
        return LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND;
    }
    
    // Free old name and set new
    char *new_name_copy = strdup(new_name);
    if (!new_name_copy) {
        return LLE_BUFFER_MGR_ERR_ALLOC_FAILED;
    }
    
    free(buf->name);
    buf->name = new_name_copy;
    
    // Update flags if changing from scratch to named
    if (buf->flags & LLE_BUFFER_FLAG_SCRATCH) {
        buf->flags &= ~LLE_BUFFER_FLAG_SCRATCH;
        buf->flags |= LLE_BUFFER_FLAG_PERSISTENT;
        manager->scratch_buffer_count--;
        manager->named_buffer_count++;
    }
    
    return LLE_BUFFER_MGR_OK;
}

// List all buffers
uint32_t* lle_buffer_manager_list_buffers(lle_buffer_manager_t *manager,
                                          size_t *count) {
    if (!manager || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    *count = manager->buffer_count;
    if (manager->buffer_count == 0) {
        return NULL;
    }
    
    uint32_t *ids = malloc(manager->buffer_count * sizeof(uint32_t));
    if (!ids) {
        *count = 0;
        return NULL;
    }
    
    size_t i = 0;
    lle_managed_buffer_t *buf = manager->first_buffer;
    while (buf) {
        ids[i++] = buf->buffer_id;
        buf = buf->next;
    }
    
    return ids;
}

// Get counts
size_t lle_buffer_manager_get_count(const lle_buffer_manager_t *manager) {
    return manager ? manager->buffer_count : 0;
}

size_t lle_buffer_manager_get_scratch_count(const lle_buffer_manager_t *manager) {
    return manager ? manager->scratch_buffer_count : 0;
}

size_t lle_buffer_manager_get_named_count(const lle_buffer_manager_t *manager) {
    return manager ? manager->named_buffer_count : 0;
}

// Check if buffer exists
bool lle_buffer_manager_has_buffer(const lle_buffer_manager_t *manager,
                                   uint32_t buffer_id) {
    if (!manager) return false;
    return find_buffer_by_id((lle_buffer_manager_t*)manager, buffer_id) != NULL;
}

bool lle_buffer_manager_has_buffer_by_name(const lle_buffer_manager_t *manager,
                                           const char *name) {
    if (!manager) return false;
    return find_buffer_by_name((lle_buffer_manager_t*)manager, name) != NULL;
}

// Clean up old scratch buffers
int lle_buffer_manager_cleanup_scratch_buffers(lle_buffer_manager_t *manager,
                                               uint64_t max_age_seconds) {
    if (!manager) {
        return LLE_BUFFER_MGR_ERR_NULL_PTR;
    }
    
    uint64_t current_time = get_time_ns();
    uint64_t max_age_ns = max_age_seconds * 1000000000ULL;
    
    lle_managed_buffer_t *buf = manager->first_buffer;
    while (buf) {
        lle_managed_buffer_t *next = buf->next;
        
        if ((buf->flags & LLE_BUFFER_FLAG_SCRATCH) &&
            (current_time - buf->last_access_time_ns > max_age_ns)) {
            lle_buffer_manager_delete_buffer(manager, buf->buffer_id);
        }
        
        buf = next;
    }
    
    return LLE_BUFFER_MGR_OK;
}

// Utility: Convert error code to string
const char* lle_buffer_mgr_error_string(int error_code) {
    switch (error_code) {
        case LLE_BUFFER_MGR_OK: return "Success";
        case LLE_BUFFER_MGR_ERR_NULL_PTR: return "Null pointer";
        case LLE_BUFFER_MGR_ERR_ALLOC_FAILED: return "Memory allocation failed";
        case LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND: return "Buffer not found";
        case LLE_BUFFER_MGR_ERR_BUFFER_EXISTS: return "Buffer already exists";
        case LLE_BUFFER_MGR_ERR_INVALID_NAME: return "Invalid buffer name";
        case LLE_BUFFER_MGR_ERR_MAX_BUFFERS: return "Maximum buffers reached";
        default: return "Unknown error";
    }
}
