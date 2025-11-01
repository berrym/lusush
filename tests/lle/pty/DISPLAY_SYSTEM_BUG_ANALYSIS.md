# Display System Bug Analysis - Missing Terminal Control Integration

## Issue Summary

The Lusush layered display system is **missing the integration between composition output and terminal control sequences**. The display controller composes prompt + command through the composition engine, but never adds the necessary terminal control sequences for line clearing and cursor positioning.

## Architectural Finding

### Current Flow (BROKEN)
```
LLE calls display_controller_display()
  ↓
Display controller composes layers (prompt + command)
  ↓
Returns composed text: "[ANSI colors]prompt[ANSI colors]command"
  ↓
LLE outputs to terminal: printf("%s", composed_output)
  ↓
PROBLEM: Output just appends, no line clearing or cursor positioning!
```

### Expected Flow (per specification)
```
LLE calls display_controller_display()
  ↓
Display controller composes layers
  ↓
Display controller wraps with terminal control:
  - \r (carriage return)
  - \e[K (clear line)
  - [composed content]
  - \e[{col}G (position cursor)
  ↓
Returns complete terminal-ready output
  ↓
LLE outputs: printf("%s", output)
```

## Evidence

### 1. Display Controller Code (`display_controller.c:745-995`)

The `display_controller_display()` function:
- ✅ Initializes layers
- ✅ Calls composition engine
- ✅ Gets composed output
- ❌ **NEVER adds terminal control sequences**
- ❌ **NEVER calls terminal_control layer functions**

### 2. Terminal Control Layer EXISTS (`terminal_control.c`)

The terminal control layer has all necessary functions:
- `terminal_control_clear_line()` - Sends `\e[2K`
- `terminal_control_move_cursor(row, col)` - Sends cursor positioning
- `terminal_control_get_cursor_position()` - Tracks cursor

**But these functions are NEVER CALLED by the display controller!**

### 3. Composition Engine Output

The composition engine (`composition_engine.c`) correctly combines prompt and command layers:
- Handles multiline prompts
- Adds proper spacing
- Preserves ANSI color codes

**But it does NOT add terminal control** (and shouldn't - that's not its job).

## The Missing Integration

There's a **critical missing step** between composition and output:

### What Should Happen (Spec-Compliant)

After `composition_engine_get_output()`, the display controller should:

```c
// After getting composed output from composition engine:
char final_output[DISPLAY_CONTROLLER_MAX_OUTPUT];

// Step 1: Calculate cursor position from internal state
size_t cursor_column = calculate_cursor_column(prompt_visible_length, command_cursor_pos);

// Step 2: Wrap composed output with terminal control
snprintf(final_output, sizeof(final_output),
         "\r"              // Go to start of line
         "\033[K"          // Clear line
         "%s"              // Composed content
         "\033[%zuG",      // Position cursor at column
         composed_output,
         cursor_column + 1);

// Step 3: Return terminal-ready output
strcpy(output, final_output);
```

OR use the terminal_control layer:

```c
// Build output with terminal control functions
terminal_control_move_cursor(control, current_row, 1);  // Start of line
terminal_control_clear_line(control);                   // Clear
// Output composed content
terminal_control_move_cursor(control, current_row, cursor_column);  // Position cursor
```

## Root Cause

The display controller was implemented with layer composition but **the final terminal control wrapping step was never implemented**.

This is the first time LLE is being used (GNU readline bypasses this system), so this bug was never discovered.

## Required Fix

### Option 1: Display Controller Adds Terminal Control

Modify `display_controller_display()` to wrap composed output with terminal control sequences before returning.

**Pros:**
- Keeps LLE architecture pure (no terminal knowledge)
- Central control point for all display output
- Consistent behavior across all callers

**Cons:**
- Display controller needs cursor position information
- Requires passing cursor state through the API

### Option 2: Terminal Control Layer Integration

Add a new layer between composition and output that manages terminal control.

**Pros:**
- Clean separation of concerns
- Reusable for other display scenarios
- Matches layered architecture better

**Cons:**
- More complex integration
- New layer to maintain

### Option 3: Display Controller Uses Terminal Control Layer

Display controller should have a `terminal_control_t` instance and use it to wrap output.

**Pros:**
- Uses existing terminal_control API
- Terminal control handles capability detection
- Proper abstraction

**Cons:**
- Display controller becomes more complex
- Needs to manage terminal control state

## Recommendation

**Option 3** is most architecturally sound:

1. Display controller should own a `terminal_control_t` instance
2. After composition, use terminal_control functions to build final output
3. Cursor position should be calculated from:
   - Prompt visible length (excluding ANSI codes)
   - Command buffer cursor position
4. All terminal control goes through the terminal_control layer

## Implementation Plan

### Phase 1: Add Terminal Control to Display Controller

```c
// In display_controller.h
typedef struct display_controller_t {
    // ... existing fields ...
    terminal_control_t *terminal_control;  // NEW
} display_controller_t;

// In display_controller_display()
// After getting composed output:
terminal_control_t *tc = controller->terminal_control;

// Clear and reposition
terminal_control_move_cursor(tc, current_row, 1);
terminal_control_clear_line(tc);

// Output composed content (this should go to terminal_control too)
// ... need to refactor how output is generated ...

// Position cursor
size_t cursor_col = calculate_cursor_position(...);
terminal_control_move_cursor(tc, current_row, cursor_col);
```

### Phase 2: Refactor Output Generation

The display controller currently returns a string. It should probably:
- Build output incrementally using terminal_control functions
- Return terminal-ready output with all control sequences

### Phase 3: Pass Cursor Information

The display controller needs to know:
- Current cursor position in command buffer
- Visible length of prompt (for cursor column calculation)

This might require API changes or additional parameters.

## Current Status

- ✅ Buffer corruption bug fixed (previous session)
- ✅ Composition engine working correctly
- ✅ Terminal control layer exists and has all needed functions
- ❌ **Display controller doesn't use terminal control layer**
- ❌ **No integration between composition and terminal output**

## Impact

This bug affects:
- **All LLE display operations** (typing, cursor movement, editing)
- **Only visible when LLE is used** (GNU readline doesn't use this system)
- **PTY tests exposed it** (automated testing working as designed!)

The bug is in the display system infrastructure, not in LLE itself. LLE is correctly using the display API - the display API just isn't complete yet.

## Next Steps

1. Add `terminal_control_t` instance to display controller
2. Implement terminal control wrapping in `display_controller_display()`
3. Add cursor position calculation
4. Test with LLE
5. Validate with PTY tests

This is infrastructure work that will benefit all display system users, not just LLE.
