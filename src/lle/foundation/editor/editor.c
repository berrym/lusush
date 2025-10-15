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
    
    // Cleanup kill ring
    for (size_t i = 0; i < editor->kill_ring.count && i < LLE_KILL_RING_SIZE; i++) {
        if (editor->kill_ring.entries[i].text) {
            free(editor->kill_ring.entries[i].text);
            editor->kill_ring.entries[i].text = NULL;
        }
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

// Helper: Check if character is a word character
static bool is_word_char(char ch) {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           (ch == '_');
}

// Helper: Find end of word at cursor (for deletion)
// Returns position right after the word, NOT including trailing whitespace
static lle_buffer_pos_t find_word_end(const lle_buffer_t *buffer,
                                       lle_buffer_pos_t start) {
    size_t size = lle_buffer_size(buffer);
    if (start >= size) {
        return size;
    }
    
    lle_buffer_pos_t pos = start;
    char ch;
    
    // Get current character
    if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
        return start;
    }
    
    // If on whitespace, don't move
    if (ch == ' ' || ch == '\t' || ch == '\n') {
        return start;
    }
    
    // Determine if we're on a word or punctuation
    bool on_word = is_word_char(ch);
    
    // Skip to end of current word/punctuation sequence
    while (pos < size) {
        if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
            break;
        }
        
        // Stop at whitespace or different character type
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            break;
        }
        
        if (on_word && !is_word_char(ch)) {
            break;
        }
        if (!on_word && is_word_char(ch)) {
            break;
        }
        
        pos++;
    }
    
    return pos;
}

// Helper: Find next word boundary (forward)
// Moves to the start of the next word (skips current word AND whitespace)
static lle_buffer_pos_t find_next_word_boundary(const lle_buffer_t *buffer,
                                                 lle_buffer_pos_t start) {
    size_t size = lle_buffer_size(buffer);
    if (start >= size) {
        return size;
    }
    
    lle_buffer_pos_t pos = start;
    char ch;
    
    // Get current character type
    if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
        return start;
    }
    
    // If we're on a word character, skip to end of current word
    if (is_word_char(ch)) {
        while (pos < size) {
            if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
                break;
            }
            if (!is_word_char(ch)) {
                break;
            }
            pos++;
        }
    } else if (ch != ' ' && ch != '\t' && ch != '\n') {
        // On punctuation - skip punctuation characters
        while (pos < size) {
            if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
                break;
            }
            // Stop at word char or whitespace
            if (is_word_char(ch) || ch == ' ' || ch == '\t' || ch == '\n') {
                break;
            }
            pos++;
        }
    }
    
    // Skip any whitespace to get to start of next word
    while (pos < size) {
        if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
            break;
        }
        if (ch != ' ' && ch != '\t') {
            break;
        }
        pos++;
    }
    
    return pos;
}

// Helper: Find previous word boundary (backward)
static lle_buffer_pos_t find_prev_word_boundary(const lle_buffer_t *buffer,
                                                 lle_buffer_pos_t start) {
    if (start == 0) {
        return 0;
    }
    
    lle_buffer_pos_t pos = start - 1;
    
    // Skip trailing whitespace
    char ch;
    while (pos > 0) {
        if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
            return 0;
        }
        if (ch != ' ' && ch != '\t') {
            break;
        }
        if (pos == 0) break;
        pos--;
    }
    
    // Get character type at current position
    if (lle_buffer_get_char(buffer, pos, &ch) != LLE_BUFFER_OK) {
        return 0;
    }
    bool in_word = is_word_char(ch);
    
    // Skip characters of the same type
    while (pos > 0) {
        lle_buffer_pos_t prev = pos - 1;
        if (lle_buffer_get_char(buffer, prev, &ch) != LLE_BUFFER_OK) {
            break;
        }
        
        bool ch_is_word = is_word_char(ch);
        if (in_word != ch_is_word) {
            break;
        }
        pos = prev;
    }
    
    return pos;
}

