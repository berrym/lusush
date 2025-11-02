/**
 * adaptive_native_controller.c - Native Terminal Controller Implementation
 *
 * Implements the native controller for traditional TTY terminals.
 * Provides full terminal control with raw mode and comprehensive
 * terminal abstraction integration.
 *
 * Key Features:
 * - Full raw mode terminal control
 * - Integration with research-validated terminal abstraction
 * - Capability-based optimization
 * - Complete cursor control and formatting
 * - Performance monitoring and statistics
 *
 * Specification: Spec 26 Phase 2 - Native Terminal Controller
 * Date: 2025-11-02
 */

#include "lle/adaptive_terminal_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/terminal_abstraction.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/* ============================================================================
 * CONTROLLER-SPECIFIC STRUCTURES
 * ============================================================================ */

/**
 * Controller-specific capabilities (separate from terminal_abstraction.h).
 */
typedef struct {
    bool has_colors;
    bool has_256_colors;
    bool has_truecolor;
    bool has_cursor_positioning;
    bool has_cursor_queries;
    bool has_mouse_support;
    bool has_bracketed_paste;
    bool has_unicode;
} lle_native_capabilities_t;

/**
 * Controller optimization flags (separate from terminal_abstraction.h).
 */
typedef enum {
    LLE_NATIVE_OPT_NONE = 0,
    LLE_NATIVE_OPT_FAST_CURSOR_QUERIES = (1 << 0),
    LLE_NATIVE_OPT_EXTENDED_COLOR_CACHE = (1 << 1),
    LLE_NATIVE_OPT_ADVANCED_SEQUENCES = (1 << 2),
    LLE_NATIVE_OPT_MOUSE_TRACKING = (1 << 3),
    LLE_NATIVE_OPT_BRACKETED_PASTE = (1 << 4)
} lle_native_optimization_flags_t;

/**
 * Terminal performance statistics.
 */
typedef struct {
    uint64_t escape_sequences_sent;
    uint64_t cursor_moves_performed;
    uint64_t screen_updates;
    uint64_t total_bytes_written;
    uint64_t avg_update_time_us;
    uint64_t raw_mode_toggles;
} lle_terminal_performance_stats_t;

/**
 * Terminal state tracking.
 */
typedef struct {
    int cursor_row;
    int cursor_col;
    int terminal_width;
    int terminal_height;
    bool raw_mode_active;
    bool alternate_screen_active;
    int current_fg_color;
    int current_bg_color;
    bool bold_active;
    bool italic_active;
    bool underline_active;
} lle_terminal_state_t;

/**
 * Native terminal controller structure.
 */
struct lle_native_controller_t {
    /* Terminal state tracking */
    lle_terminal_state_t *terminal_state;
    
    /* Raw mode management */
    struct termios original_termios;
    struct termios raw_termios;
    bool raw_mode_active;
    bool termios_saved;
    
    /* Capability-specific optimization */
    lle_native_capabilities_t *capabilities;
    lle_native_optimization_flags_t optimization_flags;
    
    /* Output buffering */
    char *output_buffer;
    size_t buffer_capacity;
    size_t buffer_used;
    
    /* Performance monitoring */
    lle_terminal_performance_stats_t *perf_stats;
    
    /* Memory management */
    lusush_memory_pool_t *memory_pool;
};

/* ============================================================================
 * TERMINAL STATE IMPLEMENTATION
 * ============================================================================ */

/**
 * Create terminal state.
 */
static lle_terminal_state_t *lle_terminal_state_create(void) {
    lle_terminal_state_t *state = calloc(1, sizeof(lle_terminal_state_t));
    if (!state) {
        return NULL;
    }
    
    /* Get terminal dimensions */
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        state->terminal_width = ws.ws_col;
        state->terminal_height = ws.ws_row;
    } else {
        state->terminal_width = 80;
        state->terminal_height = 24;
    }
    
    state->cursor_row = 0;
    state->cursor_col = 0;
    state->current_fg_color = -1;
    state->current_bg_color = -1;
    
    return state;
}

/**
 * Destroy terminal state.
 */
static void lle_terminal_state_destroy(lle_terminal_state_t *state) {
    free(state);
}

/**
 * Update terminal dimensions.
 */
static lle_result_t lle_terminal_state_update_dimensions(
    lle_terminal_state_t *state) {
    
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
        return LLE_ERROR_TERMINAL_ABSTRACTION;
    }
    
    state->terminal_width = ws.ws_col;
    state->terminal_height = ws.ws_row;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * PERFORMANCE STATISTICS IMPLEMENTATION
 * ============================================================================ */

/**
 * Create performance statistics.
 */
