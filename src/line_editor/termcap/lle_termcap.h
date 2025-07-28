/**
 * lle_termcap.h - Enhanced Portable Terminal Capability Management for LLE
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
 * - Full integration with LLE for enhanced line editing
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
 *   lle_termcap_init();                           // Initialize enhanced capabilities
 *   lle_termcap_detect_capabilities();            // Auto-detect terminal features
 *   lle_termcap_set_color(RED, BLACK);           // Set colors
 *   lle_termcap_move_cursor(10, 20);             // Advanced cursor control
 *   lle_termcap_clear_screen();                  // Screen management
 *   lle_termcap_enable_mouse(MOUSE_ALL);         // Enable mouse support
 *   lle_termcap_cleanup();                       // Cleanup terminal state
 */

#ifndef LLE_TERMCAP_H
#define LLE_TERMCAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Enhanced color definitions */
typedef enum {
    LLE_TERMCAP_COLOR_BLACK = 0,
    LLE_TERMCAP_COLOR_RED = 1,
    LLE_TERMCAP_COLOR_GREEN = 2,
    LLE_TERMCAP_COLOR_YELLOW = 3,
    LLE_TERMCAP_COLOR_BLUE = 4,
    LLE_TERMCAP_COLOR_MAGENTA = 5,
    LLE_TERMCAP_COLOR_CYAN = 6,
    LLE_TERMCAP_COLOR_WHITE = 7,
    LLE_TERMCAP_COLOR_BRIGHT_BLACK = 8,
    LLE_TERMCAP_COLOR_BRIGHT_RED = 9,
    LLE_TERMCAP_COLOR_BRIGHT_GREEN = 10,
    LLE_TERMCAP_COLOR_BRIGHT_YELLOW = 11,
    LLE_TERMCAP_COLOR_BRIGHT_BLUE = 12,
    LLE_TERMCAP_COLOR_BRIGHT_MAGENTA = 13,
    LLE_TERMCAP_COLOR_BRIGHT_CYAN = 14,
    LLE_TERMCAP_COLOR_BRIGHT_WHITE = 15,
    LLE_TERMCAP_COLOR_DEFAULT = -1
} lle_termcap_color_t;

/* Text attributes */
typedef enum {
    LLE_TERMCAP_ATTR_NORMAL = 0,
    LLE_TERMCAP_ATTR_BOLD = 1,
    LLE_TERMCAP_ATTR_DIM = 2,
    LLE_TERMCAP_ATTR_ITALIC = 3,
    LLE_TERMCAP_ATTR_UNDERLINE = 4,
    LLE_TERMCAP_ATTR_BLINK = 5,
    LLE_TERMCAP_ATTR_REVERSE = 7,
    LLE_TERMCAP_ATTR_STRIKETHROUGH = 9
} lle_termcap_attr_t;

/* Mouse support modes */
typedef enum {
    LLE_TERMCAP_MOUSE_OFF = 0,
    LLE_TERMCAP_MOUSE_CLICK = 1,
    LLE_TERMCAP_MOUSE_DRAG = 2,
    LLE_TERMCAP_MOUSE_ALL = 3
} lle_termcap_mouse_mode_t;

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
} lle_termcap_capabilities_t;

/* Enhanced terminal information structure */
typedef struct {
    int rows;                       /* Terminal height */
    int cols;                       /* Terminal width */
    bool is_tty;                    /* Is a terminal */
    const char *term_type;          /* Terminal type from $TERM */
    
    /* Enhanced capabilities */
    lle_termcap_capabilities_t caps;    /* Detected capabilities */
    
    /* Performance characteristics */
    int cursor_query_timeout_ms;    /* Cursor position query timeout */
    bool fast_cursor_queries;       /* Whether cursor queries are fast */
    
    /* Terminal identification */
    char terminal_name[64];         /* Detected terminal name */
    char terminal_version[32];      /* Terminal version if available */
    
    /* Color support details */
    int max_colors;                 /* Maximum number of colors */
    bool background_color_erase;    /* BCE support */
    
} lle_terminal_info_t;

/* Output buffer for batched operations */
typedef struct {
    char *buffer;
    size_t size;
    size_t used;
    int fd;
} lle_termcap_output_buffer_t;

/* Mouse event structure */
typedef struct {
    int row, col;                   /* Position */
    int button;                     /* Button number (0=left, 1=middle, 2=right) */
    bool pressed;                   /* True for press, false for release */
    bool shift, ctrl, alt;          /* Modifier keys */
} lle_termcap_mouse_event_t;

/* ======================= Core Functions ======================= */

/* Initialize and cleanup */
int lle_termcap_init(void);
void lle_termcap_cleanup(void);

/* Terminal information */
const lle_terminal_info_t *lle_termcap_get_info(void);
void lle_termcap_update_size(void);
int lle_termcap_detect_capabilities(void);

/* ======================= Color and Attributes ======================= */

/* Basic color operations */
int lle_termcap_set_color(lle_termcap_color_t fg, lle_termcap_color_t bg);
int lle_termcap_set_foreground(lle_termcap_color_t color);
int lle_termcap_set_background(lle_termcap_color_t color);
int lle_termcap_reset_colors(void);

