/**
 * termcap.h - Enhanced Portable Terminal Capability Management for LUSUSH
 *
 * This module provides comprehensive terminal handling using standard POSIX
 * terminal capabilities enhanced with modern terminal features. Works across
 * Linux, macOS, and BSD systems without requiring external dependencies.
 *
 * Features:
 * - Advanced color support (16, 256, truecolor)
 * - Comprehensive cursor and screen management
 * - Terminal capability detection and optimization
 * - Mouse support and bracketed paste mode
 * - Built-in terminal database for common terminals
 * - Performance-optimized escape sequence handling
 * - Full integration with linenoise for enhanced line editing
 *
 * Enhanced Features:
 * - Rich prompt styling with colors and attributes
 * - Advanced completion menus with mouse support
 * - Professional error display with color coding
 * - Progress indicators for long operations
 * - Full-screen application support
 * - Modern terminal compatibility (iTerm2, GNOME Terminal, etc.)
 *
 * Usage:
 *   termcap_init();                           // Initialize enhanced capabilities
 *   termcap_detect_capabilities();            // Auto-detect terminal features
 *   termcap_set_color(RED, BLACK);           // Set colors
 *   termcap_move_cursor(10, 20);             // Advanced cursor control
 *   termcap_clear_screen();                  // Screen management
 *   termcap_enable_mouse(MOUSE_ALL);         // Enable mouse support
 *   termcap_cleanup();                       // Cleanup terminal state
 */

#ifndef TERMCAP_H
#define TERMCAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Enhanced color definitions */
typedef enum {
    TERMCAP_COLOR_BLACK = 0,
    TERMCAP_COLOR_RED = 1,
    TERMCAP_COLOR_GREEN = 2,
    TERMCAP_COLOR_YELLOW = 3,
    TERMCAP_COLOR_BLUE = 4,
    TERMCAP_COLOR_MAGENTA = 5,
    TERMCAP_COLOR_CYAN = 6,
    TERMCAP_COLOR_WHITE = 7,
    TERMCAP_COLOR_BRIGHT_BLACK = 8,
    TERMCAP_COLOR_BRIGHT_RED = 9,
    TERMCAP_COLOR_BRIGHT_GREEN = 10,
    TERMCAP_COLOR_BRIGHT_YELLOW = 11,
    TERMCAP_COLOR_BRIGHT_BLUE = 12,
    TERMCAP_COLOR_BRIGHT_MAGENTA = 13,
    TERMCAP_COLOR_BRIGHT_CYAN = 14,
    TERMCAP_COLOR_BRIGHT_WHITE = 15,
    TERMCAP_COLOR_DEFAULT = -1
} termcap_color_t;

/* Text attributes */
typedef enum {
    TERMCAP_ATTR_NORMAL = 0,
    TERMCAP_ATTR_BOLD = 1,
    TERMCAP_ATTR_DIM = 2,
    TERMCAP_ATTR_ITALIC = 3,
    TERMCAP_ATTR_UNDERLINE = 4,
    TERMCAP_ATTR_BLINK = 5,
    TERMCAP_ATTR_REVERSE = 7,
    TERMCAP_ATTR_STRIKETHROUGH = 9
} termcap_attr_t;

/* Mouse support modes */
typedef enum {
    TERMCAP_MOUSE_OFF = 0,
    TERMCAP_MOUSE_CLICK = 1,
    TERMCAP_MOUSE_DRAG = 2,
    TERMCAP_MOUSE_ALL = 3
} termcap_mouse_mode_t;

/* Terminal capability flags */
typedef struct {
    bool colors;                    /* Basic 16-color support */
    bool colors_256;                /* 256-color support */
    bool truecolor;                 /* 24-bit RGB color support */
    bool mouse;                     /* Mouse event support */
    bool bracketed_paste;           /* Bracketed paste mode */
    bool focus_events;              /* Focus in/out events */
    bool title_setting;             /* Terminal title modification */
    bool alternate_screen;          /* Alternate screen buffer */
    bool unicode;                   /* UTF-8 Unicode support */
    bool cursor_styling;            /* Cursor shape/style changes */
    bool synchronized_output;       /* Synchronized output mode */
    bool hyperlinks;                /* OSC 8 hyperlink support */
} termcap_capabilities_t;

/* Enhanced terminal information structure */
typedef struct {
    int rows;                       /* Terminal height */
    int cols;                       /* Terminal width */
    bool is_tty;                    /* Is a terminal */
    const char *term_type;          /* Terminal type from $TERM */
    
    /* Enhanced capabilities */
    termcap_capabilities_t caps;    /* Detected capabilities */
    
    /* Performance characteristics */
    int cursor_query_timeout_ms;    /* Cursor position query timeout */
    bool fast_cursor_queries;       /* Whether cursor queries are fast */
    
    /* Terminal identification */
    char terminal_name[64];         /* Detected terminal name */
    char terminal_version[32];      /* Terminal version if available */
    
    /* Color support details */
    int max_colors;                 /* Maximum number of colors */
    bool background_color_erase;    /* BCE support */
    
} terminal_info_t;

