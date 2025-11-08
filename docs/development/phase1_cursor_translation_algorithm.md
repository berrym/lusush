# Phase 1: Cursor Position Translation Algorithm
## Detailed Specification with Edge Cases

**Document Status**: Phase 1 Design - Detailed Algorithm Specification  
**Created**: 2025-11-08  
**Purpose**: Specify the exact cursor translation algorithm with all edge cases

---

## Overview

The cursor translation algorithm maps between two coordinate spaces:

1. **Buffer Space**: Cursor as byte offset in command text (0-based, no prompts)
2. **Display Space**: Cursor as (row, column) on terminal (0-based, includes prompts)

This is **the most critical algorithm** in the continuation prompt system because:
- Any error breaks cursor positioning (unusable editor)
- Must handle all character types correctly
- Must work with prompts of varying widths
- Must handle line wrapping

---

## Why Incremental Tracking is the ONLY Correct Approach

### Research from Modern Line Editors

All modern line editors use **incremental character-by-character tracking**:

1. **Replxx** (Readable line editing library)
   - Walks character-by-character
   - Tracks visual width at each position
   - Handles UTF-8, wide chars, ANSI, tabs

2. **Fish Shell**
   - Never uses division/modulo
   - Incremental tracking through string
   - Explicit handling of each character type

3. **ZSH ZLE** (Z Shell Line Editor)
   - Character-by-character position tracking
   - Separate visual position from byte position
   - Explicit ANSI/wide char/tab handling

### Why Formulas Don't Work

Simple formulas **always fail**:

```c
// WRONG APPROACH (never works correctly):
display_row = byte_offset / terminal_width;
display_col = byte_offset % terminal_width;

// Fails on:
// 1. UTF-8 multi-byte characters (e.g., "é" is 2 bytes, 1 column)
// 2. Wide characters (e.g., "中" is 3 bytes, 2 columns)
// 3. ANSI escape sequences (e.g., "\033[31m" is 5 bytes, 0 columns)
// 4. Tab characters (e.g., "\t" is 1 byte, 1-8 columns)
// 5. Prompts (different widths: "> " vs "loop> ")
// 6. Line wrapping (must detect when width exceeded)
```

**Conclusion**: Incremental tracking is not an optimization, it's the **only correct approach**.

---

## Algorithm: Buffer to Display Translation

### High-Level Flow

```
Input:  cursor_byte_offset (position in command buffer)
Output: (display_row, display_col) (position on terminal)

Algorithm:
1. Initialize position tracking variables
2. Account for primary prompt on line 0
3. Walk through command content character-by-character
4. For each character:
   a. Check if current position matches cursor_byte_offset
   b. Decode character (UTF-8, ANSI, tab, etc.)
   c. Calculate visual width
   d. Check for line wrapping
   e. Update display position
   f. Advance byte position
5. Return display position when cursor found
```

### Detailed Implementation

