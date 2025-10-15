// src/lle/foundation/input/input_processor.c
//
// LLE Input Processor Implementation (Week 10)

#include "input_processor.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

// Initialize input processor
int lle_simple_input_init(lle_simple_input_processor_t *processor,
                             int input_fd,
                             lle_buffer_manager_t *buffer_manager,
                             lle_display_buffer_renderer_t *renderer) {
    if (!processor || !buffer_manager || !renderer) {
        return LLE_INPUT_ERR_NULL_PTR;
    }
    
    memset(processor, 0, sizeof(*processor));
    
    processor->input_fd = input_fd;
    processor->buffer_manager = buffer_manager;
    processor->renderer = renderer;
    processor->raw_mode_enabled = false;
    processor->running = false;
    processor->initialized = true;
    
    return LLE_INPUT_OK;
}

// Enable raw terminal mode
int lle_simple_input_enable_raw_mode(lle_simple_input_processor_t *processor) {
    if (!processor || !processor->initialized) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    if (processor->raw_mode_enabled) {
        return LLE_INPUT_OK;  // Already enabled
    }
    
    // Get current terminal settings
    if (tcgetattr(processor->input_fd, &processor->orig_termios) == -1) {
        return LLE_INPUT_ERR_TERMIOS;
    }
    
    // Copy to raw mode settings
    processor->raw_termios = processor->orig_termios;
    
    // Disable canonical mode, echo, signals
    processor->raw_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    
    // Disable input processing (IXON = Ctrl+S/Q flow control)
    processor->raw_termios.c_iflag &= ~(IXON | ICRNL);
    
    // Set blocking read with timeout
    // VMIN=1 means block until at least 1 character arrives
    // VTIME=1 means timeout after 100ms (used for escape sequence parsing)
    processor->raw_termios.c_cc[VMIN] = 1;   // Block until 1 byte available
    processor->raw_termios.c_cc[VTIME] = 1;  // Timeout 100ms for escape sequences
    
    // Apply raw mode settings
    if (tcsetattr(processor->input_fd, TCSAFLUSH, &processor->raw_termios) == -1) {
        return LLE_INPUT_ERR_TERMIOS;
    }
    
    processor->raw_mode_enabled = true;
    
    return LLE_INPUT_OK;
}

// Disable raw terminal mode
int lle_simple_input_disable_raw_mode(lle_simple_input_processor_t *processor) {
    if (!processor || !processor->initialized) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    if (!processor->raw_mode_enabled) {
        return LLE_INPUT_OK;  // Already disabled
    }
    
    // Restore original terminal settings
    if (tcsetattr(processor->input_fd, TCSAFLUSH, &processor->orig_termios) == -1) {
        return LLE_INPUT_ERR_TERMIOS;
    }
    
    processor->raw_mode_enabled = false;
    
    return LLE_INPUT_OK;
}

// Helper: Read single byte
static int read_byte(lle_simple_input_processor_t *processor, char *byte) {
    ssize_t n = read(processor->input_fd, byte, 1);
    if (n == -1) {
        if (errno == EINTR) {
            return LLE_INPUT_OK;  // Interrupted, try again
        }
        return LLE_INPUT_ERR_READ;
    }
    if (n == 0) {
        // With VMIN=0, VTIME=1, n==0 means timeout (no data available)
        // This is NOT an EOF, it's a timeout for escape sequence parsing
        return LLE_INPUT_ERR_TIMEOUT;
    }
    return LLE_INPUT_OK;
}

