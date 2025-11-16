# Continuation Prompt Implementation Plan: Screen Buffer Enhancement

**Date**: 2025-11-16  
**Session**: Continuation from Session 16 investigation  
**Status**: Implementation Plan - Ready for Execution  
**Priority**: HIGH - Fixes critical line wrapping bug in multiline input

---

## Executive Summary

This document outlines the correct implementation plan for continuation prompts with proper line wrapping support in LLE. After extensive research of Replxx, Fish, and ZLE implementations (documented in MODERN_EDITOR_WRAPPING_RESEARCH.md), the solution is clear:

**The Problem:**
- Line wrapping in the middle of multiline input causes display corruption
- Continuation prompts (quote>, loop>, etc.) need proper width tracking
- Current screen_buffer only handles ONE prompt (the main prompt)

**The Solution:**
- Enhance the EXISTING `screen_buffer_render()` function to support per-line prefixes
- Implement differential updates (current vs desired screen state)
- Follow the "prompt-once" architecture for continuation prompts

**Critical Learning:**
My initial approach in this session was WRONG. I tried to:
- Create new functions (screen_buffer_render_with_colors)
- Modify composition_engine
- Add complexity instead of enhancing what exists

The CORRECT approach is to enhance screen_buffer_render() incrementally.

---

## What Already Exists (Commit c26e96d)

### Working Components

1. **continuation_prompt_layer** (fully implemented):
   - `src/display/continuation_prompt_layer.c`
   - `include/display/continuation_prompt_layer.h`
   - Generates context-aware prompts (quote>, loop>, if>, etc.)
   - Has caching and performance tracking
   - **Status**: Complete, tested, working

2. **screen_buffer** (partial implementation):
   - `src/display/screen_buffer.c`
   - `include/display/screen_buffer.h`
   - Has `screen_buffer_render()` function
   - Handles main prompt correctly
   - **Missing**: Per-line prefix support for continuation prompts

3. **input_continuation** (working):
   - `src/input_continuation.c`
   - Detects multiline state (quotes, loops, etc.)
   - Provides `continuation_get_prompt()` function
   - **Status**: Working correctly

### What Works

- Main prompt displays correctly
- Basic line editing works
- Syntax highlighting works
- Line wrapping works for SINGLE-line input

### What Doesn't Work

**The ONE bug**: Line wrapping in the middle of multiline input causes display corruption.

**Example that fails:**
```bash
$ for i in 1 2 3; do
loop> echo "this is a very long line that wraps when it exceeds terminal width"
loop> done
```

When the second line wraps, the display corrupts because screen_buffer doesn't know the line starts at column 6 (after "loop> "), not column 0.

---

## Research Findings Summary

From analyzing Replxx, Fish, and ZLE (MODERN_EDITOR_WRAPPING_RESEARCH.md):

### Universal Principles

All three modern editors follow these principles:

1. **Prompt-Once Architecture**:
   - Prompt written ONCE, never redrawn during editing
   - Only command text is updated incrementally
   - Prevents prompt flickering and destruction

2. **Incremental Cursor Tracking**:
   - Walk through text character by character
   - Track visual column accounting for UTF-8 width
   - NO division/modulo formulas (they break with UTF-8)

3. **Absolute Column Positioning**:
   - Use `\033[{n}G` to move to absolute column
   - NEVER use `\r` (carriage return to column 0) after moving up
   - Prevents destroying the prompt

4. **Differential Updates**:
   - Track current vs desired screen state
   - Only output what changed
   - Minimizes terminal I/O

### Key Code Pattern (from Replxx)

```cpp
// Replxx virtual_render() - cursor calculation
void virtual_render(/* ... */) {
    int x_ = promptLen_;  // Start at prompt position
    int y_ = 0;
    
    for (char32_t c : text) {
        if (c == '\n') {
            ++y_;
            x_ = promptLen_;  // Return to prompt indentation!
        } else {
            x_ += mk_wcwidth(c);  // UTF-8 aware width
            if (x_ >= screenColumns_) {
                x_ = 0;
                ++y_;
            }
        }
    }
}
```

**Critical insight**: After newline, `x_ = promptLen_` (not 0!)

But for continuation prompts, each line has a DIFFERENT prompt width!

---

## The Correct Architecture

### Current Flow (Working for Single-Line)

```
LLE → display_bridge → command_layer → display_controller → screen_buffer_render() → terminal
                                                              ↑
                                                   Handles main prompt correctly
```

### What We Need for Multiline

