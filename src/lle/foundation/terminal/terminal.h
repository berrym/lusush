// src/lle/foundation/terminal/terminal.h
//
// LLE Terminal Abstraction Layer - Phase 0 Week 1 Implementation
// Based on: docs/lle_specification/02_terminal_abstraction_complete.md
//
// CRITICAL ARCHITECTURAL PRINCIPLES (Research-Validated):
// 1. Internal state is AUTHORITATIVE - NEVER query terminal during operation
// 2. One-time capability detection at initialization ONLY
// 3. Display through Lusush display system - NEVER direct terminal control
// 4. Calculate cursor position from internal buffer state
// 5. Generate complete display content for each update
//
// This implements the 8-subsystem architecture from Spec 02:
// 1. Internal State Authority Model
// 2. Display Content Generation System  
// 3. Lusush Display Layer Integration
// 4. Terminal Capability Detection
// 5. Input Event Processing
// 6. Unix Terminal Interface
// 7. Error Handling and Recovery
// 8. Performance Monitoring

#ifndef LLE_FOUNDATION_TERMINAL_H
#define LLE_FOUNDATION_TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>

// Forward declarations for integration with other LLE systems
typedef struct lle_error_context lle_error_context_t;
typedef struct lusush_display_context lusush_display_context_t;

//==============================================================================
// RESULT CODES (Spec 02 Section 9.1)
//==============================================================================

typedef enum {
    LLE_SUCCESS = 0,
    
    // Terminal errors (1200-1299 from Spec 16)
    LLE_ERROR_TERMINAL_INIT = 1200,
    LLE_ERROR_TERMINAL_NOT_TTY = 1201,
    LLE_ERROR_TERMINAL_SETTINGS = 1202,
    LLE_ERROR_TERMINAL_SIZE = 1203,
    LLE_ERROR_CAPABILITY_DETECTION = 1204,
    LLE_ERROR_DISPLAY_CLIENT = 1205,
    
    // Invalid parameters
    LLE_ERROR_INVALID_PARAMETER = 1000,
    LLE_ERROR_NULL_POINTER = 1001,
    
    // Memory errors
    LLE_ERROR_MEMORY_ALLOCATION = 1100,
} lle_result_t;

//==============================================================================
// SUBSYSTEM 1: INTERNAL STATE AUTHORITY MODEL (Spec 02 Section 3)
//==============================================================================

// Terminal state - THE SINGLE SOURCE OF TRUTH
// Never queried from terminal, always calculated from buffer state
typedef struct lle_internal_state {
    // Cursor position (logical, 0-based)
    uint16_t cursor_row;
    uint16_t cursor_col;
    
    // Terminal dimensions (from SIGWINCH only)
    uint16_t rows;
    uint16_t cols;
    
    // Scroll region
    uint16_t scroll_top;
    uint16_t scroll_bottom;
    
    // Terminal modes
    bool application_keypad_mode;
    bool application_cursor_mode;
    bool auto_wrap_mode;
    bool origin_mode;
    bool bracketed_paste_enabled;
    bool mouse_tracking_enabled;
    
    // State tracking metadata
    uint64_t last_update_timestamp_ns;
    uint32_t state_version;           // Increments on each state change
    bool window_size_changed_flag;    // Set by SIGWINCH handler
    
} lle_internal_state_t;

//==============================================================================
// SUBSYSTEM 4: TERMINAL CAPABILITY DETECTION (Spec 02 Section 4)
//==============================================================================

// Terminal type (detected from environment variables)
typedef enum {
    LLE_TERM_TYPE_XTERM,
    LLE_TERM_TYPE_XTERM_256COLOR,
    LLE_TERM_TYPE_XTERM_TRUE_COLOR,
    LLE_TERM_TYPE_VT100,
    LLE_TERM_TYPE_KONSOLE,
    LLE_TERM_TYPE_GNOME_TERMINAL,
    LLE_TERM_TYPE_ALACRITTY,
    LLE_TERM_TYPE_KITTY,
    LLE_TERM_TYPE_RXVT,
    LLE_TERM_TYPE_SCREEN,
    LLE_TERM_TYPE_TMUX,
    LLE_TERM_TYPE_UNKNOWN
} lle_term_type_t;