// Parse escape sequence (arrow keys, etc.)
// Returns true if complete sequence parsed
static bool parse_escape_sequence(lle_simple_input_processor_t *processor, lle_key_event_t *event) {
    // Try to read '[' after ESC
    char ch;
    int result = read_byte(processor, &ch);
    
    if (result != LLE_INPUT_OK) {
        event->key = LLE_KEY_ESCAPE;
        return true;
    }
    
    if (ch != '[') {
        // Alt+key combination (TODO: Phase 1 Month 2)
        event->key = LLE_KEY_CHAR;
        event->ch = ch;
        event->alt = true;
        return true;
    }
    
    // Read next character
    result = read_byte(processor, &ch);
    
    if (result != LLE_INPUT_OK) {
        event->key = LLE_KEY_UNKNOWN;
        return true;
    }
    
    // Parse arrow keys and other sequences
    switch (ch) {
        case 'A': event->key = LLE_KEY_ARROW_UP; return true;
        case 'B': event->key = LLE_KEY_ARROW_DOWN; return true;
        case 'C': event->key = LLE_KEY_ARROW_RIGHT; return true;
        case 'D': event->key = LLE_KEY_ARROW_LEFT; return true;
        case 'H': event->key = LLE_KEY_HOME; return true;
        case 'F': event->key = LLE_KEY_END; return true;
        
        // Page up/down, delete (sequences ending with ~)
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6': {
            char seq_num = ch;
            if (read_byte(processor, &ch) != LLE_INPUT_OK) {
                event->key = LLE_KEY_UNKNOWN;
                return true;
            }
            if (ch == '~') {
                if (seq_num == '1') event->key = LLE_KEY_HOME;
                else if (seq_num == '3') event->key = LLE_KEY_DELETE;
                else if (seq_num == '4') event->key = LLE_KEY_END;
                else if (seq_num == '5') event->key = LLE_KEY_PAGE_UP;
                else if (seq_num == '6') event->key = LLE_KEY_PAGE_DOWN;
                else event->key = LLE_KEY_UNKNOWN;
                return true;
            }
            break;
        }
    }
    
    event->key = LLE_KEY_UNKNOWN;
    return true;
}

// Read and parse next input event
int lle_simple_input_read_event(lle_simple_input_processor_t *processor,
                                   lle_key_event_t *event) {
    if (!processor || !processor->initialized || !event) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    memset(event, 0, sizeof(*event));
    
    char ch;
    int result = read_byte(processor, &ch);
    if (result != LLE_INPUT_OK) {
        return result;
    }
    
    // Handle escape sequences
    if (ch == 27) {  // ESC
        return parse_escape_sequence(processor, event) ? LLE_INPUT_OK : LLE_INPUT_ERR_UNKNOWN_KEY;
    }
    
    // Handle Ctrl+key combinations (Ctrl clears bits 5-6, so Ctrl+A = 1, Ctrl+Z = 26)
    if (ch >= 1 && ch <= 26) {
        event->ctrl = true;
        switch (ch) {
            case 1:  event->key = LLE_KEY_CTRL_A; return LLE_INPUT_OK;  // Ctrl+A
            case 3:  event->key = LLE_KEY_CTRL_C; return LLE_INPUT_OK;  // Ctrl+C
            case 4:  event->key = LLE_KEY_CTRL_D; return LLE_INPUT_OK;  // Ctrl+D
            case 5:  event->key = LLE_KEY_CTRL_E; return LLE_INPUT_OK;  // Ctrl+E
            case 11: event->key = LLE_KEY_CTRL_K; return LLE_INPUT_OK;  // Ctrl+K
            case 12: event->key = LLE_KEY_CTRL_L; return LLE_INPUT_OK;  // Ctrl+L
            case 21: event->key = LLE_KEY_CTRL_U; return LLE_INPUT_OK;  // Ctrl+U
            default:
                event->key = LLE_KEY_CHAR;
                event->ch = ch + 'a' - 1;  // Convert back to letter
                return LLE_INPUT_OK;
        }
    }
    
    // Handle special characters
    if (ch == '\r' || ch == '\n') {
        event->key = LLE_KEY_ENTER;
        return LLE_INPUT_OK;
    }
    
    if (ch == '\t') {
        event->key = LLE_KEY_TAB;
        return LLE_INPUT_OK;
    }
    
    if (ch == 127 || ch == 8) {  // DEL or BS
        event->key = LLE_KEY_BACKSPACE;
        return LLE_INPUT_OK;
    }
    
    // Regular printable character
    event->key = LLE_KEY_CHAR;
    event->ch = ch;
    
    return LLE_INPUT_OK;
}