```
screen_buffer_render() needs to:
1. Split command_text by newlines (logical lines)
2. For logical_line 0: use main prompt width
3. For logical_line > 0: get continuation prompt and use its width
4. Track visual row across all logical lines
5. Calculate cursor position accounting for all prefixes
```

### Screen Buffer Structure Enhancement

**Current structure** (from screen_buffer.h):
```c
typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;  // Number of cells used
} screen_line_t;

typedef struct {
    screen_line_t lines[SCREEN_BUFFER_MAX_ROWS];
    int num_rows;
    int cursor_row;
    int cursor_col;
    int command_start_row;
    int command_start_col;
    int terminal_width;
} screen_buffer_t;
```

**What we need to add**:
```c
typedef struct {
    char *text;              // Prefix text (e.g., "quote> ")
    size_t length;           // Byte length
    size_t visual_width;     // Visual width (UTF-8 aware)
} line_prefix_t;

typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;
    line_prefix_t *prefix;   // NEW: Continuation prompt for this line
} screen_line_t;
```

---

## Implementation Plan

### Phase 1: Add Prefix Support to screen_buffer_t

**File**: `include/display/screen_buffer.h`

**Changes**:
1. Add `line_prefix_t` structure
2. Add `prefix` field to `screen_line_t`
3. Add functions:
   - `screen_buffer_set_line_prefix(buffer, row, prefix_text)`
   - `screen_buffer_get_line_prefix_visual_width(buffer, row)`

**Why**: Allows each logical line to have its own prefix (continuation prompt)

### Phase 2: Enhance screen_buffer_render()

**File**: `src/display/screen_buffer.c`

**Current signature**:
```c
void screen_buffer_render(
    screen_buffer_t *buffer,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset
);
```

**Changes to implementation**:

1. **Split command by newlines**:
   ```c
   const char *line_start = command_text;
   int logical_line = 0;
   int visual_row = 0;
   
   while (line_start) {
       const char *line_end = strchr(line_start, '\n');
       size_t line_len = line_end ? (line_end - line_start) : strlen(line_start);
       // Process this logical line...
   }
   ```

2. **Get continuation prompt for each line**:
   ```c
   size_t prefix_width = 0;
   
   if (logical_line == 0) {
       // First line uses main prompt
       prefix_width = calculate_prompt_visual_width(prompt_text);
   } else {
       // Subsequent lines get continuation prompt
       char cont_prompt[128];
       continuation_get_prompt_for_line(logical_line, command_text, 
                                        cont_prompt, sizeof(cont_prompt));
       prefix_width = calculate_visual_width(cont_prompt);
   }
   ```

3. **Walk through characters with prefix-aware wrapping**:
   ```c
   int visual_col = prefix_width;  // Start after prefix
   size_t byte_offset = 0;
   
   while (byte_offset < line_len) {
       uint32_t codepoint;
       int bytes = lle_utf8_decode_codepoint(line_start + byte_offset, 
                                             line_len - byte_offset, 
                                             &codepoint);
       int char_width = lle_utf8_codepoint_width(codepoint);
       
       // Check for wrap
       if (visual_col + char_width > buffer->terminal_width) {
           visual_row++;
           visual_col = char_width;  // Character goes on new visual line
       } else {
           visual_col += char_width;
       }
       
       // Track cursor position
       if (global_byte_offset == cursor_byte_offset) {
           buffer->cursor_row = visual_row;
           buffer->cursor_col = visual_col;
       }
       
       byte_offset += bytes;
       global_byte_offset += bytes;
   }
   ```

4. **Move to next logical line**:
   ```c
   if (line_end) {
       visual_row++;  // Newline moves to next visual row
       line_start = line_end + 1;
       logical_line++;
   } else {
       line_start = NULL;
   }
   ```

**Key point**: Each logical line tracks independently with its own prefix width.

### Phase 3: Update display_controller to Use Continuation Prompts

**File**: `src/display/display_controller.c`

**Current code** (lines ~220-260):
```c
// Writes prompt once
if (!prompt_rendered) {
    write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
    prompt_rendered = true;
}

// Positions cursor and writes command
screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);
```

**Changes needed**:

1. **Check if multiline**:
   ```c
   int newline_count = count_newlines(cmd_layer->command_text);
   bool is_multiline = (newline_count > 0);
   ```

2. **Pass continuation prompt info to screen_buffer_render**:
   - Option A: Modify screen_buffer_render signature to accept continuation_prompt_layer
   - Option B: screen_buffer_render calls continuation_get_prompt internally
   
   **Recommendation**: Option B - keep interface simple, screen_buffer_render handles it internally