```c
/**
 * Translate buffer cursor to display cursor with continuation prompts.
 * 
 * This is the CORRECT approach based on research from Replxx, Fish, and ZSH.
 * It uses incremental character-by-character tracking to handle all edge cases.
 * 
 * @param engine The composition engine (has prompt info)
 * @param cursor_byte_offset Cursor position in command buffer (0-based)
 * @param command_content The full command text (may contain \n)
 * @param primary_prompt The primary prompt (for line 0)
 * @param continuation_prompts Array of continuation prompts (for lines 1+)
 * @param terminal_width Terminal width for wrapping detection
 * @param out_row Output: display row (0-based)
 * @param out_col Output: display column (0-based)
 * @return COMPOSITION_ENGINE_SUCCESS on success, error on failure
 */
composition_engine_error_t translate_cursor_buffer_to_display(
    const char *command_content,
    size_t cursor_byte_offset,
    const char *primary_prompt,
    const char *continuation_prompts[],
    int terminal_width,
    size_t *out_row,
    size_t *out_col
) {
    // Safety checks
    if (!command_content || !primary_prompt || !out_row || !out_col) {
        return COMPOSITION_ENGINE_ERROR_NULL_POINTER;
    }
    
    if (terminal_width <= 0) {
        return COMPOSITION_ENGINE_ERROR_INVALID_PARAM;
    }
    
    // Position tracking variables
    size_t byte_pos = 0;           // Current byte position in buffer
    size_t display_row = 0;        // Current display row
    size_t display_col = 0;        // Current display column
    size_t logical_line = 0;       // Current logical line (counts \n)
    
    // Start after primary prompt
    display_col = calculate_visual_width(primary_prompt);
    
    // Handle edge case: cursor at position 0 (before any content)
    if (cursor_byte_offset == 0) {
        *out_row = 0;
        *out_col = display_col;
        return COMPOSITION_ENGINE_SUCCESS;
    }
    
    // Walk through command content character-by-character
    const char *p = command_content;
    while (*p != '\0') {
        
        // Check if we found the cursor position
        if (byte_pos == cursor_byte_offset) {
            *out_row = display_row;
            *out_col = display_col;
            return COMPOSITION_ENGINE_SUCCESS;
        }
        
        // Handle newline (logical line boundary)
        if (*p == '\n') {
            logical_line++;
            display_row++;
            
            // Move to next line with continuation prompt
            if (continuation_prompts && continuation_prompts[logical_line]) {
                display_col = calculate_visual_width(continuation_prompts[logical_line]);
            } else {
                display_col = 0;  // No prompt (shouldn't happen)
            }
            
            byte_pos++;
            p++;
            continue;
        }
        
        // Decode next character and calculate visual width
        uint32_t codepoint;
        size_t char_bytes;
        int visual_width;
        
        if (*p == '\t') {
            // Tab: expands to next multiple of 8
            codepoint = '\t';
            char_bytes = 1;
            visual_width = 8 - (display_col % 8);
            
        } else if (*p == '\033') {
            // ANSI escape sequence: 0 visual width
            codepoint = 0;
            char_bytes = ansi_sequence_length(p);
            visual_width = 0;
            
        } else {
            // Regular character: UTF-8 decode
            char_bytes = utf8_decode(p, &codepoint);
            
            if (is_wide_character(codepoint)) {
                // Wide character (CJK): 2 columns
                visual_width = 2;
            } else if (is_control_character(codepoint)) {
                // Control character: typically 0 or 1 column
                visual_width = 0;  // or handle specially
            } else {
                // Normal character: 1 column
                visual_width = 1;
            }
        }
        
        // Check for line wrapping
        // If adding this character would exceed terminal width, wrap to next line
        if (display_col + visual_width > terminal_width) {
            display_row++;
            display_col = 0;
            
            // NOTE: Continuation prompt does NOT appear on wrapped lines
            // Only on logical line boundaries (after \n)
        }
        
        // Update position
        display_col += visual_width;
        byte_pos += char_bytes;
        p += char_bytes;
    }
    
    // Cursor is at end of buffer (after all content)
    if (byte_pos == cursor_byte_offset) {
        *out_row = display_row;
        *out_col = display_col;
        return COMPOSITION_ENGINE_SUCCESS;
    }
    
    // Cursor position is beyond buffer end (error)
    return COMPOSITION_ENGINE_ERROR_INVALID_PARAM;
}
```

---

## Character Type Handling

### 1. UTF-8 Multi-Byte Characters

**Example**: "Café" - 'é' is 2 bytes (0xC3 0xA9), but 1 visual column

