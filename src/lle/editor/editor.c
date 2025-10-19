/**
 * LLE Editor Integration - Implementation
 * 
 * Week 3: Connect buffer (Week 3) with display (Week 2)
 * 
 * This is the integration layer that proves Week 1 + Week 2 + Week 3
 * work together to create a functional editor.
 */

#include "lle/editor.h"
#include <stdlib.h>
#include <string.h>

/**
 * Editor structure
 * 
 * Combines buffer and display into unified editor.
 */
struct lle_editor_s {
    lle_buffer_t *buffer;      // Text buffer (Week 3)
    lle_display_t *display;    // Display integration (Week 2)
    bool initialized;
};

/**
 * Helper: Refresh display after buffer changes
 * 
 * Gets content from buffer and sends to display.
 */
static lle_editor_result_t refresh_display(lle_editor_t *editor) {
    // Get buffer content
    char content[4096];
    lle_buffer_result_t buf_result = lle_buffer_get_content(
        editor->buffer, content, sizeof(content)
    );
    if (buf_result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Get cursor position
    size_t cursor_pos;
    buf_result = lle_buffer_get_cursor(editor->buffer, &cursor_pos);
    if (buf_result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Update display
    lle_display_result_t disp_result = lle_display_update(
        editor->display, content, cursor_pos, LLE_DISPLAY_UPDATE_NORMAL
    );
    if (disp_result != LLE_DISPLAY_SUCCESS) {
        return LLE_EDITOR_ERROR_DISPLAY_FAILED;
    }
    
    return LLE_EDITOR_SUCCESS;
}

/**
 * Initialize editor
 */
lle_editor_result_t lle_editor_init(
    lle_editor_t **editor,
    lle_terminal_capabilities_t *capabilities,
    command_layer_t *command_layer,
    size_t initial_capacity)
{
    if (!editor) {
        return LLE_EDITOR_ERROR_INVALID_PARAMETER;
    }
    
    // Allocate editor
    lle_editor_t *ed = calloc(1, sizeof(lle_editor_t));
    if (!ed) {
        return LLE_EDITOR_ERROR_MEMORY;
    }
    
    // Initialize buffer (Week 3)
    lle_buffer_result_t buf_result = lle_buffer_init(&ed->buffer, initial_capacity);
    if (buf_result != LLE_BUFFER_SUCCESS) {
        free(ed);
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Initialize display (Week 2)
    lle_display_result_t disp_result = lle_display_init(
        &ed->display, capabilities, command_layer
    );
    if (disp_result != LLE_DISPLAY_SUCCESS) {
        lle_buffer_destroy(ed->buffer);
        free(ed);
        return LLE_EDITOR_ERROR_DISPLAY_FAILED;
    }
    
    ed->initialized = true;
    *editor = ed;
    
    // Initial display refresh (empty buffer)
    refresh_display(ed);
    
    return LLE_EDITOR_SUCCESS;
}

/**
 * Insert text at cursor
 */
lle_editor_result_t lle_editor_insert(
    lle_editor_t *editor,
    const char *text)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Insert into buffer
    lle_buffer_result_t result = lle_buffer_insert(editor->buffer, text);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Insert single character at cursor
 */
lle_editor_result_t lle_editor_insert_char(
    lle_editor_t *editor,
    char ch)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Insert into buffer
    lle_buffer_result_t result = lle_buffer_insert_char(editor->buffer, ch);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Delete character before cursor (backspace)
 */
lle_editor_result_t lle_editor_backspace(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Delete from buffer
    lle_buffer_result_t result = lle_buffer_delete_before_cursor(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Delete character at cursor (delete key)
 */
lle_editor_result_t lle_editor_delete(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Delete from buffer
    lle_buffer_result_t result = lle_buffer_delete_at_cursor(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Move cursor left
 */
lle_editor_result_t lle_editor_move_left(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Move cursor in buffer
    lle_buffer_result_t result = lle_buffer_move_cursor_left(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display (cursor only)
    return refresh_display(editor);
}

/**
 * Move cursor right
 */
lle_editor_result_t lle_editor_move_right(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Move cursor in buffer
    lle_buffer_result_t result = lle_buffer_move_cursor_right(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display (cursor only)
    return refresh_display(editor);
}

/**
 * Move cursor to start (Home)
 */
lle_editor_result_t lle_editor_move_home(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Move cursor in buffer
    lle_buffer_result_t result = lle_buffer_move_cursor_home(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Move cursor to end (End)
 */
lle_editor_result_t lle_editor_move_end(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Move cursor in buffer
    lle_buffer_result_t result = lle_buffer_move_cursor_end(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Clear editor content
 */
lle_editor_result_t lle_editor_clear(
    lle_editor_t *editor)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    // Clear buffer
    lle_buffer_result_t result = lle_buffer_clear(editor->buffer);
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    // Refresh display
    return refresh_display(editor);
}

/**
 * Get editor content (for testing)
 */
lle_editor_result_t lle_editor_get_content(
    const lle_editor_t *editor,
    char *buffer,
    size_t buffer_size)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    lle_buffer_result_t result = lle_buffer_get_content(
        editor->buffer, buffer, buffer_size
    );
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    return LLE_EDITOR_SUCCESS;
}

/**
 * Get cursor position (for testing)
 */
lle_editor_result_t lle_editor_get_cursor(
    const lle_editor_t *editor,
    size_t *position)
{
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERROR_NOT_INITIALIZED;
    }
    
    lle_buffer_result_t result = lle_buffer_get_cursor(
        editor->buffer, position
    );
    if (result != LLE_BUFFER_SUCCESS) {
        return LLE_EDITOR_ERROR_BUFFER_FAILED;
    }
    
    return LLE_EDITOR_SUCCESS;
}

/**
 * Destroy editor
 */
void lle_editor_destroy(
    lle_editor_t *editor)
{
    if (!editor) {
        return;
    }
    
    if (editor->buffer) {
        lle_buffer_destroy(editor->buffer);
    }
    
    if (editor->display) {
        lle_display_destroy(editor->display);
    }
    
    free(editor);
}
