/**
 * termcap.c - Enhanced Portable Terminal Capability Implementation for LUSUSH
 *
 * This module provides comprehensive terminal handling using standard POSIX
 * terminal capabilities enhanced with modern terminal features. Works across
 * Linux, macOS, and BSD systems without requiring external dependencies.
 *
 * Enhanced Features:
 * - Advanced color support (16, 256, truecolor)
 * - Comprehensive cursor and screen management
 * - Terminal capability detection and optimization
 * - Mouse support and bracketed paste mode
 * - Built-in terminal database for common terminals
 * - Performance-optimized escape sequence handling
 */

#include "../include/termcap.h"
#include "../include/termcap_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

/* ======================= Global State ======================= */

static terminal_info_t g_terminal_info = {0};
static termcap_internal_state_t g_termcap_state = {0};
static struct termios g_orig_termios;
static bool g_termios_saved = false;

/* ======================= Pre-compiled Escape Sequences ======================= */

const escape_sequence_t ESC_RESET_ALL = {"\x1b[0m", 4, true};
const escape_sequence_t ESC_CLEAR_SCREEN = {"\x1b[2J\x1b[H", 7, true};
const escape_sequence_t ESC_CLEAR_LINE = {"\x1b[2K", 4, true};
const escape_sequence_t ESC_CLEAR_TO_EOL = {"\x1b[K", 3, true};
const escape_sequence_t ESC_CLEAR_TO_BOL = {"\x1b[1K", 4, true};
const escape_sequence_t ESC_CLEAR_TO_EOS = {"\x1b[J", 3, true};
const escape_sequence_t ESC_CURSOR_HOME = {"\x1b[H", 3, true};
const escape_sequence_t ESC_HIDE_CURSOR = {"\x1b[?25l", 6, true};
const escape_sequence_t ESC_SHOW_CURSOR = {"\x1b[?25h", 6, true};
const escape_sequence_t ESC_SAVE_CURSOR = {"\x1b\x37", 2, true};
const escape_sequence_t ESC_RESTORE_CURSOR = {"\x1b\x38", 2, true};
const escape_sequence_t ESC_CURSOR_QUERY = {"\x1b[6n", 4, true};
const escape_sequence_t ESC_DEVICE_ATTRIBUTES = {"\x1b[c", 3, true};
const escape_sequence_t ESC_ALTERNATE_SCREEN_ENTER = {"\x1b[?1049h", 8, true};
const escape_sequence_t ESC_ALTERNATE_SCREEN_EXIT = {"\x1b[?1049l", 8, true};
const escape_sequence_t ESC_MOUSE_ENABLE_ALL = {"\x1b[?1003h", 8, true};
const escape_sequence_t ESC_MOUSE_DISABLE = {"\x1b[?1003l", 8, true};
const escape_sequence_t ESC_BRACKETED_PASTE_ENABLE = {"\x1b[?2004h", 8, true};
const escape_sequence_t ESC_BRACKETED_PASTE_DISABLE = {"\x1b[?2004l", 8, true};
const escape_sequence_t ESC_FOCUS_EVENTS_ENABLE = {"\x1b[?1004h", 8, true};
const escape_sequence_t ESC_FOCUS_EVENTS_DISABLE = {"\x1b[?1004l", 8, true};
const escape_sequence_t ESC_SYNCHRONIZED_OUTPUT_BEGIN = {"\x1b[?2026h", 8, true};
const escape_sequence_t ESC_SYNCHRONIZED_OUTPUT_END = {"\x1b[?2026l", 8, true};

/* ======================= Built-in Terminal Database ======================= */