```c
/**
 * Decode UTF-8 character and return byte count.
 * 
 * @param str Pointer to UTF-8 sequence
 * @param out_codepoint Output: Unicode codepoint
 * @return Number of bytes consumed (1-4)
 */
size_t utf8_decode(const char *str, uint32_t *out_codepoint) {
    unsigned char c = (unsigned char)str[0];
    
    // 1-byte sequence (ASCII): 0xxxxxxx
    if ((c & 0x80) == 0) {
        *out_codepoint = c;
        return 1;
    }
    
    // 2-byte sequence: 110xxxxx 10xxxxxx
    if ((c & 0xE0) == 0xC0) {
        *out_codepoint = ((c & 0x1F) << 6) | 
                         ((unsigned char)str[1] & 0x3F);
        return 2;
    }
    
    // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
    if ((c & 0xF0) == 0xE0) {
        *out_codepoint = ((c & 0x0F) << 12) | 
                         (((unsigned char)str[1] & 0x3F) << 6) |
                         ((unsigned char)str[2] & 0x3F);
        return 3;
    }
    
    // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    if ((c & 0xF8) == 0xF0) {
        *out_codepoint = ((c & 0x07) << 18) | 
                         (((unsigned char)str[1] & 0x3F) << 12) |
                         (((unsigned char)str[2] & 0x3F) << 6) |
                         ((unsigned char)str[3] & 0x3F);
        return 4;
    }
    
    // Invalid UTF-8
    *out_codepoint = 0xFFFD;  // Replacement character
    return 1;
}
```

**Visual Width**: Use wcwidth() from <wchar.h>

```c
int visual_width = wcwidth(codepoint);
if (visual_width < 0) visual_width = 0;  // Control chars
```

### 2. Wide Characters (CJK)

**Example**: "中文" - Each character is 3 bytes, but 2 visual columns

```c
/**
 * Check if codepoint is a wide character (CJK).
 * 
 * Wide characters take 2 columns on terminal.
 * 
 * @param codepoint Unicode codepoint
 * @return True if wide character
 */
bool is_wide_character(uint32_t codepoint) {
    // Use system wcwidth() for accurate detection
    return wcwidth(codepoint) == 2;
    
    // Alternatively, manual ranges:
    // return (codepoint >= 0x1100 && codepoint <= 0x115F) ||  // Hangul Jamo
    //        (codepoint >= 0x2E80 && codepoint <= 0x9FFF) ||  // CJK
    //        (codepoint >= 0xAC00 && codepoint <= 0xD7A3) ||  // Hangul Syllables
    //        (codepoint >= 0xF900 && codepoint <= 0xFAFF);    // CJK Compatibility
}
```

**Edge Case**: Emoji

Many emoji are also wide (2 columns), but some are 1 column. Use wcwidth().

### 3. ANSI Escape Sequences

**Example**: "\033[31mred\033[0m" - Escape sequences are invisible (0 columns)

```c
/**
 * Detect and measure ANSI escape sequence.
 * 
 * ANSI sequences have 0 visual width but consume bytes.
 * 
 * @param str Pointer to potential escape sequence
 * @return Length of escape sequence in bytes, or 0 if not an escape
 */
size_t ansi_sequence_length(const char *str) {
    if (str[0] != '\033' || str[1] != '[') {
        return 0;  // Not an ANSI escape
    }
    
    // CSI sequence: ESC [ ... letter
    // Walk until we find the final letter
    const char *p = str + 2;
    while (*p) {
        if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) {
            // Found final character
            return (p - str) + 1;
        }
        p++;
    }
    
    // Malformed sequence
    return 0;
}
```

**Common Sequences**:
- `\033[0m` - Reset (4 bytes, 0 columns)
- `\033[31m` - Red foreground (5 bytes, 0 columns)
- `\033[1;32m` - Bold green (7 bytes, 0 columns)

### 4. Tab Characters

**Example**: "foo\tbar" - Tab expands to align to next multiple of 8

```c
/**
 * Calculate visual width of tab character.
 * 
 * Tabs expand to next multiple of 8 columns.
 * 
 * @param current_col Current column position
 * @return Visual width of tab (1-8 columns)
 */
int tab_visual_width(int current_col) {
    return 8 - (current_col % 8);
}
```

**Example**:
- Column 0: Tab expands to 8 columns (moves to column 8)
- Column 1: Tab expands to 7 columns (moves to column 8)
- Column 7: Tab expands to 1 column (moves to column 8)
- Column 8: Tab expands to 8 columns (moves to column 16)

### 5. Control Characters

**Example**: "\x01" (SOH) - Typically invisible

