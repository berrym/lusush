// src/lle/foundation/input/input_processor.h
//
// LLE Input Parser System (Spec 06 Compliant - Week 10 Implementation)
//
// Complete implementation of Spec 06 Input Parsing System with all required
// structures and functions. Week 10 provides basic working functionality for
// character input, cursor movement, and simple key bindings. Remaining
// components are stubs to be implemented in later phases.
//
// SPEC 06 COMPLIANCE: This file implements the COMPLETE Spec 06 structure
// as defined in docs/lle_specification/06_input_parsing_complete.md
//
// Week 10 Status (FUNCTIONAL):
// ✅ Basic character input (ASCII)
// ✅ Cursor movement keys (arrows, home, end, page up/down)
// ✅ Basic editing keys (backspace, delete, enter)
// ✅ Ctrl key combinations (Ctrl+A, E, K, U, C, D, L)
// ✅ Terminal raw mode management
// ✅ Escape sequence parsing for arrow keys
//
// TODO Phase 1 Month 2 (Stubs marked with TODO_SPEC06):
// - Mouse input processing (lle_mouse_parser_t)
// - UTF-8 multi-byte processing (lle_utf8_processor_t)
// - State machine implementation (lle_parser_state_machine_t)
// - Keybinding engine integration (lle_keybinding_integration_t)
// - Widget hook triggers (lle_widget_hook_triggers_t)
// - Adaptive terminal integration (lle_adaptive_terminal_parser_t)
// - Event system integration (lle_event_system_t)
// - Input stream buffering (lle_input_stream_t, lle_input_buffer_t)
// - Conflict resolution (lle_input_conflict_resolver_t)
// - Performance monitoring (lle_performance_monitor_t)
// - Input caching (lle_input_cache_t)
// - Proper error context (lle_error_context_t)

#ifndef LLE_FOUNDATION_INPUT_PROCESSOR_H
#define LLE_FOUNDATION_INPUT_PROCESSOR_H

#include "../buffer/buffer_manager.h"
#include "../display/display_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <pthread.h>

//=============================================================================
// SECTION 1: SPEC 06 COMPLIANT STRUCTURES (All 22 fields + supporting types)
//=============================================================================

// Forward declarations for Spec 06 structures (to be fully implemented in Phase 1 Month 2)
typedef struct lle_input_stream lle_input_stream_t;
typedef struct lle_sequence_parser lle_sequence_parser_t;
typedef struct lle_utf8_processor lle_utf8_processor_t;
typedef struct lle_key_detector lle_key_detector_t;
typedef struct lle_mouse_parser lle_mouse_parser_t;
typedef struct lle_parser_state_machine lle_parser_state_machine_t;
typedef struct lle_input_buffer lle_input_buffer_t;
typedef struct lle_keybinding_integration lle_keybinding_integration_t;
typedef struct lle_widget_hook_triggers lle_widget_hook_triggers_t;
typedef struct lle_adaptive_terminal_parser lle_adaptive_terminal_parser_t;
typedef struct lle_event_system lle_event_system_t;
typedef struct lle_input_coordinator lle_input_coordinator_t;
typedef struct lle_input_conflict_resolver lle_input_conflict_resolver_t;
typedef struct lle_error_context lle_error_context_t;
typedef struct lle_performance_monitor lle_performance_monitor_t;
typedef struct lle_input_cache lle_input_cache_t;
typedef struct lle_memory_pool lle_memory_pool_t;

// Key codes for special keys (terminal escape sequences)
typedef enum {
    LLE_KEY_CHAR = 0,           // Regular character (value in ch field)
    LLE_KEY_BACKSPACE = 127,    // Backspace
    LLE_KEY_DELETE = 1000,      // Delete key
    LLE_KEY_ARROW_UP,           // Arrow up
    LLE_KEY_ARROW_DOWN,         // Arrow down
    LLE_KEY_ARROW_LEFT,         // Arrow left
    LLE_KEY_ARROW_RIGHT,        // Arrow right
    LLE_KEY_HOME,               // Home key
    LLE_KEY_END,                // End key
    LLE_KEY_PAGE_UP,            // Page up
    LLE_KEY_PAGE_DOWN,          // Page down
    LLE_KEY_ENTER,              // Enter/return
    LLE_KEY_TAB,                // Tab
    LLE_KEY_ESCAPE,             // Escape
    
    // Ctrl combinations
    LLE_KEY_CTRL_A,             // Ctrl+A (beginning of line)
    LLE_KEY_CTRL_E,             // Ctrl+E (end of line)
    LLE_KEY_CTRL_K,             // Ctrl+K (kill to end of line)
    LLE_KEY_CTRL_U,             // Ctrl+U (kill to beginning of line)
    LLE_KEY_CTRL_D,             // Ctrl+D (delete forward / EOF)
    LLE_KEY_CTRL_L,             // Ctrl+L (clear screen)
    LLE_KEY_CTRL_C,             // Ctrl+C (interrupt)
    
    LLE_KEY_UNKNOWN = 9999      // Unknown/unsupported key
} lle_key_code_t;

