# Continuation Prompt Integration Pattern Analysis
**Date**: 2025-11-17  
**Purpose**: Document the exact pattern that worked for continuation prompts  
**Context**: This is the proven pattern to inform completion menu integration

---

## Executive Summary

The continuation prompt feature successfully integrates with the display system using a simple, elegant approach:

1. **Before** calling `screen_buffer_render()`
2. **Set line prefixes** on specific rows in screen_buffer
3. **Call** `screen_buffer_render()` as normal
4. Screen buffer accounts for prefix widths in cursor calculations
5. Diff/apply outputs prefixes during terminal writes

**Key Insight**: Prefixes are metadata on screen_buffer rows, NOT part of the rendered content cells.

---

## The Complete Integration Pattern

### Location
**File**: `src/display/display_controller.c`  
**Function**: `handle_redraw_event()` (the main render cycle handler)  
**Lines**: 220-310

### Step-by-Step Breakdown

#### Step 1: Get Command Text (Line 212-218)
```c
char command_buffer[COMMAND_LAYER_MAX_HIGHLIGHTED_SIZE];
command_layer_error_t cmd_result = command_layer_get_highlighted_text(
    cmd_layer, command_buffer, sizeof(command_buffer));

if (cmd_result != COMMAND_LAYER_SUCCESS) {
    return LAYER_EVENTS_ERROR_INVALID_PARAM;
}
```

**What**: Get syntax-highlighted command text from command_layer  
**Result**: `command_buffer` contains command with ANSI color codes  
**Cursor**: Available as `cmd_layer->cursor_position` (byte offset)

#### Step 2: Detect Multiline (Line 229-231)
```c
int newline_count = count_newlines(command_buffer);
bool is_multiline = (newline_count > 0);
```

**What**: Check if command contains newlines  
**Why**: Only multiline commands need continuation prompts

#### Step 3: Analyze Lines and Set Prefixes (Line 232-306)

**If multiline, do this**:

```c
if (is_multiline) {
    // 3a. Initialize state tracker
    continuation_state_t cont_state;
    continuation_state_init(&cont_state);
    
    int logical_line = 0;
    const char *line_start = command_buffer;
    char line_buffer[4096];
    
    while (line_start && *line_start) {
        const char *newline_pos = strchr(line_start, '\n');
        
        if (newline_pos) {
            // 3b. Extract this line
            size_t line_len = newline_pos - line_start;
            memcpy(line_buffer, line_start, line_len);
            line_buffer[line_len] = '\0';
            
            // 3c. Strip ANSI codes (continuation analyzer needs plain text)
            char plain_buffer[4096];
            size_t plain_pos = 0;
            bool in_ansi = false;
            
            for (size_t i = 0; i < line_len && plain_pos < sizeof(plain_buffer) - 1; i++) {
                if (line_buffer[i] == '\033' || line_buffer[i] == '\x1b') {
                    in_ansi = true;
                    continue;
                }
                if (in_ansi) {
                    if ((line_buffer[i] >= 'A' && line_buffer[i] <= 'Z') || 
                        (line_buffer[i] >= 'a' && line_buffer[i] <= 'z') ||
                        line_buffer[i] == 'm') {
                        in_ansi = false;
                    }
                    continue;
                }
                plain_buffer[plain_pos++] = line_buffer[i];
            }
            plain_buffer[plain_pos] = '\0';
            
            // 3d. Analyze this line to update state
            continuation_analyze_line(plain_buffer, &cont_state);
            
            logical_line++;
            line_start = newline_pos + 1;
            
            // 3e. Get prompt for NEXT line based on current state
            const char *cont_prompt = continuation_get_prompt(&cont_state);
            if (cont_prompt) {
                // 3f. Set prefix on the screen_buffer row
                screen_buffer_set_line_prefix(&desired_screen, 
                                               desired_screen.command_start_row + logical_line,
                                               cont_prompt);
            }
        } else {
            break;
        }
    }
    
    // 3g. Cleanup state tracker
    continuation_state_cleanup(&cont_state);
}
```

**Critical Details**:

1. **ANSI Stripping**: Command text has syntax highlighting, but state analyzer needs plain text
2. **Line-by-Line**: Process each line incrementally to track state changes
3. **State Tracking**: `continuation_state_t` tracks `in_for_loop`, `in_if_statement`, etc.
4. **Prompt Generation**: `continuation_get_prompt()` returns appropriate prompt based on state
5. **Prefix Setting**: `screen_buffer_set_line_prefix()` is called BEFORE `screen_buffer_render()`
6. **Row Calculation**: `desired_screen.command_start_row + logical_line` = absolute screen row

#### Step 4: Render As Normal (Line 309-310)
```c
size_t cursor_byte_offset = cmd_layer->cursor_position;
screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);
```

**What**: Standard render call  
**Key**: Prefixes are already set on desired_screen  
**Result**: screen_buffer_render accounts for prefix widths when calculating cursor position

#### Step 5: Diff and Apply (Happens later in function)
```c
screen_buffer_diff(&last_screen, &desired_screen, &diff);
screen_buffer_apply_diff(&diff, STDOUT_FILENO);
```

**What**: Normal diff/apply process  
**Key**: `screen_buffer_apply_diff()` outputs prefixes during terminal writes  
**Result**: Terminal shows continuation prompts before each line

---

## How Prefixes Work in Screen Buffer

### Data Structure (from screen_buffer.h)

**Line Prefix**:
```c
typedef struct {
    char *text;              // Prefix text (e.g., "loop> ")
    size_t length;           // Length in bytes
    size_t visual_width;     // Visual width in columns (excluding ANSI)
    bool contains_ansi;      // True if prefix has ANSI codes
    bool dirty;              // True if prefix changed since last render
} screen_line_prefix_t;
```

