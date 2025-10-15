// src/lle/foundation/input/input_processor.c
//
// LLE Input Parser System Implementation (Spec 06 Compliant - Week 10)
//
// SPEC 06 COMPLIANCE: This file implements the COMPLETE Spec 06 structure
// as defined in docs/lle_specification/06_input_parsing_complete.md
//
// All function names match Spec 06 requirements:
// - lle_input_parser_system_t (not lle_simple_input_processor_t)
// - lle_input_parser_system_init() (not lle_simple_input_init())
// - lle_input_parser_* prefix for all functions
//
// Week 10 provides FUNCTIONAL implementation for basic input processing.
// Components marked TODO_SPEC06 are stubs for Phase 1 Month 2 implementation.

#include "input_processor.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

//=============================================================================
// SPEC 06 SECTION 2.2: Complete Input Parser System Initialization
//=============================================================================

/**
 * Initialize input parser system with complete Spec 06 integration
 * 
 * Week 10: Creates functional parser with working keyboard input.
 * Additional integrations are stubs marked TODO_SPEC06.
 */
int lle_input_parser_system_init(lle_input_parser_system_t **system,
                                  void *terminal,
                                  void *event_system,
                                  void *keybinding_engine,
                                  void *widget_hooks,
                                  void *adaptive_terminal,
                                  void *memory_pool) {
    if (!system) {
        return LLE_INPUT_ERR_NULL_PTR;
    }
    
    // Allocate parser system structure
    lle_input_parser_system_t *parser = calloc(1, sizeof(lle_input_parser_system_t));
    if (!parser) {
        return LLE_INPUT_ERR_NULL_PTR;
    }
    
    memset(parser, 0, sizeof(*parser));
    
    // Initialize mutex for thread safety
    if (pthread_mutex_init(&parser->parser_mutex, NULL) != 0) {
        free(parser);
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    // Set Spec 06 fields
    parser->active = true;
    parser->bytes_processed = 0;
    parser->keybinding_lookups = 0;
    parser->widget_hooks_triggered = 0;
    
    // Week 10: Initialize working components
    parser->initialized = true;
    parser->running = false;
    parser->raw_mode_enabled = false;
    
    // TODO_SPEC06: Initialize all Spec 06 components (Phase 1 Month 2)
    // These are currently NULL/uninitialized:
    // - parser->stream = NULL;
    // - parser->sequence_parser = NULL;
    // - parser->utf8_processor = NULL;
    // - parser->key_detector = NULL;
    // - parser->mouse_parser = NULL;
    // - parser->state_machine = NULL;
    // - parser->input_buffer = NULL;
    // - parser->keybinding_integration = NULL;
    // - parser->widget_hook_triggers = NULL;
    // - parser->adaptive_terminal = NULL;
    // - parser->event_system = NULL;
    // - parser->coordinator = NULL;
    // - parser->conflict_resolver = NULL;
    // - parser->error_ctx = NULL;
    // - parser->perf_monitor = NULL;
    // - parser->input_cache = NULL;
    // - parser->memory_pool = NULL;
    
    *system = parser;
    return LLE_INPUT_OK;
}

/**
 * Week 10 simplified initialization (convenience wrapper)
 * 
 * Provides simpler API for Week 10 testing without full Spec 06 parameters.
 */
int lle_input_parser_system_init_simple(lle_input_parser_system_t **system,
                                         int input_fd,
                                         lle_buffer_manager_t *buffer_manager,
                                         lle_display_buffer_renderer_t *renderer) {
    if (!system || !buffer_manager || !renderer) {
        return LLE_INPUT_ERR_NULL_PTR;
    }
    
    // Call full Spec 06 init with NULL for unimplemented components
    int result = lle_input_parser_system_init(system, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != LLE_INPUT_OK) {
        return result;
    }
    
    // Set Week 10 working fields
    (*system)->input_fd = input_fd;
    (*system)->buffer_manager = buffer_manager;
    (*system)->renderer = renderer;
    
    return LLE_INPUT_OK;
}

//=============================================================================
// SECTION 3: TERMINAL RAW MODE MANAGEMENT (Week 10 Functional)
//=============================================================================

/**
 * Enable raw terminal mode (character-by-character input)
 * 
 * Week 10: FUNCTIONAL - Configures terminal for raw input with VMIN/VTIME
 * for proper escape sequence parsing.
 */
int lle_input_parser_enable_raw_mode(lle_input_parser_system_t *parser) {
    if (!parser || !parser->initialized) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    if (parser->raw_mode_enabled) {
        return LLE_INPUT_OK;  // Already enabled
    }
    
    // Get current terminal settings
    if (tcgetattr(parser->input_fd, &parser->orig_termios) == -1) {
        return LLE_INPUT_ERR_TERMIOS;
    }
    
    // Copy to raw mode settings
    parser->raw_termios = parser->orig_termios;
    
    // Disable canonical mode, echo, signals
    parser->raw_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    
    // Disable input processing (IXON = Ctrl+S/Q flow control)
    parser->raw_termios.c_iflag &= ~(IXON | ICRNL);
    
    // Set blocking read with timeout (CRITICAL for escape sequence parsing)
    // VMIN=1: Block until at least 1 character arrives
    // VTIME=1: Timeout after 100ms for subsequent bytes (escape sequences)
    parser->raw_termios.c_cc[VMIN] = 1;
    parser->raw_termios.c_cc[VTIME] = 1;
    
    // Apply raw mode settings
    if (tcsetattr(parser->input_fd, TCSAFLUSH, &parser->raw_termios) == -1) {
        return LLE_INPUT_ERR_TERMIOS;
    }
    
    parser->raw_mode_enabled = true;
    
    return LLE_INPUT_OK;
}

/**
 * Disable raw terminal mode (restore original settings)
 * 
 * Week 10: FUNCTIONAL - Restores terminal to original state
 */
int lle_input_parser_disable_raw_mode(lle_input_parser_system_t *parser) {
    if (!parser || !parser->initialized) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    if (!parser->raw_mode_enabled) {
        return LLE_INPUT_OK;  // Already disabled
    }
    
    // Restore original terminal settings
    if (tcsetattr(parser->input_fd, TCSAFLUSH, &parser->orig_termios) == -1) {
        return LLE_INPUT_ERR_TERMIOS;
    }
    
    parser->raw_mode_enabled = false;
    
    return LLE_INPUT_OK;
}

//=============================================================================
// SECTION 4: INPUT READING AND PARSING (Week 10 Functional)
//=============================================================================

/**
 * Helper: Read single byte with timeout handling
 * 
 * Week 10: FUNCTIONAL - Handles VMIN/VTIME timeout correctly
 */
static int read_byte(lle_input_parser_system_t *parser, char *byte) {
    ssize_t n = read(parser->input_fd, byte, 1);
    if (n == -1) {
        if (errno == EINTR) {
            return LLE_INPUT_OK;  // Interrupted, try again
        }
        return LLE_INPUT_ERR_READ;
    }
    if (n == 0) {
        // With VMIN=1, VTIME=1, n==0 means timeout (no more bytes in sequence)
        return LLE_INPUT_ERR_TIMEOUT;
    }
    
    parser->bytes_processed++;
    return LLE_INPUT_OK;
}

/**
 * Parse escape sequence (arrow keys, function keys, etc.)
 * 
 * Week 10: FUNCTIONAL - Parses CSI sequences for arrow keys and common keys
 * TODO_SPEC06: Move to lle_sequence_parser_t (Spec 06 Section 6)
 */
static bool parse_escape_sequence(lle_input_parser_system_t *parser, lle_key_event_t *event) {
    char ch;
    int result = read_byte(parser, &ch);
    
    if (result != LLE_INPUT_OK) {
        // Timeout - bare ESC key
        event->key = LLE_KEY_ESCAPE;
        return true;
    }
    
    if (ch != '[') {
        // Alt+key combination
        event->key = LLE_KEY_CHAR;
        event->ch = ch;
        event->alt = true;
        return true;
    }
    
    // Read CSI parameter/final byte
    result = read_byte(parser, &ch);
    if (result != LLE_INPUT_OK) {
        event->key = LLE_KEY_UNKNOWN;
        return true;
    }
    
    // Parse standard CSI sequences
    switch (ch) {
        case 'A': event->key = LLE_KEY_ARROW_UP; return true;
        case 'B': event->key = LLE_KEY_ARROW_DOWN; return true;
        case 'C': event->key = LLE_KEY_ARROW_RIGHT; return true;
        case 'D': event->key = LLE_KEY_ARROW_LEFT; return true;
        case 'H': event->key = LLE_KEY_HOME; return true;
        case 'F': event->key = LLE_KEY_END; return true;
        
        // Sequences ending with ~ (e.g., ESC[3~ for Delete)
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6': {
            char seq_num = ch;
            if (read_byte(parser, &ch) != LLE_INPUT_OK) {
                event->key = LLE_KEY_UNKNOWN;
                return true;
            }
            if (ch == '~') {
                switch (seq_num) {
                    case '1': event->key = LLE_KEY_HOME; break;
                    case '3': event->key = LLE_KEY_DELETE; break;
                    case '4': event->key = LLE_KEY_END; break;
                    case '5': event->key = LLE_KEY_PAGE_UP; break;
                    case '6': event->key = LLE_KEY_PAGE_DOWN; break;
                    default:  event->key = LLE_KEY_UNKNOWN; break;
                }
                return true;
            }
            break;
        }
    }
    
    event->key = LLE_KEY_UNKNOWN;
    return true;
}

