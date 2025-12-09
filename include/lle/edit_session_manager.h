/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 3 */
/* Edit Session Manager: Session lifecycle and state tracking */

#ifndef LLE_EDIT_SESSION_MANAGER_H
#define LLE_EDIT_SESSION_MANAGER_H

#include "lle/command_structure.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_edit_session_manager lle_edit_session_manager_t;
typedef struct lle_edit_session lle_edit_session_t;
typedef struct lle_history_core lle_history_core_t;

/* Edit session state - need actual enum values, not forward declaration */
/**
 * Edit session state
 */
typedef enum lle_edit_session_state {
    LLE_EDIT_SESSION_INACTIVE = 0, /* No active session */
    LLE_EDIT_SESSION_ACTIVE,       /* Session active, editing in progress */
    LLE_EDIT_SESSION_MODIFIED,     /* Session has unsaved modifications */
    LLE_EDIT_SESSION_COMPLETED,    /* Session being completed */
    LLE_EDIT_SESSION_CANCELING,    /* Session being canceled */
    LLE_EDIT_SESSION_ERROR         /* Session encountered error */
} lle_edit_session_state_t;

/**
 * Edit operation type
 */
typedef enum lle_edit_operation_type {
    LLE_EDIT_OP_INSERT = 0, /* Insert text */
    LLE_EDIT_OP_DELETE,     /* Delete text */
    LLE_EDIT_OP_REPLACE,    /* Replace text */
    LLE_EDIT_OP_INDENT,     /* Change indentation */
    LLE_EDIT_OP_FORMAT      /* Apply formatting */
} lle_edit_operation_type_t;

/**
 * Edit operation record
 */
typedef struct lle_edit_operation {
    /* Operation type */
    lle_edit_operation_type_t type;

    /* Position in buffer */
    size_t offset;
    size_t length;

    /* Operation data */
    char *text;
    size_t text_length;

    /* Timestamp */
    struct timespec timestamp;

    /* Next operation in list */
    struct lle_edit_operation *next;
} lle_edit_operation_t;

/**
 * Edit session information
 */
typedef struct lle_edit_session {
    /* Session identification */
    uint64_t session_id;
    lle_edit_session_state_t state;

    /* History entry being edited */
    size_t entry_index;
    char *original_text;
    size_t original_length;

    /* Current edited content */
    char *current_text;
    size_t current_length;

    /* Command structure */
    lle_command_structure_t *structure;

    /* Edit operations */
    lle_edit_operation_t *first_operation;
    lle_edit_operation_t *last_operation;
    size_t operation_count;

    /* Session timing */
    struct timespec start_time;
    struct timespec last_modified;

    /* Flags */
    bool has_modifications;
    bool multiline_mode;

    /* Reserved for future use */
    void *reserved[2];
} lle_edit_session_t;

/**
 * Session manager configuration
 */
typedef struct lle_session_manager_config {
    /* Maximum concurrent sessions */
    size_t max_sessions;

    /* Session timeout (milliseconds, 0 = no timeout) */
    uint32_t session_timeout_ms;

    /* Track edit operations */
    bool track_operations;

    /* Maximum operations per session */
    size_t max_operations;

    /* Reserved for future use */
    void *reserved[4];
} lle_session_manager_config_t;

/**
 * Create an edit session manager
 *
 * @param manager Output parameter for created manager
 * @param memory_pool Memory pool for allocations
 * @param history_core History core for entry access
 * @param config Configuration (NULL for defaults)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_create(lle_edit_session_manager_t **manager,
                                lle_memory_pool_t *memory_pool,
                                lle_history_core_t *history_core,
                                const lle_session_manager_config_t *config);

/**
 * Destroy an edit session manager
 *
 * @param manager Manager to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_destroy(lle_edit_session_manager_t *manager);

/**
 * Start a new edit session for a history entry
 *
 * @param manager Session manager
 * @param entry_index History entry index to edit
 * @param session Output parameter for created session
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_start_session(lle_edit_session_manager_t *manager,
                                       size_t entry_index,
                                       lle_edit_session_t **session);

/**
 * Get the current active session
 *
 * @param manager Session manager
 * @param session Output parameter for current session (NULL if none)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_session_manager_get_current_session(
    lle_edit_session_manager_t *manager, lle_edit_session_t **session);

/**
 * Record an edit operation in the current session
 *
 * @param manager Session manager
 * @param session Session to record operation in
 * @param operation Operation to record
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_session_manager_record_operation(
    lle_edit_session_manager_t *manager, lle_edit_session_t *session,
    const lle_edit_operation_t *operation);

/**
 * Update the current text in a session
 *
 * @param manager Session manager
 * @param session Session to update
 * @param new_text New text content
 * @param new_length Length of new text
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_update_text(lle_edit_session_manager_t *manager,
                                     lle_edit_session_t *session,
                                     const char *new_text, size_t new_length);

/**
 * Complete an edit session (save changes)
 *
 * @param manager Session manager
 * @param session Session to complete
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_complete_session(lle_edit_session_manager_t *manager,
                                          lle_edit_session_t *session);

/**
 * Cancel an edit session (discard changes)
 *
 * @param manager Session manager
 * @param session Session to cancel
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_cancel_session(lle_edit_session_manager_t *manager,
                                        lle_edit_session_t *session);

/**
 * Check if a session has timed out
 *
 * @param manager Session manager
 * @param session Session to check
 * @param timed_out Output parameter for timeout status
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t
lle_edit_session_manager_check_timeout(lle_edit_session_manager_t *manager,
                                       lle_edit_session_t *session,
                                       bool *timed_out);

/**
 * Get default session manager configuration
 *
 * @param config Configuration structure to fill with defaults
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_session_manager_get_default_config(
    lle_session_manager_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* LLE_EDIT_SESSION_MANAGER_H */