/* Output buffer for batched operations */
typedef struct {
    char *buffer;
    size_t size;
    size_t used;
    int fd;
} termcap_output_buffer_t;

/* Mouse event structure */
typedef struct {
    int row, col;                   /* Position */
    int button;                     /* Button number (0=left, 1=middle, 2=right) */
    bool pressed;                   /* True for press, false for release */
    bool shift, ctrl, alt;          /* Modifier keys */
} termcap_mouse_event_t;

/* ======================= Core Functions ======================= */

/* Initialize and cleanup */
int termcap_init(void);
void termcap_cleanup(void);

/* Terminal information */
const terminal_info_t *termcap_get_info(void);
void termcap_update_size(void);
int termcap_detect_capabilities(void);

/* ======================= Color and Attributes ======================= */

/* Basic color operations */
int termcap_set_color(termcap_color_t fg, termcap_color_t bg);
int termcap_set_foreground(termcap_color_t color);
int termcap_set_background(termcap_color_t color);
int termcap_reset_colors(void);

/* Advanced color support */
int termcap_set_color_256(int fg, int bg);
int termcap_set_foreground_256(int color);
int termcap_set_background_256(int color);

/* True color (24-bit RGB) support */
int termcap_set_rgb_color(int r, int g, int b, bool foreground);
int termcap_set_foreground_rgb(int r, int g, int b);
int termcap_set_background_rgb(int r, int g, int b);

/* Text attributes */
int termcap_set_attribute(termcap_attr_t attr);
int termcap_set_bold(bool enable);
int termcap_set_italic(bool enable);
int termcap_set_underline(bool enable);
int termcap_set_reverse(bool enable);
int termcap_reset_attributes(void);

/* Combined color and attribute operations */
int termcap_set_style(termcap_color_t fg, termcap_color_t bg, termcap_attr_t attr);
int termcap_reset_all_formatting(void);

/* ======================= Cursor Operations ======================= */

/* Basic cursor operations using standard ANSI/VT100 sequences */
int termcap_get_cursor_pos(int *row, int *col);
int termcap_save_cursor(void);
int termcap_restore_cursor(void);

/* Enhanced cursor movement */
int termcap_move_cursor(int row, int col);
int termcap_move_cursor_up(int lines);
int termcap_move_cursor_down(int lines);
int termcap_move_cursor_left(int cols);
int termcap_move_cursor_right(int cols);
int termcap_cursor_home(void);
int termcap_cursor_to_column(int col);

/* Cursor visibility and styling */
int termcap_hide_cursor(void);
int termcap_show_cursor(void);
int termcap_set_cursor_style(int style);  /* 0=block, 1=underline, 2=bar */
int termcap_set_cursor_blink(bool enable);

/* ======================= Screen Management ======================= */

/* Screen clearing operations */
int termcap_clear_screen(void);
int termcap_clear_line(void);
int termcap_clear_to_eol(void);
int termcap_clear_to_bol(void);
int termcap_clear_to_eos(void);
int termcap_clear_to_bos(void);

/* Line operations */
int termcap_insert_line(void);
int termcap_insert_lines(int count);
int termcap_delete_line(void);
int termcap_delete_lines(int count);

/* Character operations */
int termcap_insert_char(void);
int termcap_insert_chars(int count);
int termcap_delete_char(void);
int termcap_delete_chars(int count);

/* Scrolling operations */
int termcap_scroll_up(int lines);
int termcap_scroll_down(int lines);
int termcap_set_scroll_region(int top, int bottom);
int termcap_reset_scroll_region(void);

/* ======================= Advanced Features ======================= */

/* Alternate screen buffer */
int termcap_enter_alternate_screen(void);
int termcap_exit_alternate_screen(void);

/* Mouse support */
int termcap_enable_mouse(termcap_mouse_mode_t mode);
int termcap_disable_mouse(void);
int termcap_parse_mouse_event(const char *sequence, termcap_mouse_event_t *event);

/* Bracketed paste mode */
int termcap_enable_bracketed_paste(void);
int termcap_disable_bracketed_paste(void);

/* Focus events */
int termcap_enable_focus_events(void);
int termcap_disable_focus_events(void);

/* Terminal title management */
int termcap_set_title(const char *title);
int termcap_push_title(void);
int termcap_pop_title(void);

/* Synchronized output (reduces flicker) */
int termcap_begin_synchronized_output(void);
int termcap_end_synchronized_output(void);

/* Bell operations */
int termcap_bell(void);
int termcap_visual_bell(void);

/* ======================= Performance Features ======================= */

/* Output buffering for performance */
int termcap_output_buffer_init(termcap_output_buffer_t *buf, size_t size);
int termcap_output_buffer_add(termcap_output_buffer_t *buf, const char *data, size_t len);
int termcap_output_buffer_add_formatted(termcap_output_buffer_t *buf, const char *format, ...);
int termcap_output_buffer_flush(termcap_output_buffer_t *buf);
void termcap_output_buffer_cleanup(termcap_output_buffer_t *buf);

