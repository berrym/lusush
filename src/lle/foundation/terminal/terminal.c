// src/lle/foundation/terminal/terminal.c
//
// LLE Terminal Abstraction - Phase 0 Week 1 Implementation
// Based on: docs/lle_specification/02_terminal_abstraction_complete.md
//
// CRITICAL: This implements the research-validated "never query terminal" approach
// Internal state is AUTHORITATIVE - we calculate everything from buffer state

#define _POSIX_C_SOURCE 200809L

#include "terminal.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>

//==============================================================================
// INTERNAL STATE AUTHORITY MODEL (Spec 02 Section 3)
//==============================================================================

lle_result_t lle_internal_state_init(
    lle_internal_state_t **state,
    const lle_terminal_capabilities_t *caps)
{
    if (!state || !caps) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    lle_internal_state_t *s = calloc(1, sizeof(lle_internal_state_t));
    if (!s) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize cursor at origin
    s->cursor_row = 0;
    s->cursor_col = 0;
    
    // Window size will be set by initial TIOCGWINSZ
    s->rows = 24;  // Default fallback
    s->cols = 80;
    
    // Full screen scroll region by default
    s->scroll_top = 0;
    s->scroll_bottom = s->rows - 1;
    
    // Default modes
    s->application_keypad_mode = false;
    s->application_cursor_mode = false;
    s->auto_wrap_mode = true;
    s->origin_mode = false;
    s->bracketed_paste_enabled = false;
    s->mouse_tracking_enabled = false;
    
    // Metadata
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    s->last_update_timestamp_ns = now.tv_sec * 1000000000ULL + now.tv_nsec;
    s->state_version = 1;
    s->window_size_changed_flag = false;
    
    *state = s;
    return LLE_SUCCESS;
}

void lle_internal_state_destroy(lle_internal_state_t *state) {
    free(state);
}

//==============================================================================
// UNIX TERMINAL INTERFACE (Spec 02 Section 8)
//==============================================================================

static lle_result_t unix_enter_raw_mode(lle_unix_interface_t *iface) {
    if (!iface || !iface->initialized) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (iface->raw_mode_active) {
        return LLE_SUCCESS;  // Already in raw mode
    }
    
    // Apply raw mode settings
    if (tcsetattr(iface->input_fd, TCSAFLUSH, &iface->raw_termios) < 0) {
        return LLE_ERROR_TERMINAL_SETTINGS;
    }
    
    iface->raw_mode_active = true;
    return LLE_SUCCESS;
}

static lle_result_t unix_exit_raw_mode(lle_unix_interface_t *iface) {
    if (!iface || !iface->initialized) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!iface->raw_mode_active) {
        return LLE_SUCCESS;  // Already not in raw mode
    }
    
    // Restore original settings
    if (tcsetattr(iface->input_fd, TCSAFLUSH, &iface->original_termios) < 0) {
        return LLE_ERROR_TERMINAL_SETTINGS;
    }
    
    iface->raw_mode_active = false;
    return LLE_SUCCESS;
}

static lle_result_t unix_get_window_size(lle_unix_interface_t *iface,
                                         uint16_t *rows, uint16_t *cols) {
    if (!iface || !rows || !cols) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    struct winsize ws;
    if (ioctl(iface->output_fd, TIOCGWINSZ, &ws) < 0) {
        return LLE_ERROR_TERMINAL_SIZE;
    }
    
    *rows = ws.ws_row > 0 ? ws.ws_row : 24;
    *cols = ws.ws_col > 0 ? ws.ws_col : 80;
    
    return LLE_SUCCESS;
}