3. **Render with continuation prompts**:
   After screen_buffer_render calculates layout, output to terminal:
   ```c
   // First render: write main prompt
   if (!prompt_rendered) {
       write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
       prompt_rendered = true;
   }
   
   // Subsequent renders: position at command start
   else {
       // Use absolute column positioning (never use \r!)
       char move_to_col[32];
       snprintf(move_to_col, sizeof(move_to_col), "\033[%dG", 
                (int)(prompt_visual_width + 1));
       write(STDOUT_FILENO, move_to_col, strlen(move_to_col));
       
       // Move up if needed
       if (current_screen.cursor_row > 0) {
           char move_up[32];
           snprintf(move_up, sizeof(move_up), "\033[%dA", 
                    current_screen.cursor_row);
           write(STDOUT_FILENO, move_up, strlen(move_up));
       }
       
       // Clear to end of screen
       write(STDOUT_FILENO, "\033[J", 3);
   }
   
   // Output command text with continuation prompts at newlines
   if (is_multiline) {
       const char *line = command_buffer;
       int line_num = 0;
       
       while (*line) {
           const char *nl = strchr(line, '\n');
           
           if (nl) {
               // Write line up to newline
               write(STDOUT_FILENO, line, nl - line);
               write(STDOUT_FILENO, "\n", 1);
               
               line_num++;
               line = nl + 1;
               
               // Write continuation prompt for next line
               if (*line) {  // Only if there's more content
                   char cont_prompt[128];
                   continuation_get_prompt_for_line(line_num, 
                                                     cmd_layer->command_text,
                                                     cont_prompt, 
                                                     sizeof(cont_prompt));
                   write(STDOUT_FILENO, cont_prompt, strlen(cont_prompt));
               }
           } else {
               // Last line
               write(STDOUT_FILENO, line, strlen(line));
               break;
           }
       }
   } else {
       // Simple case: no newlines
       write(STDOUT_FILENO, command_buffer, strlen(command_buffer));
   }
   
   // Position cursor using calculated coordinates from screen_buffer
   // ... (existing cursor positioning code)
   ```

### Phase 4: Implement Differential Updates (Future Optimization)

**File**: `src/display/screen_buffer.c`

This is an optimization for the future. Not required for initial functionality.

**Concept**:
```c
typedef struct {
    screen_buffer_t current;   // What's on screen now
    screen_buffer_t desired;   // What should be on screen
    bool dirty[SCREEN_BUFFER_MAX_ROWS];  // Which rows changed
} screen_state_t;

void screen_buffer_apply_diff(screen_state_t *state, int fd) {
    for (int row = 0; row < state->desired.num_rows; row++) {
        if (state->dirty[row]) {
            // Position cursor at start of row
            // Output only the changed portion
            // Update current to match desired
        }
    }
}
```

**Benefits**:
- Minimal terminal I/O
- No flicker
- Better performance

**Implementation**: After basic functionality works and is tested.

---

## Testing Plan

### Test 1: Basic Multiline with Continuation Prompts

```bash
$ for i in 1 2 3; do
loop> echo $i
loop> done
```

**Expected**: 
- "loop> " appears on lines 2 and 3
- Cursor positioning correct
- No display corruption

### Test 2: Line Wrapping in Multiline Input

```bash
$ for i in 1 2 3; do
loop> echo "this is a very long line that will definitely wrap when it exceeds the terminal width and continues on the next visual line"
loop> done
```

**Expected**:
- Line 2 wraps naturally at terminal width
- Wrapped portion does NOT show "loop> " prefix
- Cursor movement across wrap works correctly
- No display corruption

### Test 3: Quote Continuation

```bash
$ echo "hello
quote> world"
```

**Expected**:
- "quote> " appears on line 2
- Closing quote works
- Command executes correctly

### Test 4: Nested Structures

```bash
$ for i in 1 2 3; do
loop> if [ $i -eq 2 ]; then
if> echo "found two"
if> fi
loop> done
```

**Expected**:
- Context-aware prompts (loop>, if>)
- Proper nesting detection
- All prompts display correctly

### Test 5: UTF-8 in Multiline

```bash
$ echo "emoji test
quote> 🎉🎊🎈"
```

**Expected**:
- UTF-8 characters display correctly
- Width calculations correct (emoji = 2 columns)
- Cursor positioning correct

---

## Implementation Steps (Exact Order)