// Process input event (execute corresponding action)
int lle_simple_input_process_event(lle_simple_input_processor_t *processor,
                                      const lle_key_event_t *event) {
    if (!processor || !processor->initialized || !event) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    processor->keys_processed++;
    
    int result = LLE_INPUT_OK;
    
    switch (event->key) {
        case LLE_KEY_CHAR:
            result = lle_input_action_insert_char(processor, event->ch);
            break;
            
        case LLE_KEY_BACKSPACE:
            result = lle_input_action_backspace(processor);
            break;
            
        case LLE_KEY_DELETE:
            result = lle_input_action_delete(processor);
            break;
            
        case LLE_KEY_ARROW_LEFT:
            result = lle_input_action_move_left(processor);
            break;
            
        case LLE_KEY_ARROW_RIGHT:
            result = lle_input_action_move_right(processor);
            break;
            
        case LLE_KEY_ARROW_UP:
            result = lle_input_action_move_up(processor);
            break;
            
        case LLE_KEY_ARROW_DOWN:
            result = lle_input_action_move_down(processor);
            break;
            
        case LLE_KEY_HOME:
            result = lle_input_action_move_home(processor);
            break;
            
        case LLE_KEY_END:
            result = lle_input_action_move_end(processor);
            break;
            
        case LLE_KEY_PAGE_UP:
            result = lle_input_action_page_up(processor);
            break;
            
        case LLE_KEY_PAGE_DOWN:
            result = lle_input_action_page_down(processor);
            break;
            
        case LLE_KEY_ENTER:
            result = lle_input_action_newline(processor);
            break;
            
        case LLE_KEY_TAB:
            result = lle_input_action_insert_char(processor, '\t');
            break;
            
        // Ctrl+key combinations
        case LLE_KEY_CTRL_A:
            result = lle_input_action_beginning_of_line(processor);
            break;
            
        case LLE_KEY_CTRL_E:
            result = lle_input_action_end_of_line(processor);
            break;
            
        case LLE_KEY_CTRL_K:
            result = lle_input_action_kill_line(processor);
            break;
            
        case LLE_KEY_CTRL_U:
            result = lle_input_action_kill_backward(processor);
            break;
            
        case LLE_KEY_CTRL_L:
            result = lle_input_action_clear_screen(processor);
            break;
            
        case LLE_KEY_CTRL_D:
            // Delete forward or signal EOF if buffer empty
            {
                lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
                if (buf && lle_buffer_size(&buf->buffer) == 0) {
                    processor->running = false;  // Exit
                    return LLE_INPUT_OK;
                }
                result = lle_input_action_delete(processor);
            }
            break;
            
        case LLE_KEY_CTRL_C:
            // Interrupt - exit for now (TODO: Phase 1 Month 2: handle properly)
            processor->running = false;
            return LLE_INPUT_OK;
            
        case LLE_KEY_ESCAPE:
        case LLE_KEY_UNKNOWN:
        default:
            // Ignore unknown keys
            return LLE_INPUT_OK;
    }
    
    // Re-render after action
    if (result == LLE_INPUT_OK) {
        lle_display_buffer_render(processor->renderer);
        lle_display_flush(processor->renderer->display);
    }
    
    return result;
}

// Main input loop
int lle_simple_input_run(lle_simple_input_processor_t *processor) {
    if (!processor || !processor->initialized) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    processor->running = true;
    
    // Initial render
    lle_display_buffer_render(processor->renderer);
    lle_display_flush(processor->renderer->display);
    
    while (processor->running) {
        lle_key_event_t event;
        int result = lle_simple_input_read_event(processor, &event);
        
        if (result == LLE_INPUT_ERR_EOF) {
            break;  // EOF - exit
        }
        
        if (result != LLE_INPUT_OK) {
            continue;  // Skip errors, keep running
        }
        
        lle_simple_input_process_event(processor, &event);
    }
    
    return LLE_INPUT_OK;
}

// Action: Insert character
int lle_input_action_insert_char(lle_simple_input_processor_t *processor, char ch) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // Insert at cursor position
    lle_buffer_insert_char(&buf->buffer, buf->buffer.gap_start, ch);
    processor->chars_inserted++;
    
    return LLE_INPUT_OK;
}

// Action: Backspace
int lle_input_action_backspace(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // Delete character before cursor if possible
    if (buf->buffer.gap_start > 0) {
        // Delete the character at position (cursor - 1)
        lle_buffer_delete_char(&buf->buffer, buf->buffer.gap_start - 1);
        processor->chars_deleted++;
    }
    
    return LLE_INPUT_OK;
}