static const terminal_db_entry_t termcap_builtin_terminals[] = {
    {
        .name = "xterm",
        .pattern = "xterm*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = false,
            .mouse = true,
            .bracketed_paste = true,
            .focus_events = true,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = true,
            .synchronized_output = false,
            .hyperlinks = false
        },
        .priority = 5
    },
    {
        .name = "gnome-terminal",
        .pattern = "gnome*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = true,
            .mouse = true,
            .bracketed_paste = true,
            .focus_events = true,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = true,
            .synchronized_output = true,
            .hyperlinks = true
        },
        .priority = 8
    },
    {
        .name = "konsole",
        .pattern = "konsole*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = true,
            .mouse = true,
            .bracketed_paste = true,
            .focus_events = true,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = true,
            .synchronized_output = false,
            .hyperlinks = true
        },
        .priority = 7
    },
    {
        .name = "iterm2",
        .pattern = "*",  // Detected via environment
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = true,
            .mouse = true,
            .bracketed_paste = true,
            .focus_events = true,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = true,
            .synchronized_output = true,
            .hyperlinks = true
        },
        .priority = 9
    },
    {
        .name = "tmux",
        .pattern = "tmux*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = true,  // Depends on version
            .mouse = true,
            .bracketed_paste = true,
            .focus_events = true,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = false,
            .synchronized_output = false,
            .hyperlinks = false
        },
        .priority = 6
    },
    {
        .name = "screen",
        .pattern = "screen*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = false,
            .mouse = false,
            .bracketed_paste = true,
            .focus_events = false,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = false,
            .synchronized_output = false,
            .hyperlinks = false
        },
        .priority = 4
    },
    {
        .name = "vte",
        .pattern = "vte*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = true,
            .mouse = true,
            .bracketed_paste = true,
            .focus_events = true,
            .title_setting = true,
            .alternate_screen = true,
            .unicode = true,
            .cursor_styling = true,
            .synchronized_output = true,
            .hyperlinks = true
        },
        .priority = 7
    }
};

static const size_t termcap_builtin_terminals_count = 
    sizeof(termcap_builtin_terminals) / sizeof(termcap_builtin_terminals[0]);

/* ======================= Utility Functions ======================= */

static int termcap_write_raw(const char *data, size_t length) {
    if (!data || length == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    if (!g_terminal_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }
    
    ssize_t written = write(STDOUT_FILENO, data, length);
    if (written < 0) {
        return TERMCAP_ERROR;
    }
    
    return (written == (ssize_t)length) ? TERMCAP_OK : TERMCAP_ERROR;
}

static int termcap_write_sequence(const escape_sequence_t *seq) {
    if (!seq || seq->length == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_raw(seq->sequence, seq->length);
}

static int termcap_write_formatted(const char *format, ...) {
    if (!format) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    char buffer[TERMCAP_MAX_ESCAPE_SEQUENCE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len < 0 || len >= (int)sizeof(buffer)) {
        return TERMCAP_ERROR;
    }
    
    return termcap_write_raw(buffer, len);
}

static bool termcap_match_pattern(const char *text, const char *pattern) {
    if (!text || !pattern) {
        return false;
    }
    
    // Simple wildcard matching - supports * at end
    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0) {
        return false;
    }
    
    if (pattern[pattern_len - 1] == '*') {
        return strncmp(text, pattern, pattern_len - 1) == 0;
    }
    
    return strcmp(text, pattern) == 0;
}

/* ======================= Core Initialization ======================= */

int termcap_init(void) {
    if (g_termcap_state.initialized) {
        return TERMCAP_OK;
    }
    
    // Check if we're dealing with a terminal
    g_terminal_info.is_tty = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    
    // Get terminal type from environment (even for non-TTY)
    const char *term_env = getenv("TERM");
    g_terminal_info.term_type = term_env ? term_env : "unknown";
    
    // Initialize basic capabilities even for non-TTY (preserves color support)
    // This allows prompts to work with colors when output goes to terminal
    memset(&g_terminal_info.caps, 0, sizeof(g_terminal_info.caps));
    
    // Basic color support detection based on environment
    if (term_env && (strstr(term_env, "color") || strstr(term_env, "xterm") || 
                     strstr(term_env, "konsole") || strstr(term_env, "gnome") ||
                     strstr(term_env, "256"))) {
        g_terminal_info.caps.colors = true;
        g_terminal_info.caps.unicode = true;
        
        if (strstr(term_env, "256")) {
            g_terminal_info.caps.colors_256 = true;
        }
    }
    
    // Enhanced detection only for TTY
    if (!g_terminal_info.is_tty) {
        g_termcap_state.initialized = true;
        return TERMCAP_NOT_TERMINAL;
    }
    
    // Save original terminal settings
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == 0) {
        g_termios_saved = true;
    }
    
    // Get initial terminal size
    termcap_update_size();
    
    // Initialize internal state
    memset(&g_termcap_state, 0, sizeof(g_termcap_state));
    g_termcap_state.initialized = true;
    g_termcap_state.cursor_query_timeout_ms = TERMCAP_TIMEOUT_MS;
    
    return TERMCAP_OK;
}