**Screen Line**:
```c
typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;              // Number of characters in this line
    bool dirty;              // True if line content changed
    
    screen_line_prefix_t *prefix;  // Optional prefix (NULL if none)
    bool prefix_dirty;       // True if prefix changed
} screen_line_t;
```

### Setting a Prefix

**Function**: `screen_buffer_set_line_prefix(buffer, line_num, prefix_text)`

**What it does**:
1. Allocates `screen_line_prefix_t` if needed
2. Copies `prefix_text` (strdup)
3. Calculates visual width (skipping ANSI codes)
4. Sets `dirty = true`
5. Sets `contains_ansi` if ANSI codes detected

**Result**: Prefix is stored as metadata on the line, separate from content cells

### How Render Accounts for Prefixes

**In screen_buffer_render()** (Line 359-363):
```c
// Handle newlines
if (ch == '\n') {
    row++;
    if (row >= buffer->num_rows) {
        buffer->num_rows = row + 1;
    }
    
    // CONTINUATION PROMPT SUPPORT:
    // After a newline, check if the next row has a continuation prompt prefix.
    // If it does, start column position after the prefix (not at column 0).
    size_t prefix_width = screen_buffer_get_line_prefix_visual_width(buffer, row);
    col = (int)prefix_width;
    
    i++;
    bytes_processed++;
    continue;
}
```

**Key Insight**: After newline, column starts at `prefix_width`, not 0

**Effect**: Cursor calculations account for prefix space

### How Apply Outputs Prefixes

**In screen_buffer_apply_diff()** - actual implementation would be in the diff/apply code.

**Conceptually**:
1. When writing a line with prefix
2. Move cursor to line start
3. Write prefix text
4. Write line content
5. Cursor position is relative to screen, not buffer

---

## Why This Pattern Works

### Separation of Concerns
- **Prefixes**: Metadata, not content
- **Content**: Rendered into cells
- **Cursor**: Calculated accounting for both

### Independent Dirty Tracking
- Line content has `dirty` flag
- Prefix has `prefix_dirty` flag
- Can update one without the other

### Minimal Diff Impact
- Prefix changes tracked separately
- Only re-renders what changed
- Efficient for incremental updates

### No Command Text Modification
- Command text unchanged
- Prefixes added at display time
- Clean separation of data and presentation

---

## Lessons for Completion Menu Integration

### What Works (Proven)
1. **Metadata approach**: Store display extras as metadata, not in content
2. **Pre-render setup**: Set metadata BEFORE calling screen_buffer_render()
3. **Independent tracking**: Separate dirty flags for independent updates
4. **Visual width calculation**: Account for ANSI codes, UTF-8, wide chars
5. **Cursor math**: Let screen_buffer handle cursor position calculations

### What to Avoid
1. **Don't modify command text**: Keep data pure
2. **Don't bypass screen_buffer**: All rendering through established flow
3. **Don't assume terminal state**: Virtual screen is source of truth
4. **Don't over-engineer**: Simple metadata approach works

### Questions for Menu Integration

1. **Can menus use similar metadata approach?**
   - Menu rows as "suffix" lines after command?
   - Or menu state as screen_buffer extension?

2. **Where do menu lines live in screen_buffer?**
   - Additional rows after command rows?
   - Separate virtual screen area?

3. **How to handle menu clearing?**
   - Remove suffix lines from screen_buffer?
   - Mark menu rows as "not visible"?

4. **How to track menu selection highlight?**
   - Metadata on menu rows?
   - ANSI codes in menu text?

5. **When to update menu display?**
   - Every render cycle if menu active?
   - Event-driven on menu state change?

---

## Next Steps

1. **Prototype menu as additional screen_buffer rows**
   - Try simplest approach first
   - Menu rows start at `command_end_row + 1`
   - Each menu item is one row with prefix/formatting

2. **Test clearing mechanism**
   - How to remove menu rows cleanly?
   - Does diff system handle row count decrease?

3. **Test cursor behavior**
   - Cursor must stay in command area
   - Menu selection highlight separate from cursor

4. **Measure performance**
   - Large menu (100+ items) performance
   - Scrolling update efficiency
   - Diff overhead with many rows

5. **Consider alternatives if simple approach fails**
   - Separate menu buffer?
   - Composed screen approach?
   - Terminal-level menu rendering?

---

## Code Locations Reference

**Continuation State Analysis**:
- `src/input_continuation.c` - State tracker and analyzer
- Functions: `continuation_state_init()`, `continuation_analyze_line()`, `continuation_get_prompt()`

**Screen Buffer Prefix API**:
- `include/display/screen_buffer.h` - Prefix function declarations (lines 200-300)
- `src/display/screen_buffer.c` - Prefix implementations (lines 600-800)

**Display Controller Integration**:
- `src/display/display_controller.c` - Main integration (lines 220-310)
- Function: `handle_redraw_event()`

**Key Screen Buffer Functions**:
- `screen_buffer_set_line_prefix()` - Set prefix on a line
- `screen_buffer_get_line_prefix_visual_width()` - Get prefix width
- `screen_buffer_render()` - Main render (accounts for prefixes)
- `screen_buffer_diff()` - Diff algorithm
- `screen_buffer_apply_diff()` - Terminal output

---

## Conclusion

The continuation prompt pattern is elegant and minimal:
1. Set metadata before rendering
2. Render as normal
3. Screen buffer handles the details

This same pattern can likely work for completion menus with menu items as additional rows after the command, each with appropriate formatting/highlighting metadata.

The key is to maintain the separation: LLE owns the data, display system renders it, screen_buffer tracks changes.