// Action: Delete forward
int lle_input_action_delete(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // Delete at cursor if not at end
    if (buf->buffer.gap_start < lle_buffer_size(&buf->buffer)) {
        lle_buffer_delete_char(&buf->buffer, buf->buffer.gap_start);
        processor->chars_deleted++;
    }
    
    return LLE_INPUT_OK;
}

// Action: Move cursor left
int lle_input_action_move_left(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    if (buf->buffer.gap_start > 0) {
        // Move gap start left (cursor moves left)
        buf->buffer.gap_start--;
        buf->buffer.gap_end--;
        // Swap character from before gap to after gap
        char temp = buf->buffer.data[buf->buffer.gap_start];
        buf->buffer.data[buf->buffer.gap_start] = buf->buffer.data[buf->buffer.gap_end];
        buf->buffer.data[buf->buffer.gap_end] = temp;
        processor->cursor_moves++;
    }
    
    return LLE_INPUT_OK;
}

// Action: Move cursor right
int lle_input_action_move_right(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    if (buf->buffer.gap_end < buf->buffer.capacity) {
        // Move gap right (cursor moves right)
        char temp = buf->buffer.data[buf->buffer.gap_end];
        buf->buffer.data[buf->buffer.gap_end] = buf->buffer.data[buf->buffer.gap_start];
        buf->buffer.data[buf->buffer.gap_start] = temp;
        buf->buffer.gap_start++;
        buf->buffer.gap_end++;
        processor->cursor_moves++;
    }
    
    return LLE_INPUT_OK;
}

// Action: Move cursor up (TODO: Week 10 - needs line-aware cursor movement)
int lle_input_action_move_up(lle_simple_input_processor_t *processor) {
    // TODO Phase 1 Week 10: Implement line-aware up movement
    // Need to find previous line, move to same column
    (void)processor;
    return LLE_INPUT_OK;
}

// Action: Move cursor down (TODO: Week 10 - needs line-aware cursor movement)
int lle_input_action_move_down(lle_simple_input_processor_t *processor) {
    // TODO Phase 1 Week 10: Implement line-aware down movement
    // Need to find next line, move to same column
    (void)processor;
    return LLE_INPUT_OK;
}

// Action: Move to beginning of line
int lle_input_action_move_home(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // Find line start
    size_t line_start = lle_buffer_line_start(&buf->buffer, buf->buffer.gap_start);
    
    // Move cursor to line start
    while (buf->buffer.gap_start > line_start) {
        lle_input_action_move_left(processor);
    }
    
    return LLE_INPUT_OK;
}

// Action: Move to end of line
int lle_input_action_move_end(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // Find line end
    size_t line_end = lle_buffer_line_end(&buf->buffer, buf->buffer.gap_start);
    
    // Move cursor to line end
    while (buf->buffer.gap_start < line_end) {
        lle_input_action_move_right(processor);
    }
    
    return LLE_INPUT_OK;
}

// Action: Page up
int lle_input_action_page_up(lle_simple_input_processor_t *processor) {
    // Scroll viewport up by visible lines
    lle_display_buffer_scroll_up(processor->renderer, processor->renderer->viewport.visible_lines - 1);
    processor->cursor_moves++;
    return LLE_INPUT_OK;
}

// Action: Page down
int lle_input_action_page_down(lle_simple_input_processor_t *processor) {
    // Scroll viewport down by visible lines
    lle_display_buffer_scroll_down(processor->renderer, processor->renderer->viewport.visible_lines - 1);
    processor->cursor_moves++;
    return LLE_INPUT_OK;
}

// Action: Insert newline
int lle_input_action_newline(lle_simple_input_processor_t *processor) {
    return lle_input_action_insert_char(processor, '\n');
}

// Action: Beginning of line (Ctrl+A)
int lle_input_action_beginning_of_line(lle_simple_input_processor_t *processor) {
    return lle_input_action_move_home(processor);
}

// Action: End of line (Ctrl+E)
int lle_input_action_end_of_line(lle_simple_input_processor_t *processor) {
    return lle_input_action_move_end(processor);
}

// Action: Kill to end of line (Ctrl+K)
int lle_input_action_kill_line(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // TODO Phase 1 Month 2: Store in kill ring
    // For now, just delete to end of line
    size_t line_end = lle_buffer_line_end(&buf->buffer, buf->buffer.gap_start);
    size_t cursor = buf->buffer.gap_start;
    
    if (cursor < line_end) {
        lle_buffer_delete_range(&buf->buffer, cursor, line_end);
        processor->chars_deleted += (line_end - cursor);
    }
    
    return LLE_INPUT_OK;
}

