// src/lle/foundation/editor/editor.h
//
// LLE Editor Context - Integration Layer
//
// This module integrates terminal, display, and buffer systems into a unified
// editing context. It provides the main interface for line editing operations.

#ifndef LLE_FOUNDATION_EDITOR_H
#define LLE_FOUNDATION_EDITOR_H

#include "../terminal/terminal.h"
#include "../display/display.h"
#include "../buffer/buffer.h"
#include <stdint.h>
#include <stdbool.h>

// Error codes
typedef enum {
    LLE_EDITOR_OK = 0,
    LLE_EDITOR_ERR_NULL_PTR = -1,
    LLE_EDITOR_ERR_NOT_INIT = -2,
    LLE_EDITOR_ERR_TERM_INIT = -3,
    LLE_EDITOR_ERR_DISPLAY_INIT = -4,
    LLE_EDITOR_ERR_BUFFER_INIT = -5,
    LLE_EDITOR_ERR_INVALID_POS = -6
} lle_editor_error_t;

// Editor mode
typedef enum {
    LLE_EDITOR_MODE_INSERT,
    LLE_EDITOR_MODE_REPLACE,
    LLE_EDITOR_MODE_COMMAND
} lle_editor_mode_t;

// Kill ring entry
#define LLE_KILL_RING_SIZE 60
#define LLE_KILL_MAX_ENTRY_SIZE 8192

typedef struct {
    char *text;
    size_t length;
} lle_kill_entry_t;

// Kill ring
typedef struct {
    lle_kill_entry_t entries[LLE_KILL_RING_SIZE];
    size_t head;           // Next position to write
    size_t count;          // Number of entries (0 to LLE_KILL_RING_SIZE)
    size_t yank_index;     // Current yank position for yank-pop
    lle_buffer_pos_t last_yank_start;  // Start of last yank (for yank-pop)
    lle_buffer_pos_t last_yank_end;    // End of last yank (for yank-pop)
    bool last_was_yank;    // Track if last operation was yank (for yank-pop)
} lle_kill_ring_t;

// Editor state
typedef struct {
    // Cursor position (buffer position, not screen position)
    lle_buffer_pos_t cursor_pos;
    
    // Display offset (for scrolling)
    size_t display_offset;
    
    // Editor mode
    lle_editor_mode_t mode;
    
    // Flags
    bool needs_redraw;
    bool running;
} lle_editor_state_t;

// Main editor context
typedef struct {
    // Component systems
    lle_term_t term;
    lle_display_t display;
    lle_buffer_t buffer;
    
    // Editor state
    lle_editor_state_t state;
    
    // Kill ring
    lle_kill_ring_t kill_ring;
    
    // Prompt (if any)
    char *prompt;
    size_t prompt_len;
    
    // Performance tracking
    uint64_t operation_count;
    uint64_t total_op_time_ns;
    
    // Initialization
    bool initialized;
} lle_editor_t;

// Initialize editor with terminal and initial buffer
int lle_editor_init(lle_editor_t *editor, int input_fd, int output_fd);

// Initialize editor with existing buffer content
int lle_editor_init_with_buffer(lle_editor_t *editor, 
                                 int input_fd, 
                                 int output_fd,
                                 const char *initial_text,
                                 size_t text_len);

// Set prompt
int lle_editor_set_prompt(lle_editor_t *editor, const char *prompt);

// Cleanup editor
void lle_editor_cleanup(lle_editor_t *editor);

// Basic cursor operations
int lle_editor_move_cursor_left(lle_editor_t *editor);
int lle_editor_move_cursor_right(lle_editor_t *editor);
int lle_editor_move_cursor_to_start(lle_editor_t *editor);
int lle_editor_move_cursor_to_end(lle_editor_t *editor);

// Advanced navigation operations
int lle_editor_move_word_forward(lle_editor_t *editor);
int lle_editor_move_word_backward(lle_editor_t *editor);
int lle_editor_move_to_line_start(lle_editor_t *editor);
int lle_editor_move_to_line_end(lle_editor_t *editor);

// Basic editing operations
int lle_editor_insert_char(lle_editor_t *editor, char ch);
int lle_editor_insert_string(lle_editor_t *editor, const char *str, size_t len);
int lle_editor_delete_char_before_cursor(lle_editor_t *editor);  // Backspace
int lle_editor_delete_char_at_cursor(lle_editor_t *editor);      // Delete

// Advanced editing operations
int lle_editor_delete_word_before_cursor(lle_editor_t *editor);  // Meta-Backspace
int lle_editor_delete_word_at_cursor(lle_editor_t *editor);      // Meta-d
int lle_editor_kill_line(lle_editor_t *editor);                  // Ctrl-k
int lle_editor_kill_whole_line(lle_editor_t *editor);            // Ctrl-u (in some modes)

// Get editor content
int lle_editor_get_content(const lle_editor_t *editor, 
                           char *dest, 
                           size_t dest_size);

// Get current line
int lle_editor_get_current_line(const lle_editor_t *editor,
                                char *dest,
                                size_t dest_size);

// Redraw display
int lle_editor_redraw(lle_editor_t *editor);

// Refresh display (only if needed)
int lle_editor_refresh(lle_editor_t *editor);

// Clear editor
int lle_editor_clear(lle_editor_t *editor);

// Get cursor position
lle_buffer_pos_t lle_editor_get_cursor_pos(const lle_editor_t *editor);

// Set cursor position
int lle_editor_set_cursor_pos(lle_editor_t *editor, lle_buffer_pos_t pos);

// Get buffer size
size_t lle_editor_get_size(const lle_editor_t *editor);

// Check if modified
bool lle_editor_is_modified(const lle_editor_t *editor);

// Performance metrics
void lle_editor_get_metrics(const lle_editor_t *editor,
                            uint64_t *operation_count,
                            double *avg_time_us);

// Kill ring operations

// Yank (paste) most recent kill
int lle_editor_yank(lle_editor_t *editor);                      // Ctrl-y

// Yank-pop (cycle through kill ring after yank)
int lle_editor_yank_pop(lle_editor_t *editor);                  // Meta-y

// Kill region (arbitrary text range)
int lle_editor_kill_region(lle_editor_t *editor, 
                           lle_buffer_pos_t start,
                           lle_buffer_pos_t end);

// Utility: Convert error code to string
const char* lle_editor_error_string(int error_code);

#endif // LLE_FOUNDATION_EDITOR_H
