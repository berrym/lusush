# Lusush Line Editor (LLE) Development Task Breakdown

## Overview

This document breaks down the development of the Lusush Line Editor (LLE) into the smallest possible concrete development tasks. Each task is designed to be:

- **Implementable**: Can be completed in 1-4 hours
- **Testable**: Has clear acceptance criteria and tests
- **Trackable**: Can be tracked in git with meaningful commits
- **Atomic**: Can be developed, tested, and committed independently

## Development Workflow

### Git Branch Strategy
```bash
# Create feature branch for LLE development
git checkout -b feature/lusush-line-editor

# For each task, create sub-branches (optional but recommended)
git checkout -b task/lle-001-text-buffer
# ... implement task ...
git commit -m "LLE-001: Implement basic text buffer structure"
git checkout feature/lusush-line-editor
git merge task/lle-001-text-buffer
```

### Task Completion Workflow
1. **Implement** the specific task functionality
2. **Write tests** to verify the implementation
3. **Run tests** to ensure they pass
4. **Update documentation** if needed
5. **Commit changes** with task ID and description
6. **Move to next task**

---

## Phase 1: Foundation (Weeks 1-2)

### 1.1 Text Engine Foundation

#### Task LLE-001: Basic Text Buffer Structure
**File**: `src/line_editor/text_buffer.h`
**Estimated Time**: 2 hours
**Description**: Define the core text buffer data structure.

**Implementation**:
```c
typedef struct {
    char *buffer;           // UTF-8 text buffer
    size_t length;          // Current text length (bytes)
    size_t capacity;        // Buffer capacity (bytes)
    size_t cursor_pos;      // Cursor position (byte offset)
    size_t char_count;      // Number of Unicode characters
} lle_text_buffer_t;
```

**Acceptance Criteria**:
- [ ] Header file created with proper structure definition
- [ ] All fields documented with comments
- [ ] Include guards properly implemented
- [ ] Compatible with existing Lusush code style

**Tests**: Basic compilation test with `meson compile -C builddir`
**Commit**: `LLE-001: Define basic text buffer structure`

---

#### Task LLE-002: Text Buffer Initialization
**File**: `src/line_editor/text_buffer.c`
**Estimated Time**: 3 hours
**Description**: Implement text buffer creation, initialization, and destruction.

**Implementation**:
```c
lle_text_buffer_t* lle_text_buffer_create(size_t initial_capacity);
bool lle_text_buffer_init(lle_text_buffer_t *buffer, size_t initial_capacity);
void lle_text_buffer_destroy(lle_text_buffer_t *buffer);
void lle_text_buffer_clear(lle_text_buffer_t *buffer);
```

**Acceptance Criteria**:
- [ ] Functions handle NULL pointers gracefully
- [ ] Memory allocation/deallocation works correctly
- [ ] Initial capacity respected (minimum 256 bytes)
- [ ] Buffer properly zero-initialized

**Tests**:
- Create/destroy buffer successfully
- Initialize with various capacities
- Clear buffer properly
- Handle edge cases (zero capacity, NULL pointers)
- Run with `meson test -C builddir test_text_buffer`

**Commit**: `LLE-002: Implement text buffer initialization and cleanup`

---

#### Task LLE-003: Basic Text Insertion
**File**: `src/line_editor/text_buffer.c`
**Estimated Time**: 4 hours
**Description**: Implement basic text insertion functionality.

**Implementation**:
```c
bool lle_text_insert_char(lle_text_buffer_t *buffer, char c);
bool lle_text_insert_string(lle_text_buffer_t *buffer, const char *str);
bool lle_text_insert_at(lle_text_buffer_t *buffer, size_t pos, const char *str);
```

**Acceptance Criteria**:
- [ ] Insertion at cursor position works
- [ ] Insertion at arbitrary position works
- [ ] Buffer automatically resizes when needed
- [ ] Cursor position updated correctly
- [ ] ASCII characters handled correctly

**Tests**:
- Insert single characters
- Insert strings
- Insert at beginning, middle, end
- Test buffer resizing
- Verify cursor position updates
- Run with `meson test -C builddir test_text_buffer`

**Commit**: `LLE-003: Implement basic text insertion functionality`

---

#### Task LLE-004: Basic Text Deletion
**File**: `src/line_editor/text_buffer.c`
**Estimated Time**: 3 hours
**Description**: Implement text deletion functionality.

**Implementation**:
```c
bool lle_text_delete_char(lle_text_buffer_t *buffer);  // Delete at cursor
bool lle_text_backspace(lle_text_buffer_t *buffer);    // Delete before cursor
bool lle_text_delete_range(lle_text_buffer_t *buffer, size_t start, size_t end);
```

**Acceptance Criteria**:
- [ ] Delete character at cursor position
- [ ] Backspace (delete before cursor)
- [ ] Delete arbitrary ranges
- [ ] Cursor position updated correctly
- [ ] Handle edge cases (empty buffer, invalid positions)

**Tests**:
- Delete single characters
- Backspace functionality
- Delete ranges
- Edge cases (empty buffer, bounds checking)
- Run with `meson test -C builddir test_text_buffer`

**Commit**: `LLE-004: Implement basic text deletion functionality`

---

#### Task LLE-005: Cursor Movement
**File**: `src/line_editor/text_buffer.c`
**Estimated Time**: 3 hours
**Description**: Implement cursor movement within text buffer.

**Implementation**:
```c
typedef enum {
    LLE_MOVE_LEFT,
    LLE_MOVE_RIGHT,
    LLE_MOVE_HOME,
    LLE_MOVE_END,
    LLE_MOVE_WORD_LEFT,
    LLE_MOVE_WORD_RIGHT
} lle_cursor_movement_t;

bool lle_text_move_cursor(lle_text_buffer_t *buffer, lle_cursor_movement_t movement);
bool lle_text_set_cursor(lle_text_buffer_t *buffer, size_t position);
```