lle_result_t lle_unix_interface_init(
    lle_unix_interface_t **iface,
    int input_fd,
    int output_fd)
{
    if (!iface) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    // Check if input_fd is a TTY
    if (!isatty(input_fd)) {
        return LLE_ERROR_TERMINAL_NOT_TTY;
    }
    
    lle_unix_interface_t *ui = calloc(1, sizeof(lle_unix_interface_t));
    if (!ui) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    ui->input_fd = input_fd;
    ui->output_fd = output_fd;
    
    // Save original terminal settings
    if (tcgetattr(input_fd, &ui->original_termios) < 0) {
        free(ui);
        return LLE_ERROR_TERMINAL_SETTINGS;
    }
    
    // Prepare raw mode settings (but don't apply yet)
    ui->raw_termios = ui->original_termios;
    ui->raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    ui->raw_termios.c_oflag &= ~(OPOST);
    ui->raw_termios.c_cflag |= (CS8);
    ui->raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    ui->raw_termios.c_cc[VMIN] = 0;   // Non-blocking reads
    ui->raw_termios.c_cc[VTIME] = 0;
    
    ui->raw_mode_active = false;
    ui->initialized = true;
    
    // Set function pointers
    ui->enter_raw_mode = unix_enter_raw_mode;
    ui->exit_raw_mode = unix_exit_raw_mode;
    ui->get_window_size = unix_get_window_size;
    
    *iface = ui;
    return LLE_SUCCESS;
}

void lle_unix_interface_destroy(lle_unix_interface_t *iface) {
    if (!iface) return;
    
    // Restore original terminal settings if in raw mode
    if (iface->raw_mode_active && iface->initialized) {
        tcsetattr(iface->input_fd, TCSAFLUSH, &iface->original_termios);
    }
    
    free(iface);
}

//==============================================================================
// TERMINAL CAPABILITY DETECTION (Spec 02 Section 4)
//==============================================================================

lle_term_type_t detect_term_type_from_env(char *term_env, size_t size) {
    const char *term = getenv("TERM");
    if (!term) {
        term_env[0] = '\0';
        return LLE_TERM_TYPE_UNKNOWN;
    }
    
    strncpy(term_env, term, size - 1);
    term_env[size - 1] = '\0';
    
    // Match known terminal types
    if (strstr(term, "alacritty")) return LLE_TERM_TYPE_ALACRITTY;
    if (strstr(term, "kitty")) return LLE_TERM_TYPE_KITTY;
    if (strstr(term, "konsole")) return LLE_TERM_TYPE_KONSOLE;
    if (strstr(term, "gnome")) return LLE_TERM_TYPE_GNOME_TERMINAL;
    if (strstr(term, "xterm-256")) return LLE_TERM_TYPE_XTERM_256COLOR;
    if (strstr(term, "xterm")) return LLE_TERM_TYPE_XTERM;
    if (strstr(term, "rxvt")) return LLE_TERM_TYPE_RXVT;
    if (strstr(term, "screen")) return LLE_TERM_TYPE_SCREEN;
    if (strstr(term, "tmux")) return LLE_TERM_TYPE_TMUX;
    if (strstr(term, "vt100")) return LLE_TERM_TYPE_VT100;
    
    return LLE_TERM_TYPE_UNKNOWN;
}

void detect_color_capabilities(lle_terminal_capabilities_t *caps) {
    const char *colorterm = getenv("COLORTERM");
    
    // Check for 24-bit color
    if (colorterm) {
        if (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0) {
            caps->has_true_color = true;
            caps->has_256_color = true;
            caps->has_color = true;
            strncpy(caps->colorterm_env, colorterm, sizeof(caps->colorterm_env) - 1);
            return;
        }
    }
    
    // Modern terminals support truecolor
    if (caps->terminal_type == LLE_TERM_TYPE_ALACRITTY ||
        caps->terminal_type == LLE_TERM_TYPE_KITTY ||
        caps->terminal_type == LLE_TERM_TYPE_KONSOLE) {
        caps->has_true_color = true;
        caps->has_256_color = true;
        caps->has_color = true;
        return;
    }
    
    // 256 color support
    if (caps->terminal_type == LLE_TERM_TYPE_XTERM_256COLOR ||
        caps->terminal_type == LLE_TERM_TYPE_GNOME_TERMINAL) {
        caps->has_256_color = true;
        caps->has_color = true;
        return;
    }
    
    // Basic color support
    if (caps->terminal_type != LLE_TERM_TYPE_VT100 &&
        caps->terminal_type != LLE_TERM_TYPE_UNKNOWN) {
        caps->has_color = true;
    }
}