// Terminal capabilities (detected ONCE at initialization, NEVER queried after)
typedef struct lle_terminal_capabilities {
    // Color support
    bool has_color;               // Basic 8/16 color
    bool has_256_color;           // 256 color palette  
    bool has_true_color;          // 24-bit RGB
    
    // Text attributes
    bool has_unicode;             // UTF-8 support
    bool has_bold;
    bool has_underline;
    bool has_italic;
    bool has_strikethrough;
    bool has_dim;
    
    // Interactive features
    bool has_mouse;
    bool has_bracketed_paste;
    bool has_focus_events;
    bool has_alternate_screen;
    
    // Terminal identification
    lle_term_type_t terminal_type;
    char term_env[64];            // $TERM value
    char colorterm_env[64];       // $COLORTERM value
    
    // Detection metadata
    uint32_t detection_time_ms;
    bool detection_successful;
    bool detection_complete;
    
} lle_terminal_capabilities_t;

//==============================================================================
// SUBSYSTEM 2: DISPLAY CONTENT GENERATION (Spec 02 Section 5)
//==============================================================================

// Display content (generated from buffer state, sent to Lusush display)
typedef struct lle_display_content {
    char **lines;                 // Array of display lines
    size_t line_count;
    
    // Cursor position in display
    uint16_t cursor_display_row;
    uint16_t cursor_display_col;
    
    // Attributes per character position
    struct {
        uint8_t *colors;          // Color attributes
        uint8_t *styles;          // Text style attributes
    } attributes;
    
    // Content metadata
    uint32_t content_version;     // Matches state version
    uint64_t generation_time_ns;
    
} lle_display_content_t;

typedef struct lle_display_generator {
    lle_internal_state_t *state;          // Reference to state
    lle_terminal_capabilities_t *caps;     // Reference to capabilities
    
    // Content generation functions
    lle_result_t (*generate_content)(struct lle_display_generator *gen,
                                     const char *buffer_content,
                                     size_t buffer_length,
                                     size_t cursor_position,
                                     lle_display_content_t **content);
    
    // Performance tracking
    uint64_t total_generations;
    uint64_t total_generation_time_ns;
    
} lle_display_generator_t;

//==============================================================================
// SUBSYSTEM 3: LUSUSH DISPLAY LAYER INTEGRATION (Spec 02 Section 6)
//==============================================================================

// LLE as client of Lusush display system - NEVER bypass to terminal
typedef struct lle_lusush_display_client {
    lusush_display_context_t *lusush_display;  // Lusush display system
    lle_terminal_capabilities_t *caps;         // Terminal capabilities
    
    // Display client functions
    lle_result_t (*render_content)(struct lle_lusush_display_client *client,
                                   const lle_display_content_t *content);
    
    lle_result_t (*update_prompt)(struct lle_lusush_display_client *client,
                                  const char *prompt);
    
    lle_result_t (*clear_display)(struct lle_lusush_display_client *client);
    
    // Integration tracking
    uint64_t total_renders;
    uint64_t failed_renders;
    
} lle_lusush_display_client_t;

//==============================================================================
// SUBSYSTEM 5: INPUT EVENT PROCESSING (Spec 02 Section 7)
//==============================================================================

typedef enum {
    LLE_INPUT_KEY_PRESS,
    LLE_INPUT_KEY_SEQUENCE,
    LLE_INPUT_MOUSE_EVENT,
    LLE_INPUT_WINDOW_RESIZE,
    LLE_INPUT_FOCUS_CHANGE,
    LLE_INPUT_PASTE_START,
    LLE_INPUT_PASTE_END,
    LLE_INPUT_PASTE_DATA,
} lle_input_event_type_t;