```c
/**
 * Check if codepoint is a control character.
 * 
 * Control characters are typically non-printable.
 * 
 * @param codepoint Unicode codepoint
 * @return True if control character
 */
bool is_control_character(uint32_t codepoint) {
    return (codepoint < 0x20) ||              // C0 controls
           (codepoint >= 0x7F && codepoint < 0xA0);  // DEL and C1 controls
}
```

**Visual Width**: Typically 0, but some terminals show them as "^A" (2 columns).

**Decision**: Treat as 0 columns (most common behavior).

---

## Line Wrapping Handling

### Wrapping Detection

```c
// Check if adding this character would exceed terminal width
if (display_col + visual_width > terminal_width) {
    // Wrap to next line
    display_row++;
    display_col = 0;
    
    // IMPORTANT: Do NOT add continuation prompt on wrapped lines
    // Continuation prompts only appear after \n (logical line boundary)
}

// Then add character
display_col += visual_width;
```

### Logical Lines vs Display Lines

**Key Distinction**:

- **Logical Line**: Delimited by \n in buffer
  - Has continuation prompt
  - Example: "for i in 1 2 3\n" is logical line 0

- **Display Line**: Row on terminal (may span multiple rows due to wrapping)
  - No continuation prompt on wrapped portions
  - Example: Very long command may wrap across 3 display rows

**Example**:

```
Buffer:  "for i in $(very long command here that exceeds terminal width)\ndo\n    echo $i\ndone"
           ^--- line 0 (wraps to 2 display rows) ---^  ^-line 1-^  ^-line 2-^  ^line 3^

Display:
Row 0:  bash$ for i in $(very long command here that exceeds termina
Row 1:  l width)
Row 2:  for> do
Row 3:  for>     echo $i
Row 4:  > done

Note: Row 1 has NO prompt (wrapped from row 0)
      Row 2 has "for> " prompt (logical line 1)
```

---

## Continuation Prompt Handling

### Prompt Width Calculation

Each continuation prompt has a different width:

```c
/**
 * Calculate visual width of prompt (handles ANSI codes).
 * 
 * @param prompt The prompt string (may contain ANSI codes)
 * @return Visual width in columns
 */
size_t calculate_visual_width(const char *prompt) {
    size_t width = 0;
    const char *p = prompt;
    
    while (*p) {
        if (*p == '\033') {
            // Skip ANSI escape sequence
            p += ansi_sequence_length(p);
        } else if (*p == '\t') {
            // Tab expands
            width += 8 - (width % 8);
            p++;
        } else {
            // Regular character (assume 1 column for simplicity)
            // For full accuracy, use UTF-8 decode + wcwidth
            width++;
            p++;
        }
    }
    
    return width;
}
```

**Examples**:
- `"> "` → width 2
- `"loop> "` → width 6
- `"if> "` → width 4
- `"\033[32m>\033[0m "` → width 2 (ANSI codes are invisible)

### Prompt Application Rules

```c
// Line 0: Primary prompt
display_col = calculate_visual_width(primary_prompt);

// Line 1+: Continuation prompt (only after \n, not on wrapped lines)
if (*p == '\n') {
    logical_line++;
    display_row++;
    display_col = calculate_visual_width(continuation_prompts[logical_line]);
}

// Wrapped lines: NO prompt
if (display_col + visual_width > terminal_width) {
    display_row++;
    display_col = 0;  // No prompt added
}
```

---

## Edge Cases and Special Situations

### Edge Case 1: Cursor at Start (Position 0)

```c
// Buffer: "for i in 1\n"
// Cursor: 0 (before 'f')

// Expected: (row=0, col=<primary_prompt_width>)
// Example:  bash$ |for i in 1
//           ^^^^^
//           5 cols

if (cursor_byte_offset == 0) {
    *out_row = 0;
    *out_col = calculate_visual_width(primary_prompt);
    return SUCCESS;
}
```

### Edge Case 2: Cursor at End

```c
// Buffer: "for i in 1"
// Cursor: 10 (after '1', at end)

// Walk through all characters, cursor is found at final position
if (byte_pos == cursor_byte_offset) {
    *out_row = display_row;
    *out_col = display_col;
    return SUCCESS;
}
```