void termcap_cleanup(void) {
    if (!g_termcap_state.initialized) {
        return;
    }
    
    // Disable any enabled features
    if (g_terminal_info.caps.mouse) {
        termcap_disable_mouse();
    }
    
    if (g_terminal_info.caps.bracketed_paste) {
        termcap_disable_bracketed_paste();
    }
    
    if (g_terminal_info.caps.focus_events) {
        termcap_disable_focus_events();
    }
    
    // Reset terminal formatting
    termcap_reset_all_formatting();
    
    // Show cursor
    termcap_show_cursor();
    
    // Restore original terminal settings
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_termios_saved = false;
    }
    
    // Clean up output buffer if active
    if (g_termcap_state.current_buffer) {
        termcap_output_buffer_cleanup(g_termcap_state.current_buffer);
        g_termcap_state.current_buffer = NULL;
    }
    
    g_termcap_state.initialized = false;
}

const terminal_info_t *termcap_get_info(void) {
    if (!g_termcap_state.initialized) {
        termcap_init();
    }
    return &g_terminal_info;
}

void termcap_update_size(void) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        g_terminal_info.rows = ws.ws_row;
        g_terminal_info.cols = ws.ws_col;
    } else {
        // Fallback to defaults
        g_terminal_info.rows = TERMCAP_DEFAULT_ROWS;
        g_terminal_info.cols = TERMCAP_DEFAULT_COLS;
    }
    
    // Sanity check
    if (g_terminal_info.rows <= 0) {
        g_terminal_info.rows = TERMCAP_DEFAULT_ROWS;
    }
    if (g_terminal_info.cols <= 0) {
        g_terminal_info.cols = TERMCAP_DEFAULT_COLS;
    }
}

/* ======================= Terminal Capability Detection ======================= */

static const terminal_db_entry_t *termcap_find_terminal_entry(const char *term_name) {
    if (!term_name) {
        return NULL;
    }
    
    const terminal_db_entry_t *best_match = NULL;
    int best_priority = -1;
    
    // Check for iTerm2 via environment variables first
    if (termcap_is_iterm2()) {
        for (size_t i = 0; i < termcap_builtin_terminals_count; i++) {
            if (strcmp(termcap_builtin_terminals[i].name, "iterm2") == 0) {
                return &termcap_builtin_terminals[i];
            }
        }
    }
    
    // Pattern matching against terminal database
    for (size_t i = 0; i < termcap_builtin_terminals_count; i++) {
        const terminal_db_entry_t *entry = &termcap_builtin_terminals[i];
        
        if (termcap_match_pattern(term_name, entry->pattern)) {
            if (entry->priority > best_priority) {
                best_match = entry;
                best_priority = entry->priority;
            }
        }
    }
    
    return best_match;
}

int termcap_detect_capabilities(void) {
    if (!g_termcap_state.initialized) {
        int result = termcap_init();
        if (result != TERMCAP_OK) {
            return result;
        }
    }
    
    if (g_termcap_state.capabilities_detected) {
        return TERMCAP_OK;
    }
    
    // Start with default capabilities
    memset(&g_terminal_info.caps, 0, sizeof(g_terminal_info.caps));
    
    // Try to find terminal in built-in database
    const terminal_db_entry_t *db_entry = termcap_find_terminal_entry(g_terminal_info.term_type);
    if (db_entry) {
        g_terminal_info.caps = db_entry->caps;
        strncpy(g_terminal_info.terminal_name, db_entry->name, 
                sizeof(g_terminal_info.terminal_name) - 1);
        g_terminal_info.terminal_name[sizeof(g_terminal_info.terminal_name) - 1] = '\0';
    } else {
        // Conservative defaults for unknown terminals
        g_terminal_info.caps.colors = true;
        g_terminal_info.caps.colors_256 = false;
        g_terminal_info.caps.truecolor = false;
        g_terminal_info.caps.unicode = true;
        strncpy(g_terminal_info.terminal_name, "unknown", 
                sizeof(g_terminal_info.terminal_name) - 1);
    }
    
    // Set color count based on capabilities
    if (g_terminal_info.caps.truecolor) {
        g_terminal_info.max_colors = 16777216;  // 24-bit
    } else if (g_terminal_info.caps.colors_256) {
        g_terminal_info.max_colors = 256;
    } else if (g_terminal_info.caps.colors) {
        g_terminal_info.max_colors = 16;
    } else {
        g_terminal_info.max_colors = 2;  // Monochrome
    }
    
    g_termcap_state.capabilities_detected = true;
    return TERMCAP_OK;
}

/* ======================= Color and Attribute Functions ======================= */