// Input event (parsed key press) - Week 10 version
typedef struct {
    lle_key_code_t key;         // Key code
    char ch;                    // Character (for LLE_KEY_CHAR)
    bool ctrl;                  // Ctrl modifier
    bool alt;                   // Alt modifier
    bool shift;                 // Shift modifier
} lle_key_event_t;

// Error codes
typedef enum {
    LLE_INPUT_OK = 0,
    LLE_INPUT_ERR_NULL_PTR = -1,
    LLE_INPUT_ERR_NOT_INIT = -2,
    LLE_INPUT_ERR_TERMIOS = -3,
    LLE_INPUT_ERR_READ = -4,
    LLE_INPUT_ERR_UNKNOWN_KEY = -5,
    LLE_INPUT_ERR_EOF = -6,
    LLE_INPUT_ERR_TIMEOUT = -7,  // Timeout waiting for escape sequence bytes
} lle_input_error_t;

//=============================================================================
// PRIMARY INPUT PARSER SYSTEM STRUCTURE (Spec 06 Section 2.1)
// COMPLETE 22-field structure as required by Spec 06
//=============================================================================

typedef struct lle_input_parser_system {
    //-------------------------------------------------------------------------
    // Core parsing components (7 fields) - Spec 06 Section 2.1
    //-------------------------------------------------------------------------
    lle_input_stream_t *stream;                 // TODO_SPEC06: Input stream management
    lle_sequence_parser_t *sequence_parser;     // TODO_SPEC06: Escape sequence parser
    lle_utf8_processor_t *utf8_processor;       // TODO_SPEC06: UTF-8 Unicode processor
    lle_key_detector_t *key_detector;           // TODO_SPEC06: Key sequence detector
    lle_mouse_parser_t *mouse_parser;           // TODO_SPEC06: Mouse event parser
    lle_parser_state_machine_t *state_machine;  // TODO_SPEC06: Parser state machine
    lle_input_buffer_t *input_buffer;           // TODO_SPEC06: Input buffering system
    
    //-------------------------------------------------------------------------
    // Critical system integrations (3 fields) - Spec 06 Section 3, 4, 5
    //-------------------------------------------------------------------------
    lle_keybinding_integration_t *keybinding_integration;   // TODO_SPEC06: Keybinding system integration (Spec 06 Section 3)
    lle_widget_hook_triggers_t *widget_hook_triggers;       // TODO_SPEC06: Widget hook trigger system (Spec 06 Section 4)
    lle_adaptive_terminal_parser_t *adaptive_terminal;      // TODO_SPEC06: Adaptive terminal integration (Spec 06 Section 5)
    
    //-------------------------------------------------------------------------
    // System coordination (3 fields) - Spec 06 Section 2.1
    //-------------------------------------------------------------------------
    lle_event_system_t *event_system;           // TODO_SPEC06: Event system integration
    lle_input_coordinator_t *coordinator;       // TODO_SPEC06: Cross-system input coordination
    lle_input_conflict_resolver_t *conflict_resolver; // TODO_SPEC06: Input processing conflict resolution
    
    //-------------------------------------------------------------------------
    // Performance and optimization (4 fields) - Spec 06 Sections 13, 17
    //-------------------------------------------------------------------------
    lle_error_context_t *error_ctx;             // TODO_SPEC06: Error handling context
    lle_performance_monitor_t *perf_monitor;    // TODO_SPEC06: Performance monitoring
    lle_input_cache_t *input_cache;             // TODO_SPEC06: Input processing cache
    lle_memory_pool_t *memory_pool;             // TODO_SPEC06: Memory management
    
    //-------------------------------------------------------------------------
    // Synchronization and state (5 fields) - Spec 06 Section 2.1
    //-------------------------------------------------------------------------
    pthread_mutex_t parser_mutex;               // Thread synchronization
    bool active;                                // Parser active state
    uint64_t bytes_processed;                   // Total bytes processed
    uint64_t keybinding_lookups;                // Keybinding lookup count
    uint64_t widget_hooks_triggered;            // Widget hooks triggered count
    
    //-------------------------------------------------------------------------
    // WEEK 10 WORKING IMPLEMENTATION (embedded in Spec 06 structure)
    //-------------------------------------------------------------------------
    // Terminal state (Week 10 functional implementation)
    int input_fd;                               // Input file descriptor
    struct termios orig_termios;                // Original terminal settings
    struct termios raw_termios;                 // Raw mode terminal settings
    bool raw_mode_enabled;                      // Raw mode active
    
    // Integration references (Week 10 - not owned)
    lle_buffer_manager_t *buffer_manager;       // Buffer manager reference
    lle_display_buffer_renderer_t *renderer;    // Display renderer reference
    
    // Input buffer for escape sequence parsing (Week 10)
    char input_buffer_data[32];                 // Buffer for reading escape sequences
    size_t input_buffer_len;                    // Current length
    
    // Statistics (Week 10)
    uint64_t keys_processed;                    // Keys processed
    uint64_t chars_inserted;                    // Characters inserted
    uint64_t chars_deleted;                     // Characters deleted
    uint64_t cursor_moves;                      // Cursor movements
    
    // State (Week 10)
    bool initialized;                           // Initialized flag
    bool running;                               // Running flag
    
} lle_input_parser_system_t;