typedef struct lle_input_event {
    lle_input_event_type_t type;
    uint64_t timestamp_ns;
    
    union {
        struct {
            char key;
            uint8_t modifiers;        // Ctrl, Alt, Shift
        } key_press;
        
        struct {
            char sequence[32];
            size_t length;
        } key_sequence;
        
        struct {
            uint16_t row;
            uint16_t col;
            uint8_t button;
            uint8_t action;           // Press, release, move
        } mouse;
        
        struct {
            uint16_t new_rows;
            uint16_t new_cols;
        } resize;
        
        struct {
            bool has_focus;
        } focus;
        
        struct {
            const char *data;
            size_t length;
        } paste;
    } data;
    
} lle_input_event_t;

typedef struct lle_input_processor {
    lle_terminal_capabilities_t *caps;
    
    // Input processing function
    lle_result_t (*process_input)(struct lle_input_processor *proc,
                                  const char *input_data,
                                  size_t input_length,
                                  lle_input_event_t **events,
                                  size_t *event_count);
    
    // Performance tracking
    uint64_t total_input_bytes;
    uint64_t total_events_generated;
    
} lle_input_processor_t;

//==============================================================================
// SUBSYSTEM 6: UNIX TERMINAL INTERFACE (Spec 02 Section 8)
//==============================================================================

// Minimal Unix terminal interface (abstracted, not used directly)
typedef struct lle_unix_interface {
    // File descriptors
    int input_fd;                 // Typically STDIN_FILENO
    int output_fd;                // Typically STDOUT_FILENO
    
    // Terminal settings
    struct termios original_termios;
    struct termios raw_termios;
    bool raw_mode_active;
    
    // Low-level operations
    lle_result_t (*enter_raw_mode)(struct lle_unix_interface *iface);
    lle_result_t (*exit_raw_mode)(struct lle_unix_interface *iface);
    lle_result_t (*get_window_size)(struct lle_unix_interface *iface,
                                    uint16_t *rows, uint16_t *cols);
    
    // State
    bool initialized;
    
} lle_unix_interface_t;

//==============================================================================
// SUBSYSTEM 8: PERFORMANCE MONITORING (Spec 02 Section 10)
//==============================================================================

typedef struct lle_performance_monitor {
    const char *component_name;
    
    // Timing measurements
    struct {
        uint64_t total_operations;
        uint64_t total_time_ns;
        uint64_t min_time_ns;
        uint64_t max_time_ns;
    } metrics;
    
    // Performance targets from spec
    uint64_t target_max_latency_ns;   // 500000ns (500μs)
    
    // Functions
    void (*start_operation)(struct lle_performance_monitor *mon);
    void (*end_operation)(struct lle_performance_monitor *mon);
    double (*get_avg_latency_us)(const struct lle_performance_monitor *mon);
    
} lle_performance_monitor_t;

//==============================================================================
// MAIN TERMINAL ABSTRACTION STRUCTURE (Spec 02 Section 2)
//==============================================================================

// Complete terminal abstraction with all 8 subsystems
typedef struct lle_terminal_abstraction {
    // Subsystem 1: Internal State Authority Model (CORE)
    lle_internal_state_t *internal_state;
    
    // Subsystem 2: Display Content Generation System
    lle_display_generator_t *display_generator;
    
    // Subsystem 3: Lusush Display Layer Integration
    lle_lusush_display_client_t *display_client;
    
    // Subsystem 4: Terminal Capability Detection
    lle_terminal_capabilities_t *capabilities;
    
    // Subsystem 5: Input Event Processing
    lle_input_processor_t *input_processor;
    
    // Subsystem 6: Unix Terminal Interface
    lle_unix_interface_t *unix_interface;
    
    // Subsystem 7: Error Handling (integrated with global error system)
    lle_error_context_t *error_ctx;
    
    // Subsystem 8: Performance Monitoring
    lle_performance_monitor_t *perf_monitor;
    
    // Initialization state
    bool initialized;
    uint32_t initialization_flags;
    
} lle_terminal_abstraction_t;

