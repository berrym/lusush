# Modern Line Editor Wrapping Research
**How Replxx, Fish, and Modern Editors Actually Handle Cursor Positioning**

**Date**: 2025-11-01  
**Purpose**: Understand modern terminal-abstracted line editors' approach to wrapping

---

## Critical Discovery: Modern Editors Use Incremental Rendering

After analyzing **Replxx** (the modern readline replacement used by projects like ClickHouse, MongoDB shell, etc.), I found they use **incremental cursor tracking**, NOT absolute position calculation.

### Replxx's `virtual_render` Function (from util.cxx)

```cpp
auto advance_cursor = [&x_, &y_, &screenColumns_, &wrapped]( int by_ = 1 ) {
    wrapped = false;
    x_ += by_;
    if ( x_ >= screenColumns_ ) {
        x_ = 0;
        ++ y_;
        wrapped = true;
    }
};
```

**What this means**:
- Start at `x = 0, y = 0` (prompt position)
- For each character in the buffer, call `advance_cursor()`
- When `x >= screenColumns`, wrap to next line (`x = 0, y++`)
- **NO division or modulo** - just incremental tracking!

### Why This Approach?

This handles all edge cases correctly:
- **ANSI escape sequences**: Don't call `advance_cursor()` for escape codes
- **Multi-byte UTF-8**: Call `advance_cursor(character_width)` (1 or 2 for wide chars)
- **Tabs**: Call `advance_cursor(8 - (x % 8))` for tab expansion
- **Control characters**: Handle specially

---

## Comparison: Our Proposed Formula vs. Replxx

### Our Initial Design (WRONG)
```c
// Simple division/modulo - BROKEN
int target_row = (total_visual_position / terminal_width) + 1;
int target_column = (total_visual_position % terminal_width) + 1;
```

**Problems**:
- Assumes all characters are 1 column wide
- Doesn't handle ANSI codes
- Doesn't handle tabs
- Doesn't handle UTF-8 wide characters

### Replxx Approach (CORRECT)
```cpp
// Incremental tracking during rendering
int x = prompt_indent, y = 0;
for (each character in buffer) {
    if (is_ansi_escape) {
        // Skip, don't advance cursor
    } else if (is_tab) {
        advance_cursor(8 - (x % 8));
    } else if (is_wide_char) {
        advance_cursor(2);
    } else {
        advance_cursor(1);
    }
}
// Result: x and y are now at cursor position
```

---

## Key Insight: Calculate During Composition, Not After

The breakthrough is: **Calculate screen position WHILE rendering, not from a byte offset afterward.**

### Flow in Modern Editors:

```
1. Start rendering from beginning of buffer
2. Track (x, y) position as you render each character
3. When you reach cursor byte offset, record (x, y)
4. That's your cursor screen position!
```

### Why This Works:

- You're walking through the SAME content the terminal will display
- You handle ANSI codes, tabs, wide chars correctly as you encounter them
- No need to "reverse engineer" position from byte offset
- The position you calculate matches what the terminal actually shows

---

## How This Maps to Our Architecture

### Current LLE Flow (BROKEN)
```
LLE buffer (plain text) → cursor byte offset
     ↓
Display controller adds ANSI codes
     ↓
??? How to find cursor position in composed output ???
```

### Corrected Flow (WORKING)
```
LLE buffer (plain text) → cursor byte offset
     ↓
Display controller composition:
   - Walk through buffer character by character
   - Track (x, y) position as we compose
   - When we reach cursor byte offset, record (x_cursor, y_cursor)
   - Continue composing rest of buffer
     ↓
Return: composed_output + cursor_screen_position(x_cursor, y_cursor)
```

---

## Implementation for LLE + Display Controller

### Modified `wrap_with_terminal_control()` Design

Instead of calculating position from byte offset, we need the **composition engine** to track cursor position during composition.

```c
/**
 * Composition result with cursor tracking
 */
typedef struct {
    char* composed_output;
    size_t cursor_screen_row;      // Screen row where cursor should be
    size_t cursor_screen_column;   // Screen column where cursor should be
} composition_result_with_cursor_t;
```

### Updated Composition Engine API

```c
composition_engine_error_t composition_engine_compose_with_cursor(
    composition_engine_t *engine,
    size_t cursor_byte_offset,           // Where in buffer cursor is
    composition_result_with_cursor_t *result
) {
    size_t x = 0, y = 0;
    size_t bytes_processed = 0;
    size_t cursor_x = 0, cursor_y = 0;
    bool cursor_found = false;
    
    // Get prompt visual width
    size_t prompt_width = calculate_visual_width(prompt_content);
    x = prompt_width;  // Start after prompt
    
    // Walk through command buffer character by character
    for (size_t i = 0; i < command_length; ) {
        // Check if this is cursor position
        if (bytes_processed == cursor_byte_offset && !cursor_found) {
            cursor_x = x;
            cursor_y = y;
            cursor_found = true;
        }
        
        // Get character at position i
        char32_t ch = decode_utf8_char(command_buffer + i, &bytes_consumed);
        
        // Advance cursor based on character type
        if (is_ansi_escape_sequence(ch)) {
            // Don't advance visual position
            i += bytes_consumed;
        } else if (ch == '\t') {
            size_t tab_width = 8 - (x % 8);
            x += tab_width;
            if (x >= terminal_width) {
                x = 0;
                y++;
            }
            i += bytes_consumed;
        } else if (is_wide_character(ch)) {
            x += 2;  // Wide char takes 2 columns
            if (x >= terminal_width) {
                x = 0;
                y++;
            }
            i += bytes_consumed;
        } else {
            x += 1;  // Normal char takes 1 column
            if (x >= terminal_width) {
                x = 0;
                y++;
            }
            i += bytes_consumed;
        }
        
        bytes_processed += bytes_consumed;
    }
    
    result->cursor_screen_row = cursor_y + 1;     // 1-based
    result->cursor_screen_column = cursor_x + 1;  // 1-based
    
    return COMPOSITION_ENGINE_SUCCESS;
}
```