**Acceptance Criteria**:
- [ ] Cursor moves left/right correctly
- [ ] Home/End functionality works
- [ ] Word-based movement implemented
- [ ] Bounds checking prevents invalid positions
- [ ] Position setting works correctly

**Tests**:
- All movement types
- Bounds checking
- Word movement accuracy
- Position setting
- Run with `meson test -C builddir test_text_buffer`

**Commit**: `LLE-005: Implement cursor movement functionality`

---

### 1.2 Mathematical Framework

#### Task LLE-006: Cursor Position Structure
**File**: `src/line_editor/cursor_math.h`
**Estimated Time**: 2 hours
**Description**: Define cursor position calculation structures.

**Implementation**:
```c
typedef struct {
    size_t absolute_row;    // Terminal row (0-based)
    size_t absolute_col;    // Terminal column (0-based)
    size_t relative_row;    // Row relative to prompt start
    size_t relative_col;    // Column within current line
    bool at_boundary;       // At line wrap boundary
    bool valid;             // Position calculation valid
} lle_cursor_position_t;

typedef struct {
    size_t width;           // Terminal width in characters
    size_t height;          // Terminal height in characters
    size_t prompt_width;    // Width of current prompt
    size_t prompt_height;   // Height of current prompt
} lle_terminal_geometry_t;
```

**Acceptance Criteria**:
- [ ] Structure properly documented
- [ ] All fields have clear semantics
- [ ] Compatible with terminal geometry systems

**Tests**: Compilation and structure validation with `meson compile -C builddir`
**Commit**: `LLE-006: Define cursor position calculation structures`

---

#### Task LLE-007: Basic Cursor Position Calculation
**File**: `src/line_editor/cursor_math.c`
**Estimated Time**: 4 hours
**Description**: Implement basic cursor position calculations.

**Implementation**:
```c
lle_cursor_position_t lle_calculate_cursor_position(
    const lle_text_buffer_t *buffer,
    const lle_terminal_geometry_t *geometry,
    size_t prompt_width
);

bool lle_validate_cursor_position(const lle_cursor_position_t *pos);
```

**Acceptance Criteria**:
- [ ] Calculates absolute position correctly
- [ ] Calculates relative position correctly
- [ ] Handles line wrapping
- [ ] Detects boundary conditions
- [ ] Input validation works

**Tests**:
- Various text lengths and cursor positions
- Different terminal widths
- Line wrapping scenarios
- Edge cases (empty text, zero width)
- Run with `meson test -C builddir test_cursor_math`

**Commit**: `LLE-007: Implement basic cursor position calculation`

---

#### Task LLE-008: Prompt Geometry Calculation
**File**: `src/line_editor/cursor_math.c`
**Estimated Time**: 3 hours
**Description**: Calculate prompt geometry for multiline prompts.

**Implementation**:
```c
typedef struct {
    size_t width;           // Total prompt width
    size_t height;          // Total prompt height
    size_t last_line_width; // Width of last prompt line
} lle_prompt_geometry_t;

lle_prompt_geometry_t lle_calculate_prompt_geometry(
    const char *prompt,
    const lle_terminal_geometry_t *terminal
);
```

**Acceptance Criteria**:
- [ ] Calculates multiline prompt dimensions
- [ ] Handles ANSI escape sequences correctly
- [ ] Accounts for line wrapping
- [ ] Returns accurate measurements

**Tests**:
- Single line prompts
- Multiline prompts
- Prompts with ANSI codes
- Various terminal widths
- Run with `meson test -C builddir test_cursor_math`

**Commit**: `LLE-008: Implement prompt geometry calculation`

---

### 1.3 Basic Terminal Integration

#### Task LLE-009: Termcap System Integration
**File**: `src/line_editor/termcap/` (multiple files)
**Estimated Time**: 6 hours
**Description**: Transfer and integrate Lusush termcap system into LLE as standalone component.

**Implementation**:
```c
// Transfer from lusush/src/termcap.c → src/line_editor/termcap/lle_termcap.c
// Transfer from lusush/include/termcap.h → src/line_editor/termcap/lle_termcap.h
// Namespace all functions with lle_ prefix

typedef struct {
    lle_termcap_context_t *termcap;
    lle_terminal_geometry_t geometry;
    bool initialized;
} lle_terminal_manager_t;

// Namespaced termcap functions
bool lle_termcap_init(void);
const lle_terminal_info_t *lle_termcap_get_info(void);
bool lle_termcap_is_iterm2(void);
```

**Acceptance Criteria**:
- [ ] Complete termcap system transferred (2000+ lines)
- [ ] All functions namespaced with lle_ prefix
- [ ] iTerm2 and macOS support preserved
- [ ] Terminal database integrated (50+ terminals)
- [ ] No external termcap dependencies

**Tests**: Full termcap test suite transferred and passing
**Commit**: `LLE-009: Integrate Lusush termcap system as standalone component`

---

#### Task LLE-010: Terminal Manager Implementation
**File**: `src/line_editor/terminal_manager.c`
**Estimated Time**: 3 hours
**Description**: Implement terminal manager using integrated termcap system.

**Implementation**:
```c
bool lle_terminal_init(lle_terminal_manager_t *tm) {
    // Initialize integrated termcap system
    if (!lle_termcap_init()) return false;
    
    // Detect capabilities using Lusush termcap system
    if (!lle_termcap_detect_capabilities()) return false;
    
    // Get terminal info from termcap
    tm->termcap = lle_termcap_get_context();
    
    // Set up LLE-specific geometry calculations
    return lle_terminal_update_geometry(tm);
}
```

**Acceptance Criteria**:
- [ ] Uses integrated termcap for all terminal operations
- [ ] Leverages existing iTerm2 detection
- [ ] Maintains all Lusush terminal compatibility
- [ ] Integrates with LLE geometry calculations

**Tests**:
- Termcap integration verification
- iTerm2 detection working
- All terminal profiles supported
- Run with `meson test -C builddir test_terminal_manager`

**Commit**: `LLE-010: Implement terminal manager using integrated termcap`