/**
 * Read and parse next input event
 * 
 * Week 10: FUNCTIONAL - Parses keyboard input into key events
 * TODO_SPEC06: Integrate with lle_key_detector_t and lle_sequence_parser_t
 */
int lle_input_parser_read_event(lle_input_parser_system_t *parser,
                                 lle_key_event_t *event) {
    if (!parser || !parser->initialized || !event) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    memset(event, 0, sizeof(*event));
    
    char ch;
    int result = read_byte(parser, &ch);
    if (result != LLE_INPUT_OK) {
        return result;
    }
    
    // Handle escape sequences
    if (ch == 27) {  // ESC
        return parse_escape_sequence(parser, event) ? LLE_INPUT_OK : LLE_INPUT_ERR_UNKNOWN_KEY;
    }
    
    // Handle Ctrl+key combinations (Ctrl clears bits 5-6)
    if (ch >= 1 && ch <= 26) {
        event->ctrl = true;
        switch (ch) {
            case 1:  event->key = LLE_KEY_CTRL_A; return LLE_INPUT_OK;
            case 3:  event->key = LLE_KEY_CTRL_C; return LLE_INPUT_OK;
            case 4:  event->key = LLE_KEY_CTRL_D; return LLE_INPUT_OK;
            case 5:  event->key = LLE_KEY_CTRL_E; return LLE_INPUT_OK;
            case 11: event->key = LLE_KEY_CTRL_K; return LLE_INPUT_OK;
            case 12: event->key = LLE_KEY_CTRL_L; return LLE_INPUT_OK;
            case 21: event->key = LLE_KEY_CTRL_U; return LLE_INPUT_OK;
            default:
                event->key = LLE_KEY_CHAR;
                event->ch = ch + 'a' - 1;
                return LLE_INPUT_OK;
        }
    }
    
    // Special characters
    if (ch == '\r' || ch == '\n') {
        event->key = LLE_KEY_ENTER;
        return LLE_INPUT_OK;
    }
    if (ch == '\t') {
        event->key = LLE_KEY_TAB;
        return LLE_INPUT_OK;
    }
    if (ch == 127 || ch == 8) {
        event->key = LLE_KEY_BACKSPACE;
        return LLE_INPUT_OK;
    }
    
    // Regular printable character
    event->key = LLE_KEY_CHAR;
    event->ch = ch;
    
    return LLE_INPUT_OK;
}