// Check for known enhanced terminal signatures (Spec 26 Section 2.1)
static bool is_enhanced_terminal_environment(void) {
    const char *term_program = getenv("TERM_PROGRAM");
    
    // Modern editor terminals that support full capabilities even without TTY
    if (term_program) {
        if (strstr(term_program, "zed") ||
            strstr(term_program, "vscode") ||
            strstr(term_program, "cursor") ||
            strstr(term_program, "iTerm") ||
            strstr(term_program, "Hyper")) {
            return true;
        }
    }
    
    // AI assistant environments
    if (getenv("AI_ENVIRONMENT") || getenv("ANTHROPIC_API_KEY")) {
        return true;
    }
    
    return false;
}

lle_result_t lle_capabilities_detect_environment(
    lle_terminal_capabilities_t **caps,
    const lle_unix_interface_t *unix_iface)
{
    if (!caps || !unix_iface) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    lle_terminal_capabilities_t *c = calloc(1, sizeof(lle_terminal_capabilities_t));
    if (!c) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // ADAPTIVE DETECTION (Spec 26): Check TTY status but don't limit capabilities
    bool is_tty = isatty(STDIN_FILENO);
    bool stdout_is_tty = isatty(STDOUT_FILENO);
    bool is_enhanced_env = is_enhanced_terminal_environment();
    
    // Detect terminal type from environment
    c->terminal_type = detect_term_type_from_env(c->term_env, sizeof(c->term_env));
    
    // Detect color support (works in both TTY and non-TTY if env vars present)
    detect_color_capabilities(c);
    
    // Detect Unicode from locale (works everywhere)
    const char *lang = getenv("LANG");
    const char *lc_all = getenv("LC_ALL");
    const char *locale = lc_all ? lc_all : lang;
    c->has_unicode = (locale && strstr(locale, "UTF-8"));
    
    // ADAPTIVE: Enhanced environments get full capabilities even without TTY
    if (is_enhanced_env) {
        // Modern editor terminals support full capabilities
        c->has_bold = true;
        c->has_underline = true;
        c->has_italic = true;
        c->has_strikethrough = true;
        c->has_dim = true;
        c->has_mouse = true;
        c->has_bracketed_paste = true;
        c->has_focus_events = true;
        c->has_alternate_screen = true;
    } else if (is_tty || stdout_is_tty) {
        // TTY environments get capabilities based on terminal type
        c->has_bold = true;
        c->has_underline = true;
        c->has_italic = (c->terminal_type != LLE_TERM_TYPE_VT100);
        c->has_strikethrough = (c->terminal_type == LLE_TERM_TYPE_ALACRITTY ||
                                c->terminal_type == LLE_TERM_TYPE_KITTY);
        c->has_dim = true;
        c->has_mouse = (c->terminal_type != LLE_TERM_TYPE_VT100);
        c->has_bracketed_paste = (c->terminal_type != LLE_TERM_TYPE_VT100);
        c->has_focus_events = (c->terminal_type == LLE_TERM_TYPE_ALACRITTY ||
                               c->terminal_type == LLE_TERM_TYPE_KITTY ||
                               c->terminal_type == LLE_TERM_TYPE_XTERM ||
                               c->terminal_type == LLE_TERM_TYPE_XTERM_256COLOR);
        c->has_alternate_screen = (c->terminal_type != LLE_TERM_TYPE_VT100);
    } else {
        // Pure non-TTY: Detect from environment what we can support
        // Still provide color/unicode if env vars indicate support
        c->has_bold = c->has_color;          // If we have color, we have bold
        c->has_underline = c->has_color;
        c->has_italic = c->has_color;
        c->has_strikethrough = false;         // Conservative
        c->has_dim = c->has_color;
        c->has_mouse = false;                 // No mouse in pure non-TTY
        c->has_bracketed_paste = false;
        c->has_focus_events = false;
        c->has_alternate_screen = false;
    }
    
    // Calculate detection time
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    c->detection_time_ms = (uint32_t)((end.tv_sec - start.tv_sec) * 1000 +
                                      (end.tv_nsec - start.tv_nsec) / 1000000);
    
    c->detection_successful = true;
    c->detection_complete = true;
    
    *caps = c;
    return LLE_SUCCESS;
}