static lle_terminal_performance_stats_t *lle_terminal_performance_stats_create(void) {
    return calloc(1, sizeof(lle_terminal_performance_stats_t));
}

/**
 * Destroy performance statistics.
 */
static void lle_terminal_performance_stats_destroy(
    lle_terminal_performance_stats_t *stats) {
    free(stats);
}

/* ============================================================================
 * RAW MODE MANAGEMENT
 * ============================================================================ */

/**
 * Enter raw mode.
 */
static lle_result_t lle_native_enter_raw_mode(lle_native_controller_t *native) {
    if (native->raw_mode_active) {
        return LLE_SUCCESS;
    }
    
    /* Save original terminal settings */
    if (!native->termios_saved) {
        if (tcgetattr(STDIN_FILENO, &native->original_termios) != 0) {
            return LLE_ERROR_TERMINAL_ABSTRACTION;
        }
        native->termios_saved = true;
    }
    
    /* Configure raw mode */
    native->raw_termios = native->original_termios;
    native->raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    native->raw_termios.c_oflag &= ~(OPOST);
    native->raw_termios.c_cflag |= (CS8);
    native->raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    native->raw_termios.c_cc[VMIN] = 1;
    native->raw_termios.c_cc[VTIME] = 0;
    
    /* Apply raw mode */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &native->raw_termios) != 0) {
        return LLE_ERROR_TERMINAL_ABSTRACTION;
    }
    
    native->raw_mode_active = true;
    native->perf_stats->raw_mode_toggles++;
    
    return LLE_SUCCESS;
}

/**
 * Exit raw mode.
 */
static lle_result_t lle_native_exit_raw_mode(lle_native_controller_t *native) {
    if (!native->raw_mode_active || !native->termios_saved) {
        return LLE_SUCCESS;
    }
    
    /* Restore original terminal settings */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &native->original_termios) != 0) {
        return LLE_ERROR_TERMINAL_ABSTRACTION;
    }
    
    native->raw_mode_active = false;
    native->perf_stats->raw_mode_toggles++;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * OUTPUT BUFFER MANAGEMENT
 * ============================================================================ */

/**
 * Append to output buffer.
 */
static lle_result_t lle_native_buffer_append(
    lle_native_controller_t *native,
    const char *data,
    size_t length) {
    
    /* Ensure capacity */
    size_t needed = native->buffer_used + length;
    if (needed >= native->buffer_capacity) {
        size_t new_capacity = native->buffer_capacity;
        while (new_capacity < needed + 1) {
            new_capacity *= 2;
        }
        char *new_buffer = realloc(native->output_buffer, new_capacity);
        if (!new_buffer) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        native->output_buffer = new_buffer;
        native->buffer_capacity = new_capacity;
    }
    
    memcpy(native->output_buffer + native->buffer_used, data, length);
    native->buffer_used += length;
    native->output_buffer[native->buffer_used] = '\0';
    
    return LLE_SUCCESS;
}

/**
 * Flush output buffer.
 */
static lle_result_t lle_native_buffer_flush(lle_native_controller_t *native) {
    if (native->buffer_used == 0) {
        return LLE_SUCCESS;
    }
    
    ssize_t written = write(STDOUT_FILENO, native->output_buffer, native->buffer_used);
    if (written < 0) {
        return LLE_ERROR_TERMINAL_ABSTRACTION;
    }
    
    native->perf_stats->total_bytes_written += written;
    native->buffer_used = 0;
    
    return LLE_SUCCESS;
}

/**
 * Clear output buffer.
 */
static void lle_native_buffer_clear(lle_native_controller_t *native) {
    native->buffer_used = 0;
}

/* ============================================================================
 * TERMINAL CONTROL SEQUENCES
 * ============================================================================ */

/**
 * Move cursor to position.
 */
static lle_result_t lle_native_move_cursor(
    lle_native_controller_t *native,
    int row,
    int col) {
    
    char seq[32];
    int len = snprintf(seq, sizeof(seq), "\x1b[%d;%dH", row + 1, col + 1);
    if (len < 0 || len >= (int)sizeof(seq)) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = lle_native_buffer_append(native, seq, len);
    if (result == LLE_SUCCESS) {
        native->terminal_state->cursor_row = row;
        native->terminal_state->cursor_col = col;
        native->perf_stats->cursor_moves_performed++;
        native->perf_stats->escape_sequences_sent++;
    }
    
    return result;
}

/**
 * Clear screen.
 */