//=============================================================================
// SECTION 5: EVENT PROCESSING (Week 10 Functional)
//=============================================================================

/**
 * Process input event (execute corresponding action)
 * 
 * Week 10: FUNCTIONAL - Dispatches events to action handlers
 * TODO_SPEC06: Integrate with keybinding_integration (Spec 06 Section 3)
 * TODO_SPEC06: Trigger widget_hook_triggers (Spec 06 Section 4)
 */
int lle_input_parser_process_event(lle_input_parser_system_t *parser,
                                    const lle_key_event_t *event) {
    if (!parser || !parser->initialized || !event) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    parser->keys_processed++;
    
    int result = LLE_INPUT_OK;
    
    // TODO_SPEC06: Call lle_input_process_with_keybinding_lookup() here
    
    switch (event->key) {
        case LLE_KEY_CHAR:
            result = lle_input_action_insert_char(parser, event->ch);
            break;
        case LLE_KEY_BACKSPACE:
            result = lle_input_action_backspace(parser);
            break;
        case LLE_KEY_DELETE:
            result = lle_input_action_delete(parser);
            break;
        case LLE_KEY_ARROW_LEFT:
            result = lle_input_action_move_left(parser);
            break;
        case LLE_KEY_ARROW_RIGHT:
            result = lle_input_action_move_right(parser);
            break;
        case LLE_KEY_ARROW_UP:
            result = lle_input_action_move_up(parser);
            break;
        case LLE_KEY_ARROW_DOWN:
            result = lle_input_action_move_down(parser);
            break;
        case LLE_KEY_HOME:
            result = lle_input_action_move_home(parser);
            break;
        case LLE_KEY_END:
            result = lle_input_action_move_end(parser);
            break;
        case LLE_KEY_PAGE_UP:
            result = lle_input_action_page_up(parser);
            break;
        case LLE_KEY_PAGE_DOWN:
            result = lle_input_action_page_down(parser);
            break;
        case LLE_KEY_ENTER:
            result = lle_input_action_newline(parser);
            break;
        case LLE_KEY_TAB:
            result = lle_input_action_insert_char(parser, '\t');
            break;
        case LLE_KEY_CTRL_A:
            result = lle_input_action_beginning_of_line(parser);
            break;
        case LLE_KEY_CTRL_E:
            result = lle_input_action_end_of_line(parser);
            break;
        case LLE_KEY_CTRL_K:
            result = lle_input_action_kill_line(parser);
            break;
        case LLE_KEY_CTRL_U:
            result = lle_input_action_kill_backward(parser);
            break;
        case LLE_KEY_CTRL_L:
            result = lle_input_action_clear_screen(parser);
            break;
        case LLE_KEY_CTRL_D: {
            lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
            if (buf && lle_buffer_size(&buf->buffer) == 0) {
                parser->running = false;  // Exit on empty buffer
                return LLE_INPUT_OK;
            }
            result = lle_input_action_delete(parser);
            break;
        }
        case LLE_KEY_CTRL_C:
            parser->running = false;  // Exit
            return LLE_INPUT_OK;
        case LLE_KEY_ESCAPE:
        case LLE_KEY_UNKNOWN:
        default:
            return LLE_INPUT_OK;  // Ignore unknown keys
    }
    
    // Re-render after action
    // TODO_SPEC06: Generate event for event_system instead of direct render
    if (result == LLE_INPUT_OK) {
        lle_display_buffer_render(parser->renderer);
        lle_display_flush(parser->renderer->display);
    }
    
    return result;
}