---

#### Task LLE-011: Terminal Output Integration
**File**: `src/line_editor/terminal_manager.c`
**Estimated Time**: 2 hours
**Description**: Implement terminal output using integrated termcap functions.

**Implementation**:
```c
// Use integrated termcap functions with LLE wrappers
bool lle_terminal_write(lle_terminal_manager_t *tm, const char *data, size_t length);
bool lle_terminal_move_cursor(lle_terminal_manager_t *tm, size_t row, size_t col) {
    return lle_termcap_move_cursor(row, col);
}
bool lle_terminal_clear_line(lle_terminal_manager_t *tm) {
    return lle_termcap_clear_line();
}
bool lle_terminal_set_color(lle_terminal_manager_t *tm, lle_termcap_color_t fg, lle_termcap_color_t bg) {
    return lle_termcap_set_color(fg, bg);
}
```

**Acceptance Criteria**:
- [ ] Writes data to terminal correctly
- [ ] Cursor movement works
- [ ] Line/screen clearing functions
- [ ] Error handling for write failures

**Tests**:
- Write various text strings
- Cursor movement to different positions
- Clear operations
- Error condition handling
- Run with `meson test -C builddir test_terminal_manager`

**Commit**: `LLE-011: Implement basic terminal output functions`

---

### 1.4 Testing Infrastructure

#### Task LLE-012: Test Framework Setup
**File**: `tests/line_editor/test_framework.h`
**Estimated Time**: 3 hours
**Description**: Set up testing framework for line editor components.

**Implementation**:
```c
// Simple test framework
#define LLE_TEST(name) void test_##name(void)
#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            abort(); \
        } \
    } while(0)

#define LLE_ASSERT_EQ(a, b) LLE_ASSERT((a) == (b))
#define LLE_ASSERT_STR_EQ(a, b) LLE_ASSERT(strcmp((a), (b)) == 0)

// Test runner
void lle_run_all_tests(void);
```

**Acceptance Criteria**:
- [ ] Basic assertion macros work
- [ ] Test runner infrastructure
- [ ] Clear failure reporting
- [ ] Easy to add new tests

**Tests**: Test the test framework itself with `meson test -C builddir`
**Commit**: `LLE-012: Set up line editor test framework`

---

#### Task LLE-013: Text Buffer Tests
**File**: `tests/line_editor/test_text_buffer.c`
**Estimated Time**: 4 hours
**Description**: Comprehensive tests for text buffer functionality.

**Implementation**:
- Test all text buffer operations
- Edge case testing
- Memory leak detection
- Performance basic validation

**Acceptance Criteria**:
- [ ] All text buffer functions tested
- [ ] Edge cases covered
- [ ] Memory management verified
- [ ] Tests pass consistently

**Tests**: Meta-test - testing the tests with `meson test -C builddir test_text_buffer`
**Commit**: `LLE-013: Implement comprehensive text buffer tests`

---

#### Task LLE-014: Cursor Math Tests
**File**: `tests/line_editor/test_cursor_math.c`
**Estimated Time**: 3 hours
**Description**: Tests for cursor position calculations.

**Implementation**:
- Test position calculations
- Test geometry calculations
- Test edge cases and boundary conditions
- Test various terminal sizes

**Acceptance Criteria**:
- [ ] Position calculations verified
- [ ] Geometry calculations correct
- [ ] Edge cases handled
- [ ] Various scenarios tested

**Tests**: Verify mathematical correctness with `meson test -C builddir test_cursor_math`
**Commit**: `LLE-014: Implement cursor mathematics tests`

---

## Phase 2: Core Functionality (Weeks 3-4)

### 2.1 Multiline Prompt Support

#### Task LLE-015: Prompt Structure Definition
**File**: `src/line_editor/prompt.h`
**Estimated Time**: 2 hours
**Description**: Define structures for handling complex prompts.

**Implementation**:
```c
typedef struct {
    char *text;                    // Full prompt text
    size_t length;                 // Text length in bytes
    lle_prompt_geometry_t geometry; // Calculated geometry
    bool has_ansi_codes;           // Contains ANSI escape sequences
    char **lines;                  // Split into lines
    size_t line_count;             // Number of lines
} lle_prompt_t;
```

**Acceptance Criteria**:
- [ ] Structure supports multiline prompts
- [ ] ANSI code handling planned
- [ ] Geometry integration ready

**Tests**: Structure compilation and basic usage
**Commit**: `LLE-015: Define prompt handling structures`

---

#### Task LLE-016: Prompt Parsing
**File**: `src/line_editor/prompt.c`
**Estimated Time**: 4 hours
**Description**: Parse and analyze prompt structure.

**Implementation**:
```c
bool lle_prompt_parse(lle_prompt_t *prompt, const char *text);
bool lle_prompt_split_lines(lle_prompt_t *prompt);
bool lle_prompt_strip_ansi(const char *input, char *output, size_t output_size);
size_t lle_prompt_display_width(const char *text);
```

**Acceptance Criteria**:
- [ ] Parses multiline prompts correctly
- [ ] Handles ANSI escape sequences
- [ ] Calculates display width accurately
- [ ] Splits into lines properly

**Tests**:
- Simple prompts
- Multiline prompts
- Prompts with ANSI codes
- Various prompt formats

**Commit**: `LLE-016: Implement prompt parsing functionality`

---

#### Task LLE-017: Prompt Rendering
**File**: `src/line_editor/prompt.c`
**Estimated Time**: 4 hours
**Description**: Render prompts to terminal.

**Implementation**:
```c
bool lle_prompt_render(
    lle_terminal_manager_t *tm,
    const lle_prompt_t *prompt,
    bool clear_previous
);

bool lle_prompt_position_cursor(
    lle_terminal_manager_t *tm,
    const lle_prompt_t *prompt,
    const lle_cursor_position_t *cursor_pos
);
```

**Acceptance Criteria**:
- [ ] Renders single line prompts
- [ ] Renders multiline prompts correctly
- [ ] Positions cursor accurately
- [ ] Handles terminal wrapping

