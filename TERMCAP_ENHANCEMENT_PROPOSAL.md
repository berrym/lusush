# Termcap Enhancement Proposal for Lusush Shell

## Executive Summary

This proposal outlines a comprehensive enhancement plan for lusush's terminal capabilities (termcap.c) to provide advanced terminal features while maintaining MIT license compatibility and zero external dependencies.

## Current State Analysis

### Existing Capabilities ✅
- Basic terminal size detection via `TIOCGWINSZ`
- Cursor position detection using ANSI escape sequences
- Portable POSIX terminal handling
- Integration with linenoise for line editing
- Platform detection (iTerm2, tmux, screen)
- Bottom-line protection for shell applications

### Current Limitations ⚠️
- Limited color support (basic ANSI only)
- No advanced cursor movement capabilities
- Minimal terminal capability database
- No terminfo integration
- Limited screen management features
- No support for advanced terminal features (mouse, bracketed paste, etc.)

## Enhancement Strategy

### **Approach: Enhanced Self-Contained Implementation**
**Recommended Difficulty: Medium (2-3 days)**
**License: MIT (internal enhancement)**
**Dependencies: None (maintains lusush's zero-dependency philosophy)**

## Proposed Enhancements

### 1. Enhanced Color and Attribute Support

```c
// Extended color support
typedef enum {
    TERMCAP_COLOR_BLACK = 0,
    TERMCAP_COLOR_RED,
    TERMCAP_COLOR_GREEN,
    TERMCAP_COLOR_YELLOW,
    TERMCAP_COLOR_BLUE,
    TERMCAP_COLOR_MAGENTA,
    TERMCAP_COLOR_CYAN,
    TERMCAP_COLOR_WHITE,
    TERMCAP_COLOR_BRIGHT_BLACK,
    TERMCAP_COLOR_BRIGHT_RED,
    TERMCAP_COLOR_BRIGHT_GREEN,
    TERMCAP_COLOR_BRIGHT_YELLOW,
    TERMCAP_COLOR_BRIGHT_BLUE,
    TERMCAP_COLOR_BRIGHT_MAGENTA,
    TERMCAP_COLOR_BRIGHT_CYAN,
    TERMCAP_COLOR_BRIGHT_WHITE,
    TERMCAP_COLOR_DEFAULT = -1
} termcap_color_t;

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

// Enhanced color functions
int termcap_set_color(termcap_color_t fg, termcap_color_t bg);
int termcap_set_attribute(termcap_attr_t attr);
int termcap_reset_attributes(void);
bool termcap_supports_256_colors(void);
bool termcap_supports_truecolor(void);
int termcap_set_rgb_color(int r, int g, int b, bool foreground);
```

### 2. Advanced Cursor and Screen Management

```c
// Enhanced cursor operations
int termcap_move_cursor(int row, int col);
int termcap_move_cursor_up(int lines);
int termcap_move_cursor_down(int lines);
int termcap_move_cursor_left(int cols);
int termcap_move_cursor_right(int cols);
int termcap_cursor_home(void);
int termcap_hide_cursor(void);
int termcap_show_cursor(void);

// Screen management
int termcap_clear_screen(void);
int termcap_clear_line(void);
int termcap_clear_to_eol(void);
int termcap_clear_to_bol(void);
int termcap_clear_to_eos(void);
int termcap_scroll_up(int lines);
int termcap_scroll_down(int lines);
int termcap_insert_line(void);
int termcap_delete_line(void);
```

### 3. Enhanced Terminal Capability Detection

```c
// Extended terminal information
typedef struct {
    int rows;
    int cols;
    bool is_tty;
    const char *term_type;
    
    // Enhanced capabilities
    bool supports_colors;
    bool supports_256_colors;
    bool supports_truecolor;
    bool supports_mouse;
    bool supports_bracketed_paste;
    bool supports_focus_events;
    bool supports_title_setting;
    bool supports_alternate_screen;
    bool supports_unicode;
    
    // Performance characteristics
    int cursor_query_timeout_ms;
    bool fast_cursor_queries;
    
    // Terminal identification
    char *terminal_name;
    char *terminal_version;
    
} terminal_info_t;

// Capability detection functions
int termcap_detect_capabilities(void);
bool termcap_query_terminal_id(char *buffer, size_t size);
int termcap_benchmark_cursor_queries(void);
```

### 4. Advanced Terminal Features

```c
// Mouse support
typedef enum {
    TERMCAP_MOUSE_OFF = 0,
    TERMCAP_MOUSE_CLICK,
    TERMCAP_MOUSE_DRAG,
    TERMCAP_MOUSE_ALL
} termcap_mouse_mode_t;

int termcap_enable_mouse(termcap_mouse_mode_t mode);
int termcap_disable_mouse(void);

// Bracketed paste mode
int termcap_enable_bracketed_paste(void);
int termcap_disable_bracketed_paste(void);

// Alternate screen buffer
int termcap_enter_alternate_screen(void);
int termcap_exit_alternate_screen(void);

// Terminal title management
int termcap_set_title(const char *title);
int termcap_push_title(void);
int termcap_pop_title(void);

// Focus events
int termcap_enable_focus_events(void);
int termcap_disable_focus_events(void);
```

### 5. Built-in Terminal Database

Instead of external terminfo/termcap files, implement a built-in capability database:

```c
// Built-in terminal database
typedef struct {
    const char *name;
    const char *pattern;  // For pattern matching
    terminal_capabilities_t caps;
} terminal_db_entry_t;

// Common terminal profiles
static const terminal_db_entry_t terminal_database[] = {
    {
        .name = "xterm",
        .pattern = "xterm*",
        .caps = {
            .colors = true,
            .colors_256 = true,
            .truecolor = false,
            .mouse = true,
            .bracketed_paste = true,
            .alternate_screen = true,
            .unicode = true
        }
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
            .alternate_screen = true,
            .unicode = true
        }
    },
    // ... more entries
};
```

### 6. Performance-Optimized Implementation

```c
// Efficient escape sequence generation
typedef struct {
    char buffer[64];
    size_t length;
} escape_sequence_t;

// Pre-compiled escape sequences for performance
static const escape_sequence_t ESC_CLEAR_SCREEN = {"\x1b[2J\x1b[H", 7};
static const escape_sequence_t ESC_CURSOR_HOME = {"\x1b[H", 3};
static const escape_sequence_t ESC_HIDE_CURSOR = {"\x1b[?25l", 6};
static const escape_sequence_t ESC_SHOW_CURSOR = {"\x1b[?25h", 6};

// Batch output for efficiency
typedef struct {
    char *buffer;
    size_t size;
    size_t used;
} termcap_output_buffer_t;

int termcap_output_buffer_init(termcap_output_buffer_t *buf, size_t size);
int termcap_output_buffer_add(termcap_output_buffer_t *buf, const char *data, size_t len);
int termcap_output_buffer_flush(termcap_output_buffer_t *buf);
void termcap_output_buffer_cleanup(termcap_output_buffer_t *buf);
```

## Implementation Plan

### Phase 1: Core Enhancements (1 day)
1. **Enhanced color support** - 16 colors, 256 colors, true color detection
2. **Advanced cursor operations** - precise movement, visibility control
3. **Screen management** - clearing, scrolling, line operations
4. **Capability detection** - comprehensive terminal feature detection

### Phase 2: Advanced Features (1 day)
1. **Built-in terminal database** - common terminal profiles
2. **Mouse support** - click and drag detection
3. **Bracketed paste mode** - safe paste handling
4. **Alternate screen buffer** - full-screen applications support

### Phase 3: Performance Optimization (0.5 days)
1. **Escape sequence optimization** - pre-compiled sequences
2. **Batched output** - reduce system calls
3. **Smart caching** - capability detection caching
4. **Timeout optimization** - adaptive query timeouts

## Integration with Linenoise

Enhanced termcap will provide linenoise with:

```c
// Enhanced linenoise integration points
int linenoise_set_color_scheme(termcap_color_t prompt_color, 
                              termcap_color_t input_color,
                              termcap_color_t completion_color);
int linenoise_enable_mouse_selection(bool enable);
int linenoise_set_bracketed_paste(bool enable);
```

## Benefits

### For Users
- **Rich prompt colors and styling** - professional appearance
- **Mouse support in completion menus** - improved usability  
- **Safer paste operations** - bracketed paste prevents accidents
- **Better terminal compatibility** - works optimally across terminals
- **Full-screen capabilities** - enables advanced shell features

### For Developers
- **Clean API** - simple, consistent interface
- **Zero dependencies** - maintains lusush philosophy
- **High performance** - optimized for shell responsiveness
- **Easy integration** - drop-in enhancement to existing code
- **Comprehensive testing** - works across all major terminals

## Compatibility Matrix

| Terminal | Colors | 256 Colors | True Color | Mouse | Paste | Alt Screen |
|----------|--------|------------|------------|-------|-------|------------|
| xterm    | ✅     | ✅         | ❌         | ✅    | ✅    | ✅         |
| GNOME    | ✅     | ✅         | ✅         | ✅    | ✅    | ✅         |
| iTerm2   | ✅     | ✅         | ✅         | ✅    | ✅    | ✅         |
| tmux     | ✅     | ✅         | ✅*        | ✅    | ✅    | ✅         |
| screen   | ✅     | ✅         | ❌         | ❌    | ✅    | ✅         |
| Konsole  | ✅     | ✅         | ✅         | ✅    | ✅    | ✅         |
| Terminal | ✅     | ✅         | ✅         | ✅    | ✅    | ✅         |

*Depends on tmux version and configuration

## Testing Strategy

```c
// Comprehensive test suite
int termcap_test_basic_operations(void);
int termcap_test_color_support(void);
int termcap_test_cursor_operations(void);
int termcap_test_screen_management(void);
int termcap_test_capability_detection(void);
int termcap_test_performance_benchmarks(void);
int termcap_test_terminal_compatibility(void);

// Interactive test program
int termcap_interactive_demo(void);
```

## File Structure

```
lusush/src/termcap/
├── termcap_core.c           # Core functionality
├── termcap_colors.c         # Color and attribute support  
├── termcap_cursor.c         # Cursor operations
├── termcap_screen.c         # Screen management
├── termcap_detect.c         # Capability detection
├── termcap_database.c       # Built-in terminal database
├── termcap_performance.c    # Performance optimizations
└── termcap_test.c          # Test suite

lusush/include/
├── termcap.h               # Enhanced public API
└── termcap_internal.h      # Internal definitions
```

## Migration Strategy

1. **Backward Compatibility** - All existing termcap functions remain unchanged
2. **Gradual Enhancement** - New features added incrementally
3. **Feature Detection** - Runtime capability detection prevents issues
4. **Fallback Support** - Graceful degradation on limited terminals
5. **Testing** - Comprehensive test suite ensures stability

## Conclusion

This enhancement proposal provides lusush with enterprise-grade terminal capabilities while maintaining its core philosophy of simplicity and zero dependencies. The implementation leverages modern terminal standards while providing fallbacks for legacy systems.

**Estimated Implementation Time: 2-3 days**
**License: MIT (internal enhancement)**
**Dependencies: None**
**Risk Level: Low (backward compatible)**

The enhanced termcap system will position lusush as a modern, feature-rich shell with professional-grade terminal handling capabilities, rivaling or exceeding those of major shells while maintaining its lightweight, portable design.