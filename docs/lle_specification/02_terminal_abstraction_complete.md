# Terminal Abstraction Complete Specification

**Document**: 02_terminal_abstraction_complete.md  
**Version**: 1.0.0  
**Date**: 2025-01-07  
**Status**: Implementation-Ready Specification  
**Classification**: Critical Foundation Component  

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Terminal Capability Detection System](#3-terminal-capability-detection-system)
4. [Unix Terminal Management](#4-unix-terminal-management)
5. [Input Event Processing](#5-input-event-processing)
6. [Safe Terminal Querying](#6-safe-terminal-querying)
7. [Key Sequence Management](#7-key-sequence-management)
8. [Error Handling and Recovery](#8-error-handling-and-recovery)
9. [State Machine Definitions](#9-state-machine-definitions)
10. [Integration with Lusush Systems](#10-integration-with-lusush-systems)
11. [Performance Requirements](#11-performance-requirements)
12. [Testing and Validation](#12-testing-and-validation)

---

## 1. Executive Summary

### 1.1 Purpose

The Terminal Abstraction Layer provides intelligent, capability-based terminal management for the Lusush Line Editor (LLE), eliminating fragile VT100 dependencies through research-driven Unix-native implementation with dynamic capability detection and graceful degradation.

### 1.2 Key Features

- **Intelligent Capability Detection**: Runtime terminal feature detection with timeout-based probing
- **Unix-Native Implementation**: termios-based control with selective ANSI enhancement
- **Event-Driven Input**: High-performance poll/select-based input processing
- **Graceful Degradation**: Works from basic terminals to advanced modern terminals
- **Terminal-Specific Optimization**: Known terminal types get optimized code paths
- **Memory Safety**: Comprehensive bounds checking and resource management

### 1.3 Critical Design Principles

1. **No Universal Assumptions**: Every terminal is different, detect and adapt
2. **Timeout-Based Safety**: All terminal queries have timeouts and fallbacks
3. **Progressive Enhancement**: Start basic, add features based on capabilities
4. **Platform Optimization**: Unix/Linux-specific implementation for maximum reliability
5. **Zero Fragility**: System must never break on unknown terminals

---

## 2. Architecture Overview

### 2.1 Component Structure

```c
// Primary abstraction layer components
typedef struct lle_terminal_system {
    lle_unix_terminal_t *terminal;           // Unix terminal management
    lle_terminal_capabilities_t *caps;       // Detected capabilities
    lle_input_processor_t *input_processor;  // Input event processing
    lle_key_sequence_manager_t *key_mgr;     // Key sequence management
    lle_terminal_state_t *state;             // Current terminal state
    lle_error_context_t *error_ctx;          // Error handling context
    lle_performance_monitor_t *perf_monitor; // Performance monitoring
} lle_terminal_system_t;
```

### 2.2 Initialization Flow

```c
// Complete terminal system initialization
lle_result_t lle_terminal_system_init(lle_terminal_system_t **system) {
    lle_result_t result = LLE_SUCCESS;
    lle_terminal_system_t *sys = NULL;
    
    // Step 1: Allocate system structure
    sys = calloc(1, sizeof(lle_terminal_system_t));
    if (!sys) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Step 2: Initialize error handling context first
    result = lle_error_context_init(&sys->error_ctx);
    if (result != LLE_SUCCESS) {
        free(sys);
        return result;
    }
    
    // Step 3: Initialize performance monitoring
    result = lle_performance_monitor_init(&sys->perf_monitor);
    if (result != LLE_SUCCESS) {
        lle_error_context_destroy(sys->error_ctx);
        free(sys);
        return result;
    }
    
    // Step 4: Initialize Unix terminal management
    result = lle_unix_terminal_init(&sys->terminal);
    if (result != LLE_SUCCESS) {
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        free(sys);
        return result;
    }
    
    // Step 5: Detect terminal capabilities
    result = lle_terminal_capabilities_detect(&sys->caps, sys->terminal);
    if (result != LLE_SUCCESS) {
        lle_unix_terminal_destroy(sys->terminal);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        free(sys);
        return result;
    }
    
    // Step 6: Initialize input processor
    result = lle_input_processor_init(&sys->input_processor, sys->caps);
    if (result != LLE_SUCCESS) {
        lle_terminal_capabilities_destroy(sys->caps);
        lle_unix_terminal_destroy(sys->terminal);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        free(sys);
        return result;
    }
    
    // Step 7: Initialize key sequence manager
    result = lle_key_sequence_manager_init(&sys->key_mgr, sys->caps);
    if (result != LLE_SUCCESS) {
        lle_input_processor_destroy(sys->input_processor);
        lle_terminal_capabilities_destroy(sys->caps);
        lle_unix_terminal_destroy(sys->terminal);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        free(sys);
        return result;
    }
    
    // Step 8: Initialize terminal state
    result = lle_terminal_state_init(&sys->state, sys->caps);
    if (result != LLE_SUCCESS) {
        lle_key_sequence_manager_destroy(sys->key_mgr);
        lle_input_processor_destroy(sys->input_processor);
        lle_terminal_capabilities_destroy(sys->caps);
        lle_unix_terminal_destroy(sys->terminal);
        lle_performance_monitor_destroy(sys->perf_monitor);
        lle_error_context_destroy(sys->error_ctx);
        free(sys);
        return result;
    }
    
    // Step 9: Apply terminal-specific optimizations
    result = lle_apply_terminal_optimizations(sys);
    if (result != LLE_SUCCESS) {
        // Log warning but continue - optimizations are not critical
        lle_error_context_log_warning(sys->error_ctx, 
            "Terminal optimizations failed, continuing with defaults");
    }
    
    // Step 10: Perform initialization validation
    result = lle_terminal_system_validate(sys);
    if (result != LLE_SUCCESS) {
        lle_terminal_system_destroy(sys);
        return result;
    }
    
    *system = sys;
    return LLE_SUCCESS;
}
```

---

## 3. Terminal Capability Detection System

### 3.1 Capability Structure

```c
typedef struct lle_terminal_capabilities {
    // Basic identification
    char terminal_name[64];           // Terminal name from DA1/DA2
    char terminal_version[32];        // Version information
    char term_env[64];                // $TERM environment variable
    char colorterm_env[64];           // $COLORTERM environment variable
    bool is_tty;                      // Is a TTY (not redirected)
    
    // Color support capabilities
    bool supports_ansi_colors;        // Basic 16 ANSI colors
    bool supports_256_colors;         // 256-color palette
    bool supports_truecolor;          // 24-bit RGB colors
    uint8_t detected_color_depth;     // Detected color depth (4, 8, 24 bits)
    
    // Text attribute support
    bool supports_bold;               // Bold text
    bool supports_italic;             // Italic text
    bool supports_underline;          // Underline text
    bool supports_strikethrough;      // Strikethrough text
    bool supports_reverse;            // Reverse video
    bool supports_dim;                // Dim/faint text
    
    // Cursor and positioning
    bool supports_cursor_position;    // Cursor position reporting
    bool supports_cursor_style;       // Cursor style changes
    bool supports_alternate_screen;   // Alternate screen buffer
    bool supports_scrolling_region;   // Scrolling region control
    
    // Input capabilities
    bool supports_mouse_reporting;    // Mouse event reporting
    bool supports_bracketed_paste;    // Bracketed paste mode
    bool supports_focus_events;       // Focus in/out events
    bool supports_modify_keys;        // Modified key reporting
    bool supports_application_keypad; // Application keypad mode
    
    // Modern protocol support
    bool supports_kitty_keyboard;     // Kitty keyboard protocol
    bool supports_synchronized_output; // Synchronized output
    bool supports_osc52_clipboard;    // OSC 52 clipboard access
    bool supports_unicode;            // Unicode support level
    
    // Key sequence capabilities
    lle_key_sequence_map_t *key_sequences; // Detected key sequences
    size_t key_sequence_count;        // Number of key sequences
    bool function_keys_detected;      // F1-F12 key support
    bool arrow_keys_detected;         // Arrow key support
    bool home_end_detected;           // Home/End key support
    bool page_keys_detected;          // Page Up/Down support
    
    // Terminal-specific optimizations
    lle_terminal_type_t terminal_type; // Detected terminal type
    lle_optimization_flags_t optimizations; // Available optimizations
    uint32_t performance_flags;       // Performance-related flags
    
    // Timing and performance characteristics
    uint32_t response_time_ms;        // Average response time
    uint32_t escape_timeout_ms;       // Escape sequence timeout
    uint32_t query_timeout_ms;        // Query response timeout
    bool supports_fast_updates;       // Fast screen update support
    
    // Limitations and quirks
    lle_terminal_quirks_t quirks;     // Known terminal quirks
    lle_terminal_limits_t limits;     // Terminal limitations
    char unsupported_sequences[256];  // Known unsupported sequences
    
} lle_terminal_capabilities_t;
```

### 3.2 Capability Detection Implementation

```c
lle_result_t lle_terminal_capabilities_detect(lle_terminal_capabilities_t **caps, 
                                              lle_unix_terminal_t *terminal) {
    lle_result_t result = LLE_SUCCESS;
    lle_terminal_capabilities_t *detected_caps = NULL;
    
    // Step 1: Allocate capabilities structure
    detected_caps = calloc(1, sizeof(lle_terminal_capabilities_t));
    if (!detected_caps) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Step 2: Check if we're running in a TTY
    detected_caps->is_tty = isatty(terminal->stdin_fd);
    if (!detected_caps->is_tty) {
        // Set minimal capabilities for non-TTY operation
        lle_set_minimal_capabilities(detected_caps);
        *caps = detected_caps;
        return LLE_SUCCESS;
    }
    
    // Step 3: Analyze environment variables
    result = lle_analyze_environment_variables(detected_caps);
    if (result != LLE_SUCCESS) {
        // Warning only - continue with detection
        lle_log_warning("Environment variable analysis failed");
    }
    
    // Step 4: Enable raw mode for accurate detection
    result = lle_unix_terminal_enter_raw_mode(terminal);
    if (result != LLE_SUCCESS) {
        free(detected_caps);
        return result;
    }
    
    // Step 5: Query terminal identification (DA1/DA2)
    result = lle_query_terminal_identification(terminal, detected_caps);
    if (result != LLE_SUCCESS) {
        // Warning only - continue with other detection methods
        lle_log_warning("Terminal identification query failed");
    }
    
    // Step 6: Detect color support progressively
    result = lle_detect_color_capabilities(terminal, detected_caps);
    if (result != LLE_SUCCESS) {
        // Set safe color defaults
        detected_caps->supports_ansi_colors = true; // Safe assumption
        detected_caps->supports_256_colors = false;
        detected_caps->supports_truecolor = false;
    }
    
    // Step 7: Test cursor and positioning capabilities
    result = lle_detect_cursor_capabilities(terminal, detected_caps);
    if (result != LLE_SUCCESS) {
        // Set conservative cursor defaults
        detected_caps->supports_cursor_position = true; // Usually safe
    }
    
    // Step 8: Detect input capabilities
    result = lle_detect_input_capabilities(terminal, detected_caps);
    if (result != LLE_SUCCESS) {
        // Set basic input defaults
        lle_set_basic_input_capabilities(detected_caps);
    }
    
    // Step 9: Test modern protocol support
    result = lle_detect_modern_protocols(terminal, detected_caps);
    if (result != LLE_SUCCESS) {
        // Modern protocols are optional - continue
    }
    
    // Step 10: Build key sequence map
    result = lle_build_key_sequence_map(terminal, detected_caps);
    if (result != LLE_SUCCESS) {
        // Create basic key sequence map
        result = lle_create_basic_key_sequence_map(detected_caps);
        if (result != LLE_SUCCESS) {
            lle_unix_terminal_exit_raw_mode(terminal);
            free(detected_caps);
            return result;
        }
    }
    
    // Step 11: Apply terminal-specific settings
    result = lle_apply_terminal_specific_settings(detected_caps);
    if (result != LLE_SUCCESS) {
        // Warning only - terminal-specific settings are optional
        lle_log_warning("Terminal-specific settings application failed");
    }
    
    // Step 12: Set timing parameters based on terminal type
    lle_set_terminal_timing_parameters(detected_caps);
    
    // Step 13: Detect and record terminal quirks and limitations
    lle_detect_terminal_quirks(detected_caps);
    
    // Step 14: Exit raw mode
    result = lle_unix_terminal_exit_raw_mode(terminal);
    if (result != LLE_SUCCESS) {
        // This is critical - we must restore terminal state
        free(detected_caps);
        return result;
    }
    
    // Step 15: Validate detected capabilities
    result = lle_validate_detected_capabilities(detected_caps);
    if (result != LLE_SUCCESS) {
        free(detected_caps);
        return result;
    }
    
    *caps = detected_caps;
    return LLE_SUCCESS;
}
```

### 3.3 Safe Terminal Querying Implementation

```c
lle_result_t lle_safe_terminal_query(lle_unix_terminal_t *terminal,
                                     const char *query_sequence,
                                     const char *expected_response_prefix,
                                     char *response_buffer,
                                     size_t buffer_size,
                                     uint32_t timeout_ms) {
    if (!terminal || !query_sequence || !response_buffer || buffer_size == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Step 1: Save current terminal state
    struct termios saved_termios;
    if (tcgetattr(terminal->stdin_fd, &saved_termios) != 0) {
        return LLE_ERROR_TERMINAL_STATE;
    }
    
    // Step 2: Clear any pending input
    tcflush(terminal->stdin_fd, TCIFLUSH);
    
    // Step 3: Send query sequence with error checking
    ssize_t query_len = strlen(query_sequence);
    ssize_t written = write(terminal->stdout_fd, query_sequence, query_len);
    if (written != query_len) {
        tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
        return LLE_ERROR_WRITE_FAILED;
    }
    
    // Step 4: Force output to terminal
    if (fsync(terminal->stdout_fd) != 0) {
        tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
        return LLE_ERROR_SYNC_FAILED;
    }
    
    // Step 5: Poll for response with timeout
    struct pollfd pfd = { 
        .fd = terminal->stdin_fd, 
        .events = POLLIN,
        .revents = 0
    };
    
    int poll_result = poll(&pfd, 1, timeout_ms);
    if (poll_result < 0) {
        tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
        return LLE_ERROR_POLL_FAILED;
    }
    
    if (poll_result == 0) {
        // Timeout - no response received
        tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
        return LLE_ERROR_TIMEOUT;
    }
    
    // Step 6: Read response with bounds checking
    memset(response_buffer, 0, buffer_size);
    ssize_t bytes_read = read(terminal->stdin_fd, response_buffer, buffer_size - 1);
    if (bytes_read < 0) {
        tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
        return LLE_ERROR_READ_FAILED;
    }
    
    if (bytes_read == 0) {
        tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
        return LLE_ERROR_NO_DATA;
    }
    
    // Step 7: Null-terminate response
    response_buffer[bytes_read] = '\0';
    
    // Step 8: Restore terminal state
    tcsetattr(terminal->stdin_fd, TCSANOW, &saved_termios);
    
    // Step 9: Validate response if expected prefix provided
    if (expected_response_prefix) {
        if (strstr(response_buffer, expected_response_prefix) == NULL) {
            return LLE_ERROR_UNEXPECTED_RESPONSE;
        }
    }
    
    return LLE_SUCCESS;
}
```

---

## 4. Unix Terminal Management

### 4.1 Unix Terminal Structure

```c
typedef struct lle_unix_terminal {
    // File descriptors
    int stdin_fd;                     // Standard input file descriptor
    int stdout_fd;                    // Standard output file descriptor
    int stderr_fd;                    // Standard error file descriptor
    
    // Terminal state management
    struct termios original_termios;  // Original terminal settings
    struct termios raw_termios;       // Raw mode settings
    struct termios current_termios;   // Current terminal settings
    bool raw_mode_active;             // Raw mode status flag
    bool terminal_state_modified;     // State modification flag
    
    // Signal handling
    struct sigaction original_sigint; // Original SIGINT handler
    struct sigaction original_sigterm; // Original SIGTERM handler
    struct sigaction original_sigwinch; // Original SIGWINCH handler
    bool signal_handlers_installed;   // Signal handler status
    
    // Terminal characteristics
    struct winsize window_size;       // Current window size
    bool window_size_valid;           // Window size validity flag
    uint32_t last_winch_time;         // Last window change time
    
    // Input processing
    lle_input_parser_state_t parser_state; // Input parser state
    char input_buffer[LLE_INPUT_BUFFER_SIZE]; // Input buffer
    size_t input_buffer_len;          // Current input buffer length
    size_t input_buffer_pos;          // Current input buffer position
    
    // Escape sequence processing
    char escape_buffer[LLE_ESCAPE_BUFFER_SIZE]; // Escape sequence buffer
    size_t escape_buffer_len;         // Current escape buffer length
    uint64_t escape_start_time;       // Escape sequence start timestamp
    bool escape_in_progress;          // Escape sequence processing flag
    
    // Performance optimization
    struct pollfd poll_fds[3];        // Poll file descriptors array
    int poll_timeout;                 // Current poll timeout value
    uint64_t last_input_time;         // Last input timestamp
    uint32_t input_rate;              // Input rate estimation
    
    // Error context
    lle_error_context_t *error_ctx;   // Error handling context
    uint32_t error_count;             // Error counter
    uint32_t warning_count;           // Warning counter
    
} lle_unix_terminal_t;
```

### 4.2 Terminal Initialization

```c
lle_result_t lle_unix_terminal_init(lle_unix_terminal_t **terminal) {
    lle_unix_terminal_t *term = NULL;
    lle_result_t result = LLE_SUCCESS;
    
    // Step 1: Allocate terminal structure
    term = calloc(1, sizeof(lle_unix_terminal_t));
    if (!term) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Step 2: Initialize file descriptors
    term->stdin_fd = STDIN_FILENO;
    term->stdout_fd = STDOUT_FILENO;
    term->stderr_fd = STDERR_FILENO;
    
    // Step 3: Verify file descriptors are valid TTYs
    if (!isatty(term->stdin_fd) || !isatty(term->stdout_fd)) {
        // Not a TTY - set up for non-interactive mode
        term->raw_mode_active = false;
        term->terminal_state_modified = false;
    } else {
        // Step 4: Get original terminal attributes
        if (tcgetattr(term->stdin_fd, &term->original_termios) != 0) {
            free(term);
            return LLE_ERROR_TERMINAL_ATTR_GET;
        }
        
        // Step 5: Set up raw mode configuration
        term->raw_termios = term->original_termios;
        
        // Disable canonical mode and echo
        term->raw_termios.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
        
        // Disable signal generation for special characters
        term->raw_termios.c_lflag &= ~(ISIG | IEXTEN);
        
        // Disable input processing
        term->raw_termios.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR);
        
        // Disable output processing
        term->raw_termios.c_oflag &= ~(OPOST);
        
        // Set minimum read parameters
        term->raw_termios.c_cc[VMIN] = 1;   // Minimum characters to read
        term->raw_termios.c_cc[VTIME] = 0;  // No timeout
        
        term->current_termios = term->original_termios;
        term->raw_mode_active = false;
        term->terminal_state_modified = false;
    }
    
    // Step 6: Get initial window size
    if (ioctl(term->stdout_fd, TIOCGWINSZ, &term->window_size) == 0) {
        term->window_size_valid = true;
    } else {
        // Set safe defaults
        term->window_size.ws_row = 24;
        term->window_size.ws_col = 80;
        term->window_size_valid = false;
    }
    
    // Step 7: Initialize input parser state
    result = lle_input_parser_state_init(&term->parser_state);
    if (result != LLE_SUCCESS) {
        free(term);
        return result;
    }
    
    // Step 8: Initialize input buffer
    term->input_buffer_len = 0;
    term->input_buffer_pos = 0;
    memset(term->input_buffer, 0, sizeof(term->input_buffer));
    
    // Step 9: Initialize escape sequence processing
    term->escape_buffer_len = 0;
    term->escape_start_time = 0;
    term->escape_in_progress = false;
    memset(term->escape_buffer, 0, sizeof(term->escape_buffer));
    
    // Step 10: Set up polling configuration
    term->poll_fds[0].fd = term->stdin_fd;
    term->poll_fds[0].events = POLLIN;
    term->poll_fds[0].revents = 0;
    
    term->poll_fds[1].fd = term->stdout_fd;
    term->poll_fds[1].events = POLLOUT;
    term->poll_fds[1].revents = 0;
    
    term->poll_fds[2].fd = term->stderr_fd;
    term->poll_fds[2].events = POLLOUT;
    term->poll_fds[2].revents = 0;
    
    term->poll_timeout = LLE_DEFAULT_POLL_TIMEOUT;
    
    // Step 11: Initialize performance tracking
    term->last_input_time = lle_get_current_time_us();
    term->input_rate = 0;
    
    // Step 12: Initialize error context
    result = lle_error_context_init(&term->error_ctx);
    if (result != LLE_SUCCESS) {
        free(term);
        return result;
    }
    
    term->error_count = 0;
    term->warning_count = 0;
    term->signal_handlers_installed = false;
    
    *terminal = term;
    return LLE_SUCCESS;
}
```

### 4.3 Raw Mode Management

```c
lle_result_t lle_unix_terminal_enter_raw_mode(lle_unix_terminal_t *terminal) {
    if (!terminal) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (terminal->raw_mode_active) {
        return LLE_SUCCESS; // Already in raw mode
    }
    
    if (!isatty(terminal->stdin_fd)) {
        return LLE_ERROR_NOT_A_TTY;
    }
    
    // Step 1: Apply raw mode terminal attributes
    if (tcsetattr(terminal->stdin_fd, TCSANOW, &terminal->raw_termios) != 0) {
        return LLE_ERROR_TERMINAL_ATTR_SET;
    }
    
    // Step 2: Verify attributes were set correctly
    struct termios verify_termios;
    if (tcgetattr(terminal->stdin_fd, &verify_termios) != 0) {
        // Try to restore original settings
        tcsetattr(terminal->stdin_fd, TCSANOW, &terminal->original_termios);
        return LLE_ERROR_TERMINAL_ATTR_VERIFY;
    }
    
    // Step 3: Update terminal state
    terminal->current_termios = verify_termios;
    terminal->raw_mode_active = true;
    terminal->terminal_state_modified = true;
    
    // Step 4: Clear any pending input/output
    tcflush(terminal->stdin_fd, TCIOFLUSH);
    
    return LLE_SUCCESS;
}

lle_result_t lle_unix_terminal_exit_raw_mode(lle_unix_terminal_t *terminal) {
    if (!terminal) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!terminal->raw_mode_active) {
        return LLE_SUCCESS; // Already in normal mode
    }
    
    if (!isatty(terminal->stdin_fd)) {
        return LLE_ERROR_NOT_A_TTY;
    }
    
    // Step 1: Restore original terminal attributes
    if (tcsetattr(terminal->stdin_fd, TCSANOW, &terminal->original_termios) != 0) {
        return LLE_ERROR_TERMINAL_ATTR_RESTORE;
    }
    
    // Step 2: Verify restoration
    struct termios verify_termios;
    if (tcgetattr(terminal->stdin_fd, &verify_termios) != 0) {
        // Critical error - terminal state may be corrupted
        return LLE_ERROR_TERMINAL_ATTR_VERIFY;
    }
    
    // Step 3: Update terminal state
    terminal->current_termios = verify_termios;
    terminal->raw_mode_active = false;
    terminal->terminal_state_modified = false;
    
    // Step 4: Flush any remaining output
    fflush(stdout);
    fflush(stderr);
    
    return LLE_SUCCESS;
}
```

---

## 5. Input Event Processing

### 5.1 Input Event Types

```c
typedef enum {
    LLE_INPUT_TYPE_UNKNOWN = 0,       // Unknown input type
    LLE_INPUT_TYPE_CHARACTER,         // Regular character input
    LLE_INPUT_TYPE_SPECIAL_KEY,       // Special keys (F1, arrows, etc.)
    LLE_INPUT_TYPE_FUNCTION_KEY,      // Function keys (F1-F12)
    LLE_INPUT_TYPE_ARROW_KEY,         // Arrow keys
    LLE_INPUT_TYPE_MODIFIER_KEY,      // Modifier key combinations
    LLE_INPUT_TYPE_MOUSE_EVENT,       // Mouse events
    LLE_INPUT_TYPE_PASTE_EVENT,       // Bracketed paste
    LLE_INPUT_TYPE_FOCUS_EVENT,       // Focus in/out
    LLE_INPUT_TYPE_WINDOW_RESIZE,     // Window size change
    LLE_INPUT_TYPE_SIGNAL,            // Signal received
    LLE_INPUT_TYPE_TIMEOUT,           // Input timeout
    LLE_INPUT_TYPE_ERROR,             // Error condition
    LLE_INPUT_TYPE_EOF,               // End of file
} lle_input_type_t;

typedef struct lle_input_event {
    lle_input_type_t type;            // Event type
    uint64_t timestamp;               // Event timestamp (microseconds)
    uint32_t sequence_number;         // Event sequence number
    
    union {
        // Character input
        struct {
            uint32_t codepoint;       // Unicode codepoint
            char utf8_bytes[8];       // UTF-8 representation
            uint8_t byte_count;       // Number of UTF-8 bytes
        } character;
        
        // Special key input
        struct {
            lle_special_key_t key;    // Special key identifier
            uint32_t modifiers;       // Modifier flags
            char raw_sequence[32];    // Raw escape sequence
            uint8_t sequence_length;  // Sequence length
        } special_key;
        
        // Mouse event
        struct {
            lle_mouse_event_type_t event_type; // Mouse event type
            uint16_t x, y;            // Mouse coordinates
            uint32_t buttons;         // Button state
            uint32_t modifiers;       // Modifier keys
        } mouse;
        
        // Paste event
        struct {
            char *data;               // Pasted data
            size_t length;            // Data length
            bool is_complete;         // Paste completion status
        } paste;
        
        // Focus event
        struct {
            bool focused;             // Focus state
        } focus;
        
        // Window resize
        struct {
            uint16_t rows, cols;      // New window size
            uint16_t pixel_width, pixel_height; // Pixel dimensions
        } resize;
        
        // Signal event
        struct {
            int signal_number;        // Signal number
            const char *signal_name;  // Signal name
        } signal;
        
        // Timeout event
        struct {
            uint32_t timeout_ms;      // Timeout duration
            lle_timeout_type_t type;  // Timeout type
        } timeout;
        
        // Error event
        struct {
            lle_error_code_t error_code; // Error code
            char error_message[256];  // Error description
            const char *source_function; // Source function name
            int source_line;          // Source line number
        } error;
        
    } data;
    
    // Event metadata
    bool handled;                     // Event handling status
    uint32_t handler_count;           // Number of handlers called
    uint64_t processing_time_us;      // Processing time
    
} lle_input_event_t;
```

### 5.2 Input Processing Implementation

```c
lle_result_t lle_unix_read_input_event(lle_unix_terminal_t *terminal, 
                                       lle_input_event_t **event) {
    if (!terminal || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_input_event_t *input_event = NULL;
    lle_result_t result = LLE_SUCCESS;
    
    // Step 1: Check for buffered data first
    if (terminal->input_buffer_len > terminal->input_buffer_pos) {
        result = lle_process_buffered_input(terminal, &input_event);
        if (result == LLE_SUCCESS) {
            *event = input_event;
            return LLE_SUCCESS;
        }
    }
    
    // Step 2: Poll for new input with timeout
    struct pollfd pfd = { 
        .fd = terminal->stdin_fd, 
        .events = POLLIN,
        .revents = 0 
    };
    
    int poll_result = poll(&pfd, 1, terminal->poll_timeout);
    
    if (poll_result < 0) {
        if (errno == EINTR) {
            // Signal interrupted - create signal event
            return lle_create_signal_event(terminal, event);
        }
        return LLE_ERROR_POLL_FAILED;
    }
    
    if (poll_result == 0) {
        // Timeout occurred
        return lle_create_timeout_event(terminal, event);
    }
    
    // Step 3: Data is available - read into buffer
    char read_buffer[256];
    ssize_t bytes_read = read(terminal->stdin_fd, read_buffer, sizeof(read_buffer) - 1);
    
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data actually available
            return lle_create_timeout_event(terminal, event);
        }
        return LLE_ERROR_READ_FAILED;
    }
    
    if (bytes_read == 0) {
        // End of file
        return lle_create_eof_event(terminal, event);
    }
    
    // Step 4: Add to input buffer with overflow protection
    if (terminal->input_buffer_len + bytes_read >= sizeof(terminal->input_buffer)) {
        // Buffer overflow - reset buffer and log error
        terminal->input_buffer_len = 0;
        terminal->input_buffer_pos = 0;
        lle_error_context_log_error(terminal->error_ctx, 
            "Input buffer overflow, resetting buffer");
        return LLE_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(terminal->input_buffer + terminal->input_buffer_len, 
           read_buffer, bytes_read);
    terminal->input_buffer_len += bytes_read;
    
    // Step 5: Process the buffered input
    result = lle_process_buffered_input(terminal, &input_event);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    *event = input_event;
    return LLE_SUCCESS;
}

lle_result_t lle_process_buffered_input(lle_unix_terminal_t *terminal,
                                        lle_input_event_t **event) {
    if (!terminal || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Check if we have any data to process
    if (terminal->input_buffer_pos >= terminal->input_buffer_len) {
        return LLE_ERROR_NO_DATA;
    }
    
    char *buffer = terminal->input_buffer + terminal->input_buffer_pos;
    size_t remaining = terminal->input_buffer_len - terminal->input_buffer_pos;
    
    // Step 1: Check for escape sequences
    if (buffer[0] == '\x1b' && remaining > 1) {
        return lle_process_escape_sequence(terminal, event);
    }
    
    // Step 2: Check for control characters
    if (buffer[0] < 32) {
        return lle_process_control_character(terminal, event);
    }
    
    // Step 3: Process as regular character or UTF-8 sequence
    return lle_process_character_input(terminal, event);
}
```

---

## 6. Safe Terminal Querying

### 6.1 Query Functions

```c
lle_result_t lle_query_terminal_identification(lle_unix_terminal_t *terminal,
                                               lle_terminal_capabilities_t *caps) {
    char response_buffer[256];
    lle_result_t result;
    
    // Query DA1 (Device Attributes 1) - Terminal identification
    result = lle_safe_terminal_query(terminal, "\x1b[c", "\x1b[?", 
                                     response_buffer, sizeof(response_buffer), 200);
    
    if (result == LLE_SUCCESS) {
        lle_parse_da1_response(response_buffer, caps);
    }
    
    // Query DA2 (Device Attributes 2) - Terminal version
    memset(response_buffer, 0, sizeof(response_buffer));
    result = lle_safe_terminal_query(terminal, "\x1b[>c", "\x1b[>", 
                                     response_buffer, sizeof(response_buffer), 200);
    
    if (result == LLE_SUCCESS) {
        lle_parse_da2_response(response_buffer, caps);
    }
    
    return LLE_SUCCESS; // Non-critical if queries fail
}

lle_result_t lle_detect_color_capabilities(lle_unix_terminal_t *terminal,
                                           lle_terminal_capabilities_t *caps) {
    char response_buffer[256];
    lle_result_t result;
    
    // Test basic ANSI color support
    caps->supports_ansi_colors = true; // Safe default assumption
    
    // Test 256-color support
    result = lle_safe_terminal_query(terminal, "\x1b[38;5;196m\x1b[0m\x1b[6n", 
                                     "\x1b[", response_buffer, sizeof(response_buffer), 100);
    
    if (result == LLE_SUCCESS) {
        caps->supports_256_colors = true;
        caps->detected_color_depth = 8;
        
        // Test truecolor (24-bit) support
        memset(response_buffer, 0, sizeof(response_buffer));
        result = lle_safe_terminal_query(terminal, "\x1b[38;2;255;0;0m\x1b[0m\x1b[6n", 
                                         "\x1b[", response_buffer, sizeof(response_buffer), 100);
        
        if (result == LLE_SUCCESS) {
            caps->supports_truecolor = true;
            caps->detected_color_depth = 24;
        }
    } else {
        caps->supports_256_colors = false;
        caps->supports_truecolor = false;
        caps->detected_color_depth = 4;
    }
    
    return LLE_SUCCESS;
}
```

---

## 7. Key Sequence Management

### 7.1 Key Sequence Structure

```c
typedef struct lle_key_sequence {
    char sequence[32];                // Escape sequence
    uint8_t sequence_length;          // Sequence length
    lle_special_key_t key;            // Mapped key
    uint32_t modifiers;               // Modifier flags
    lle_terminal_type_t terminal_type; // Specific terminal type
    bool is_universal;                // Universal sequence
} lle_key_sequence_t;

typedef struct lle_key_sequence_map {
    lle_key_sequence_t *sequences;    // Array of sequences
    size_t count;                     // Number of sequences
    size_t capacity;                  // Allocated capacity
    lle_terminal_type_t terminal_type; // Target terminal type
} lle_key_sequence_map_t;
```

### 7.2 Key Sequence Detection

```c
lle_result_t lle_build_key_sequence_map(lle_unix_terminal_t *terminal,
                                        lle_terminal_capabilities_t *caps) {
    lle_result_t result;
    
    // Allocate key sequence map
    caps->key_sequences = calloc(1, sizeof(lle_key_sequence_map_t));
    if (!caps->key_sequences) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize with basic capacity
    caps->key_sequences->capacity = 64;
    caps->key_sequences->sequences = calloc(caps->key_sequences->capacity, 
                                            sizeof(lle_key_sequence_t));
    if (!caps->key_sequences->sequences) {
        free(caps->key_sequences);
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Test common key sequences
    result = lle_test_arrow_keys(terminal, caps->key_sequences);
    if (result == LLE_SUCCESS) {
        caps->arrow_keys_detected = true;
    }
    
    result = lle_test_function_keys(terminal, caps->key_sequences);
    if (result == LLE_SUCCESS) {
        caps->function_keys_detected = true;
    }
    
    result = lle_test_home_end_keys(terminal, caps->key_sequences);
    if (result == LLE_SUCCESS) {
        caps->home_end_detected = true;
    }
    
    result = lle_test_page_keys(terminal, caps->key_sequences);
    if (result == LLE_SUCCESS) {
        caps->page_keys_detected = true;
    }
    
    // Apply terminal-specific key mappings
    lle_apply_terminal_specific_keys(caps);
    
    caps->key_sequence_count = caps->key_sequences->count;
    
    return LLE_SUCCESS;
}
```

---

## 8. Error Handling and Recovery

### 8.1 Error Recovery System

```c
typedef struct lle_error_recovery_context {
    lle_unix_terminal_t *terminal;    // Terminal reference
    struct termios saved_termios;     // Saved terminal state
    bool terminal_state_saved;        // State save status
    uint32_t recovery_attempt_count;  // Recovery attempts
    uint64_t last_recovery_time;      // Last recovery timestamp
    lle_error_recovery_strategy_t strategy; // Recovery strategy
} lle_error_recovery_context_t;

lle_result_t lle_handle_terminal_error(lle_unix_terminal_t *terminal, 
                                       lle_error_code_t error_code,
                                       const char *error_context) {
    lle_error_recovery_context_t recovery_ctx;
    lle_result_t result;
    
    // Initialize recovery context
    recovery_ctx.terminal = terminal;
    recovery_ctx.terminal_state_saved = false;
    recovery_ctx.recovery_attempt_count = 0;
    recovery_ctx.last_recovery_time = lle_get_current_time_us();
    
    // Save current terminal state if possible
    if (tcgetattr(terminal->stdin_fd, &recovery_ctx.saved_termios) == 0) {
        recovery_ctx.terminal_state_saved = true;
    }
    
    // Determine recovery strategy based on error type
    switch (error_code) {
        case LLE_ERROR_TERMINAL_ATTR_SET:
        case LLE_ERROR_TERMINAL_ATTR_GET:
            recovery_ctx.strategy = LLE_RECOVERY_RESTORE_TERMINAL_STATE;
            break;
            
        case LLE_ERROR_READ_FAILED:
        case LLE_ERROR_WRITE_FAILED:
            recovery_ctx.strategy = LLE_RECOVERY_RESET_IO;
            break;
            
        case LLE_ERROR_POLL_FAILED:
            recovery_ctx.strategy = LLE_RECOVERY_RESET_POLLING;
            break;
            
        default:
            recovery_ctx.strategy = LLE_RECOVERY_FULL_RESET;
            break;
    }
    
    // Execute recovery strategy
    result = lle_execute_recovery_strategy(&recovery_ctx);
    
    // Log recovery attempt
    lle_error_context_log_recovery(terminal->error_ctx, error_code, 
                                   error_context, recovery_ctx.strategy, result);
    
    return result;
}
```

---

## 9. State Machine Definitions

### 9.1 Input Processing State Machine

```c
typedef enum {
    LLE_INPUT_STATE_NORMAL = 0,       // Normal character input
    LLE_INPUT_STATE_ESCAPE,           // Escape sequence start
    LLE_INPUT_STATE_CSI,              // Control Sequence Introducer
    LLE_INPUT_STATE_OSC,              // Operating System Command
    LLE_INPUT_STATE_DCS,              // Device Control String
    LLE_INPUT_STATE_MOUSE,            // Mouse sequence processing
    LLE_INPUT_STATE_PASTE,            // Bracketed paste mode
    LLE_INPUT_STATE_ERROR,            // Error recovery state
} lle_input_state_t;

typedef struct lle_input_state_machine {
    lle_input_state_t current_state;  // Current state
    lle_input_state_t previous_state; // Previous state
    uint64_t state_enter_time;        // State entry time
    uint32_t transition_count;        // State transitions
    char sequence_buffer[64];         // Sequence accumulation
    size_t sequence_length;           // Current sequence length
    uint32_t timeout_ms;              // State timeout
} lle_input_state_machine_t;

lle_result_t lle_input_state_machine_process(lle_input_state_machine_t *sm,
                                              char input_char,
                                              lle_input_event_t **event) {
    lle_result_t result = LLE_SUCCESS;
    
    // Check for state timeout
    uint64_t current_time = lle_get_current_time_us();
    if ((current_time - sm->state_enter_time) > (sm->timeout_ms * 1000)) {
        // Timeout - reset to normal state
        lle_input_state_machine_reset(sm);
    }
    
    switch (sm->current_state) {
        case LLE_INPUT_STATE_NORMAL:
            result = lle_process_normal_state(sm, input_char, event);
            break;
            
        case LLE_INPUT_STATE_ESCAPE:
            result = lle_process_escape_state(sm, input_char, event);
            break;
            
        case LLE_INPUT_STATE_CSI:
            result = lle_process_csi_state(sm, input_char, event);
            break;
            
        case LLE_INPUT_STATE_OSC:
            result = lle_process_osc_state(sm, input_char, event);
            break;
            
        case LLE_INPUT_STATE_MOUSE:
            result = lle_process_mouse_state(sm, input_char, event);
            break;
            
        case LLE_INPUT_STATE_PASTE:
            result = lle_process_paste_state(sm, input_char, event);
            break;
            
        case LLE_INPUT_STATE_ERROR:
            result = lle_process_error_state(sm, input_char, event);
            break;
            
        default:
            lle_input_state_machine_reset(sm);
            result = LLE_ERROR_INVALID_STATE;
            break;
    }
    
    return result;
}
```

---

## 10. Integration with Lusush Systems

### 10.1 Display System Integration

```c
typedef struct lle_lusush_integration {
    // Display system references
    lle_display_controller_t *display_controller;
    lle_prompt_layer_t *prompt_layer;
    lle_composition_engine_t *composition_engine;
    
    // Memory system integration
    lusush_memory_pool_t *memory_pool;
    
    // Performance monitoring integration
    lle_performance_monitor_t *perf_monitor;
    
    // Theme system integration
    lusush_theme_t *current_theme;
    
} lle_lusush_integration_t;

lle_result_t lle_integrate_with_lusush_display(lle_terminal_system_t *terminal_sys,
                                               lle_lusush_integration_t *integration) {
    // Register terminal capabilities with display system
    lle_result_t result = lle_display_register_capabilities(
        integration->display_controller,
        terminal_sys->caps
    );
    
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Configure display layers based on terminal capabilities
    if (terminal_sys->caps->supports_truecolor) {
        lle_display_controller_enable_truecolor(integration->display_controller);
    } else if (terminal_sys->caps->supports_256_colors) {
        lle_display_controller_enable_256_colors(integration->display_controller);
    }
    
    // Set up performance monitoring integration
    lle_performance_monitor_register_terminal_metrics(
        integration->perf_monitor,
        terminal_sys->perf_monitor
    );
    
    return LLE_SUCCESS;
}
```

---

## 11. Performance Requirements

### 11.1 Performance Specifications

- **Input Response Time**: < 1ms average, < 5ms maximum
- **Capability Detection Time**: < 100ms total initialization
- **Memory Usage**: < 64KB for terminal abstraction layer
- **CPU Usage**: < 1% during idle, < 5% during active input processing
- **Terminal Query Timeout**: 50-200ms per query with exponential backoff

### 11.2 Performance Monitoring Integration

```c
typedef struct lle_terminal_performance_metrics {
    uint64_t input_processing_time_total;
    uint64_t input_processing_time_average;
    uint32_t input_events_processed;
    uint32_t escape_sequences_processed;
    uint32_t capability_queries_sent;
    uint32_t capability_queries_successful;
    uint64_t terminal_state_changes;
    uint32_t error_recovery_attempts;
} lle_terminal_performance_metrics_t;
```

---

## 12. Testing and Validation

### 12.1 Comprehensive Test Framework

```c
typedef struct lle_terminal_test_suite {
    // Basic functionality tests
    bool (*test_terminal_initialization)(void);
    bool (*test_capability_detection)(void);
    bool (*test_raw_mode_operations)(void);
    bool (*test_input_processing)(void);
    
    // Error handling tests
    bool (*test_error_recovery)(void);
    bool (*test_timeout_handling)(void);
    bool (*test_signal_handling)(void);
    
    // Performance tests
    bool (*test_response_times)(void);
    bool (*test_memory_usage)(void);
    bool (*test_cpu_usage)(void);
    
    // Integration tests
    bool (*test_lusush_integration)(void);
    bool (*test_display_system_integration)(void);
    
} lle_terminal_test_suite_t;
```

### 12.2 Validation Requirements

1. **Functional Validation**: All terminal operations must work correctly
2. **Performance Validation**: Must meet specified performance requirements
3. **Error Handling Validation**: Must gracefully handle all error conditions
4. **Integration Validation**: Must integrate seamlessly with Lusush systems
5. **Cross-Terminal Validation**: Must work across different terminal types
6. **Memory Safety Validation**: No memory leaks or buffer overflows
7. **Signal Safety Validation**: Proper signal handling and recovery

---

**Implementation Status**: Complete specification ready for development  
**Next Priority**: Buffer Management Complete Specification (03_buffer_management_complete.md)