//=============================================================================
// SECTION 2: SPEC 06 INITIALIZATION FUNCTION (Section 2.2)
// Complete 7-parameter signature as required by Spec 06
//=============================================================================

/**
 * Initialize input parser system with complete Spec 06 integration
 * 
 * SPEC 06 SECTION 2.2: Complete input parser system initialization
 * 
 * Week 10 Implementation: Creates functional input parser with working
 * keyboard input. Additional integrations are stubs marked TODO_SPEC06.
 * 
 * @param system Output parameter for initialized parser system
 * @param terminal Terminal system reference (TODO_SPEC06: not used in Week 10)
 * @param event_system Event system reference (TODO_SPEC06: not used in Week 10)
 * @param keybinding_engine Keybinding engine reference (TODO_SPEC06: stub)
 * @param widget_hooks Widget hooks manager reference (TODO_SPEC06: stub)
 * @param adaptive_terminal Adaptive terminal integration (TODO_SPEC06: stub)
 * @param memory_pool Memory pool for allocations (TODO_SPEC06: not used in Week 10)
 * @return LLE_INPUT_OK on success, error code otherwise
 */
int lle_input_parser_system_init(lle_input_parser_system_t **system,
                                  void *terminal,              // TODO_SPEC06: lle_terminal_system_t
                                  void *event_system,          // TODO_SPEC06: lle_event_system_t
                                  void *keybinding_engine,     // TODO_SPEC06: lle_keybinding_engine_t
                                  void *widget_hooks,          // TODO_SPEC06: lle_widget_hooks_manager_t
                                  void *adaptive_terminal,     // TODO_SPEC06: lle_adaptive_terminal_integration_t
                                  void *memory_pool);          // TODO_SPEC06: lle_memory_pool_t

/**
 * Week 10 simplified initialization (convenience wrapper)
 * 
 * Provides simpler initialization for Week 10 testing without full
 * Spec 06 integration parameters. Internally calls lle_input_parser_system_init
 * with NULL for unimplemented components.
 */
int lle_input_parser_system_init_simple(lle_input_parser_system_t **system,
                                         int input_fd,
                                         lle_buffer_manager_t *buffer_manager,
                                         lle_display_buffer_renderer_t *renderer);

//=============================================================================
// SECTION 3: WEEK 10 FUNCTIONAL API (Working Implementation)
//=============================================================================

// Enable raw terminal mode (for character-by-character input)
int lle_input_parser_enable_raw_mode(lle_input_parser_system_t *parser);

// Disable raw terminal mode (restore original settings)
int lle_input_parser_disable_raw_mode(lle_input_parser_system_t *parser);

// Read and parse next input event
int lle_input_parser_read_event(lle_input_parser_system_t *parser,
                                 lle_key_event_t *event);

// Process input event (execute corresponding action)
int lle_input_parser_process_event(lle_input_parser_system_t *parser,
                                    const lle_key_event_t *event);

// Main input loop (read event → process → render until exit)
int lle_input_parser_run(lle_input_parser_system_t *parser);

//=============================================================================
// SECTION 4: INPUT ACTION HANDLERS (Week 10 Functional)
//=============================================================================

int lle_input_action_insert_char(lle_input_parser_system_t *parser, char ch);
int lle_input_action_backspace(lle_input_parser_system_t *parser);
int lle_input_action_delete(lle_input_parser_system_t *parser);
int lle_input_action_move_left(lle_input_parser_system_t *parser);
int lle_input_action_move_right(lle_input_parser_system_t *parser);
int lle_input_action_move_up(lle_input_parser_system_t *parser);
int lle_input_action_move_down(lle_input_parser_system_t *parser);
int lle_input_action_move_home(lle_input_parser_system_t *parser);
int lle_input_action_move_end(lle_input_parser_system_t *parser);
int lle_input_action_page_up(lle_input_parser_system_t *parser);
int lle_input_action_page_down(lle_input_parser_system_t *parser);
int lle_input_action_newline(lle_input_parser_system_t *parser);