int termcap_set_color(termcap_color_t fg, termcap_color_t bg) {
    if (!g_terminal_info.caps.colors) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    char sequence[64];
    int len = 0;
    
    if (fg != TERMCAP_COLOR_DEFAULT && bg != TERMCAP_COLOR_DEFAULT) {
        len = snprintf(sequence, sizeof(sequence), "\x1b[3%d;4%dm", 
                      (int)fg, (int)bg);
    } else if (fg != TERMCAP_COLOR_DEFAULT) {
        len = snprintf(sequence, sizeof(sequence), "\x1b[3%dm", (int)fg);
    } else if (bg != TERMCAP_COLOR_DEFAULT) {
        len = snprintf(sequence, sizeof(sequence), "\x1b[4%dm", (int)bg);
    } else {
        return TERMCAP_OK;  // No color change needed
    }
    
    if (len > 0 && len < (int)sizeof(sequence)) {
        return termcap_write_raw(sequence, len);
    }
    
    return TERMCAP_ERROR;
}

int termcap_set_foreground(termcap_color_t color) {
    return termcap_set_color(color, TERMCAP_COLOR_DEFAULT);
}

int termcap_set_background(termcap_color_t color) {
    return termcap_set_color(TERMCAP_COLOR_DEFAULT, color);
}

int termcap_set_color_256(int fg, int bg) {
    if (!g_terminal_info.caps.colors_256) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    if ((fg < -1 || fg >= 256) || (bg < -1 || bg >= 256)) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    char sequence[64];
    int len = 0;
    
    if (fg >= 0 && bg >= 0) {
        len = snprintf(sequence, sizeof(sequence), "\x1b[38;5;%d;48;5;%dm", fg, bg);
    } else if (fg >= 0) {
        len = snprintf(sequence, sizeof(sequence), "\x1b[38;5;%dm", fg);
    } else if (bg >= 0) {
        len = snprintf(sequence, sizeof(sequence), "\x1b[48;5;%dm", bg);
    } else {
        return TERMCAP_OK;
    }
    
    if (len > 0 && len < (int)sizeof(sequence)) {
        return termcap_write_raw(sequence, len);
    }
    
    return TERMCAP_ERROR;
}

int termcap_set_foreground_256(int color) {
    return termcap_set_color_256(color, -1);
}

int termcap_set_background_256(int color) {
    return termcap_set_color_256(-1, color);
}

int termcap_set_rgb_color(int r, int g, int b, bool foreground) {
    if (!g_terminal_info.caps.truecolor) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    char sequence[32];
    int len = snprintf(sequence, sizeof(sequence), "\x1b[%s;2;%d;%d;%dm",
                      foreground ? "38" : "48", r, g, b);
    
    if (len > 0 && len < (int)sizeof(sequence)) {
        return termcap_write_raw(sequence, len);
    }
    
    return TERMCAP_ERROR;
}

int termcap_set_foreground_rgb(int r, int g, int b) {
    return termcap_set_rgb_color(r, g, b, true);
}

int termcap_set_background_rgb(int r, int g, int b) {
    return termcap_set_rgb_color(r, g, b, false);
}

int termcap_set_attribute(termcap_attr_t attr) {
    char sequence[16];
    int len = snprintf(sequence, sizeof(sequence), "\x1b[%dm", (int)attr);
    
    if (len > 0 && len < (int)sizeof(sequence)) {
        return termcap_write_raw(sequence, len);
    }
    
    return TERMCAP_ERROR;
}

int termcap_set_bold(bool enable) {
    return termcap_set_attribute(enable ? TERMCAP_ATTR_BOLD : TERMCAP_ATTR_NORMAL);
}

int termcap_set_italic(bool enable) {
    return termcap_set_attribute(enable ? TERMCAP_ATTR_ITALIC : TERMCAP_ATTR_NORMAL);
}

int termcap_set_underline(bool enable) {
    return termcap_set_attribute(enable ? TERMCAP_ATTR_UNDERLINE : TERMCAP_ATTR_NORMAL);
}

int termcap_set_reverse(bool enable) {
    return termcap_set_attribute(enable ? TERMCAP_ATTR_REVERSE : TERMCAP_ATTR_NORMAL);
}

int termcap_reset_colors(void) {
    return termcap_write_raw("\x1b[39;49m", 8);
}

int termcap_reset_attributes(void) {
    return termcap_write_raw("\x1b[0m", 4);
}

int termcap_reset_all_formatting(void) {
    return termcap_write_sequence(&ESC_RESET_ALL);
}

/* ======================= Cursor Operations ======================= */