void lle_capabilities_destroy(lle_terminal_capabilities_t *caps) {
    free(caps);
}

//==============================================================================
// PERFORMANCE MONITORING (Spec 02 Section 10)
//==============================================================================

static void perf_start_operation(lle_performance_monitor_t *mon) {
    // Will be implemented with high-resolution timing
    (void)mon;
}

static void perf_end_operation(lle_performance_monitor_t *mon) {
    // Will be implemented with metrics tracking
    (void)mon;
}

static double perf_get_avg_latency_us(const lle_performance_monitor_t *mon) {
    if (!mon || mon->metrics.total_operations == 0) {
        return 0.0;
    }
    return (double)mon->metrics.total_time_ns / (double)mon->metrics.total_operations / 1000.0;
}

lle_result_t lle_performance_monitor_init(
    lle_performance_monitor_t **monitor,
    const char *component_name)
{
    if (!monitor || !component_name) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    lle_performance_monitor_t *pm = calloc(1, sizeof(lle_performance_monitor_t));
    if (!pm) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    pm->component_name = component_name;
    pm->target_max_latency_ns = 500000;  // 500μs from spec
    
    pm->metrics.total_operations = 0;
    pm->metrics.total_time_ns = 0;
    pm->metrics.min_time_ns = UINT64_MAX;
    pm->metrics.max_time_ns = 0;
    
    pm->start_operation = perf_start_operation;
    pm->end_operation = perf_end_operation;
    pm->get_avg_latency_us = perf_get_avg_latency_us;
    
    *monitor = pm;
    return LLE_SUCCESS;
}

void lle_performance_monitor_destroy(lle_performance_monitor_t *monitor) {
    free(monitor);
}

//==============================================================================
// MAIN TERMINAL ABSTRACTION INITIALIZATION (Spec 02 Section 2.2)
//==============================================================================

