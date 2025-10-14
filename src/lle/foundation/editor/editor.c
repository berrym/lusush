// src/lle/foundation/editor/editor.c
//
// LLE Editor Context Implementation

#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get timestamp in nanoseconds
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int lle_editor_init(lle_editor_t *editor, int input_fd, int output_fd) {
    if (!editor) {
        return LLE_EDITOR_ERR_NULL_PTR;
    }
    
    // Zero-initialize
    memset(editor, 0, sizeof(*editor));
    
    // Initialize terminal
    int result = lle_term_init(&editor->term, input_fd, output_fd);
    if (result != LLE_TERM_OK) {
        return LLE_EDITOR_ERR_TERM_INIT;
    }
    
    // Get terminal dimensions
    const lle_term_state_t *term_state = lle_term_get_state(&editor->term);
    if (!term_state) {
        lle_term_cleanup(&editor->term);
        return LLE_EDITOR_ERR_TERM_INIT;
    }
    
    // Initialize display with terminal dimensions
    result = lle_display_init(&editor->display, &editor->term,
                              term_state->rows, term_state->cols);
    if (result != LLE_DISPLAY_OK) {
        lle_term_cleanup(&editor->term);
        return LLE_EDITOR_ERR_DISPLAY_INIT;
    }
    
    // Initialize empty buffer
    result = lle_buffer_init(&editor->buffer, 1024);
    if (result != LLE_BUFFER_OK) {
        lle_display_cleanup(&editor->display);
        lle_term_cleanup(&editor->term);
        return LLE_EDITOR_ERR_BUFFER_INIT;
    }
    
    // Initialize editor state
    editor->state.cursor_pos = 0;
    editor->state.display_offset = 0;
    editor->state.mode = LLE_EDITOR_MODE_INSERT;
    editor->state.needs_redraw = true;
    editor->state.running = false;
    
    editor->initialized = true;
    
    return LLE_EDITOR_OK;
}

int lle_editor_init_with_buffer(lle_editor_t *editor,
                                 int input_fd,
                                 int output_fd,
                                 const char *initial_text,
                                 size_t text_len) {
    if (!editor || !initial_text) {
        return LLE_EDITOR_ERR_NULL_PTR;
    }
    
    // Initialize with empty buffer first
    int result = lle_editor_init(editor, input_fd, output_fd);
    if (result != LLE_EDITOR_OK) {
        return result;
    }
    
    // Replace buffer with initial content
    lle_buffer_cleanup(&editor->buffer);
    result = lle_buffer_init_from_string(&editor->buffer, initial_text, text_len);
    if (result != LLE_BUFFER_OK) {
        lle_display_cleanup(&editor->display);
        lle_term_cleanup(&editor->term);
        editor->initialized = false;
        return LLE_EDITOR_ERR_BUFFER_INIT;
    }
    
    // Set cursor to end
    editor->state.cursor_pos = lle_buffer_size(&editor->buffer);
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

int lle_editor_set_prompt(lle_editor_t *editor, const char *prompt) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Free old prompt
    if (editor->prompt) {
        free(editor->prompt);
        editor->prompt = NULL;
        editor->prompt_len = 0;
    }
    
    // Copy new prompt
    if (prompt) {
        editor->prompt_len = strlen(prompt);
        editor->prompt = malloc(editor->prompt_len + 1);
        if (!editor->prompt) {
            editor->prompt_len = 0;
            return LLE_EDITOR_ERR_NULL_PTR;
        }
        strcpy(editor->prompt, prompt);
    }
    
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

void lle_editor_cleanup(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return;
    }
    
    if (editor->prompt) {
        free(editor->prompt);
        editor->prompt = NULL;
    }
    
    lle_buffer_cleanup(&editor->buffer);
    lle_display_cleanup(&editor->display);
    lle_term_cleanup(&editor->term);
    
    editor->initialized = false;
}

int lle_editor_move_cursor_left(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (editor->state.cursor_pos > 0) {
        uint64_t start = get_timestamp_ns();
        
        editor->state.cursor_pos--;
        editor->state.needs_redraw = true;
        
        uint64_t end = get_timestamp_ns();
        editor->total_op_time_ns += (end - start);
        editor->operation_count++;
    }
    
    return LLE_EDITOR_OK;
}

int lle_editor_move_cursor_right(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (editor->state.cursor_pos < size) {
        uint64_t start = get_timestamp_ns();
        
        editor->state.cursor_pos++;
        editor->state.needs_redraw = true;
        
        uint64_t end = get_timestamp_ns();
        editor->total_op_time_ns += (end - start);
        editor->operation_count++;
    }
    
    return LLE_EDITOR_OK;
}