int termcap_get_cursor_pos(int *row, int *col) {
    if (!g_terminal_info.is_tty || !row || !col) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    // Send cursor position query
    if (termcap_write_sequence(&ESC_CURSOR_QUERY) != TERMCAP_OK) {
        return TERMCAP_ERROR;
    }
    
    // Read response with timeout
    fd_set readfds;
    struct timeval timeout;
    char response[32];
    int response_len = 0;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = g_termcap_state.cursor_query_timeout_ms * 1000;
    
    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) <= 0) {
        return TERMCAP_TIMEOUT;
    }
    
    response_len = read(STDIN_FILENO, response, sizeof(response) - 1);
    if (response_len <= 0) {
        return TERMCAP_ERROR;
    }
    
    response[response_len] = '\0';
    
    // Parse response: ESC[row;colR
    if (sscanf(response, "\x1b[%d;%dR", row, col) == 2) {
        return TERMCAP_OK;
    }
    
    return TERMCAP_ERROR;
}

int termcap_move_cursor(int row, int col) {
    if (row < 1 || col < 1) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%d;%dH", row, col);
}

int termcap_move_cursor_up(int lines) {
    if (lines <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%dA", lines);
}

int termcap_move_cursor_down(int lines) {
    if (lines <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%dB", lines);
}

int termcap_move_cursor_left(int cols) {
    if (cols <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%dD", cols);
}

int termcap_move_cursor_right(int cols) {
    if (cols <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%dC", cols);
}

int termcap_cursor_home(void) {
    return termcap_write_sequence(&ESC_CURSOR_HOME);
}

int termcap_hide_cursor(void) {
    return termcap_write_sequence(&ESC_HIDE_CURSOR);
}

int termcap_show_cursor(void) {
    return termcap_write_sequence(&ESC_SHOW_CURSOR);
}

int termcap_save_cursor(void) {
    return termcap_write_sequence(&ESC_SAVE_CURSOR);
}

int termcap_restore_cursor(void) {
    return termcap_write_sequence(&ESC_RESTORE_CURSOR);
}

/* ======================= Screen Management ======================= */

int termcap_clear_screen(void) {
    return termcap_write_sequence(&ESC_CLEAR_SCREEN);
}

int termcap_clear_line(void) {
    return termcap_write_sequence(&ESC_CLEAR_LINE);
}

int termcap_clear_to_eol(void) {
    return termcap_write_sequence(&ESC_CLEAR_TO_EOL);
}

int termcap_clear_to_bol(void) {
    return termcap_write_sequence(&ESC_CLEAR_TO_BOL);
}

int termcap_clear_to_eos(void) {
    return termcap_write_sequence(&ESC_CLEAR_TO_EOS);
}

int termcap_scroll_up(int lines) {
    if (lines <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%dS", lines);
}

int termcap_scroll_down(int lines) {
    if (lines <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_formatted("\x1b[%dT", lines);
}

/* ======================= Advanced Features ======================= */

int termcap_enter_alternate_screen(void) {
    if (!g_terminal_info.caps.alternate_screen) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    return termcap_write_sequence(&ESC_ALTERNATE_SCREEN_ENTER);
}

int termcap_exit_alternate_screen(void) {
    if (!g_terminal_info.caps.alternate_screen) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    return termcap_write_sequence(&ESC_ALTERNATE_SCREEN_EXIT);
}

int termcap_enable_mouse(termcap_mouse_mode_t mode) {
    if (!g_terminal_info.caps.mouse) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    const char *sequence = NULL;
    
    switch (mode) {
    case TERMCAP_MOUSE_CLICK:
        sequence = "\x1b[?1000h";
        break;
    case TERMCAP_MOUSE_DRAG:
        sequence = "\x1b[?1002h";
        break;
    case TERMCAP_MOUSE_ALL:
        sequence = "\x1b[?1003h";
        break;
    default:
        return TERMCAP_INVALID_PARAMETER;
    }
    
    return termcap_write_raw(sequence, 8);
}

int termcap_disable_mouse(void) {
    return termcap_write_sequence(&ESC_MOUSE_DISABLE);
}

int termcap_enable_focus_events(void) {
    if (!g_terminal_info.caps.focus_events) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    return termcap_write_sequence(&ESC_FOCUS_EVENTS_ENABLE);
}

int termcap_disable_focus_events(void) {
    return termcap_write_sequence(&ESC_FOCUS_EVENTS_DISABLE);
}

int termcap_enable_bracketed_paste(void) {
    if (!g_terminal_info.caps.bracketed_paste) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    return termcap_write_sequence(&ESC_BRACKETED_PASTE_ENABLE);
}

int termcap_disable_bracketed_paste(void) {
    return termcap_write_sequence(&ESC_BRACKETED_PASTE_DISABLE);
}

int termcap_set_title(const char *title) {
    if (!g_terminal_info.caps.title_setting || !title) {
        return TERMCAP_NOT_SUPPORTED;
    }
    
    return termcap_write_formatted("\x1b]0;%s\x07", title);
}

/* ======================= Feature Support Functions ======================= */

bool termcap_supports_colors(void) {
    return g_terminal_info.caps.colors;
}

bool termcap_supports_256_colors(void) {
    return g_terminal_info.caps.colors_256;
}

bool termcap_supports_truecolor(void) {
    return g_terminal_info.caps.truecolor;
}

bool termcap_supports_mouse(void) {
    return g_terminal_info.caps.mouse;
}

bool termcap_supports_bracketed_paste(void) {
    return g_terminal_info.caps.bracketed_paste;
}

bool termcap_supports_alternate_screen(void) {
    return g_terminal_info.caps.alternate_screen;
}

bool termcap_supports_focus_events(void) {
    return g_terminal_info.caps.focus_events;
}

bool termcap_supports_title_setting(void) {
    return g_terminal_info.caps.title_setting;
}

bool termcap_supports_unicode(void) {
    return g_terminal_info.caps.unicode;
}

/* ======================= High-Level Convenience Functions ======================= */

int termcap_print_colored(termcap_color_t fg, termcap_color_t bg, const char *text) {
    if (!text) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    int result = TERMCAP_OK;
    
    if (fg != TERMCAP_COLOR_DEFAULT || bg != TERMCAP_COLOR_DEFAULT) {
        result = termcap_set_color(fg, bg);
        if (result != TERMCAP_OK) {
            return result;
        }
    }
    
    printf("%s", text);
    
    if (fg != TERMCAP_COLOR_DEFAULT || bg != TERMCAP_COLOR_DEFAULT) {
        termcap_reset_all_formatting();
    }
    
    return result;
}

/* ======================= Buffer Building Functions ======================= */

int termcap_build_color_sequence(char *buffer, size_t buffer_size, termcap_color_t fg, termcap_color_t bg) {
    if (!buffer || buffer_size == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    if (fg == TERMCAP_COLOR_DEFAULT && bg == TERMCAP_COLOR_DEFAULT) {
        buffer[0] = '\0';
        return TERMCAP_OK;
    }
    
    int len = 0;
    if (fg != TERMCAP_COLOR_DEFAULT && bg != TERMCAP_COLOR_DEFAULT) {
        len = snprintf(buffer, buffer_size, "\x1b[3%d;4%dm", (int)fg, (int)bg);
    } else if (fg != TERMCAP_COLOR_DEFAULT) {
        len = snprintf(buffer, buffer_size, "\x1b[3%dm", (int)fg);
    } else if (bg != TERMCAP_COLOR_DEFAULT) {
        len = snprintf(buffer, buffer_size, "\x1b[4%dm", (int)bg);
    }
    
    if (len < 0 || (size_t)len >= buffer_size) {
        return TERMCAP_ERROR;
    }
    
    return TERMCAP_OK;
}

int termcap_build_reset_sequence(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 5) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    strcpy(buffer, "\x1b[0m");
    return TERMCAP_OK;
}

int termcap_build_cursor_move_sequence(char *buffer, size_t buffer_size, int cols, bool left) {
    if (!buffer || buffer_size == 0 || cols <= 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    int len = snprintf(buffer, buffer_size, "\x1b[%d%c", cols, left ? 'D' : 'C');
    
    if (len < 0 || (size_t)len >= buffer_size) {
        return TERMCAP_ERROR;
    }
    
    return TERMCAP_OK;
}

int termcap_build_clear_sequence(char *buffer, size_t buffer_size, int clear_type) {
    if (!buffer || buffer_size < 5) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    const char *sequence;
    switch (clear_type) {
        case TERMCAP_CLEAR_TO_EOL:
            sequence = "\x1b[K";
            break;
        case TERMCAP_CLEAR_LINE:
            sequence = "\x1b[2K";
            break;
        case TERMCAP_CLEAR_TO_BOL:
            sequence = "\x1b[1K";
            break;
        default:
            return TERMCAP_INVALID_PARAMETER;
    }
    
    strcpy(buffer, sequence);
    return TERMCAP_OK;
}

int termcap_build_attribute_sequence(char *buffer, size_t buffer_size, termcap_attr_t attr) {
    if (!buffer || buffer_size == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    int len = snprintf(buffer, buffer_size, "\x1b[%dm", (int)attr);
    
    if (len < 0 || (size_t)len >= buffer_size) {
        return TERMCAP_ERROR;
    }
    
    return TERMCAP_OK;
}

int termcap_format_colored_text(char *buffer, size_t buffer_size, const char *text, 
                               termcap_color_t fg, termcap_color_t bg) {
    if (!buffer || !text || buffer_size == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    char color_start[32], color_end[32];
    
    if (fg == TERMCAP_COLOR_DEFAULT && bg == TERMCAP_COLOR_DEFAULT) {
        // No color formatting needed
        if (strlen(text) >= buffer_size) {
            return TERMCAP_ERROR;
        }
        strcpy(buffer, text);
        return TERMCAP_OK;
    }
    
    // Build color start sequence
    int result = termcap_build_color_sequence(color_start, sizeof(color_start), fg, bg);
    if (result != TERMCAP_OK) {
        return result;
    }
    
    // Build color end sequence
    result = termcap_build_reset_sequence(color_end, sizeof(color_end));
    if (result != TERMCAP_OK) {
        return result;
    }
    
    // Format the complete colored text
    int len = snprintf(buffer, buffer_size, "%s%s%s", color_start, text, color_end);
    
    if (len < 0 || (size_t)len >= buffer_size) {
        return TERMCAP_ERROR;
    }
    
    return TERMCAP_OK;
}

int termcap_print_bold(const char *text) {
    if (!text) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    termcap_set_bold(true);
    int result = termcap_write_raw(text, strlen(text));
    termcap_reset_attributes();
    
    return result;
}

int termcap_print_italic(const char *text) {
    if (!text) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    termcap_set_italic(true);
    int result = termcap_write_raw(text, strlen(text));
    termcap_reset_attributes();
    
    return result;
}

int termcap_print_error(const char *text) {
    return termcap_print_colored(TERMCAP_RED, TERMCAP_DEFAULT, text);
}

int termcap_print_warning(const char *text) {
    return termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, text);
}

int termcap_print_success(const char *text) {
    return termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, text);
}

int termcap_print_info(const char *text) {
    return termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, text);
}

/* ======================= Output Buffer Management ======================= */

int termcap_output_buffer_init(termcap_output_buffer_t *buf, size_t size) {
    if (!buf || size == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    buf->buffer = malloc(size);
    if (!buf->buffer) {
        return TERMCAP_ERROR;
    }
    
    buf->size = size;
    buf->used = 0;
    buf->fd = STDOUT_FILENO;
    
    return TERMCAP_OK;
}

int termcap_output_buffer_add(termcap_output_buffer_t *buf, const char *data, size_t len) {
    if (!buf || !data || len == 0) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    if (buf->used + len >= buf->size) {
        return TERMCAP_BUFFER_FULL;
    }
    
    memcpy(buf->buffer + buf->used, data, len);
    buf->used += len;
    
    return TERMCAP_OK;
}

int termcap_output_buffer_flush(termcap_output_buffer_t *buf) {
    if (!buf || !buf->buffer) {
        return TERMCAP_INVALID_PARAMETER;
    }
    
    if (buf->used == 0) {
        return TERMCAP_OK;
    }
    
    ssize_t written = write(buf->fd, buf->buffer, buf->used);
    if (written < 0) {
        return TERMCAP_ERROR;
    }
    
    buf->used = 0;
    return TERMCAP_OK;
}

void termcap_output_buffer_cleanup(termcap_output_buffer_t *buf) {
    if (!buf) {
        return;
    }
    
    if (buf->buffer) {
        free(buf->buffer);
        buf->buffer = NULL;
    }
    
    buf->size = 0;
    buf->used = 0;
}

/* ======================= Legacy Functions (Bottom-line Protection) ======================= */

bool termcap_is_at_bottom_line(void) {
    int row, col;

    if (termcap_get_cursor_pos(&row, &col) != TERMCAP_OK) {
        return false;
    }

    return (row >= g_terminal_info.rows);
}

int termcap_ensure_bottom_margin(void) {
    if (!g_terminal_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    static bool margin_created = false;
    if (margin_created) {
        return TERMCAP_OK;
    }

    if (termcap_save_cursor() != TERMCAP_OK) {
        return TERMCAP_ERROR;
    }

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "\x1b[%d;1H", g_terminal_info.rows);
    termcap_write_raw(cmd, strlen(cmd));
    termcap_write_raw("\n", 1);

    if (termcap_restore_cursor() != TERMCAP_OK) {
        return TERMCAP_ERROR;
    }

    margin_created = true;
    return TERMCAP_OK;
}

int termcap_create_safe_margin(void) {
    if (!g_terminal_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    int row, col;
    if (termcap_get_cursor_pos(&row, &col) != TERMCAP_OK) {
        return termcap_ensure_bottom_margin();
    }

    if (row >= g_terminal_info.rows - 1) {
        termcap_write_raw("\x1b[A", 3);  // Move up
        termcap_write_raw("\n", 1);      // Create newline
        return TERMCAP_OK;
    }

    return TERMCAP_OK;
}

/* ======================= Platform Detection (Legacy) ======================= */

bool termcap_is_iterm2(void) {
    const char *term_program = getenv("TERM_PROGRAM");
    const char *iterm_session = getenv("ITERM_SESSION_ID");

    return (iterm_session != NULL) ||
           (term_program && strstr(term_program, "iTerm"));
}

bool termcap_is_tmux(void) {
    const char *tmux = getenv("TMUX");
    return (tmux != NULL) ||
           (g_terminal_info.term_type && strstr(g_terminal_info.term_type, "tmux"));
}

bool termcap_is_screen(void) {
    return (g_terminal_info.term_type && strstr(g_terminal_info.term_type, "screen"));
}

/* ======================= Internal State Access ======================= */

termcap_internal_state_t *termcap_get_internal_state(void) {
    return &g_termcap_state;
}

/* ======================= Shell Integration Functions ======================= */

int termcap_shell_setup(void) {
    if (!g_termcap_state.initialized) {
        int result = termcap_init();
        if (result != TERMCAP_OK) {
            return result;
        }
    }

    // Detect and enable optimal features for shell use
    termcap_detect_capabilities();

    // Enable bracketed paste for safer input handling
    if (termcap_supports_bracketed_paste()) {
        termcap_enable_bracketed_paste();
    }

    // Enable focus events for better terminal integration
    if (termcap_supports_focus_events()) {
        termcap_enable_focus_events();
    }

    // Create safe bottom margin for shell operations
    if (g_terminal_info.is_tty) {
        termcap_ensure_bottom_margin();
    }

    return TERMCAP_OK;
}

int termcap_shell_cleanup(void) {
    if (!g_termcap_state.initialized) {
        return TERMCAP_OK;
    }

    // Disable features that were enabled for shell use
    if (g_terminal_info.caps.bracketed_paste) {
        termcap_disable_bracketed_paste();
    }

    if (g_terminal_info.caps.focus_events) {
        termcap_disable_focus_events();
    }

    // Reset all formatting
    termcap_reset_all_formatting();

    return TERMCAP_OK;
}

int termcap_shell_resize_handler(void) {
    // Update terminal size information
    termcap_update_size();

    // Re-detect capabilities that might be affected by resize
    termcap_detect_capabilities();

    // Ensure bottom margin is still valid
    if (g_terminal_info.is_tty) {
        termcap_ensure_bottom_margin();
    }

    return TERMCAP_OK;
}

/* ======================= Linenoise Integration Functions ======================= */

int termcap_linenoise_set_colors(termcap_color_t prompt, termcap_color_t input, termcap_color_t completion) {
    if (!g_terminal_info.is_tty || !termcap_supports_colors()) {
        return TERMCAP_NOT_SUPPORTED;
    }

    // Store colors for linenoise to use
    g_termcap_state.linenoise_prompt_color = prompt;
    g_termcap_state.linenoise_input_color = input;
    g_termcap_state.linenoise_completion_color = completion;

    return TERMCAP_OK;
}

int termcap_linenoise_enable_mouse(bool enable) {
    if (!g_terminal_info.is_tty || !termcap_supports_mouse()) {
        return TERMCAP_NOT_SUPPORTED;
    }

    if (enable) {
        return termcap_enable_mouse(TERMCAP_MOUSE_CLICK);
    } else {
        return termcap_disable_mouse();
    }
}

int termcap_linenoise_set_completion_style(int style) {
    if (!g_terminal_info.is_tty) {
        return TERMCAP_NOT_SUPPORTED;
    }

    // Store completion style preference
    g_termcap_state.linenoise_completion_style = style;

    return TERMCAP_OK;
}