### Step 1: Add Prefix Support to screen_buffer.h

1. Add `line_prefix_t` structure
2. Add `prefix` field to `screen_line_t`
3. Add function declarations
4. **Test**: Code compiles

### Step 2: Implement Prefix Functions in screen_buffer.c

1. Implement `screen_buffer_set_line_prefix()`
2. Implement `screen_buffer_get_line_prefix_visual_width()`
3. Update `screen_buffer_clear()` to handle prefixes
4. Update `screen_buffer_init()` to initialize prefix pointers
5. **Test**: Code compiles, basic tests pass

### Step 3: Enhance screen_buffer_render()

1. Add logic to split command by newlines
2. Add logic to get continuation prompts for each line
3. Implement prefix-aware cursor tracking
4. Calculate cursor position accounting for all prefixes
5. **Test**: Code compiles, cursor calculations correct

### Step 4: Update display_controller.c

1. Add `count_newlines()` helper function
2. Modify command output section to render continuation prompts
3. Ensure prompt-once architecture maintained
4. Use absolute column positioning (no `\r` after move up)
5. **Test**: Multiline input displays correctly

### Step 5: Integration Testing

1. Test all 5 test cases above
2. Fix any bugs found
3. Test edge cases (empty lines, very long commands, etc.)
4. **Test**: All scenarios work correctly

### Step 6: Documentation and Commit

1. Update ARCHITECTURE_IMPACT_ANALYSIS.md with screen_buffer changes
2. Update AI_ASSISTANT_HANDOFF_DOCUMENT.md
3. Commit with message: "Display: Add continuation prompt support to screen_buffer"

---

## Critical Don'ts (Lessons Learned)

### Don't Do What I Did This Session

❌ **Don't create new rendering functions** (screen_buffer_render_with_colors)
✅ **Do enhance existing functions** (screen_buffer_render)

❌ **Don't modify composition_engine** for this
✅ **Do keep changes in screen_buffer and display_controller**

❌ **Don't add debug statements everywhere**
✅ **Do write focused, clean code**

❌ **Don't make big architectural changes**
✅ **Do make incremental, focused enhancements**

❌ **Don't commit without explicit approval**
✅ **Do document plans and get approval first**

### Key Principles

1. **Enhance, don't replace**: Work with existing code
2. **Incremental changes**: Small, testable steps
3. **Test after each step**: Verify nothing breaks
4. **Follow research**: Use proven patterns from Replxx/Fish/ZLE
5. **Keep it simple**: Minimal changes for maximum effect

---

## Why This Will Work

### Architectural Soundness

1. **Maintains boundaries**: LLE still knows nothing about terminals
2. **Follows research**: Uses proven patterns from modern editors
3. **Incremental**: Enhances existing working code
4. **Testable**: Each step can be verified independently

### Technical Correctness

1. **UTF-8 aware**: Uses existing UTF-8 width calculation
2. **Prompt-once**: Follows established architecture
3. **Incremental tracking**: No division/modulo cursor math
4. **Absolute positioning**: Prevents prompt destruction

### Practicality

1. **Focused scope**: Only touches screen_buffer and display_controller
2. **Backward compatible**: Single-line input still works
3. **Clean implementation**: No hacks or workarounds
4. **Well-tested approach**: Based on production editor code

---

## Success Criteria

Implementation is complete when:

1. ✅ Multiline input shows continuation prompts at newlines
2. ✅ Line wrapping works correctly in middle of multiline input
3. ✅ Cursor positioning correct across wraps and newlines
4. ✅ No display corruption or flickering
5. ✅ All 5 test cases pass
6. ✅ UTF-8 and wide characters work correctly
7. ✅ Code is clean, well-commented, and minimal

---

## Next Session Pickup

When starting next session:

1. Read AI_ASSISTANT_HANDOFF_DOCUMENT.md
2. Read this document (CONTINUATION_PROMPT_SCREEN_BUFFER_PLAN.md)
3. Read MODERN_EDITOR_WRAPPING_RESEARCH.md for context
4. Confirm understanding of the plan
5. Get explicit approval to proceed with Step 1
6. Execute implementation steps in order
7. Test after each step
8. Get approval before committing

**Do not deviate from this plan without user approval.**

---

**Document Version**: 1.0  
**Status**: Ready for Implementation  
**Estimated Implementation Time**: 4-6 hours (with testing)  
**Risk Level**: LOW (enhancing existing working code)  
**Expected Outcome**: Fully functional continuation prompts with correct line wrapping