int lle_editor_move_cursor_to_start(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    editor->state.cursor_pos = 0;
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

int lle_editor_move_cursor_to_end(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    editor->state.cursor_pos = lle_buffer_size(&editor->buffer);
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

int lle_editor_insert_char(lle_editor_t *editor, char ch) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    int result = lle_buffer_insert_char(&editor->buffer, editor->state.cursor_pos, ch);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    editor->state.cursor_pos++;
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

int lle_editor_insert_string(lle_editor_t *editor, const char *str, size_t len) {
    if (!editor || !editor->initialized || !str) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (len == 0) {
        return LLE_EDITOR_OK;
    }
    
    uint64_t start = get_timestamp_ns();
    
    int result = lle_buffer_insert_string(&editor->buffer, 
                                         editor->state.cursor_pos, 
                                         str, len);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    editor->state.cursor_pos += len;
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

int lle_editor_delete_char_before_cursor(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (editor->state.cursor_pos == 0) {
        return LLE_EDITOR_OK;  // Nothing to delete
    }
    
    uint64_t start = get_timestamp_ns();
    
    int result = lle_buffer_delete_char(&editor->buffer, editor->state.cursor_pos - 1);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    editor->state.cursor_pos--;
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

int lle_editor_delete_char_at_cursor(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (editor->state.cursor_pos >= size) {
        return LLE_EDITOR_OK;  // Nothing to delete
    }
    
    uint64_t start = get_timestamp_ns();
    
    int result = lle_buffer_delete_char(&editor->buffer, editor->state.cursor_pos);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

int lle_editor_get_content(const lle_editor_t *editor,
                           char *dest,
                           size_t dest_size) {
    if (!editor || !editor->initialized || !dest) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    return lle_buffer_get_contents(&editor->buffer, dest, dest_size);
}

int lle_editor_get_current_line(const lle_editor_t *editor,
                                char *dest,
                                size_t dest_size) {
    if (!editor || !editor->initialized || !dest) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Find line boundaries
    lle_buffer_pos_t line_start = lle_buffer_line_start(&editor->buffer,
                                                         editor->state.cursor_pos);
    lle_buffer_pos_t line_end = lle_buffer_line_end(&editor->buffer,
                                                     editor->state.cursor_pos);
    
    return lle_buffer_get_substring(&editor->buffer, line_start, line_end,
                                   dest, dest_size);
}

int lle_editor_redraw(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Clear display
    lle_display_clear(&editor->display);
    
    // Render prompt if present
    uint16_t row = 0;
    uint16_t col = 0;
    
    if (editor->prompt && editor->prompt_len > 0) {
        lle_display_render_line(&editor->display, row, 
                               editor->prompt, editor->prompt_len);
        col = editor->prompt_len;
    }
    
    // Render buffer content
    size_t buffer_size = lle_buffer_size(&editor->buffer);
    if (buffer_size > 0) {
        char *content = malloc(buffer_size + 1);
        if (content) {
            lle_buffer_get_contents(&editor->buffer, content, buffer_size + 1);
            lle_display_render_multiline(&editor->display, row, content, buffer_size,
                                        editor->state.cursor_pos);
            free(content);
        }
    }
    
    // Set cursor position (prompt + buffer cursor)
    lle_display_set_cursor(&editor->display, row, col + editor->state.cursor_pos);
    
    // Flush to terminal
    lle_display_flush(&editor->display);
    
    editor->state.needs_redraw = false;
    
    return LLE_EDITOR_OK;
}

int lle_editor_refresh(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (editor->state.needs_redraw) {
        return lle_editor_redraw(editor);
    }
    
    return LLE_EDITOR_OK;
}

int lle_editor_clear(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    int result = lle_buffer_clear(&editor->buffer);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    editor->state.cursor_pos = 0;
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

lle_buffer_pos_t lle_editor_get_cursor_pos(const lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return 0;
    }
    return editor->state.cursor_pos;
}

int lle_editor_set_cursor_pos(lle_editor_t *editor, lle_buffer_pos_t pos) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (pos > size) {
        return LLE_EDITOR_ERR_INVALID_POS;
    }
    
    editor->state.cursor_pos = pos;
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

size_t lle_editor_get_size(const lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return 0;
    }
    return lle_buffer_size(&editor->buffer);
}

bool lle_editor_is_modified(const lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return false;
    }
    return lle_buffer_is_modified(&editor->buffer);
}

void lle_editor_get_metrics(const lle_editor_t *editor,
                            uint64_t *operation_count,
                            double *avg_time_us) {
    if (!editor || !editor->initialized) {
        return;
    }
    
    if (operation_count) {
        *operation_count = editor->operation_count;
    }
    
    if (avg_time_us && editor->operation_count > 0) {
        *avg_time_us = (double)editor->total_op_time_ns / 
                       editor->operation_count / 1000.0;
    }
}

const char* lle_editor_error_string(int error_code) {
    switch (error_code) {
        case LLE_EDITOR_OK:
            return "Success";
        case LLE_EDITOR_ERR_NULL_PTR:
            return "Null pointer argument";
        case LLE_EDITOR_ERR_NOT_INIT:
            return "Editor not initialized";
        case LLE_EDITOR_ERR_TERM_INIT:
            return "Terminal initialization failed";
        case LLE_EDITOR_ERR_DISPLAY_INIT:
            return "Display initialization failed";
        case LLE_EDITOR_ERR_BUFFER_INIT:
            return "Buffer initialization failed";
        case LLE_EDITOR_ERR_INVALID_POS:
            return "Invalid cursor position";
        default:
            return "Unknown error";
    }
}