// Ctrl key actions
int lle_input_action_beginning_of_line(lle_input_parser_system_t *parser);  // Ctrl+A
int lle_input_action_end_of_line(lle_input_parser_system_t *parser);        // Ctrl+E
int lle_input_action_kill_line(lle_input_parser_system_t *parser);          // Ctrl+K
int lle_input_action_kill_backward(lle_input_parser_system_t *parser);      // Ctrl+U
int lle_input_action_clear_screen(lle_input_parser_system_t *parser);       // Ctrl+L

//=============================================================================
// SECTION 5: UTILITY FUNCTIONS
//=============================================================================

// Get statistics
void lle_input_parser_get_stats(const lle_input_parser_system_t *parser,
                                 uint64_t *keys_processed,
                                 uint64_t *chars_inserted,
                                 uint64_t *chars_deleted,
                                 uint64_t *cursor_moves);

// Cleanup and destroy parser system
void lle_input_parser_system_cleanup(lle_input_parser_system_t *parser);

// Convert error code to string
const char* lle_input_error_string(int error_code);

// Convert key code to string (for debugging)
const char* lle_key_code_string(lle_key_code_t key);

//=============================================================================
// SECTION 6: SPEC 06 STUB FUNCTION DECLARATIONS (TODO Phase 1 Month 2)
// These are declared here to document the complete Spec 06 API surface
//=============================================================================

// TODO_SPEC06 Section 3: Keybinding Integration
// int lle_input_process_with_keybinding_lookup(lle_input_parser_system_t *parser, lle_input_event_t *event);
// int lle_keybinding_integration_init(lle_keybinding_integration_t **integration, void *keybinding_engine, void *memory_pool);

// TODO_SPEC06 Section 4: Widget Hook Triggers
// int lle_input_trigger_widget_hooks(lle_input_parser_system_t *parser, lle_input_event_t *event);
// int lle_widget_hook_triggers_init(lle_widget_hook_triggers_t **triggers, void *widget_hooks, void *memory_pool);

// TODO_SPEC06 Section 5: Adaptive Terminal Integration
// int lle_input_parse_with_adaptive_terminal(lle_input_parser_system_t *parser, const char *data, size_t len, void **events);
// int lle_adaptive_terminal_parser_init(lle_adaptive_terminal_parser_t **parser, void *adaptive_terminal, void *memory_pool);

// TODO_SPEC06 Section 6: Terminal Sequence Parsing
// int lle_sequence_parser_parse_with_keybinding(lle_sequence_parser_t *parser, lle_input_stream_t *stream, lle_keybinding_integration_t *kb);
// int lle_sequence_parser_init(lle_sequence_parser_t **parser, void *memory_pool);

// TODO_SPEC06 Section 7: Unicode and UTF-8 Processing
// int lle_utf8_processor_process_with_hooks(lle_utf8_processor_t *processor, lle_widget_hook_triggers_t *hooks, const uint8_t *bytes, size_t count);
// int lle_utf8_processor_init(lle_utf8_processor_t **processor);

// TODO_SPEC06 Section 8: Key Sequence Detection
// int lle_key_detector_detect_with_integration(lle_key_detector_t *detector, lle_input_parser_system_t *parser, lle_input_stream_t *stream);
// int lle_key_detector_init(lle_key_detector_t **detector, void *terminal_caps, void *memory_pool);

// TODO_SPEC06 Section 9: Mouse Input Processing
// int lle_mouse_parser_parse_sequence(lle_mouse_parser_t *parser, const char *sequence, size_t len, void **mouse_info);
// int lle_mouse_parser_init(lle_mouse_parser_t **parser, void *terminal_caps, void *memory_pool);

// TODO_SPEC06 Section 10: Input Stream Management
// int lle_input_stream_init(lle_input_stream_t **stream, void *terminal, void *memory_pool);

// TODO_SPEC06 Section 11: Parser State Machine
// int lle_parser_state_machine_init(lle_parser_state_machine_t **machine, void *error_ctx, void *memory_pool);

// TODO_SPEC06 Section 12: Error Handling and Recovery
// int lle_input_parser_recover_from_error(lle_input_parser_system_t *parser, int error_code, const char *data, size_t len);

// TODO_SPEC06 Section 13: Performance Optimization
// int lle_performance_monitor_init(lle_performance_monitor_t **monitor);

// TODO_SPEC06 Section 14: Event System Integration
// int lle_input_parser_generate_events(lle_input_parser_system_t *parser, void *parsed_input);

#endif // LLE_FOUNDATION_INPUT_PROCESSOR_H