//==============================================================================
// PUBLIC API (Spec 02 Section 2.2 - 2.4)
//==============================================================================

// Initialize terminal abstraction (performs one-time capability detection)
// This is the ONLY time we detect capabilities - NEVER query again
lle_result_t lle_terminal_abstraction_init(
    lle_terminal_abstraction_t **abstraction,
    lusush_display_context_t *lusush_display,
    int input_fd,
    int output_fd
);

// Update internal state (NEVER queries terminal, only updates internal model)
lle_result_t lle_terminal_update_cursor(
    lle_terminal_abstraction_t *term,
    uint16_t row,
    uint16_t col
);

// Update window size (called ONLY from SIGWINCH handler)
// This is the ONE exception where we query terminal via ioctl(TIOCGWINSZ)
lle_result_t lle_terminal_update_window_size(
    lle_terminal_abstraction_t *term
);

// Process input bytes into events
lle_result_t lle_terminal_process_input(
    lle_terminal_abstraction_t *term,
    const char *input_data,
    size_t input_length,
    lle_input_event_t **events,
    size_t *event_count
);

// Generate display content from buffer state
lle_result_t lle_terminal_generate_display(
    lle_terminal_abstraction_t *term,
    const char *buffer_content,
    size_t buffer_length,
    size_t cursor_position,
    lle_display_content_t **display_content
);

// Render display content through Lusush display system
lle_result_t lle_terminal_render_display(
    lle_terminal_abstraction_t *term,
    const lle_display_content_t *display_content
);

// Get internal state (read-only, NEVER queries terminal)
const lle_internal_state_t* lle_terminal_get_state(
    const lle_terminal_abstraction_t *term
);

// Get capabilities (read-only, detected once at init)
const lle_terminal_capabilities_t* lle_terminal_get_capabilities(
    const lle_terminal_abstraction_t *term
);

// Cleanup and restore terminal
void lle_terminal_abstraction_cleanup(
    lle_terminal_abstraction_t *term
);

//==============================================================================
// INTERNAL SUBSYSTEM INITIALIZATION (Implementation Only)
//==============================================================================

// These are internal functions used by lle_terminal_abstraction_init()
// Following Spec 02 Section 2.2 initialization flow

lle_result_t lle_unix_interface_init(
    lle_unix_interface_t **iface,
    int input_fd,
    int output_fd
);

lle_result_t lle_capabilities_detect_environment(
    lle_terminal_capabilities_t **caps,
    const lle_unix_interface_t *unix_iface
);

lle_result_t lle_internal_state_init(
    lle_internal_state_t **state,
    const lle_terminal_capabilities_t *caps
);

lle_result_t lle_display_generator_init(
    lle_display_generator_t **generator,
    const lle_terminal_capabilities_t *caps,
    lle_internal_state_t *state
);

lle_result_t lle_lusush_display_client_init(
    lle_lusush_display_client_t **client,
    lusush_display_context_t *lusush_display,
    const lle_terminal_capabilities_t *caps
);

lle_result_t lle_input_processor_init(
    lle_input_processor_t **processor,
    const lle_terminal_capabilities_t *caps,
    const lle_unix_interface_t *unix_iface
);

lle_result_t lle_performance_monitor_init(
    lle_performance_monitor_t **monitor,
    const char *component_name
);

// Cleanup functions
void lle_unix_interface_destroy(lle_unix_interface_t *iface);
void lle_capabilities_destroy(lle_terminal_capabilities_t *caps);
void lle_internal_state_destroy(lle_internal_state_t *state);
void lle_display_generator_destroy(lle_display_generator_t *generator);
void lle_lusush_display_client_destroy(lle_lusush_display_client_t *client);
void lle_input_processor_destroy(lle_input_processor_t *processor);
void lle_performance_monitor_destroy(lle_performance_monitor_t *monitor);

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

const char* lle_result_to_string(lle_result_t result);

#endif // LLE_FOUNDATION_TERMINAL_H