/**
 * Main input loop
 * 
 * Week 10: FUNCTIONAL - Read-Process-Render loop
 */
int lle_input_parser_run(lle_input_parser_system_t *parser) {
    if (!parser || !parser->initialized) {
        return LLE_INPUT_ERR_NOT_INIT;
    }
    
    parser->running = true;
    
    // Initial render
    lle_display_buffer_render(parser->renderer);
    lle_display_flush(parser->renderer->display);
    
    while (parser->running) {
        lle_key_event_t event;
        int result = lle_input_parser_read_event(parser, &event);
        
        if (result == LLE_INPUT_ERR_EOF) {
            break;
        }
        if (result != LLE_INPUT_OK) {
            continue;  // Skip errors
        }
        
        lle_input_parser_process_event(parser, &event);
    }
    
    return LLE_INPUT_OK;
}

//=============================================================================
// SECTION 6: INPUT ACTION HANDLERS (Week 10 Functional)
//=============================================================================

int lle_input_action_insert_char(lle_input_parser_system_t *parser, char ch) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    lle_buffer_insert_char(&buf->buffer, buf->buffer.gap_start, ch);
    parser->chars_inserted++;
    return LLE_INPUT_OK;
}

int lle_input_action_backspace(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    if (buf->buffer.gap_start > 0) {
        lle_buffer_delete_char(&buf->buffer, buf->buffer.gap_start - 1);
        parser->chars_deleted++;
    }
    return LLE_INPUT_OK;
}

int lle_input_action_delete(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    if (buf->buffer.gap_start < lle_buffer_size(&buf->buffer)) {
        lle_buffer_delete_char(&buf->buffer, buf->buffer.gap_start);
        parser->chars_deleted++;
    }
    return LLE_INPUT_OK;
}

int lle_input_action_move_left(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    if (buf->buffer.gap_start > 0) {
        buf->buffer.gap_start--;
        buf->buffer.gap_end--;
        char temp = buf->buffer.data[buf->buffer.gap_start];
        buf->buffer.data[buf->buffer.gap_start] = buf->buffer.data[buf->buffer.gap_end];
        buf->buffer.data[buf->buffer.gap_end] = temp;
        parser->cursor_moves++;
    }
    return LLE_INPUT_OK;
}

int lle_input_action_move_right(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    if (buf->buffer.gap_end < buf->buffer.capacity) {
        char temp = buf->buffer.data[buf->buffer.gap_end];
        buf->buffer.data[buf->buffer.gap_end] = buf->buffer.data[buf->buffer.gap_start];
        buf->buffer.data[buf->buffer.gap_start] = temp;
        buf->buffer.gap_start++;
        buf->buffer.gap_end++;
        parser->cursor_moves++;
    }
    return LLE_INPUT_OK;
}