**Tests**:
- Various prompt types
- Different terminal sizes
- Cursor positioning accuracy

**Commit**: `LLE-017: Implement prompt rendering functionality`

---

#### Task LLE-018: Multiline Input Display
**File**: `src/line_editor/display.c`
**Estimated Time**: 4 hours
**Description**: Display multiline input text with prompt.

**Implementation**:
```c
typedef struct {
    lle_prompt_t *prompt;
    lle_text_buffer_t *buffer;
    lle_terminal_manager_t *terminal;
    lle_cursor_position_t cursor_pos;
    bool needs_refresh;
} lle_display_state_t;

bool lle_display_render(lle_display_state_t *state);
bool lle_display_update_cursor(lle_display_state_t *state);
```

**Acceptance Criteria**:
- [ ] Displays prompt and input text
- [ ] Handles multiline input correctly
- [ ] Updates cursor position accurately
- [ ] Efficient redraw strategy

**Tests**:
- Single and multiline input
- Various cursor positions
- Text wrapping scenarios

**Commit**: `LLE-018: Implement multiline input display`

---

### 2.2 Theme Integration

#### Task LLE-019: Theme Interface Definition
**File**: `src/line_editor/theme_integration.h`
**Estimated Time**: 2 hours
**Description**: Define interface for theme integration.

**Implementation**:
```c
typedef struct {
    const char *name;
    const char *prompt_color;
    const char *input_color;
    const char *cursor_color;
    const char *highlight_color;
} lle_theme_colors_t;

typedef struct {
    lle_theme_colors_t colors;
    bool theme_active;
    char *theme_name;
} lle_theme_integration_t;
```

**Acceptance Criteria**:
- [ ] Interface matches Lusush theme system
- [ ] Color definitions complete
- [ ] Integration points identified

**Tests**: Compilation and interface validation
**Commit**: `LLE-019: Define theme integration interface`

---

#### Task LLE-020: Basic Theme Application
**File**: `src/line_editor/theme_integration.c`
**Estimated Time**: 3 hours
**Description**: Apply themes to line editor display.

**Implementation**:
```c
bool lle_theme_init(lle_theme_integration_t *ti);
bool lle_theme_apply(lle_theme_integration_t *ti, const char *theme_name);
const char *lle_theme_get_color(lle_theme_integration_t *ti, const char *element);
bool lle_theme_colorize_prompt(lle_theme_integration_t *ti, lle_prompt_t *prompt);
```

**Acceptance Criteria**:
- [ ] Loads theme configurations
- [ ] Applies colors to prompts
- [ ] Integrates with existing theme system
- [ ] Handles theme changes

**Tests**:
- Theme loading and application
- Color retrieval
- Prompt colorization

**Commit**: `LLE-020: Implement basic theme application`

---

### 2.3 Basic Editing Operations

#### Task LLE-021: Key Input Handling
**File**: `src/line_editor/input_handler.h`
**Estimated Time**: 2 hours
**Description**: Define key input handling structures.

**Implementation**:
```c
typedef enum {
    LLE_KEY_CHAR,
    LLE_KEY_BACKSPACE,
    LLE_KEY_DELETE,
    LLE_KEY_ENTER,
    LLE_KEY_TAB,
    LLE_KEY_ARROW_LEFT,
    LLE_KEY_ARROW_RIGHT,
    LLE_KEY_ARROW_UP,
    LLE_KEY_ARROW_DOWN,
    LLE_KEY_HOME,
    LLE_KEY_END,
    LLE_KEY_CTRL_C,
    LLE_KEY_CTRL_D,
    LLE_KEY_UNKNOWN
} lle_key_type_t;

typedef struct {
    lle_key_type_t type;
    char character;
    bool ctrl;
    bool alt;
    bool shift;
} lle_key_event_t;
```

**Acceptance Criteria**:
- [ ] Covers all essential keys
- [ ] Supports modifier keys
- [ ] Extensible for future keys

**Tests**: Compilation and structure validation
**Commit**: `LLE-021: Define key input handling structures`

---

#### Task LLE-022: Key Event Processing
**File**: `src/line_editor/input_handler.c`
**Estimated Time**: 4 hours
**Description**: Process raw input into key events.

**Implementation**:
```c
bool lle_input_read_key(lle_terminal_manager_t *tm, lle_key_event_t *event);
bool lle_input_parse_escape_sequence(const char *seq, lle_key_event_t *event);
bool lle_input_is_printable(const lle_key_event_t *event);
```

**Acceptance Criteria**:
- [ ] Reads keys from terminal correctly
- [ ] Parses escape sequences for special keys
- [ ] Identifies printable characters
- [ ] Handles multi-byte sequences

**Tests**:
- Various key types
- Escape sequence parsing
- Multi-byte character handling

**Commit**: `LLE-022: Implement key event processing`

---

#### Task LLE-023: Basic Editing Commands
**File**: `src/line_editor/edit_commands.c`
**Estimated Time**: 4 hours
**Description**: Implement basic editing command handling.

**Implementation**:
```c
typedef enum {
    LLE_CMD_INSERT_CHAR,
    LLE_CMD_DELETE_CHAR,
    LLE_CMD_BACKSPACE,
    LLE_CMD_MOVE_CURSOR,
    LLE_CMD_ACCEPT_LINE,
    LLE_CMD_CANCEL_LINE
} lle_command_type_t;

bool lle_execute_command(
    lle_display_state_t *state,
    lle_command_type_t cmd,
    const void *data
);
```

**Acceptance Criteria**:
- [ ] Handles character insertion
- [ ] Handles deletion operations
- [ ] Handles cursor movement
- [ ] Updates display appropriately

**Tests**:
- All command types
- Display update verification
- Command chaining

**Commit**: `LLE-023: Implement basic editing commands`

---

### 2.4 History System

#### Task LLE-024: History Structure
**File**: `src/line_editor/history.h`
**Estimated Time**: 2 hours
**Description**: Define command history data structures.