/* Advanced color support */
int lle_termcap_set_color_256(int fg, int bg);
int lle_termcap_set_foreground_256(int color);
int lle_termcap_set_background_256(int color);

/* True color (24-bit RGB) support */
int lle_termcap_set_rgb_color(int r, int g, int b, bool foreground);
int lle_termcap_set_foreground_rgb(int r, int g, int b);
int lle_termcap_set_background_rgb(int r, int g, int b);

/* Text attributes */
int lle_termcap_set_attribute(lle_termcap_attr_t attr);
int lle_termcap_set_bold(bool enable);
int lle_termcap_set_italic(bool enable);
int lle_termcap_set_underline(bool enable);
int lle_termcap_set_reverse(bool enable);
int lle_termcap_reset_attributes(void);

/* Combined color and attribute operations */
int lle_termcap_set_style(lle_termcap_color_t fg, lle_termcap_color_t bg, lle_termcap_attr_t attr);
int lle_termcap_reset_all_formatting(void);

/* ======================= Cursor Operations ======================= */

/* Basic cursor operations using standard ANSI/VT100 sequences */
int lle_termcap_get_cursor_pos(int *row, int *col);
int lle_termcap_save_cursor(void);
int lle_termcap_restore_cursor(void);

/* Enhanced cursor movement */
int lle_termcap_move_cursor(int row, int col);
int lle_termcap_move_cursor_up(int lines);
int lle_termcap_move_cursor_down(int lines);
int lle_termcap_move_cursor_left(int cols);
int lle_termcap_move_cursor_right(int cols);
int lle_termcap_cursor_home(void);
int lle_termcap_cursor_to_column(int col);

/* Cursor visibility and styling */
int lle_termcap_hide_cursor(void);
int lle_termcap_show_cursor(void);
int lle_termcap_set_cursor_style(int style);  /* 0=block, 1=underline, 2=bar */
int lle_termcap_set_cursor_blink(bool enable);

/* ======================= Screen Management ======================= */

/* Screen clearing operations */
int lle_termcap_clear_screen(void);
int lle_termcap_clear_line(void);
int lle_termcap_clear_to_eol(void);
int lle_termcap_clear_to_bol(void);
int lle_termcap_clear_to_eos(void);
int lle_termcap_clear_to_bos(void);

/* Line operations */
int lle_termcap_insert_line(void);
int lle_termcap_insert_lines(int count);
int lle_termcap_delete_line(void);
int lle_termcap_delete_lines(int count);

/* Character operations */
int lle_termcap_insert_char(void);
int lle_termcap_insert_chars(int count);
int lle_termcap_delete_char(void);
int lle_termcap_delete_chars(int count);

/* Scrolling operations */
int lle_termcap_scroll_up(int lines);
int lle_termcap_scroll_down(int lines);
int lle_termcap_set_scroll_region(int top, int bottom);
int lle_termcap_reset_scroll_region(void);

/* ======================= Advanced Features ======================= */

/* Alternate screen buffer */
int lle_termcap_enter_alternate_screen(void);
int lle_termcap_exit_alternate_screen(void);

/* Mouse support */
int lle_termcap_enable_mouse(lle_termcap_mouse_mode_t mode);
int lle_termcap_disable_mouse(void);
int lle_termcap_parse_mouse_event(const char *sequence, lle_termcap_mouse_event_t *event);

/* Bracketed paste mode */
int lle_termcap_enable_bracketed_paste(void);
int lle_termcap_disable_bracketed_paste(void);

/* Focus events */
int lle_termcap_enable_focus_events(void);
int lle_termcap_disable_focus_events(void);

/* Terminal title management */
int lle_termcap_set_title(const char *title);
int lle_termcap_push_title(void);
int lle_termcap_pop_title(void);

/* Synchronized output (reduces flicker) */
int lle_termcap_begin_synchronized_output(void);
int lle_termcap_end_synchronized_output(void);

/* Bell operations */
int lle_termcap_bell(void);
int lle_termcap_visual_bell(void);

/* ======================= Performance Features ======================= */

/* Output buffering for performance */
int lle_termcap_output_buffer_init(lle_termcap_output_buffer_t *buf, size_t size);
int lle_termcap_output_buffer_add(lle_termcap_output_buffer_t *buf, const char *data, size_t len);
int lle_termcap_output_buffer_add_formatted(lle_termcap_output_buffer_t *buf, const char *format, ...);
int lle_termcap_output_buffer_flush(lle_termcap_output_buffer_t *buf);
void lle_termcap_output_buffer_cleanup(lle_termcap_output_buffer_t *buf);

/* Batch operations */
int lle_termcap_begin_batch(void);
int lle_termcap_end_batch(void);

/* ======================= Capability Testing ======================= */

/* Feature detection */
bool lle_termcap_supports_colors(void);
bool lle_termcap_supports_256_colors(void);
bool lle_termcap_supports_truecolor(void);
bool lle_termcap_supports_mouse(void);
bool lle_termcap_supports_bracketed_paste(void);
bool lle_termcap_supports_focus_events(void);
bool lle_termcap_supports_title_setting(void);
bool lle_termcap_supports_alternate_screen(void);
bool lle_termcap_supports_unicode(void);