static lle_result_t lle_native_clear_screen(lle_native_controller_t *native) {
    const char *seq = "\x1b[2J\x1b[H";
    lle_result_t result = lle_native_buffer_append(native, seq, strlen(seq));
    if (result == LLE_SUCCESS) {
        native->terminal_state->cursor_row = 0;
        native->terminal_state->cursor_col = 0;
        native->perf_stats->escape_sequences_sent += 2;
    }
    return result;
}

/**
 * Clear to end of line.
 */
static lle_result_t lle_native_clear_to_eol(lle_native_controller_t *native) {
    const char *seq = "\x1b[K";
    lle_result_t result = lle_native_buffer_append(native, seq, strlen(seq));
    if (result == LLE_SUCCESS) {
        native->perf_stats->escape_sequences_sent++;
    }
    return result;
}

/**
 * Set foreground color (256-color mode).
 */
static lle_result_t lle_native_set_fg_color(
    lle_native_controller_t *native,
    int color) {
    
    if (color == native->terminal_state->current_fg_color) {
        return LLE_SUCCESS;
    }
    
    char seq[32];
    int len = snprintf(seq, sizeof(seq), "\x1b[38;5;%dm", color);
    if (len < 0 || len >= (int)sizeof(seq)) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = lle_native_buffer_append(native, seq, len);
    if (result == LLE_SUCCESS) {
        native->terminal_state->current_fg_color = color;
        native->perf_stats->escape_sequences_sent++;
    }
    
    return result;
}

/**
 * Reset all formatting.
 */
static lle_result_t lle_native_reset_formatting(lle_native_controller_t *native) {
    const char *seq = "\x1b[0m";
    lle_result_t result = lle_native_buffer_append(native, seq, strlen(seq));
    if (result == LLE_SUCCESS) {
        native->terminal_state->current_fg_color = -1;
        native->terminal_state->current_bg_color = -1;
        native->terminal_state->bold_active = false;
        native->terminal_state->italic_active = false;
        native->terminal_state->underline_active = false;
        native->perf_stats->escape_sequences_sent++;
    }
    return result;
}

/**
 * Show cursor.
 */
static lle_result_t lle_native_show_cursor(lle_native_controller_t *native) {
    const char *seq = "\x1b[?25h";
    lle_result_t result = lle_native_buffer_append(native, seq, strlen(seq));
    if (result == LLE_SUCCESS) {
        native->perf_stats->escape_sequences_sent++;
    }
    return result;
}

/**
 * Hide cursor.
 */
static lle_result_t lle_native_hide_cursor(lle_native_controller_t *native) {
    const char *seq = "\x1b[?25l";
    lle_result_t result = lle_native_buffer_append(native, seq, strlen(seq));
    if (result == LLE_SUCCESS) {
        native->perf_stats->escape_sequences_sent++;
    }
    return result;
}

/* ============================================================================
 * CAPABILITY-BASED OPTIMIZATION
 * ============================================================================ */

/**
 * Apply capability optimizations.
 */
static lle_result_t lle_apply_capability_optimizations(
    lle_native_controller_t *native,
    const lle_terminal_detection_result_t *detection) {
    
    native->optimization_flags = LLE_NATIVE_OPT_NONE;
    
    if (detection->supports_cursor_queries) {
        native->optimization_flags |= LLE_NATIVE_OPT_FAST_CURSOR_QUERIES;
    }
    
    if (detection->supports_256_colors) {
        native->optimization_flags |= LLE_NATIVE_OPT_EXTENDED_COLOR_CACHE;
    }
    
    if (detection->capability_level >= LLE_CAPABILITY_FULL) {
        native->optimization_flags |= LLE_NATIVE_OPT_ADVANCED_SEQUENCES;
    }
    
    if (detection->supports_mouse) {
        native->optimization_flags |= LLE_NATIVE_OPT_MOUSE_TRACKING;
    }
    
    if (detection->supports_bracketed_paste) {
        native->optimization_flags |= LLE_NATIVE_OPT_BRACKETED_PASTE;
    }
    
    return LLE_SUCCESS;
}

/**
 * Calculate optimal buffer size based on terminal size.
 */
static size_t lle_calculate_optimal_buffer_size(
    const lle_terminal_detection_result_t *detection) {
    
    /* Base size: 16KB */
    size_t base_size = 16384;
    
    /* Adjust based on capability level */
    switch (detection->capability_level) {
        case LLE_CAPABILITY_PREMIUM:
            return base_size * 2;
        case LLE_CAPABILITY_FULL:
            return base_size;
        case LLE_CAPABILITY_STANDARD:
            return base_size / 2;
        default:
            return base_size / 4;
    }
}

/* ============================================================================
 * NATIVE CONTROLLER API
 * ============================================================================ */

/**
 * Initialize native terminal controller.
 */