**Implementation**:
```c
typedef struct {
    char **entries;        // History entries
    size_t count;          // Number of entries
    size_t capacity;       // Array capacity
    size_t current;        // Current position for navigation
    size_t max_entries;    // Maximum entries to keep
} lle_history_t;
```

**Acceptance Criteria**:
- [ ] Supports configurable history size
- [ ] Efficient navigation structure
- [ ] Memory management planned

**Tests**: Structure compilation and validation
**Commit**: `LLE-024: Define command history structures`

---

#### Task LLE-025: History Management
**File**: `src/line_editor/history.c`
**Estimated Time**: 4 hours
**Description**: Implement history management functions.

**Implementation**:
```c
lle_history_t *lle_history_create(size_t max_entries);
void lle_history_destroy(lle_history_t *history);
bool lle_history_add(lle_history_t *history, const char *entry);
const char *lle_history_get(lle_history_t *history, size_t index);
bool lle_history_save(lle_history_t *history, const char *filename);
bool lle_history_load(lle_history_t *history, const char *filename);
```

**Acceptance Criteria**:
- [ ] Adds entries correctly
- [ ] Manages memory efficiently
- [ ] Handles capacity limits
- [ ] Supports persistence

**Tests**:
- Add/retrieve entries
- Capacity management
- File save/load operations

**Commit**: `LLE-025: Implement history management functions`

---

#### Task LLE-026: History Navigation
**File**: `src/line_editor/history.c`
**Estimated Time**: 3 hours
**Description**: Implement history navigation functionality.

**Implementation**:
```c
const char *lle_history_prev(lle_history_t *history);
const char *lle_history_next(lle_history_t *history);
bool lle_history_reset_position(lle_history_t *history);
size_t lle_history_get_position(lle_history_t *history);
bool lle_history_set_position(lle_history_t *history, size_t position);
```

**Acceptance Criteria**:
- [ ] Navigates backward/forward correctly
- [ ] Handles bounds properly
- [ ] Maintains current position
- [ ] Integrates with editing

**Tests**:
- Navigation in both directions
- Boundary conditions
- Position management

**Commit**: `LLE-026: Implement history navigation functionality`

---

## Phase 3: Advanced Features (Weeks 5-6)

### 3.1 Unicode Support

#### Task LLE-027: UTF-8 Text Handling
**File**: `src/line_editor/unicode.h`
**Estimated Time**: 3 hours
**Description**: Add proper UTF-8 support to text operations.

**Implementation**:
```c
typedef struct {
    size_t byte_length;    // Length in bytes
    size_t char_length;    // Length in Unicode characters
    bool valid_utf8;       // Valid UTF-8 encoding
} lle_utf8_info_t;

lle_utf8_info_t lle_utf8_analyze(const char *text);
size_t lle_utf8_char_at(const char *text, size_t char_index);
size_t lle_utf8_next_char(const char *text, size_t byte_pos);
size_t lle_utf8_prev_char(const char *text, size_t byte_pos);
```

**Acceptance Criteria**:
- [ ] Correctly handles UTF-8 encoding
- [ ] Character vs byte position distinction
- [ ] Navigation works with multibyte characters
- [ ] Validation functions work

**Tests**:
- ASCII text
- UTF-8 text with various characters
- Invalid UTF-8 sequences
- Navigation accuracy

**Commit**: `LLE-027: Implement UTF-8 text handling support`

---

#### Task LLE-028: Unicode Cursor Movement
**File**: `src/line_editor/text_buffer.c`
**Estimated Time**: 3 hours
**Description**: Update cursor movement to handle Unicode correctly.

**Implementation**:
- Update existing cursor movement functions
- Add Unicode-aware word boundary detection
- Fix character counting and positioning

**Acceptance Criteria**:
- [ ] Cursor moves by characters, not bytes
- [ ] Word boundaries work with Unicode
- [ ] Position calculations correct
- [ ] Display width calculated properly

**Tests**:
- Movement through Unicode text
- Word boundary detection
- Position accuracy

**Commit**: `LLE-028: Update cursor movement for Unicode support`

---

### 3.2 Completion Engine

#### Task LLE-029: Completion Framework
**File**: `src/line_editor/completion.h`
**Estimated Time**: 3 hours
**Description**: Define completion system architecture.

**Implementation**:
```c
typedef struct {
    char *text;            // Completion text
    char *description;     // Optional description
    int priority;          // Completion priority
} lle_completion_item_t;

typedef struct {
    lle_completion_item_t *items;
    size_t count;
    size_t capacity;
    size_t selected;       // Currently selected item
} lle_completion_list_t;

typedef bool (*lle_completion_provider_t)(
    const char *input,
    size_t cursor_pos,
    lle_completion_list_t *completions
);
```

**Acceptance Criteria**:
- [ ] Flexible completion item structure
- [ ] Support for multiple providers
- [ ] Selection mechanism defined
- [ ] Extensible architecture

**Tests**: Structure compilation and basic usage
**Commit**: `LLE-029: Define completion system framework`

---

#### Task LLE-030: Basic File Completion
**File**: `src/line_editor/completion.c`
**Estimated Time**: 4 hours
**Description**: Implement file and directory completion.

**Implementation**:
```c
bool lle_complete_files(
    const char *input,
    size_t cursor_pos,
    lle_completion_list_t *completions
);

bool lle_completion_extract_word(
    const char *input,
    size_t cursor_pos,
    char *word,
    size_t word_size,
    size_t *word_start
);
```

**Acceptance Criteria**:
- [ ] Completes file names correctly
- [ ] Handles directories appropriately
- [ ] Supports partial matches
- [ ] Efficient word extraction

**Tests**:
- File completion in various directories
- Partial name matching
- Directory vs file handling

**Commit**: `LLE-030: Implement basic file completion`

---

#### Task LLE-031: Completion Display
**File**: `src/line_editor/completion_display.c`
**Estimated Time**: 4 hours
**Description**: Display completion candidates to user.