// Action: Kill to beginning of line (Ctrl+U)
int lle_input_action_kill_backward(lle_simple_input_processor_t *processor) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(processor->buffer_manager);
    if (!buf) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // TODO Phase 1 Month 2: Store in kill ring
    // For now, just delete to beginning of line
    size_t line_start = lle_buffer_line_start(&buf->buffer, buf->buffer.gap_start);
    size_t cursor = buf->buffer.gap_start;
    
    if (cursor > line_start) {
        lle_buffer_delete_range(&buf->buffer, line_start, cursor);
        processor->chars_deleted += (cursor - line_start);
    }
    
    return LLE_INPUT_OK;
}

// Action: Clear screen (Ctrl+L)
int lle_input_action_clear_screen(lle_simple_input_processor_t *processor) {
    lle_display_clear(processor->renderer->display);
    return LLE_INPUT_OK;
}

// Get statistics
void lle_simple_input_get_stats(const lle_simple_input_processor_t *processor,
                                   uint64_t *keys_processed,
                                   uint64_t *chars_inserted,
                                   uint64_t *chars_deleted,
                                   uint64_t *cursor_moves) {
    if (processor) {
        if (keys_processed) *keys_processed = processor->keys_processed;
        if (chars_inserted) *chars_inserted = processor->chars_inserted;
        if (chars_deleted) *chars_deleted = processor->chars_deleted;
        if (cursor_moves) *cursor_moves = processor->cursor_moves;
    }
}

// Cleanup
void lle_simple_input_cleanup(lle_simple_input_processor_t *processor) {
    if (processor) {
        // Restore terminal mode if still in raw mode
        if (processor->raw_mode_enabled) {
            lle_simple_input_disable_raw_mode(processor);
        }
        memset(processor, 0, sizeof(*processor));
    }
}

// Convert error code to string
const char* lle_input_error_string(int error_code) {
    switch (error_code) {
        case LLE_INPUT_OK: return "Success";
        case LLE_INPUT_ERR_NULL_PTR: return "Null pointer";
        case LLE_INPUT_ERR_NOT_INIT: return "Not initialized";
        case LLE_INPUT_ERR_TERMIOS: return "Terminal control error";
        case LLE_INPUT_ERR_READ: return "Read error";
        case LLE_INPUT_ERR_UNKNOWN_KEY: return "Unknown key";
        case LLE_INPUT_ERR_EOF: return "End of file";
        case LLE_INPUT_ERR_TIMEOUT: return "Timeout";
        default: return "Unknown error";
    }
}

// Convert key code to string
const char* lle_key_code_string(lle_key_code_t key) {
    switch (key) {
        case LLE_KEY_CHAR: return "CHAR";
        case LLE_KEY_BACKSPACE: return "BACKSPACE";
        case LLE_KEY_DELETE: return "DELETE";
        case LLE_KEY_ARROW_UP: return "ARROW_UP";
        case LLE_KEY_ARROW_DOWN: return "ARROW_DOWN";
        case LLE_KEY_ARROW_LEFT: return "ARROW_LEFT";
        case LLE_KEY_ARROW_RIGHT: return "ARROW_RIGHT";
        case LLE_KEY_HOME: return "HOME";
        case LLE_KEY_END: return "END";
        case LLE_KEY_PAGE_UP: return "PAGE_UP";
        case LLE_KEY_PAGE_DOWN: return "PAGE_DOWN";
        case LLE_KEY_ENTER: return "ENTER";
        case LLE_KEY_TAB: return "TAB";
        case LLE_KEY_ESCAPE: return "ESCAPE";
        case LLE_KEY_CTRL_A: return "CTRL+A";
        case LLE_KEY_CTRL_E: return "CTRL+E";
        case LLE_KEY_CTRL_K: return "CTRL+K";
        case LLE_KEY_CTRL_U: return "CTRL+U";
        case LLE_KEY_CTRL_D: return "CTRL+D";
        case LLE_KEY_CTRL_L: return "CTRL+L";
        case LLE_KEY_CTRL_C: return "CTRL+C";
        case LLE_KEY_UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}
