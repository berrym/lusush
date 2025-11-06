/**
 * @file lle_editor.c
 * @brief LLE Editor Context Lifecycle Management
 *
 * Implements the lifecycle functions for lle_editor_t, including creation,
 * initialization, and destruction of the editor context and all its subsystems.
 *
 * Specification: docs/lle_specification/critical_gaps/25_default_keybindings_complete.md
 * Date: 2025-11-06
 */

#include "lle/lle_editor.h"
#include "lle/buffer_management.h"
#include "lle/kill_ring.h"
#include "lle/memory_management.h"
#include "lle/error_handling.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * EDITOR LIFECYCLE FUNCTIONS
 * ============================================================================ */

/**
 * Create a new editor context
 *
 * Initializes the editor with core subsystems. Subsystems are initialized
 * in dependency order:
 * 1. Memory pool (if not provided)
 * 2. Buffer
 * 3. Cursor manager (depends on buffer)
 * 4. Kill ring
 * 5. Other subsystems initialized on-demand
 *
 * @param editor Pointer to receive new editor context
 * @param pool Memory pool for allocations (NULL = use global pool)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_editor_create(lle_editor_t **editor, lusush_memory_pool_t *pool) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Use global pool if none provided */
    if (!pool) {
        extern lusush_memory_pool_t *global_memory_pool;
        pool = global_memory_pool;
    }
    
    /* Allocate editor structure */
    lle_editor_t *ed = (lle_editor_t *)lle_pool_alloc(sizeof(lle_editor_t));
    if (!ed) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Zero-initialize entire structure */
    memset(ed, 0, sizeof(lle_editor_t));
    
    /* Store Lusush memory pool reference */
    ed->lusush_pool = pool;
    
    /* Create LLE memory pool wrapper for unified memory management */
    lle_result_t result = lle_memory_pool_create_from_lusush(
        &ed->lle_pool, 
        pool, 
        LLE_POOL_BUFFER  /* Editor uses buffer pool type */
    );
    if (result != LLE_SUCCESS) {
        lle_pool_free(ed);
        return result;
    }
    
    /* Initialize editing mode to Emacs (default) */
    ed->editing_mode = LLE_EDITING_MODE_EMACS;
    ed->current_keymap = LLE_KEYMAP_EMACS;
    
    /* Create buffer (core subsystem) */
    result = lle_buffer_create(&ed->buffer, pool, 0);
    if (result != LLE_SUCCESS) {
        lle_pool_free(ed);
        return result;
    }
    
    /* Create cursor manager (depends on buffer) */
    result = lle_cursor_manager_init(&ed->cursor_manager, ed->buffer);
    if (result != LLE_SUCCESS) {
        lle_buffer_destroy(ed->buffer);
        lle_pool_free(ed);
        return result;
    }
    
    /* Create kill ring with default max entries (100) using unified pool */
    result = lle_kill_ring_create(&ed->kill_ring, 100, ed->lle_pool);
    if (result != LLE_SUCCESS) {
        lle_cursor_manager_destroy(ed->cursor_manager);
        lle_buffer_destroy(ed->buffer);
        lle_memory_pool_destroy(ed->lle_pool);
        lle_pool_free(ed);
        return result;
    }
    
    /* Initialize statistics */
    ed->total_keystrokes = 0;
    ed->command_count = 0;
    ed->edit_session_start = 0;  /* Will be set when editing starts */
    
    /* Other subsystems (history, completion, etc.) are initialized on-demand */
    ed->history_system = NULL;
    ed->history_buffer_integration = NULL;
    ed->keybinding_manager = NULL;
    ed->display_controller = NULL;
    ed->completion_system = NULL;
    ed->vi_state = NULL;
    ed->plugin_manager = NULL;
    ed->script_integration = NULL;
    ed->history_edit_callbacks = NULL;
    
    /* Initialize state flags */
    ed->history_search_active = false;
    ed->history_search_direction = 0;
    ed->quoted_insert_mode = false;
    
    *editor = ed;
    return LLE_SUCCESS;
}

/**
 * Destroy editor context
 *
 * Frees all resources associated with the editor, including all subsystems.
 * Subsystems are destroyed in reverse dependency order.
 *
 * @param editor Editor context to destroy
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_editor_destroy(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Destroy subsystems in reverse order of dependencies */
    
    /* Destroy optional subsystems if allocated */
    if (editor->vi_state) {
        lle_pool_free(editor->vi_state);
        editor->vi_state = NULL;
    }
    
    /* Note: history_system, keybinding_manager, display_controller, etc.
     * should have their own destroy functions called if they're not NULL.
     * For now, we only handle the core subsystems we initialize. */
    
    /* Destroy kill ring */
    if (editor->kill_ring) {
        lle_kill_ring_destroy(editor->kill_ring);
        editor->kill_ring = NULL;
    }
    
    /* Destroy cursor manager */
    if (editor->cursor_manager) {
        lle_cursor_manager_destroy(editor->cursor_manager);
        editor->cursor_manager = NULL;
    }
    
    /* Destroy buffer */
    if (editor->buffer) {
        lle_buffer_destroy(editor->buffer);
        editor->buffer = NULL;
    }
    
    /* Destroy LLE memory pool wrapper */
    if (editor->lle_pool) {
        lle_memory_pool_destroy(editor->lle_pool);
        editor->lle_pool = NULL;
    }
    
    /* Note: lusush_pool is not destroyed here - it's managed externally */
    
    /* Free editor structure */
    lle_pool_free(editor);
    
    return LLE_SUCCESS;
}

/**
 * Reset editor to initial state
 *
 * Clears the buffer and resets cursor to beginning, but keeps all
 * subsystems initialized.
 *
 * @param editor Editor context to reset
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_editor_reset(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear buffer */
    lle_result_t result = lle_buffer_clear(editor->buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Reset cursor to beginning */
    if (editor->cursor_manager) {
        result = lle_cursor_manager_move_to_byte_offset(editor->cursor_manager, 0);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    /* Clear history search state */
    editor->history_search_active = false;
    editor->history_search_direction = 0;
    
    /* Clear quoted insert mode */
    editor->quoted_insert_mode = false;
    
    return LLE_SUCCESS;
}

/**
 * Get buffer contents as string
 *
 * Returns the current buffer contents. The returned string is owned by
 * the buffer and should not be modified or freed by the caller.
 *
 * @param editor Editor context
 * @return Buffer contents or NULL on error
 */
const char *lle_editor_get_content(const lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return NULL;
    }
    
    return editor->buffer->data;
}

/**
 * Check if editor has unsaved changes
 *
 * @param editor Editor context
 * @return true if buffer has been modified, false otherwise
 */
bool lle_editor_has_unsaved_changes(const lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return false;
    }
    
    return editor->buffer->modification_count > 0;
}

/**
 * Cancel current operation
 *
 * Used for Ctrl-G (abort) functionality. Resets any active modes
 * and returns editor to normal state.
 *
 * @param editor Editor context
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_editor_cancel_operation(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Cancel history search if active */
    editor->history_search_active = false;
    editor->history_search_direction = 0;
    
    /* Exit quoted insert mode */
    editor->quoted_insert_mode = false;
    
    /* If in vi command mode, return to insert mode */
    if (editor->editing_mode == LLE_EDITING_MODE_VI_COMMAND) {
        editor->editing_mode = LLE_EDITING_MODE_VI_INSERT;
    }
    
    return LLE_SUCCESS;
}