**Implementation**:
```c
typedef struct {
    lle_completion_list_t *completions;
    size_t display_start;     // First visible item
    size_t display_count;     // Number of visible items
    bool show_descriptions;   // Show item descriptions
} lle_completion_display_t;

bool lle_completion_show(
    lle_terminal_manager_t *tm,
    lle_completion_display_t *display
);

bool lle_completion_navigate(
    lle_completion_display_t *display,
    int direction  // -1 for up, 1 for down
);
```

**Acceptance Criteria**:
- [ ] Displays completion options clearly
- [ ] Handles scrolling for many items
- [ ] Shows selection indicator
- [ ] Integrates with main display

**Tests**:
- Various numbers of completions
- Navigation testing
- Display formatting

**Commit**: `LLE-031: Implement completion display system`

---

### 3.3 Undo/Redo System

#### Task LLE-032: Undo Stack Structure
**File**: `src/line_editor/undo.h`
**Estimated Time**: 2 hours
**Description**: Define undo/redo data structures.

**Implementation**:
```c
typedef enum {
    LLE_UNDO_INSERT,
    LLE_UNDO_DELETE,
    LLE_UNDO_MOVE_CURSOR,
    LLE_UNDO_REPLACE
} lle_undo_action_type_t;

typedef struct {
    lle_undo_action_type_t type;
    size_t position;       // Where action occurred
    char *text;           // Text involved in action
    size_t length;        // Length of text
    size_t old_cursor;    // Cursor position before action
} lle_undo_action_t;

typedef struct {
    lle_undo_action_t *actions;
    size_t count;
    size_t capacity;
    size_t current;       // Current undo position
    size_t max_actions;   // Maximum actions to keep
} lle_undo_stack_t;
```

**Acceptance Criteria**:
- [ ] Supports all editing operations
- [ ] Efficient action storage
- [ ] Configurable stack size
- [ ] Cursor position tracking

**Tests**: Structure compilation and basic operations
**Commit**: `LLE-032: Define undo/redo system structures`

---

#### Task LLE-033: Undo Operation Recording
**File**: `src/line_editor/undo.c`
**Estimated Time**: 4 hours
**Description**: Record editing operations for undo functionality.

**Implementation**:
```c
lle_undo_stack_t *lle_undo_create(size_t max_actions);
void lle_undo_destroy(lle_undo_stack_t *stack);
bool lle_undo_record_action(
    lle_undo_stack_t *stack,
    lle_undo_action_type_t type,
    size_t position,
    const char *text,
    size_t old_cursor
);
```

**Acceptance Criteria**:
- [ ] Records all editing operations
- [ ] Manages stack capacity correctly
- [ ] Handles memory efficiently
- [ ] Maintains action order

**Tests**:
- Record various action types
- Stack capacity management
- Memory leak verification

**Commit**: `LLE-033: Implement undo operation recording`

---

#### Task LLE-034: Undo/Redo Execution
**File**: `src/line_editor/undo.c`
**Estimated Time**: 4 hours
**Description**: Execute undo and redo operations.

**Implementation**:
```c
bool lle_undo_execute(
    lle_undo_stack_t *stack,
    lle_text_buffer_t *buffer
);

bool lle_redo_execute(
    lle_undo_stack_t *stack,
    lle_text_buffer_t *buffer
);

bool lle_undo_can_undo(lle_undo_stack_t *stack);
bool lle_undo_can_redo(lle_undo_stack_t *stack);
```

**Acceptance Criteria**:
- [ ] Correctly reverses operations
- [ ] Maintains redo capability
- [ ] Updates cursor position
- [ ] Handles edge cases

**Tests**:
- Undo various operation types
- Redo after undo
- Multiple undo/redo cycles
- Edge case handling

**Commit**: `LLE-034: Implement undo/redo execution`

---

### 3.4 Syntax Highlighting

#### Task LLE-035: Syntax Highlighting Framework
**File**: `src/line_editor/syntax.h`
**Estimated Time**: 3 hours
**Description**: Define syntax highlighting system.

**Implementation**:
```c
typedef enum {
    LLE_SYNTAX_NORMAL,
    LLE_SYNTAX_KEYWORD,
    LLE_SYNTAX_STRING,
    LLE_SYNTAX_COMMENT,
    LLE_SYNTAX_NUMBER,
    LLE_SYNTAX_OPERATOR,
    LLE_SYNTAX_ERROR
} lle_syntax_type_t;

typedef struct {
    size_t start;         // Start position in text
    size_t length;        // Length of highlighted region
    lle_syntax_type_t type; // Type of syntax element
} lle_syntax_region_t;

typedef struct {
    lle_syntax_region_t *regions;
    size_t count;
    size_t capacity;
} lle_syntax_highlight_t;
```

**Acceptance Criteria**:
- [ ] Extensible syntax types
- [ ] Efficient region storage
- [ ] Integration with display system
- [ ] Real-time highlighting support

**Tests**: Structure validation and basic usage
**Commit**: `LLE-035: Define syntax highlighting framework`

---

#### Task LLE-036: Basic Shell Syntax
**File**: `src/line_editor/syntax.c`
**Estimated Time**: 4 hours
**Description**: Implement basic shell command syntax highlighting.

**Implementation**:
```c
bool lle_syntax_highlight_shell(
    const char *text,
    lle_syntax_highlight_t *highlight
);

bool lle_syntax_is_shell_keyword(const char *word);
bool lle_syntax_parse_string(const char *text, size_t start, size_t *end);
bool lle_syntax_parse_comment(const char *text, size_t start, size_t *end);
```

**Acceptance Criteria**:
- [ ] Highlights shell keywords
- [ ] Identifies strings correctly
- [ ] Handles comments
- [ ] Efficient parsing

**Tests**:
- Various shell commands
- String and comment detection
- Keyword recognition
- Performance with long commands

**Commit**: `LLE-036: Implement basic shell syntax highlighting`

---