### Edge Case 3: Cursor on Newline

```c
// Buffer: "for i in 1\ndo"
//                    ^--- cursor here (byte 10, on the \n)

// When we reach \n:
if (*p == '\n') {
    if (byte_pos == cursor_byte_offset) {
        // Cursor is ON the newline
        // Position it at end of current line (before wrap)
        *out_row = display_row;
        *out_col = display_col;
        return SUCCESS;
    }
    
    // Move to next line
    logical_line++;
    display_row++;
    display_col = calculate_visual_width(continuation_prompts[logical_line]);
    byte_pos++;
    p++;
    continue;
}
```

### Edge Case 4: Empty Lines

```c
// Buffer: "for i in 1\n\ndo"
//                    ^--^--- empty line

// Line 1 is empty, but still has continuation prompt:
// bash$ for i in 1
// for> 
// for> do
```

Walk through the empty line normally:
- Hit \n at end of line 0 → move to line 1
- Immediately hit \n at start of line 1 → move to line 2
- Line 1 prompt is rendered even though content is empty

### Edge Case 5: Cursor on Multi-Byte Character Boundary

```c
// Buffer: "Café"
//           ^--- 'é' is bytes [3, 4]
// Cursor: 3 (start of 'é')

// Decode: byte 3 starts 2-byte UTF-8 sequence
// Position cursor BEFORE the 'é'
// Do NOT allow cursor at byte 4 (middle of character)
```

**Validation**: Cursor should only be at character boundaries, not in middle of multi-byte sequence.

### Edge Case 6: Cursor After ANSI Escape

```c
// Buffer: "foo\033[31mbar"
//                    ^--- cursor here (byte 8, after escape)

// ANSI sequence is bytes [3-7] (5 bytes)
// Visual position unchanged by escape
// Cursor at byte 8 has same display_col as byte 3
```

### Edge Case 7: Very Long Line Wrapping

```c
// Buffer: "<500 character command>\ndo"
// Terminal width: 80

// Line 0 wraps across ~6.25 display rows
// Row 0: <80 chars>
// Row 1: <80 chars>
// Row 2: <80 chars>
// Row 3: <80 chars>
// Row 4: <80 chars>
// Row 5: <80 chars>
// Row 6: <20 chars>
// Row 7: for> do  ← continuation prompt here
```

### Edge Case 8: Wide Character at Wrap Boundary

```c
// Display column: 79 (one column remaining)
// Next character: "中" (wide char, 2 columns)

// Cannot fit wide character in remaining space
// Wrap BEFORE character:
if (display_col + visual_width > terminal_width) {
    display_row++;
    display_col = 0;
}

// Now add character at start of new line
display_col += visual_width;  // +2
```

### Edge Case 9: Tab at Wrap Boundary

```c
// Display column: 77
// Next character: \t (tab)
// Tab would expand to 8 columns (next multiple of 8 is 80)

// But 77 + (8 - 77%8) = 77 + 7 = 84 > 80

// Wrap BEFORE tab
if (display_col + tab_visual_width(display_col) > terminal_width) {
    display_row++;
    display_col = 0;
}

// Calculate tab width from new position
display_col += tab_visual_width(display_col);  // 8 (at column 0)
```

### Edge Case 10: Zero-Width Terminal

```c
// Terminal width: 0 or negative (error condition)

if (terminal_width <= 0) {
    return COMPOSITION_ENGINE_ERROR_INVALID_PARAM;
}
```

---

## Algorithm: Display to Buffer Translation (Reverse)

### Use Cases

1. **Mouse Clicks**: User clicks at (row, col) on terminal → find buffer position
2. **Search Results**: Find display position of search match
3. **Debugging**: Validate buffer↔display mappings

### Algorithm

Same incremental tracking, but stop when display position matches:

```c
composition_engine_error_t translate_cursor_display_to_buffer(
    const char *command_content,
    size_t target_row,
    size_t target_col,
    const char *primary_prompt,
    const char *continuation_prompts[],
    int terminal_width,
    size_t *out_byte_offset
) {
    // Same setup as buffer_to_display
    size_t byte_pos = 0;
    size_t display_row = 0;
    size_t display_col = calculate_visual_width(primary_prompt);
    size_t logical_line = 0;
    
    // Walk character-by-character
    const char *p = command_content;
    while (*p != '\0') {
        
        // Check if we found the target position
        if (display_row == target_row && display_col == target_col) {
            *out_byte_offset = byte_pos;
            return COMPOSITION_ENGINE_SUCCESS;
        }
        
        // If we passed the target row, cursor is at end of previous line
        if (display_row > target_row) {
            // Went too far, cursor was at end of previous line
            // Return position of last character on target row
            *out_byte_offset = byte_pos - 1;  // Previous character
            return COMPOSITION_ENGINE_SUCCESS;
        }
        
        // Same character processing as buffer_to_display
        // ... (handle \n, UTF-8, ANSI, tabs, wrapping)
        
        // Update positions
        display_col += visual_width;
        byte_pos += char_bytes;
        p += char_bytes;
    }
    
    // Cursor is beyond content (at end of buffer)
    if (display_row == target_row && display_col == target_col) {
        *out_byte_offset = byte_pos;
        return COMPOSITION_ENGINE_SUCCESS;
    }
    
    // Target position is beyond content (error or position at end)
    *out_byte_offset = byte_pos;  // End of buffer
    return COMPOSITION_ENGINE_SUCCESS;
}
```

### Edge Case: Click Between Characters

```c
// User clicks at (row=0, col=15)
// Character at col 14 ends, character at col 16 starts
// Column 15 is "between" characters

// Decision: Position cursor at start of next character (col 16)
// Implementation: Stop when display_col >= target_col
```

### Edge Case: Click in Prompt Area

```c
// User clicks at (row=1, col=2)
// Continuation prompt is "for> " (6 columns)
// Column 2 is inside prompt

// Decision: Position cursor at start of content (after prompt)
if (target_col < calculate_visual_width(continuation_prompts[logical_line])) {
    // Click is in prompt area, move to start of content
    target_col = calculate_visual_width(continuation_prompts[logical_line]);
}
```

---

## Performance Optimization

### Early Exit Optimization

```c
// If cursor is on line N, no need to process lines N+1
if (display_row > target_row) {
    // Already passed target, return immediately
    return COMPOSITION_ENGINE_SUCCESS;
}
```

### Caching Line Boundaries

```c
// Cache the start positions of each logical line
typedef struct {
    size_t byte_offset;      // Byte offset in buffer
    size_t display_row;      // Display row at line start
    size_t display_col;      // Display col at line start (after prompt)
} line_boundary_t;

// Build once, reuse for multiple cursor translations
line_boundary_t line_boundaries[MAX_LINES];
```

**Benefit**: If cursor is on line 5, skip lines 0-4 by jumping to cached position.

### Partial Translation

```c
// If cursor moves within same line, reuse previous translation
if (cursor_line == prev_cursor_line) {
    // Start from beginning of current line (cached position)
    // Only walk through current line
}
```

**Complexity**:
- Full translation: O(N) where N = buffer size
- Cached translation: O(M) where M = current line size
- Typical M << N (line ~50 bytes, buffer ~500 bytes)

---

## Testing Strategy

### Unit Tests

```c
// Test: Basic translation
assert(translate("hello", 0, ...) == (row=0, col=prompt_width));
assert(translate("hello", 5, ...) == (row=0, col=prompt_width+5));

// Test: Multi-byte UTF-8
assert(translate("Café", 3, ...) == (row=0, col=prompt_width+3));  // Start of 'é'
assert(translate("Café", 5, ...) == (row=0, col=prompt_width+4));  // After 'é'

// Test: Wide characters
assert(translate("中文", 3, ...) == (row=0, col=prompt_width+2));  // After '中'

// Test: ANSI codes
assert(translate("a\033[31mb", 5, ...) == (row=0, col=prompt_width+1));  // After 'a'

// Test: Tabs
assert(translate("a\tb", 2, ...) == (row=0, col=prompt_width+8));  // After tab

// Test: Newlines
assert(translate("line1\nline2", 6, ...) == (row=1, col=cont_prompt_width));

// Test: Line wrapping
char long_line[200];
memset(long_line, 'x', 199);
long_line[199] = '\0';
assert(translate(long_line, 100, 80, ...).row > 0);  // Wrapped to next row

// Test: Edge cases
assert(translate("", 0, ...) == (row=0, col=prompt_width));  // Empty
assert(translate("a", 1, ...) == (row=0, col=prompt_width+1));  // End of buffer
```