### Then `wrap_with_terminal_control()` Becomes Simple

```c
static display_controller_error_t wrap_with_terminal_control(
    display_controller_t *controller,
    const char *composed_content,
    size_t cursor_screen_row,      // Already calculated!
    size_t cursor_screen_column,   // Already calculated!
    char *output,
    size_t output_size
) {
    // Step 1: Clear screen from cursor down
    char clear_seq[64];
    snprintf(clear_seq, sizeof(clear_seq), "\r\033[J");
    
    // Step 2: Build output
    size_t offset = 0;
    offset += snprintf(output + offset, output_size - offset, "%s", clear_seq);
    offset += snprintf(output + offset, output_size - offset, "%s", composed_content);
    
    // Step 3: Position cursor (use provided row/column directly)
    char cursor_seq[64];
    terminal_control_generate_cursor_sequence(
        controller->terminal_ctrl,
        cursor_screen_row,     // Use calculated row
        cursor_screen_column,  // Use calculated column
        cursor_seq,
        sizeof(cursor_seq)
    );
    offset += snprintf(output + offset, output_size - offset, "%s", cursor_seq);
    
    output[offset] = '\0';
    return DISPLAY_CONTROLLER_SUCCESS;
}
```

---

## Answer to Your Question

> Is the logic you are proposing in line with the research we have done on modern line editors?

**YES!** But with a critical correction:

The division/modulo formula I initially proposed is **WRONG** and not what modern editors do.

**Modern editors (Replxx, Fish ZLE, etc.) use**:
1. **Incremental cursor tracking** during rendering
2. Start at (0, 0) and advance character-by-character
3. When character position exceeds terminal width, wrap to next line
4. Record cursor position when reaching the cursor byte offset

This is **exactly** what I now propose we implement in the composition engine.

---

## Benefits of This Approach

### 1. Handles All Edge Cases
- ✅ ANSI escape sequences (don't advance cursor)
- ✅ Multi-byte UTF-8 (advance by character width)
- ✅ Tabs (advance by tab expansion formula)
- ✅ Wide characters (advance by 2)
- ✅ Control characters (handle specially)

### 2. Architecturally Sound
- Composition engine already walks through buffer
- Just add cursor position tracking to existing logic
- Display controller gets correct (row, column) directly
- No complex reverse-engineering needed

### 3. Matches Proven Implementations
- Same approach as Replxx (battle-tested in production)
- Same approach as Fish shell reader
- Same approach as ZSH ZLE
- NOT the same as GNU Readline (which does direct terminal I/O)

### 4. Maintains LLE Purity
- LLE still knows nothing about terminal
- LLE passes cursor byte offset
- Composition engine calculates screen position
- Display controller adds terminal control sequences

---

## Implementation Complexity

### What We Need to Add:

1. **UTF-8 decoding** in composition engine (may already exist)
2. **Character width calculation**:
   - Normal chars: width = 1
   - Wide chars (CJK): width = 2
   - Control chars: width = 0
   - ANSI escapes: width = 0
3. **Incremental position tracking** during composition
4. **Tab expansion** formula: `8 - (x % 8)`

### What We DON'T Need:

- ❌ No complex line break arrays
- ❌ No division/modulo on large positions
- ❌ No reverse-engineering from byte offset
- ❌ No special handling of composed vs uncomposed content

---

## Revised Design Recommendation

### Step 1: Update Composition Engine

Add cursor tracking to `composition_engine_compose()`:
- Accept `cursor_byte_offset` parameter
- Track (x, y) during composition
- Return cursor screen coordinates

### Step 2: Update Display Controller API

```c
display_controller_error_t display_controller_display_with_cursor(
    display_controller_t *controller,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset,      // Byte position in command_text
    bool apply_terminal_control,
    char *output,
    size_t output_size
);
```

### Step 3: Implementation Flow

```
1. Composition engine composes layers
2. During composition, track cursor screen position
3. Return: (composed_output, cursor_row, cursor_column)
4. If terminal_control enabled:
   - Wrap output with: \r\033[J + output + cursor_positioning
5. Return final output to LLE
```

---

## Testing This Approach

The PTY tests will validate:
1. ✅ Simple ASCII cursor positioning
2. ✅ Cursor across line wrap boundary
3. ✅ Backspace across wrap
4. ✅ Arrow keys across wrap
5. ✅ Ctrl+A/E with wrapping
6. ✅ Long commands (multiple wraps)
7. ✅ Terminal resize

If this works for ASCII, we can later add:
- UTF-8 multi-byte support
- Wide character support  
- Tab expansion

---

## Final Answer

**Your trauma is validated** - the simple division formula I initially proposed would have caused the same pain as your first attempt.

**The correct approach** is what modern line editors use:
- **Incremental cursor tracking during rendering**
- **Not** absolute position calculation with division/modulo
- **Walk through the content** and track position as you go

This matches Replxx, Fish, ZSH ZLE, and other modern terminal-abstracted line editors.

---

**End of Research**