#### Task LLE-037: Syntax Display Integration
**File**: `src/line_editor/display.c`
**Estimated Time**: 3 hours
**Description**: Integrate syntax highlighting with display system.

**Implementation**:
- Update display rendering to apply syntax colors
- Add syntax highlighting to display state
- Ensure efficient redraw with highlighting

**Acceptance Criteria**:
- [ ] Displays syntax colors correctly
- [ ] Integrates with theme system
- [ ] Maintains display performance
- [ ] Updates highlighting on text changes

**Tests**:
- Visual verification of highlighting
- Performance with highlighting enabled
- Theme integration
- Real-time updates

**Commit**: `LLE-037: Integrate syntax highlighting with display`

---

## Phase 4: Integration & Polish (Weeks 7-8)

### 4.1 Main Line Editor Interface

#### Task LLE-038: Core Line Editor API
**File**: `src/line_editor/line_editor.h`
**Estimated Time**: 3 hours
**Description**: Define the main line editor public API.

**Implementation**:
```c
typedef struct {
    lle_text_buffer_t *buffer;
    lle_terminal_manager_t *terminal;
    lle_display_state_t *display;
    lle_history_t *history;
    lle_theme_integration_t *theme;
    lle_completion_list_t *completions;
    lle_undo_stack_t *undo_stack;
    bool multiline_mode;
    bool syntax_highlighting;
} lle_line_editor_t;

// Main API functions
lle_line_editor_t *lle_create(void);
void lle_destroy(lle_line_editor_t *editor);
char *lle_readline(lle_line_editor_t *editor, const char *prompt);
bool lle_add_history(lle_line_editor_t *editor, const char *line);
```

**Acceptance Criteria**:
- [ ] Clean, simple public API
- [ ] Encapsulates all components
- [ ] Memory management clear
- [ ] Compatible with existing code

**Tests**: API compilation and basic usage
**Commit**: `LLE-038: Define main line editor public API`

---

#### Task LLE-039: Line Editor Implementation
**File**: `src/line_editor/line_editor.c`
**Estimated Time**: 4 hours
**Description**: Implement the main line editor functionality.

**Implementation**:
```c
lle_line_editor_t *lle_create(void) {
    // Initialize all components
    // Set up default configuration
    // Return configured editor
}

char *lle_readline(lle_line_editor_t *editor, const char *prompt) {
    // Display prompt
    // Enter input loop
    // Process keys and update display
    // Return completed line
}
```

**Acceptance Criteria**:
- [ ] Initializes all components correctly
- [ ] Main input loop functions properly
- [ ] Integrates all features seamlessly
- [ ] Handles errors gracefully

**Tests**:
- Basic line editing functionality
- Integration of all features
- Error handling
- Memory management

**Commit**: `LLE-039: Implement main line editor functionality`

---

#### Task LLE-040: Input Event Loop
**File**: `src/line_editor/line_editor.c`
**Estimated Time**: 4 hours
**Description**: Implement the main input processing loop.

**Implementation**:
```c
static char *lle_input_loop(lle_line_editor_t *editor) {
    lle_key_event_t event;
    bool done = false;
    
    while (!done) {
        // Read key event
        // Process event
        // Update display
        // Check for completion
    }
    
    return lle_text_buffer_to_string(editor->buffer);
}
```

**Acceptance Criteria**:
- [ ] Handles all key events correctly
- [ ] Updates display efficiently
- [ ] Manages state properly
- [ ] Exits cleanly

**Tests**:
- Various input scenarios
- Key event processing
- Display updates
- State management

**Commit**: `LLE-040: Implement main input event loop`

---

### 4.2 Lusush Integration

#### Task LLE-041: Replace Linenoise Integration
**File**: Multiple files (replacing linenoise calls)
**Estimated Time**: 4 hours
**Description**: Replace linenoise calls with LLE throughout Lusush.

**Implementation**:
- Identify all linenoise usage in Lusush
- Replace with LLE API calls
- Update build system
- Ensure compatibility

**Acceptance Criteria**:
- [ ] All linenoise calls replaced
- [ ] Functionality equivalent or better
- [ ] Build system updated
- [ ] No regression in features

**Tests**:
- Full Lusush functionality testing
- Comparison with previous behavior
- Integration testing

**Commit**: `LLE-041: Replace linenoise with LLE throughout Lusush`

---

#### Task LLE-042: Theme System Integration
**File**: `src/line_editor/theme_integration.c`
**Estimated Time**: 3 hours
**Description**: Complete integration with Lusush theme system.

**Implementation**:
- Connect to Lusush theme change events
- Implement real-time theme updates
- Ensure all colors properly themed
- Add theme-specific line editor settings

**Acceptance Criteria**:
- [ ] Responds to theme changes immediately
- [ ] All elements properly themed
- [ ] Performance maintained
- [ ] Consistent with Lusush theming

**Tests**:
- Theme switching during editing
- All theme elements verified
- Performance testing

**Commit**: `LLE-042: Complete Lusush theme system integration`

---

#### Task LLE-043: Configuration Integration
**File**: `src/line_editor/config.c`
**Estimated Time**: 3 hours
**Description**: Integrate with Lusush configuration system.

**Implementation**:
```c
typedef struct {
    bool multiline_mode;
    bool syntax_highlighting;
    bool show_completions;
    size_t history_size;
    size_t undo_levels;
} lle_config_t;

bool lle_config_load(lle_config_t *config);
bool lle_config_apply(lle_line_editor_t *editor, const lle_config_t *config);
```

**Acceptance Criteria**:
- [ ] Loads configuration from Lusush config
- [ ] Supports all configurable options
- [ ] Updates configuration dynamically
- [ ] Provides sensible defaults

**Tests**:
- Configuration loading
- Dynamic updates
- Default values
- Invalid configuration handling

**Commit**: `LLE-043: Integrate with Lusush configuration system`

---

### 4.3 Performance Optimization

#### Task LLE-044: Display Optimization
**File**: `src/line_editor/display.c`
**Estimated Time**: 4 hours
**Description**: Optimize display rendering for performance.