### Property-Based Tests

```c
// Property: Round-trip translation
for all valid (buffer, cursor):
    (row, col) = buffer_to_display(buffer, cursor)
    cursor2 = display_to_buffer(buffer, row, col)
    assert(cursor == cursor2)

// Property: Monotonicity
for all cursor1 < cursor2:
    (row1, col1) = buffer_to_display(buffer, cursor1)
    (row2, col2) = buffer_to_display(buffer, cursor2)
    assert((row1 < row2) || (row1 == row2 && col1 <= col2))
```

### Stress Tests

```c
// Test: Maximum buffer size
char huge_buffer[MAX_BUFFER_SIZE];
// Fill with various character types
// Verify translation succeeds and is correct

// Test: Maximum line count
char multiline[MAX_BUFFER_SIZE];
// Create MAX_LINES newlines
// Verify all line boundaries are correct

// Test: Very wide terminal
translate(buffer, cursor, 1000);  // 1000 column terminal

// Test: Very narrow terminal
translate(buffer, cursor, 10);   // 10 column terminal (lots of wrapping)
```

---

## Implementation Checklist

- [ ] Implement utf8_decode()
- [ ] Implement is_wide_character() (use wcwidth)
- [ ] Implement ansi_sequence_length()
- [ ] Implement tab_visual_width()
- [ ] Implement is_control_character()
- [ ] Implement calculate_visual_width() (for prompts)
- [ ] Implement translate_cursor_buffer_to_display()
- [ ] Implement translate_cursor_display_to_buffer()
- [ ] Write unit tests for all character types
- [ ] Write unit tests for all edge cases
- [ ] Write property-based tests (round-trip, monotonicity)
- [ ] Write stress tests (large buffers, many lines)
- [ ] Performance profiling (target <100 microseconds)
- [ ] Add caching for line boundaries (optimization)
- [ ] Documentation of algorithm in code comments

---

## References

1. **Replxx** - Modern readline alternative
   - Source: https://github.com/AmokHuginnsson/replxx
   - Algorithm: src/ConvertUTF.cpp, src/terminal.cxx

2. **Fish Shell** - Incremental cursor tracking
   - Source: https://github.com/fish-shell/fish-shell
   - Algorithm: src/reader.cpp (reader_data::screen_update_line)

3. **ZSH ZLE** - Z Shell Line Editor
   - Source: http://zsh.sourceforge.net/
   - Algorithm: Src/Zle/zle_refresh.c

4. **wcwidth()** - Character width detection
   - Reference: POSIX.1-2008 specification
   - Header: <wchar.h>

5. **UTF-8 Specification**
   - RFC 3629: UTF-8, a transformation format of ISO 10646
   - Reference: https://tools.ietf.org/html/rfc3629

---

## Conclusion

The cursor translation algorithm is the **critical component** that makes continuation prompts work correctly. Key principles:

1. **Incremental tracking is mandatory**: Formulas always fail
2. **Character-by-character is the only correct approach**: Researched from modern editors
3. **Handle all character types explicitly**: UTF-8, wide, ANSI, tabs, control chars
4. **Line wrapping vs logical lines**: Continuation prompts only on \n, not wraps
5. **Comprehensive edge case handling**: Empty lines, cursor on boundaries, etc.

This algorithm is proven correct by decades of line editor development and is used by Replxx, Fish, and ZSH.

**Next Phase**: Create implementation checklist for all Phase 1 deliverables.