lle_result_t lle_terminal_abstraction_init(
    lle_terminal_abstraction_t **abstraction,
    lusush_display_context_t *lusush_display,
    int input_fd,
    int output_fd)
{
    if (!abstraction) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    lle_result_t result;
    lle_terminal_abstraction_t *term = calloc(1, sizeof(lle_terminal_abstraction_t));
    if (!term) {
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Step 1: Initialize Unix interface
    result = lle_unix_interface_init(&term->unix_interface, input_fd, output_fd);
    if (result != LLE_SUCCESS) {
        free(term);
        return result;
    }
    
    // Step 2: One-time capability detection (environment-based, no terminal queries)
    result = lle_capabilities_detect_environment(&term->capabilities, term->unix_interface);
    if (result != LLE_SUCCESS) {
        lle_unix_interface_destroy(term->unix_interface);
        free(term);
        return result;
    }
    
    // Step 3: Initialize internal state (the authoritative model)
    result = lle_internal_state_init(&term->internal_state, term->capabilities);
    if (result != LLE_SUCCESS) {
        lle_capabilities_destroy(term->capabilities);
        lle_unix_interface_destroy(term->unix_interface);
        free(term);
        return result;
    }
    
    // Step 4: Get initial window size (ONLY acceptable terminal query)
    uint16_t rows, cols;
    result = term->unix_interface->get_window_size(term->unix_interface, &rows, &cols);
    if (result == LLE_SUCCESS) {
        term->internal_state->rows = rows;
        term->internal_state->cols = cols;
        term->internal_state->scroll_bottom = rows - 1;
    }
    
    // Step 5: Initialize performance monitoring
    result = lle_performance_monitor_init(&term->perf_monitor, "terminal_abstraction");
    if (result != LLE_SUCCESS) {
        lle_internal_state_destroy(term->internal_state);
        lle_capabilities_destroy(term->capabilities);
        lle_unix_interface_destroy(term->unix_interface);
        free(term);
        return result;
    }
    
    // TODO: Initialize remaining subsystems (display_generator, display_client, input_processor)
    // These will be implemented as we progress through Phase 0
    term->display_generator = NULL;  // Week 2
    term->display_client = NULL;     // Week 2  
    term->input_processor = NULL;    // Week 3
    term->error_ctx = NULL;          // Week 4
    
    term->initialized = true;
    term->initialization_flags = 0;
    
    *abstraction = term;
    return LLE_SUCCESS;
}

//==============================================================================
// PUBLIC API IMPLEMENTATION
//==============================================================================

lle_result_t lle_terminal_update_cursor(
    lle_terminal_abstraction_t *term,
    uint16_t row,
    uint16_t col)
{
    if (!term || !term->internal_state) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    // CRITICAL: This NEVER queries terminal - just updates internal state
    term->internal_state->cursor_row = row;
    term->internal_state->cursor_col = col;
    term->internal_state->state_version++;
    
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    term->internal_state->last_update_timestamp_ns = 
        now.tv_sec * 1000000000ULL + now.tv_nsec;
    
    return LLE_SUCCESS;
}

lle_result_t lle_terminal_update_window_size(
    lle_terminal_abstraction_t *term)
{
    if (!term || !term->unix_interface || !term->internal_state) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    // This is the ONE exception - we query terminal for window size
    // Called ONLY from SIGWINCH signal handler
    uint16_t rows, cols;
    lle_result_t result = term->unix_interface->get_window_size(
        term->unix_interface, &rows, &cols);
    
    if (result == LLE_SUCCESS) {
        term->internal_state->rows = rows;
        term->internal_state->cols = cols;
        term->internal_state->scroll_bottom = rows - 1;
        term->internal_state->window_size_changed_flag = true;
        term->internal_state->state_version++;
    }
    
    return result;
}

const lle_internal_state_t* lle_terminal_get_state(
    const lle_terminal_abstraction_t *term)
{
    return term ? term->internal_state : NULL;
}

const lle_terminal_capabilities_t* lle_terminal_get_capabilities(
    const lle_terminal_abstraction_t *term)
{
    return term ? term->capabilities : NULL;
}

void lle_terminal_abstraction_cleanup(lle_terminal_abstraction_t *term) {
    if (!term) return;
    
    lle_performance_monitor_destroy(term->perf_monitor);
    lle_internal_state_destroy(term->internal_state);
    lle_capabilities_destroy(term->capabilities);
    lle_unix_interface_destroy(term->unix_interface);
    
    free(term);
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

const char* lle_result_to_string(lle_result_t result) {
    switch (result) {
        case LLE_SUCCESS: return "Success";
        case LLE_ERROR_TERMINAL_INIT: return "Terminal initialization failed";
        case LLE_ERROR_TERMINAL_NOT_TTY: return "Not a TTY";
        case LLE_ERROR_TERMINAL_SETTINGS: return "Terminal settings error";
        case LLE_ERROR_TERMINAL_SIZE: return "Failed to get terminal size";
        case LLE_ERROR_CAPABILITY_DETECTION: return "Capability detection failed";
        case LLE_ERROR_DISPLAY_CLIENT: return "Display client error";
        case LLE_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case LLE_ERROR_NULL_POINTER: return "Null pointer";
        case LLE_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        default: return "Unknown error";
    }
}