**Implementation**:
- Implement incremental display updates
- Minimize terminal writes
- Cache rendered content
- Optimize cursor positioning

**Acceptance Criteria**:
- [ ] Sub-millisecond response times
- [ ] Minimal terminal output
- [ ] Efficient memory usage
- [ ] Smooth visual experience

**Tests**:
- Performance benchmarking
- Large text handling
- Rapid input testing
- Memory usage monitoring

**Commit**: `LLE-044: Implement display rendering optimizations`

---

#### Task LLE-045: Memory Optimization
**File**: Multiple files (memory management)
**Estimated Time**: 3 hours
**Description**: Optimize memory usage throughout LLE.

**Implementation**:
- Review all memory allocations
- Implement memory pooling where beneficial
- Optimize data structures
- Add memory leak detection

**Acceptance Criteria**:
- [ ] No memory leaks detected
- [ ] Efficient memory usage
- [ ] Predictable memory patterns
- [ ] Good performance with large inputs

**Tests**:
- Memory leak testing
- Large input testing
- Long-running session testing
- Memory profiling

**Commit**: `LLE-045: Implement memory usage optimizations`

---

### 4.4 Testing and Documentation

#### Task LLE-046: Comprehensive Integration Tests
**File**: `tests/line_editor/test_integration.c`
**Estimated Time**: 4 hours
**Description**: Create comprehensive integration tests.

**Implementation**:
- Test all features together
- Test edge cases and error conditions
- Test performance characteristics
- Test cross-platform compatibility

**Acceptance Criteria**:
- [ ] All features tested together
- [ ] Edge cases covered
- [ ] Performance validated
- [ ] Cross-platform testing

**Tests**: Integration test suite
**Commit**: `LLE-046: Implement comprehensive integration tests`

---

#### Task LLE-047: Performance Benchmarks
**File**: `tests/line_editor/benchmark.c`
**Estimated Time**: 3 hours
**Description**: Create performance benchmarks for LLE.

**Implementation**:
```c
// Benchmark various operations
void benchmark_text_insertion(void);
void benchmark_cursor_movement(void);
void benchmark_display_updates(void);
void benchmark_large_text_handling(void);
```

**Acceptance Criteria**:
- [ ] Measures key performance metrics
- [ ] Establishes performance baselines
- [ ] Identifies performance bottlenecks
- [ ] Validates performance requirements

**Tests**: Performance benchmark suite
**Commit**: `LLE-047: Implement performance benchmark suite`

---

#### Task LLE-048: API Documentation
**File**: `docs/line_editor/API.md`
**Estimated Time**: 4 hours
**Description**: Create comprehensive API documentation.

**Implementation**:
- Document all public functions
- Provide usage examples
- Document configuration options
- Create integration guide

**Acceptance Criteria**:
- [ ] All APIs documented
- [ ] Examples provided
- [ ] Integration guide complete
- [ ] Clear and comprehensive

**Tests**: Documentation review and validation
**Commit**: `LLE-048: Create comprehensive API documentation`

---

#### Task LLE-049: User Documentation
**File**: `docs/line_editor/USER_GUIDE.md`
**Estimated Time**: 3 hours
**Description**: Create user-facing documentation.

**Implementation**:
- Document all features
- Provide usage examples
- Document keyboard shortcuts
- Create troubleshooting guide

**Acceptance Criteria**:
- [ ] All features documented
- [ ] Examples clear and helpful
- [ ] Keyboard shortcuts listed
- [ ] Troubleshooting guide complete

**Tests**: Documentation review
**Commit**: `LLE-049: Create comprehensive user documentation`

---

#### Task LLE-050: Final Integration and Testing
**File**: Multiple files (final cleanup)
**Estimated Time**: 4 hours
**Description**: Final integration testing and cleanup.

**Implementation**:
- Final integration with Lusush
- Comprehensive testing
- Code cleanup and optimization
- Final documentation updates

**Acceptance Criteria**:
- [ ] Full integration complete
- [ ] All tests passing
- [ ] Code clean and optimized
- [ ] Documentation up to date

**Tests**: Complete system testing
**Commit**: `LLE-050: Complete final integration and testing`

---

## Summary Statistics

### Task Breakdown by Phase
- **Phase 1 (Foundation)**: 14 tasks, ~42 hours
- **Phase 2 (Core Functionality)**: 12 tasks, ~38 hours  
- **Phase 3 (Advanced Features)**: 11 tasks, ~35 hours
- **Phase 4 (Integration & Polish)**: 13 tasks, ~45 hours

### Total Project Statistics
- **Total Tasks**: 50 tasks
- **Estimated Time**: ~160 hours (4-5 weeks full-time)
- **Average Task Size**: ~3.2 hours
- **Commits Expected**: 50+ commits
- **Test Coverage**: Comprehensive unit and integration tests

### Development Milestones
1. **Week 1**: Tasks LLE-001 through LLE-014 (Foundation complete)
2. **Week 2**: Tasks LLE-015 through LLE-026 (Core functionality complete)
3. **Week 3**: Tasks LLE-027 through LLE-037 (Advanced features complete)
4. **Week 4**: Tasks LLE-038 through LLE-050 (Integration and polish complete)

### Quality Assurance
- Each task includes specific acceptance criteria
- Comprehensive test suite developed alongside functionality
- Performance benchmarks establish quality baselines
- Documentation ensures maintainability
- Git workflow ensures traceability and rollback capability

### Success Metrics Validation
By completion of all tasks, the LLE will achieve:
- ✅ **Perfect multiline prompt support** across all terminals
- ✅ **Sub-millisecond response times** for all operations
- ✅ **Zero mathematical errors** in cursor positioning
- ✅ **100% theme integration** with visual consistency
- ✅ **Extensible architecture** for future enhancements

This task breakdown provides a clear roadmap for implementing a world-class line editor that meets Lusush's exacting standards while maintaining development velocity and code quality.