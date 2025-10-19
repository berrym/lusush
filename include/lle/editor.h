/**
 * LLE Editor Integration - Public API
 * 
 * Week 3 Integration: Connect buffer with display
 * 
 * Goal: Demonstrate that Week 3 buffer integrates with Week 2 display
 *       to create a minimal working editor.
 * 
 * Design Principles (MANDATORY):
 * - Thin integration layer between buffer and display
 * - No direct terminal writes (through display system)
 * - Simple API for basic editing operations
 * - ASCII-only, single-line for Week 3
 * 
 * This is the integration point that proves Week 1 + Week 2 + Week 3
 * work together correctly.
 */

#ifndef LLE_EDITOR_H
#define LLE_EDITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <lle/buffer.h>
#include <lle/display.h>
#include <lle/terminal.h>

/* Forward declarations for Lusush types */
typedef struct command_layer_s command_layer_t;

/**
 * Editor handle (opaque)
 */
typedef struct lle_editor_s lle_editor_t;

/**
 * Editor operation result codes
 */
typedef enum {
    LLE_EDITOR_SUCCESS = 0,
    LLE_EDITOR_ERROR_MEMORY,
    LLE_EDITOR_ERROR_INVALID_PARAMETER,
    LLE_EDITOR_ERROR_NOT_INITIALIZED,
    LLE_EDITOR_ERROR_BUFFER_FAILED,
    LLE_EDITOR_ERROR_DISPLAY_FAILED,
} lle_editor_result_t;

/**
 * Initialize editor
 * 
 * Creates a complete editor with buffer + display integration.
 * 
 * @param editor Output: Editor handle (allocated by function)
 * @param capabilities Terminal capabilities from Week 1
 * @param command_layer Lusush command_layer for rendering
 * @param initial_capacity Initial buffer capacity (bytes)
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_init(
    lle_editor_t **editor,
    lle_terminal_capabilities_t *capabilities,
    command_layer_t *command_layer,
    size_t initial_capacity
);

/**
 * Insert text at cursor
 * 
 * @param editor Editor handle
 * @param text Text to insert (NULL-terminated)
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_insert(
    lle_editor_t *editor,
    const char *text
);

/**
 * Insert single character at cursor
 * 
 * @param editor Editor handle
 * @param ch Character to insert
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_insert_char(
    lle_editor_t *editor,
    char ch
);

/**
 * Delete character before cursor (backspace)
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_backspace(
    lle_editor_t *editor
);

/**
 * Delete character at cursor (delete key)
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_delete(
    lle_editor_t *editor
);

/**
 * Move cursor left
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_move_left(
    lle_editor_t *editor
);

/**
 * Move cursor right
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_move_right(
    lle_editor_t *editor
);

/**
 * Move cursor to start (Home)
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_move_home(
    lle_editor_t *editor
);

/**
 * Move cursor to end (End)
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_move_end(
    lle_editor_t *editor
);

/**
 * Clear editor content
 * 
 * @param editor Editor handle
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_clear(
    lle_editor_t *editor
);

/**
 * Get editor content (for testing)
 * 
 * @param editor Editor handle
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_get_content(
    const lle_editor_t *editor,
    char *buffer,
    size_t buffer_size
);

/**
 * Get cursor position (for testing)
 * 
 * @param editor Editor handle
 * @param position Output: Cursor position
 * @return LLE_EDITOR_SUCCESS or error code
 */
lle_editor_result_t lle_editor_get_cursor(
    const lle_editor_t *editor,
    size_t *position
);

/**
 * Destroy editor
 * 
 * @param editor Editor handle to destroy
 */
void lle_editor_destroy(
    lle_editor_t *editor
);

#endif /* LLE_EDITOR_H */