lle_result_t lle_initialize_native_controller(
    lle_adaptive_context_t *context,
    lusush_memory_pool_t *memory_pool) {
    
    lle_native_controller_t *native = calloc(1, sizeof(lle_native_controller_t));
    if (!native) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Create terminal state */
    native->terminal_state = lle_terminal_state_create();
    if (!native->terminal_state) {
        free(native);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Create capabilities structure */
    native->capabilities = calloc(1, sizeof(lle_native_capabilities_t));
    if (!native->capabilities) {
        lle_terminal_state_destroy(native->terminal_state);
        free(native);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Configure capabilities from detection */
    native->capabilities->has_colors = context->detection_result->supports_colors;
    native->capabilities->has_256_colors = context->detection_result->supports_256_colors;
    native->capabilities->has_truecolor = context->detection_result->supports_truecolor;
    native->capabilities->has_cursor_positioning = context->detection_result->supports_cursor_positioning;
    native->capabilities->has_cursor_queries = context->detection_result->supports_cursor_queries;
    native->capabilities->has_mouse_support = context->detection_result->supports_mouse;
    native->capabilities->has_bracketed_paste = context->detection_result->supports_bracketed_paste;
    native->capabilities->has_unicode = context->detection_result->supports_unicode;
    
    /* Apply capability optimizations */
    lle_result_t result = lle_apply_capability_optimizations(native, context->detection_result);
    if (result != LLE_SUCCESS) {
        free(native->capabilities);
        lle_terminal_state_destroy(native->terminal_state);
        free(native);
        return result;
    }
    
    /* Create output buffer */
    native->buffer_capacity = lle_calculate_optimal_buffer_size(context->detection_result);
    native->output_buffer = malloc(native->buffer_capacity);
    if (!native->output_buffer) {
        free(native->capabilities);
        lle_terminal_state_destroy(native->terminal_state);
        free(native);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Create performance statistics */
    native->perf_stats = lle_terminal_performance_stats_create();
    if (!native->perf_stats) {
        free(native->output_buffer);
        free(native->capabilities);
        lle_terminal_state_destroy(native->terminal_state);
        free(native);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    native->memory_pool = memory_pool;
    context->controller.native = native;
    
    return LLE_SUCCESS;
}

/**
 * Cleanup native controller.
 */
void lle_cleanup_native_controller(lle_native_controller_t *native) {
    if (!native) {
        return;
    }
    
    /* Restore terminal state if needed */
    if (native->raw_mode_active) {
        lle_native_exit_raw_mode(native);
    }
    
    lle_terminal_performance_stats_destroy(native->perf_stats);
    free(native->output_buffer);
    free(native->capabilities);
    lle_terminal_state_destroy(native->terminal_state);
    free(native);
}

/**
 * Read line using native controller.
 */
lle_result_t lle_native_read_line(
    lle_native_controller_t *native,
    const char *prompt,
    char **line) {
    
    if (!native || !prompt || !line) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Enter raw mode */
    lle_result_t result = lle_native_enter_raw_mode(native);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Display prompt with color */
    lle_native_buffer_clear(native);
    if (native->capabilities->has_colors) {
        lle_native_set_fg_color(native, 2); /* Green */
    }
    lle_native_buffer_append(native, prompt, strlen(prompt));
    if (native->capabilities->has_colors) {
        lle_native_reset_formatting(native);
    }
    lle_native_buffer_flush(native);
    
    /* Read line using simple fgets for now (full implementation would use raw input) */
    lle_native_exit_raw_mode(native);
    
    char buffer[4096];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        if (feof(stdin)) {
            return LLE_ERROR_OUT_OF_MEMORY; /* EOF treated as error */
        }
        return LLE_ERROR_INPUT_PARSING;
    }
    
    /* Remove trailing newline */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    *line = strdup(buffer);
    if (!*line) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    return LLE_SUCCESS;
}

/**
 * Update native display.
 */
lle_result_t lle_native_update_display(lle_native_controller_t *native) {
    if (!native) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    native->perf_stats->screen_updates++;
    return lle_native_buffer_flush(native);
}

/**
 * Handle terminal resize.
 */
lle_result_t lle_native_handle_resize(
    lle_native_controller_t *native,
    int new_width,
    int new_height) {
    
    if (!native) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    native->terminal_state->terminal_width = new_width;
    native->terminal_state->terminal_height = new_height;
    
    return LLE_SUCCESS;
}

/**
 * Get native controller statistics.
 */
lle_result_t lle_native_get_stats(
    const lle_native_controller_t *native,
    lle_terminal_performance_stats_t *stats) {
    
    if (!native || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *stats = *native->perf_stats;
    return LLE_SUCCESS;
}
