// src/lle/foundation/buffer/buffer_manager.h
//
// LLE Buffer Manager (Phase 1 Week 7)
//
// Manages multiple buffer instances with support for:
// - Named buffers (persistent buffers with user-assigned names)
// - Scratch buffers (temporary unnamed buffers)
// - Buffer switching and lifecycle management
// - Integration with undo/redo system per buffer
//
// NOTE: This manages multiple instances of the current gap buffer.
// TODO Phase 1 Month 2: Add UTF-8 indexing to buffer.h
// TODO Phase 1 Month 3: Add multiline support to buffer.h
// TODO Phase 2: Add buffer pool for memory efficiency

#ifndef LLE_FOUNDATION_BUFFER_MANAGER_H
#define LLE_FOUNDATION_BUFFER_MANAGER_H

#include "buffer.h"
#include "undo.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Result codes
typedef enum {
    LLE_BUFFER_MGR_OK = 0,
    LLE_BUFFER_MGR_ERR_NULL_PTR = -1,
    LLE_BUFFER_MGR_ERR_ALLOC_FAILED = -2,
    LLE_BUFFER_MGR_ERR_BUFFER_NOT_FOUND = -3,
    LLE_BUFFER_MGR_ERR_BUFFER_EXISTS = -4,
    LLE_BUFFER_MGR_ERR_INVALID_NAME = -5,
    LLE_BUFFER_MGR_ERR_MAX_BUFFERS = -6,
} lle_buffer_mgr_error_t;

// Buffer flags
typedef enum {
    LLE_BUFFER_FLAG_NONE = 0,
    LLE_BUFFER_FLAG_SCRATCH = (1 << 0),     // Temporary buffer
    LLE_BUFFER_FLAG_READONLY = (1 << 1),    // Read-only buffer
    LLE_BUFFER_FLAG_MODIFIED = (1 << 2),    // Modified since creation
    LLE_BUFFER_FLAG_PERSISTENT = (1 << 3),  // Named buffer (persistent)
} lle_buffer_flags_t;

// Forward declarations
typedef struct lle_buffer_manager lle_buffer_manager_t;
typedef struct lle_managed_buffer lle_managed_buffer_t;

// Managed buffer wrapper
struct lle_managed_buffer {
    // Identity
    uint32_t buffer_id;            // Unique buffer ID
    char *name;                    // Buffer name (NULL for scratch)
    
    // Buffer and undo tracker
    lle_buffer_t buffer;           // The actual gap buffer
    lle_change_tracker_t tracker;  // Undo/redo for this buffer
    
    // Metadata
    lle_buffer_flags_t flags;      // Buffer flags
    uint64_t creation_time_ns;     // Creation timestamp
    uint64_t last_access_time_ns;  // Last access timestamp
    size_t access_count;           // Access counter
    
    // Linking
    struct lle_managed_buffer *next;
    struct lle_managed_buffer *prev;
};

// Buffer manager
struct lle_buffer_manager {
    // Buffer list
    lle_managed_buffer_t *first_buffer;
    lle_managed_buffer_t *last_buffer;
    lle_managed_buffer_t *current_buffer;
    
    // Statistics
    uint32_t next_buffer_id;
    size_t buffer_count;
    size_t scratch_buffer_count;
    size_t named_buffer_count;
    
    // Configuration
    size_t max_buffers;
    size_t max_scratch_buffers;
    size_t default_buffer_capacity;
    size_t undo_max_sequences;
    size_t undo_max_memory;
};

// Initialize buffer manager
int lle_buffer_manager_init(lle_buffer_manager_t *manager,
                            size_t max_buffers,
                            size_t default_buffer_capacity);

// Cleanup buffer manager (destroys all buffers)
void lle_buffer_manager_cleanup(lle_buffer_manager_t *manager);

// Create named buffer
int lle_buffer_manager_create_buffer(lle_buffer_manager_t *manager,
                                     const char *name,
                                     uint32_t *buffer_id);

// Create scratch buffer
int lle_buffer_manager_create_scratch(lle_buffer_manager_t *manager,
                                      uint32_t *buffer_id);

// Delete buffer by ID
int lle_buffer_manager_delete_buffer(lle_buffer_manager_t *manager,
                                     uint32_t buffer_id);

// Delete buffer by name
int lle_buffer_manager_delete_buffer_by_name(lle_buffer_manager_t *manager,
                                             const char *name);

// Switch to buffer by ID
int lle_buffer_manager_switch_to_buffer(lle_buffer_manager_t *manager,
                                        uint32_t buffer_id);

// Switch to buffer by name
int lle_buffer_manager_switch_to_buffer_by_name(lle_buffer_manager_t *manager,
                                                const char *name);

// Get current buffer
lle_managed_buffer_t* lle_buffer_manager_get_current(lle_buffer_manager_t *manager);

// Get buffer by ID
lle_managed_buffer_t* lle_buffer_manager_get_buffer(lle_buffer_manager_t *manager,
                                                    uint32_t buffer_id);

// Get buffer by name
lle_managed_buffer_t* lle_buffer_manager_get_buffer_by_name(lle_buffer_manager_t *manager,
                                                            const char *name);

// Rename buffer
int lle_buffer_manager_rename_buffer(lle_buffer_manager_t *manager,
                                     uint32_t buffer_id,
                                     const char *new_name);

// List all buffers (returns array of IDs, caller must free)
uint32_t* lle_buffer_manager_list_buffers(lle_buffer_manager_t *manager,
                                          size_t *count);

// Get counts
size_t lle_buffer_manager_get_count(const lle_buffer_manager_t *manager);
size_t lle_buffer_manager_get_scratch_count(const lle_buffer_manager_t *manager);
size_t lle_buffer_manager_get_named_count(const lle_buffer_manager_t *manager);

// Check if buffer exists
bool lle_buffer_manager_has_buffer(const lle_buffer_manager_t *manager,
                                   uint32_t buffer_id);
bool lle_buffer_manager_has_buffer_by_name(const lle_buffer_manager_t *manager,
                                           const char *name);

// Clean up old scratch buffers
int lle_buffer_manager_cleanup_scratch_buffers(lle_buffer_manager_t *manager,
                                               uint64_t max_age_seconds);

// Utility
const char* lle_buffer_mgr_error_string(int error_code);

// Default configuration
#define LLE_BUFFER_MGR_DEFAULT_MAX_BUFFERS       50
#define LLE_BUFFER_MGR_DEFAULT_MAX_SCRATCH       10
#define LLE_BUFFER_MGR_DEFAULT_CAPACITY          256
#define LLE_BUFFER_MGR_SCRATCH_TIMEOUT_SECONDS   300

#endif // LLE_FOUNDATION_BUFFER_MANAGER_H