// Advanced navigation: Move forward by word
int lle_editor_move_word_forward(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t new_pos = find_next_word_boundary(&editor->buffer,
                                                        editor->state.cursor_pos);
    if (new_pos != editor->state.cursor_pos) {
        editor->state.cursor_pos = new_pos;
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Advanced navigation: Move backward by word
int lle_editor_move_word_backward(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t new_pos = find_prev_word_boundary(&editor->buffer,
                                                        editor->state.cursor_pos);
    if (new_pos != editor->state.cursor_pos) {
        editor->state.cursor_pos = new_pos;
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Advanced navigation: Move to line start
int lle_editor_move_to_line_start(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t line_start = lle_buffer_line_start(&editor->buffer,
                                                         editor->state.cursor_pos);
    if (line_start != editor->state.cursor_pos) {
        editor->state.cursor_pos = line_start;
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Advanced navigation: Move to line end
int lle_editor_move_to_line_end(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t line_end = lle_buffer_line_end(&editor->buffer,
                                                     editor->state.cursor_pos);
    if (line_end != editor->state.cursor_pos) {
        editor->state.cursor_pos = line_end;
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Advanced editing: Delete word before cursor (Meta-Backspace)
int lle_editor_delete_word_before_cursor(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (editor->state.cursor_pos == 0) {
        return LLE_EDITOR_OK;  // Nothing to delete
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t word_start = find_prev_word_boundary(&editor->buffer,
                                                           editor->state.cursor_pos);
    
    if (word_start < editor->state.cursor_pos) {
        int result = lle_buffer_delete_range(&editor->buffer,
                                            word_start,
                                            editor->state.cursor_pos);
        if (result != LLE_BUFFER_OK) {
            return result;
        }
        
        editor->state.cursor_pos = word_start;
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Advanced editing: Delete word at cursor (Meta-d)
int lle_editor_delete_word_at_cursor(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (editor->state.cursor_pos >= size) {
        return LLE_EDITOR_OK;  // Nothing to delete
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Use find_word_end which doesn't skip trailing whitespace
    lle_buffer_pos_t word_end = find_word_end(&editor->buffer,
                                               editor->state.cursor_pos);
    
    if (word_end > editor->state.cursor_pos) {
        int result = lle_buffer_delete_range(&editor->buffer,
                                            editor->state.cursor_pos,
                                            word_end);
        if (result != LLE_BUFFER_OK) {
            return result;
        }
        
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// ============================================================================
// Kill Ring Helper Functions
// ============================================================================

// Helper: Add text to kill ring
static int kill_ring_add(lle_kill_ring_t *ring, const char *text, size_t length) {
    if (!ring || !text || length == 0) {
        return LLE_EDITOR_ERR_NULL_PTR;
    }
    
    // Limit entry size
    if (length > LLE_KILL_MAX_ENTRY_SIZE) {
        length = LLE_KILL_MAX_ENTRY_SIZE;
    }
    
    // Free old entry if ring is full
    if (ring->count == LLE_KILL_RING_SIZE) {
        if (ring->entries[ring->head].text) {
            free(ring->entries[ring->head].text);
            ring->entries[ring->head].text = NULL;
        }
    }
    
    // Allocate new entry
    ring->entries[ring->head].text = malloc(length + 1);
    if (!ring->entries[ring->head].text) {
        return LLE_EDITOR_ERR_NULL_PTR;
    }
    
    // Copy text
    memcpy(ring->entries[ring->head].text, text, length);
    ring->entries[ring->head].text[length] = '\0';
    ring->entries[ring->head].length = length;
    
    // Update ring state
    ring->head = (ring->head + 1) % LLE_KILL_RING_SIZE;
    if (ring->count < LLE_KILL_RING_SIZE) {
        ring->count++;
    }
    
    // Reset yank state
    ring->yank_index = (ring->head == 0) ? (ring->count - 1) : (ring->head - 1);
    ring->last_was_yank = false;
    
    return LLE_EDITOR_OK;
}

// Helper: Get most recent kill entry
static const lle_kill_entry_t* kill_ring_get_recent(const lle_kill_ring_t *ring) {
    if (!ring || ring->count == 0) {
        return NULL;
    }
    
    size_t index = (ring->head == 0) ? (ring->count - 1) : (ring->head - 1);
    return &ring->entries[index];
}

// Helper: Get entry at yank index
static const lle_kill_entry_t* kill_ring_get_at_yank_index(const lle_kill_ring_t *ring) {
    if (!ring || ring->count == 0) {
        return NULL;
    }
    
    return &ring->entries[ring->yank_index];
}

// Helper: Cycle yank index to previous entry
static void kill_ring_cycle_yank_index(lle_kill_ring_t *ring) {
    if (!ring || ring->count == 0) {
        return;
    }
    
    if (ring->yank_index == 0) {
        ring->yank_index = ring->count - 1;
    } else {
        ring->yank_index--;
    }
}

// ============================================================================
// Kill and Yank Operations
// ============================================================================

// Advanced editing: Kill line from cursor to end (Ctrl-k)
int lle_editor_kill_line(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (editor->state.cursor_pos >= size) {
        return LLE_EDITOR_OK;  // Nothing to delete
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t line_end = lle_buffer_line_end(&editor->buffer,
                                                     editor->state.cursor_pos);
    
    if (line_end > editor->state.cursor_pos) {
        // Get text to save to kill ring
        size_t kill_len = line_end - editor->state.cursor_pos;
        char *kill_text = malloc(kill_len + 1);
        if (kill_text) {
            int result = lle_buffer_get_substring(&editor->buffer,
                                                 editor->state.cursor_pos,
                                                 line_end,
                                                 kill_text, kill_len + 1);
            if (result == LLE_BUFFER_OK) {
                kill_ring_add(&editor->kill_ring, kill_text, kill_len);
            }
            free(kill_text);
        }
        
        // Delete the text
        int result = lle_buffer_delete_range(&editor->buffer,
                                            editor->state.cursor_pos,
                                            line_end);
        if (result != LLE_BUFFER_OK) {
            return result;
        }
        
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Advanced editing: Kill whole line (Ctrl-u)
int lle_editor_kill_whole_line(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (size == 0) {
        return LLE_EDITOR_OK;  // Nothing to delete
    }
    
    uint64_t start = get_timestamp_ns();
    
    lle_buffer_pos_t line_start = lle_buffer_line_start(&editor->buffer,
                                                         editor->state.cursor_pos);
    lle_buffer_pos_t line_end = lle_buffer_line_end(&editor->buffer,
                                                     editor->state.cursor_pos);
    
    if (line_end > line_start) {
        // Get text to save to kill ring
        size_t kill_len = line_end - line_start;
        char *kill_text = malloc(kill_len + 1);
        if (kill_text) {
            int result = lle_buffer_get_substring(&editor->buffer,
                                                 line_start,
                                                 line_end,
                                                 kill_text, kill_len + 1);
            if (result == LLE_BUFFER_OK) {
                kill_ring_add(&editor->kill_ring, kill_text, kill_len);
            }
            free(kill_text);
        }
        
        // Delete the text
        int result = lle_buffer_delete_range(&editor->buffer,
                                            line_start,
                                            line_end);
        if (result != LLE_BUFFER_OK) {
            return result;
        }
        
        editor->state.cursor_pos = line_start;
        editor->state.needs_redraw = true;
    }
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Kill ring: Yank (paste most recent kill)
int lle_editor_yank(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    const lle_kill_entry_t *entry = kill_ring_get_recent(&editor->kill_ring);
    if (!entry || !entry->text) {
        return LLE_EDITOR_OK;  // Nothing to yank
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Insert the killed text at cursor
    int result = lle_buffer_insert_string(&editor->buffer,
                                         editor->state.cursor_pos,
                                         entry->text,
                                         entry->length);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Track yank for yank-pop
    editor->kill_ring.last_yank_start = editor->state.cursor_pos;
    editor->kill_ring.last_yank_end = editor->state.cursor_pos + entry->length;
    editor->kill_ring.last_was_yank = true;
    
    // Move cursor to end of yanked text
    editor->state.cursor_pos += entry->length;
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Kill ring: Yank-pop (cycle through kill ring)
int lle_editor_yank_pop(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Only works immediately after yank
    if (!editor->kill_ring.last_was_yank) {
        return LLE_EDITOR_OK;  // Silently ignore if not after yank
    }
    
    // Need at least 2 entries to pop
    if (editor->kill_ring.count < 2) {
        return LLE_EDITOR_OK;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Delete the previously yanked text
    int result = lle_buffer_delete_range(&editor->buffer,
                                        editor->kill_ring.last_yank_start,
                                        editor->kill_ring.last_yank_end);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Cycle to previous entry
    kill_ring_cycle_yank_index(&editor->kill_ring);
    
    // Get the new entry to yank
    const lle_kill_entry_t *entry = kill_ring_get_at_yank_index(&editor->kill_ring);
    if (!entry || !entry->text) {
        return LLE_EDITOR_ERR_NULL_PTR;
    }
    
    // Insert the new text
    result = lle_buffer_insert_string(&editor->buffer,
                                     editor->kill_ring.last_yank_start,
                                     entry->text,
                                     entry->length);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Update yank tracking
    editor->kill_ring.last_yank_end = editor->kill_ring.last_yank_start + entry->length;
    
    // Move cursor to end of yanked text
    editor->state.cursor_pos = editor->kill_ring.last_yank_end;
    editor->state.needs_redraw = true;
    
    uint64_t end = get_timestamp_ns();
    editor->total_op_time_ns += (end - start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// Kill ring: Kill region
int lle_editor_kill_region(lle_editor_t *editor,
                           lle_buffer_pos_t start,
                           lle_buffer_pos_t end) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    size_t size = lle_buffer_size(&editor->buffer);
    if (start > size || end > size || start >= end) {
        return LLE_EDITOR_ERR_INVALID_POS;
    }
    
    uint64_t op_start = get_timestamp_ns();
    
    // Get the text to kill
    size_t kill_len = end - start;
    char *kill_text = malloc(kill_len + 1);
    if (!kill_text) {
        return LLE_EDITOR_ERR_NULL_PTR;
    }
    
    int result = lle_buffer_get_substring(&editor->buffer, start, end,
                                         kill_text, kill_len + 1);
    if (result != LLE_BUFFER_OK) {
        free(kill_text);
        return result;
    }
    
    // Add to kill ring
    result = kill_ring_add(&editor->kill_ring, kill_text, kill_len);
    free(kill_text);
    if (result != LLE_EDITOR_OK) {
        return result;
    }
    
    // Delete the region
    result = lle_buffer_delete_range(&editor->buffer, start, end);
    if (result != LLE_BUFFER_OK) {
        return result;
    }
    
    // Move cursor to start of killed region
    editor->state.cursor_pos = start;
    editor->state.needs_redraw = true;
    
    uint64_t op_end = get_timestamp_ns();
    editor->total_op_time_ns += (op_end - op_start);
    editor->operation_count++;
    
    return LLE_EDITOR_OK;
}

// ============================================================================
// Incremental Search Operations
// ============================================================================

// Start incremental search forward
int lle_editor_search_forward(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Initialize search state
    editor->search.active = true;
    editor->search.forward = true;
    editor->search.pattern_len = 0;
    editor->search.pattern[0] = '\0';
    editor->search.search_start_pos = editor->state.cursor_pos;
    editor->search.found = false;
    editor->search.match_count = 0;
    editor->search.match_start = 0;
    editor->search.match_end = 0;
    
    return LLE_EDITOR_OK;
}

// Start incremental search backward
int lle_editor_search_backward(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Initialize search state
    editor->search.active = true;
    editor->search.forward = false;
    editor->search.pattern_len = 0;
    editor->search.pattern[0] = '\0';
    editor->search.search_start_pos = editor->state.cursor_pos;
    editor->search.found = false;
    editor->search.match_count = 0;
    editor->search.match_start = 0;
    editor->search.match_end = 0;
    
    return LLE_EDITOR_OK;
}

// Helper: Search forward from position
static lle_buffer_pos_t search_forward_from(lle_editor_t *editor, 
                                             lle_buffer_pos_t start_pos) {
    if (editor->search.pattern_len == 0) {
        return (lle_buffer_pos_t)-1;  // No pattern
    }
    
    return lle_buffer_find_string(&editor->buffer,
                                  start_pos,
                                  editor->search.pattern,
                                  editor->search.pattern_len);
}

// Helper: Search backward from position
static lle_buffer_pos_t search_backward_from(lle_editor_t *editor,
                                             lle_buffer_pos_t start_pos) {
    if (editor->search.pattern_len == 0 || start_pos == 0) {
        return (lle_buffer_pos_t)-1;  // No pattern or at beginning
    }
    
    // Search backwards by trying each position from start_pos-1 to 0
    lle_buffer_pos_t pos = start_pos;
    while (pos > 0) {
        pos--;
        
        // Check if pattern matches at this position
        char buffer[LLE_SEARCH_MAX_PATTERN];
        size_t available = lle_buffer_size(&editor->buffer) - pos;
        size_t check_len = (available < editor->search.pattern_len) ? 
                          available : editor->search.pattern_len;
        
        if (check_len < editor->search.pattern_len) {
            continue;  // Not enough space for pattern
        }
        
        int result = lle_buffer_get_substring(&editor->buffer, pos, 
                                              pos + editor->search.pattern_len,
                                              buffer, sizeof(buffer));
        if (result == LLE_BUFFER_OK) {
            if (memcmp(buffer, editor->search.pattern, editor->search.pattern_len) == 0) {
                return pos;  // Found match
            }
        }
    }
    
    return (lle_buffer_pos_t)-1;  // Not found
}

// Add character to search pattern
int lle_editor_search_add_char(lle_editor_t *editor, char ch) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (!editor->search.active) {
        return LLE_EDITOR_OK;  // Not searching
    }
    
    if (editor->search.pattern_len >= LLE_SEARCH_MAX_PATTERN - 1) {
        return LLE_EDITOR_OK;  // Pattern too long
    }
    
    // Add character to pattern
    editor->search.pattern[editor->search.pattern_len++] = ch;
    editor->search.pattern[editor->search.pattern_len] = '\0';
    
    // Search for new pattern
    lle_buffer_pos_t match_pos;
    if (editor->search.forward) {
        match_pos = search_forward_from(editor, editor->search.search_start_pos);
    } else {
        match_pos = search_backward_from(editor, editor->search.search_start_pos);
    }
    
    if (match_pos != (lle_buffer_pos_t)-1) {
        // Found match
        editor->search.found = true;
        editor->search.match_start = match_pos;
        editor->search.match_end = match_pos + editor->search.pattern_len;
        editor->search.match_count = 1;
        editor->state.cursor_pos = match_pos;
        editor->state.needs_redraw = true;
    } else {
        // No match
        editor->search.found = false;
        editor->search.match_count = 0;
    }
    
    return LLE_EDITOR_OK;
}

// Remove last character from search pattern
int lle_editor_search_backspace(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (!editor->search.active || editor->search.pattern_len == 0) {
        return LLE_EDITOR_OK;
    }
    
    // Remove last character
    editor->search.pattern_len--;
    editor->search.pattern[editor->search.pattern_len] = '\0';
    
    if (editor->search.pattern_len == 0) {
        // Empty pattern - return to start position
        editor->search.found = false;
        editor->search.match_count = 0;
        editor->state.cursor_pos = editor->search.search_start_pos;
        editor->state.needs_redraw = true;
        return LLE_EDITOR_OK;
    }
    
    // Re-search with shorter pattern
    lle_buffer_pos_t match_pos;
    if (editor->search.forward) {
        match_pos = search_forward_from(editor, editor->search.search_start_pos);
    } else {
        match_pos = search_backward_from(editor, editor->search.search_start_pos);
    }
    
    if (match_pos != (lle_buffer_pos_t)-1) {
        editor->search.found = true;
        editor->search.match_start = match_pos;
        editor->search.match_end = match_pos + editor->search.pattern_len;
        editor->search.match_count = 1;
        editor->state.cursor_pos = match_pos;
        editor->state.needs_redraw = true;
    } else {
        editor->search.found = false;
        editor->search.match_count = 0;
    }
    
    return LLE_EDITOR_OK;
}

// Find next match
int lle_editor_search_next(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (!editor->search.active || editor->search.pattern_len == 0) {
        return LLE_EDITOR_OK;
    }
    
    // Start search from after current match
    lle_buffer_pos_t start_pos;
    if (editor->search.found) {
        start_pos = editor->search.match_end;
    } else {
        start_pos = editor->state.cursor_pos + 1;
    }
    
    lle_buffer_pos_t match_pos;
    if (editor->search.forward) {
        match_pos = search_forward_from(editor, start_pos);
    } else {
        match_pos = search_backward_from(editor, start_pos);
    }
    
    if (match_pos != (lle_buffer_pos_t)-1) {
        editor->search.found = true;
        editor->search.match_start = match_pos;
        editor->search.match_end = match_pos + editor->search.pattern_len;
        editor->search.match_count++;
        editor->state.cursor_pos = match_pos;
        editor->state.needs_redraw = true;
    } else {
        // Wrap around - search from beginning/end
        if (editor->search.forward) {
            match_pos = search_forward_from(editor, 0);
        } else {
            match_pos = search_backward_from(editor, lle_buffer_size(&editor->buffer));
        }
        
        if (match_pos != (lle_buffer_pos_t)-1) {
            editor->search.found = true;
            editor->search.match_start = match_pos;
            editor->search.match_end = match_pos + editor->search.pattern_len;
            editor->state.cursor_pos = match_pos;
            editor->state.needs_redraw = true;
        }
    }
    
    return LLE_EDITOR_OK;
}

// Find previous match
int lle_editor_search_previous(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (!editor->search.active || editor->search.pattern_len == 0) {
        return LLE_EDITOR_OK;
    }
    
    // Reverse direction temporarily
    bool was_forward = editor->search.forward;
    editor->search.forward = !editor->search.forward;
    
    // Find next in reversed direction
    int result = lle_editor_search_next(editor);
    
    // Restore direction
    editor->search.forward = was_forward;
    
    return result;
}

// Cancel search
int lle_editor_search_cancel(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (!editor->search.active) {
        return LLE_EDITOR_OK;
    }
    
    // Return to original position
    editor->state.cursor_pos = editor->search.search_start_pos;
    
    // Clear search state
    editor->search.active = false;
    editor->search.found = false;
    editor->search.pattern_len = 0;
    editor->search.match_count = 0;
    
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

// Accept search
int lle_editor_search_accept(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    if (!editor->search.active) {
        return LLE_EDITOR_OK;
    }
    
    // Stay at current position (cursor already moved to match)
    
    // Clear search state
    editor->search.active = false;
    editor->search.found = false;
    editor->search.pattern_len = 0;
    editor->search.match_count = 0;
    
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

// Get search state
const lle_search_state_t* lle_editor_get_search_state(const lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return NULL;
    }
    
    return &editor->search;
}

// History operations

// Set history system
void lle_editor_set_history(lle_editor_t *editor, lle_history_t *history) {
    if (!editor || !editor->initialized) {
        return;
    }
    
    editor->history = history;
}

// Navigate to previous history entry
int lle_editor_history_previous(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // No history system set
    if (!editor->history) {
        return LLE_EDITOR_OK;
    }
    
    size_t history_count = lle_history_count(editor->history);
    if (history_count == 0) {
        return LLE_EDITOR_OK;  // No history
    }
    
    // First time navigating history - save current line
    if (!editor->history_nav.navigating) {
        // Get current buffer contents
        size_t size = lle_buffer_size(&editor->buffer);
        if (size > 0) {
            editor->history_nav.saved_line = malloc(size + 1);
            if (!editor->history_nav.saved_line) {
                return LLE_EDITOR_ERR_NOT_INIT;  // Memory allocation failed
            }
            
            lle_buffer_get_contents(&editor->buffer, 
                                   editor->history_nav.saved_line, 
                                   size + 1);
            editor->history_nav.saved_line_len = size;
        } else {
            editor->history_nav.saved_line = NULL;
            editor->history_nav.saved_line_len = 0;
        }
        
        editor->history_nav.navigating = true;
        editor->history_nav.current_index = 0;
    } else {
        // Already navigating, move to older entry
        if (editor->history_nav.current_index < history_count - 1) {
            editor->history_nav.current_index++;
        } else {
            return LLE_EDITOR_OK;  // At oldest entry
        }
    }
    
    // Get history entry
    const lle_history_entry_t *entry = lle_history_get(editor->history, 
                                                        editor->history_nav.current_index);
    if (!entry || !entry->line) {
        return LLE_EDITOR_OK;
    }
    
    // Replace buffer contents with history entry
    lle_buffer_clear(&editor->buffer);
    lle_buffer_insert_string(&editor->buffer, 0, entry->line, entry->line_len);
    editor->state.cursor_pos = entry->line_len;
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

// Navigate to next history entry
int lle_editor_history_next(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Not navigating history
    if (!editor->history_nav.navigating) {
        return LLE_EDITOR_OK;
    }
    
    // At the newest entry (current line)
    if (editor->history_nav.current_index == 0) {
        // Restore original line
        lle_buffer_clear(&editor->buffer);
        if (editor->history_nav.saved_line && editor->history_nav.saved_line_len > 0) {
            lle_buffer_insert_string(&editor->buffer, 0, 
                                    editor->history_nav.saved_line,
                                    editor->history_nav.saved_line_len);
            editor->state.cursor_pos = editor->history_nav.saved_line_len;
        } else {
            editor->state.cursor_pos = 0;
        }
        
        // Cleanup navigation state
        if (editor->history_nav.saved_line) {
            free(editor->history_nav.saved_line);
            editor->history_nav.saved_line = NULL;
        }
        editor->history_nav.navigating = false;
        editor->state.needs_redraw = true;
        
        return LLE_EDITOR_OK;
    }
    
    // Move to newer entry
    editor->history_nav.current_index--;
    
    // Get history entry
    const lle_history_entry_t *entry = lle_history_get(editor->history, 
                                                        editor->history_nav.current_index);
    if (!entry || !entry->line) {
        return LLE_EDITOR_OK;
    }
    
    // Replace buffer contents with history entry
    lle_buffer_clear(&editor->buffer);
    lle_buffer_insert_string(&editor->buffer, 0, entry->line, entry->line_len);
    editor->state.cursor_pos = entry->line_len;
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
}

// Accept current line and add to history
int lle_editor_history_accept_line(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Cleanup navigation state if active
    if (editor->history_nav.navigating) {
        if (editor->history_nav.saved_line) {
            free(editor->history_nav.saved_line);
            editor->history_nav.saved_line = NULL;
        }
        editor->history_nav.navigating = false;
        editor->history_nav.current_index = 0;
    }
    
    // No history system set
    if (!editor->history) {
        return LLE_EDITOR_OK;
    }
    
    // Get current buffer contents
    size_t size = lle_buffer_size(&editor->buffer);
    if (size == 0) {
        return LLE_EDITOR_OK;  // Don't add empty lines
    }
    
    char *line = malloc(size + 1);
    if (!line) {
        return LLE_EDITOR_ERR_NOT_INIT;  // Memory allocation failed
    }
    
    lle_buffer_get_contents(&editor->buffer, line, size + 1);
    
    // Add to history
    lle_history_add(editor->history, line);
    
    free(line);
    
    return LLE_EDITOR_OK;
}

// Cancel history navigation and restore original line
int lle_editor_history_cancel(lle_editor_t *editor) {
    if (!editor || !editor->initialized) {
        return LLE_EDITOR_ERR_NOT_INIT;
    }
    
    // Not navigating history
    if (!editor->history_nav.navigating) {
        return LLE_EDITOR_OK;
    }
    
    // Restore original line
    lle_buffer_clear(&editor->buffer);
    if (editor->history_nav.saved_line && editor->history_nav.saved_line_len > 0) {
        lle_buffer_insert_string(&editor->buffer, 0, 
                                editor->history_nav.saved_line,
                                editor->history_nav.saved_line_len);
        editor->state.cursor_pos = editor->history_nav.saved_line_len;
    } else {
        editor->state.cursor_pos = 0;
    }
    
    // Cleanup navigation state
    if (editor->history_nav.saved_line) {
        free(editor->history_nav.saved_line);
        editor->history_nav.saved_line = NULL;
    }
    editor->history_nav.navigating = false;
    editor->history_nav.current_index = 0;
    editor->state.needs_redraw = true;
    
    return LLE_EDITOR_OK;
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