/* Terminal identification */
bool lle_termcap_query_terminal_id(char *buffer, size_t size);
int lle_termcap_benchmark_cursor_queries(void);

/* ======================= Bottom-line Protection (Legacy) ======================= */

/* Bottom-line protection for shells */
int lle_termcap_ensure_bottom_margin(void);
bool lle_termcap_is_at_bottom_line(void);
int lle_termcap_create_safe_margin(void);

/* ======================= Platform Detection (Legacy) ======================= */

/* Platform detection */
bool lle_termcap_is_iterm2(void);
bool lle_termcap_is_tmux(void);
bool lle_termcap_is_screen(void);

/* ======================= Error Codes and Constants ======================= */

/* Error codes */
#define LLE_TERMCAP_OK 0
#define LLE_TERMCAP_ERROR -1
#define LLE_TERMCAP_NOT_TERMINAL -2
#define LLE_TERMCAP_TIMEOUT -3
#define LLE_TERMCAP_NOT_SUPPORTED -4
#define LLE_TERMCAP_BUFFER_FULL -5
#define LLE_TERMCAP_INVALID_PARAMETER -6

/* Default values */
#define LLE_TERMCAP_DEFAULT_ROWS 24
#define LLE_TERMCAP_DEFAULT_COLS 80
#define LLE_TERMCAP_TIMEOUT_MS 100
#define LLE_TERMCAP_MAX_SEQUENCE_LENGTH 64
#define LLE_TERMCAP_DEFAULT_BUFFER_SIZE 4096

/* Color constants for convenience */
#define LLE_TERMCAP_BLACK      LLE_TERMCAP_COLOR_BLACK
#define LLE_TERMCAP_RED        LLE_TERMCAP_COLOR_RED
#define LLE_TERMCAP_GREEN      LLE_TERMCAP_COLOR_GREEN
#define LLE_TERMCAP_YELLOW     LLE_TERMCAP_COLOR_YELLOW
#define LLE_TERMCAP_BLUE       LLE_TERMCAP_COLOR_BLUE
#define LLE_TERMCAP_MAGENTA    LLE_TERMCAP_COLOR_MAGENTA
#define LLE_TERMCAP_CYAN       LLE_TERMCAP_COLOR_CYAN
#define LLE_TERMCAP_WHITE      LLE_TERMCAP_COLOR_WHITE
#define LLE_TERMCAP_DEFAULT    LLE_TERMCAP_COLOR_DEFAULT

/* ======================= High-Level Convenience Functions ======================= */

/* Styled output functions */
int lle_termcap_print_colored(lle_termcap_color_t fg, lle_termcap_color_t bg, const char *text);
int lle_termcap_print_bold(const char *text);
int lle_termcap_print_italic(const char *text);
int lle_termcap_print_underline(const char *text);

/* Escape sequence building functions for buffer operations */
int lle_termcap_build_color_sequence(char *buffer, size_t buffer_size, lle_termcap_color_t fg, lle_termcap_color_t bg);
int lle_termcap_build_reset_sequence(char *buffer, size_t buffer_size);
int lle_termcap_build_cursor_move_sequence(char *buffer, size_t buffer_size, int cols, bool left);
int lle_termcap_build_clear_sequence(char *buffer, size_t buffer_size, int clear_type);
int lle_termcap_build_attribute_sequence(char *buffer, size_t buffer_size, lle_termcap_attr_t attr);
int lle_termcap_format_colored_text(char *buffer, size_t buffer_size, const char *text, 
                               lle_termcap_color_t fg, lle_termcap_color_t bg);
int lle_termcap_print_error(const char *text);
int lle_termcap_print_warning(const char *text);
int lle_termcap_print_success(const char *text);
int lle_termcap_print_info(const char *text);

/* Progress indicators */
int lle_termcap_show_progress_bar(int percentage, int width);
int lle_termcap_show_spinner(int position);
int lle_termcap_clear_progress(void);

/* Box drawing */
int lle_termcap_draw_horizontal_line(int row, int col, int length);
int lle_termcap_draw_vertical_line(int row, int col, int length);
int lle_termcap_draw_box(int top_row, int left_col, int height, int width);

/* ======================= Integration Functions ======================= */

/* Clear type constants for lle_termcap_build_clear_sequence */
#define LLE_TERMCAP_CLEAR_TO_EOL 0
#define LLE_TERMCAP_CLEAR_LINE 1
#define LLE_TERMCAP_CLEAR_TO_BOL 2

/* LLE integration */
int lle_termcap_line_editor_set_colors(lle_termcap_color_t prompt, lle_termcap_color_t input, lle_termcap_color_t completion);
int lle_termcap_line_editor_enable_mouse(bool enable);
int lle_termcap_line_editor_set_completion_style(int style);
int lle_termcap_line_editor_replace_sequences(void);

/* Shell integration */
int lle_termcap_shell_setup(void);
int lle_termcap_shell_cleanup(void);
int lle_termcap_shell_resize_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* LLE_TERMCAP_H */