/* Batch operations */
int termcap_begin_batch(void);
int termcap_end_batch(void);

/* ======================= Capability Testing ======================= */

/* Feature detection */
bool termcap_supports_colors(void);
bool termcap_supports_256_colors(void);
bool termcap_supports_truecolor(void);
bool termcap_supports_mouse(void);
bool termcap_supports_bracketed_paste(void);
bool termcap_supports_focus_events(void);
bool termcap_supports_title_setting(void);
bool termcap_supports_alternate_screen(void);
bool termcap_supports_unicode(void);

/* Terminal identification */
bool termcap_query_terminal_id(char *buffer, size_t size);
int termcap_benchmark_cursor_queries(void);

/* ======================= Bottom-line Protection (Legacy) ======================= */

/* Bottom-line protection for shells */
int termcap_ensure_bottom_margin(void);
bool termcap_is_at_bottom_line(void);
int termcap_create_safe_margin(void);

/* ======================= Platform Detection (Legacy) ======================= */

/* Platform detection */
bool termcap_is_iterm2(void);
bool termcap_is_tmux(void);
bool termcap_is_screen(void);

/* ======================= Error Codes and Constants ======================= */

/* Error codes */
#define TERMCAP_OK 0
#define TERMCAP_ERROR -1
#define TERMCAP_NOT_TERMINAL -2
#define TERMCAP_TIMEOUT -3
#define TERMCAP_NOT_SUPPORTED -4
#define TERMCAP_BUFFER_FULL -5
#define TERMCAP_INVALID_PARAMETER -6

/* Default values */
#define TERMCAP_DEFAULT_ROWS 24
#define TERMCAP_DEFAULT_COLS 80
#define TERMCAP_TIMEOUT_MS 100
#define TERMCAP_MAX_SEQUENCE_LENGTH 64
#define TERMCAP_DEFAULT_BUFFER_SIZE 4096

/* Color constants for convenience */
#define TERMCAP_BLACK      TERMCAP_COLOR_BLACK
#define TERMCAP_RED        TERMCAP_COLOR_RED
#define TERMCAP_GREEN      TERMCAP_COLOR_GREEN
#define TERMCAP_YELLOW     TERMCAP_COLOR_YELLOW
#define TERMCAP_BLUE       TERMCAP_COLOR_BLUE
#define TERMCAP_MAGENTA    TERMCAP_COLOR_MAGENTA
#define TERMCAP_CYAN       TERMCAP_COLOR_CYAN
#define TERMCAP_WHITE      TERMCAP_COLOR_WHITE
#define TERMCAP_DEFAULT    TERMCAP_COLOR_DEFAULT

/* ======================= High-Level Convenience Functions ======================= */

/* Styled output functions */
int termcap_print_colored(termcap_color_t fg, termcap_color_t bg, const char *text);
int termcap_print_bold(const char *text);
int termcap_print_italic(const char *text);
int termcap_print_underline(const char *text);

/* Escape sequence building functions for buffer operations */
int termcap_build_color_sequence(char *buffer, size_t buffer_size, termcap_color_t fg, termcap_color_t bg);
int termcap_build_reset_sequence(char *buffer, size_t buffer_size);
int termcap_build_cursor_move_sequence(char *buffer, size_t buffer_size, int cols, bool left);
int termcap_build_clear_sequence(char *buffer, size_t buffer_size, int clear_type);
int termcap_build_attribute_sequence(char *buffer, size_t buffer_size, termcap_attr_t attr);
int termcap_format_colored_text(char *buffer, size_t buffer_size, const char *text, 
                               termcap_color_t fg, termcap_color_t bg);
int termcap_print_error(const char *text);
int termcap_print_warning(const char *text);
int termcap_print_success(const char *text);
int termcap_print_info(const char *text);

/* Progress indicators */
int termcap_show_progress_bar(int percentage, int width);
int termcap_show_spinner(int position);
int termcap_clear_progress(void);

/* Box drawing */
int termcap_draw_horizontal_line(int row, int col, int length);
int termcap_draw_vertical_line(int row, int col, int length);
int termcap_draw_box(int top_row, int left_col, int height, int width);

/* ======================= Integration Functions ======================= */

/* Clear type constants for termcap_build_clear_sequence */
#define TERMCAP_CLEAR_TO_EOL 0
#define TERMCAP_CLEAR_LINE 1
#define TERMCAP_CLEAR_TO_BOL 2

/* Linenoise integration */
int termcap_linenoise_set_colors(termcap_color_t prompt, termcap_color_t input, termcap_color_t completion);
int termcap_linenoise_enable_mouse(bool enable);
int termcap_linenoise_set_completion_style(int style);
int termcap_linenoise_replace_sequences(void);

/* Shell integration */
int termcap_shell_setup(void);
int termcap_shell_cleanup(void);
int termcap_shell_resize_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* TERMCAP_H */