int lle_input_action_move_up(lle_input_parser_system_t *parser) {
    // TODO_SPEC06: Implement line-aware up movement
    (void)parser;
    return LLE_INPUT_OK;
}

int lle_input_action_move_down(lle_input_parser_system_t *parser) {
    // TODO_SPEC06: Implement line-aware down movement
    (void)parser;
    return LLE_INPUT_OK;
}

int lle_input_action_move_home(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    size_t line_start = lle_buffer_line_start(&buf->buffer, buf->buffer.gap_start);
    while (buf->buffer.gap_start > line_start) {
        lle_input_action_move_left(parser);
    }
    return LLE_INPUT_OK;
}

int lle_input_action_move_end(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    size_t line_end = lle_buffer_line_end(&buf->buffer, buf->buffer.gap_start);
    while (buf->buffer.gap_start < line_end) {
        lle_input_action_move_right(parser);
    }
    return LLE_INPUT_OK;
}

int lle_input_action_page_up(lle_input_parser_system_t *parser) {
    lle_display_buffer_scroll_up(parser->renderer, parser->renderer->viewport.visible_lines - 1);
    parser->cursor_moves++;
    return LLE_INPUT_OK;
}

int lle_input_action_page_down(lle_input_parser_system_t *parser) {
    lle_display_buffer_scroll_down(parser->renderer, parser->renderer->viewport.visible_lines - 1);
    parser->cursor_moves++;
    return LLE_INPUT_OK;
}

int lle_input_action_newline(lle_input_parser_system_t *parser) {
    return lle_input_action_insert_char(parser, '\n');
}

int lle_input_action_beginning_of_line(lle_input_parser_system_t *parser) {
    return lle_input_action_move_home(parser);
}

int lle_input_action_end_of_line(lle_input_parser_system_t *parser) {
    return lle_input_action_move_end(parser);
}

int lle_input_action_kill_line(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    // TODO_SPEC06: Store in kill ring
    size_t line_end = lle_buffer_line_end(&buf->buffer, buf->buffer.gap_start);
    size_t cursor = buf->buffer.gap_start;
    
    if (cursor < line_end) {
        lle_buffer_delete_range(&buf->buffer, cursor, line_end);
        parser->chars_deleted += (line_end - cursor);
    }
    return LLE_INPUT_OK;
}

int lle_input_action_kill_backward(lle_input_parser_system_t *parser) {
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(parser->buffer_manager);
    if (!buf) return LLE_INPUT_ERR_NOT_INIT;
    
    // TODO_SPEC06: Store in kill ring
    size_t line_start = lle_buffer_line_start(&buf->buffer, buf->buffer.gap_start);
    size_t cursor = buf->buffer.gap_start;
    
    if (cursor > line_start) {
        lle_buffer_delete_range(&buf->buffer, line_start, cursor);
        parser->chars_deleted += (cursor - line_start);
    }
    return LLE_INPUT_OK;
}

int lle_input_action_clear_screen(lle_input_parser_system_t *parser) {
    lle_display_clear(parser->renderer->display);
    return LLE_INPUT_OK;
}

//=============================================================================
// SECTION 7: UTILITY FUNCTIONS
//=============================================================================

void lle_input_parser_get_stats(const lle_input_parser_system_t *parser,
                                 uint64_t *keys_processed,
                                 uint64_t *chars_inserted,
                                 uint64_t *chars_deleted,
                                 uint64_t *cursor_moves) {
    if (parser) {
        if (keys_processed) *keys_processed = parser->keys_processed;
        if (chars_inserted) *chars_inserted = parser->chars_inserted;
        if (chars_deleted) *chars_deleted = parser->chars_deleted;
        if (cursor_moves) *cursor_moves = parser->cursor_moves;
    }
}

void lle_input_parser_system_cleanup(lle_input_parser_system_t *parser) {
    if (parser) {
        if (parser->raw_mode_enabled) {
            lle_input_parser_disable_raw_mode(parser);
        }
        
        pthread_mutex_destroy(&parser->parser_mutex);
        
        // TODO_SPEC06: Cleanup all Spec 06 components
        // - lle_input_stream_destroy(parser->stream);
        // - lle_sequence_parser_destroy(parser->sequence_parser);
        // - etc...
        
        free(parser);
    }
}

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
