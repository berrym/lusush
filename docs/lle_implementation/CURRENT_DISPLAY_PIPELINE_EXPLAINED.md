# Current Display Pipeline Architecture (e237da6)

## Executive Summary

**Key Insight**: screen_buffer is NOT being used for differential updates. Instead, it's being used as a **VIRTUAL LAYOUT ENGINE** that calculates where everything should be positioned, accounting for line wrapping, wide characters, ANSI codes, and continuation prompts. The actual rendering uses "prompt-once + clear-and-redraw" approach.

## The Complete Pipeline

### 1. User Keypress → LLE Buffer Update

```
User types 'a'
  ↓
lle_readline.c: handle_character_input()
  ↓
lle_input_buffer: Insert character at cursor
  ↓
Mark buffer dirty
  ↓
Trigger redraw event
```

### 2. Redraw Event → Display Controller

```
display_controller.c: dc_handle_redraw_needed()
  ↓
Get prompt from prompt_layer
Get command from command_layer (with syntax highlighting)
  ↓
Check for multiline → analyze continuation prompts
```

### 3. The Critical Part: screen_buffer_render()

**This is where the magic happens for line wrapping!**

```c
screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);
```

**What screen_buffer_render() does:**

1. **Creates a VIRTUAL representation** of how the terminal WOULD look
2. **Simulates text layout** character by character:
   - Tracks current row/column position
   - Detects when text reaches terminal width → wrap to next row
   - Handles UTF-8 multi-byte characters (1-4 bytes)
   - Calculates visual width (1 column for ASCII, 2 for CJK/emoji)
   - Skips ANSI escape sequences (don't count toward position)
   - Accounts for continuation prompt widths after newlines
   - Handles tabs (expand to next 8-column boundary)

3. **Calculates critical metadata:**
   - `command_start_row` - which row the command starts on
   - `command_start_col` - which column the command starts at
   - `cursor_row` - which row the cursor should be on
   - `cursor_col` - which column the cursor should be at
   - `num_rows` - total rows occupied by prompt + command

**The Virtual Screen:**

```
Row 0: [P][r][o][m][p][t][:][█]                  command_start_col = 8
                              ↑ command starts here
Row 1: [l][o][n][g][ ][c][o][m][m][a][n][d][ ][t]
Row 2: [h][a][t][ ][w][r][a][p][s][ ][a][r][o][u][n][d]
                    ↑ cursor here               cursor_row = 2, cursor_col = 9
Row 3: [█][█][█][█][█][█][█][█][█][█][█][█][█][█]
```

### 4. Terminal Output (Clear and Redraw)

**First render only:**
```c
if (!prompt_rendered) {
    write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
    prompt_rendered = true;
}
```

**Every subsequent render:**

```c
// Step 1: Move to command start position
write("\033[{command_start_col+1}G");  // Absolute column

// Step 2: Move up if needed (multi-line prompts)
if (current_screen.cursor_row > command_row) {
    write("\033[{rows_up}A");
}

// Step 3: CLEAR everything from here to end of screen
write("\033[J");  // Clear to end of screen

// Step 4: Write command text with continuation prompts
for each line in command_buffer:
    write(line);
    write("\n");
    if (has_next_line):
        write(continuation_prompt);  // e.g., "loop> "

// Step 5: Write completion menu (if active)
if (menu_text):
    write("\n");
    write(menu_text);

// Step 6: Position cursor using screen_buffer's calculated position
int rows_to_move_up = (final_row - cursor_row) + menu_lines;
write("\033[{rows_to_move_up}A");     // Relative movement up
write("\033[{cursor_col+1}G");        // Absolute column
```

### 5. State Tracking

```c
// After render, save state for next cycle
screen_buffer_copy(&current_screen, &desired_screen);
```

This allows Step 2 to know where we left the cursor last time.

## Why screen_buffer is Essential for Line Wrapping

**Without screen_buffer (naive approach):**

```c
// Count visual columns naively
int col = 0;
for (char *p = text; *p; p++) {
    col++;  // WRONG! Doesn't handle:
            // - Multi-byte UTF-8 (ö = 2 bytes but 1 column)
            // - Wide characters (中 = 3 bytes but 2 columns!)
            // - ANSI codes (^[[31m = 5 bytes but 0 columns!)
            // - Tabs (1 byte but 8 columns!)
    if (col >= terminal_width) {
        // Insert newline? No! Breaks ANSI codes mid-sequence!
    }
}
```

**This breaks horribly with:**
- Syntax highlighting (ANSI codes counted as characters)
- International text (byte count ≠ visual width)
- Tab characters
- Wide emoji

**With screen_buffer (correct approach):**

```c
screen_buffer_render(&buffer, prompt, command, cursor_offset);

// Now we know:
// - Exactly which row/col the cursor should be at
// - Accounting for ALL wrapping, ANSI codes, UTF-8, etc.
// - Without modifying the actual command text!

// Just position cursor to buffer.cursor_row/col
```

## Key Architectural Decisions

### Why NOT Use Differential Updates?

**Current approach (clear-and-redraw):**
- ✅ Simple and reliable
- ✅ Works perfectly for short commands (< 10 lines)
- ✅ No state synchronization bugs
- ❌ Redraws entire command area every keystroke
- ❌ Can cause flicker on slow terminals

**Differential approach (what I tried to implement):**
- ✅ More efficient (only update changes)
- ✅ No flicker
- ❌ Complex state management (current_screen vs desired_screen)
- ❌ Cursor position tracking across multiple renders
- ❌ Easy to get wrong (as demonstrated!)

**Decision**: For LLE's use case (interactive editing of relatively short commands), clear-and-redraw is simpler and "fast enough". Differential updates would be an optimization, not a necessity.

### Why Prompt-Once Architecture?

**Problem with redrawing prompt:**
```
terminal_row_5: [prompt]> command text here
                          ↑ cursor

User types 'x'
  ↓
Clear screen with \033[2J
  ↓
Redraw from top: [prompt]> command text herex
  ↑
  Entire screen flashes, prompt scrolls if at bottom
```

**Prompt-once solution:**
```
First render:
  write(prompt);  // Draw once
  mark prompt_rendered = true

Subsequent renders:
  Move to command_start_col (AFTER prompt)
  Clear from here to end (\033[J)
  Redraw only command area
  ↑
  Prompt never touched, no flicker!
```

### Why Use Absolute Column Positioning?

**From MODERN_EDITOR_WRAPPING_RESEARCH.md:**

```c
// WRONG: \r moves to column 0
write("\r");  // Cursor now at column 0
write("\033[J");  // CLEARS FROM COLUMN 0 = DESTROYS PROMPT!

// CORRECT: Absolute column positioning
write("\033[{command_start_col+1}G");  // Move to exact column
write("\033[J");  // Only clears command area
```

## The Two screen_buffer Instances

```c
static screen_buffer_t current_screen;   // Last rendered state
static screen_buffer_t desired_screen;   // New state to render
```

**Current usage:**
- `desired_screen` - Virtual layout engine (calculated every render)
- `current_screen` - Tracks where cursor was left (for Step 2 positioning)

**NOT currently used:**
- `screen_buffer_diff()` - Would compare current vs desired
- `screen_buffer_apply_diff()` - Would output only changes

## Why Line Wrapping Works Perfectly

**The Critical Flow:**

1. `screen_buffer_render()` simulates layout:
   ```
   "echo hello world this is a very long command that wraps"
   Terminal width = 40
   
   Simulation:
   col=0: 'e' → col=1
   col=1: 'c' → col=2
   ...
   col=39: 'w' → col=40 → WRAP! → row++, col=0
   col=0: 'o' → col=1
   ...
   
   Result: cursor_row=1, cursor_col=15
   ```

2. Terminal output writes the ACTUAL text:
   ```
   write("echo hello world this is a very long command that wraps");
   ```
   Terminal's OWN wrapping logic wraps at column 40 (same place!)

3. Cursor positioning uses calculated position:
   ```
   write("\033[1A");   // Move up to row 1
   write("\033[16G");  // Move to column 15
   ```

**They match perfectly because:**
- screen_buffer uses terminal_width from terminfo
- Terminal wraps at same terminal_width
- screen_buffer's simulation matches terminal's behavior

## Continuation Prompts Integration

```c
// In screen_buffer_render():
if (ch == '\n') {
    row++;
    
    // Check for continuation prompt on next row
    size_t prefix_width = screen_buffer_get_line_prefix_visual_width(buffer, row);
    col = (int)prefix_width;  // Start AFTER prompt!
    ...
}
```

**Effect:**
```
Row 0: prompt> for i in 1 2 3; do
Row 1: loop>     echo $i           ← col starts at 6 (after "loop> ")
Row 2: loop>   done
              ↑ cursor_col accounts for prefix width
```

## Current Issues at e237da6

### TAB Completion Menu Rendering Corruption

**Symptom**: Menu formatting changes when cycling with TAB (spaces disappear)

**Likely Cause**: 
- Menu text contains ANSI codes for highlighting selected item
- screen_buffer_render() is called with menu text appended to command
- OR menu text is written separately but its ANSI codes aren't properly handled

**Why Everything Else Works:**
- Syntax highlighting works because command_layer outputs ANSI codes
- screen_buffer_render() skips ANSI codes correctly in cursor position calculation
- History recall works because it just replaces command text
- Multiline works because continuation prompts are properly accounted for

**What's Different About Menu:**
- Menu is written AFTER screen_buffer calculations
- Menu row count is calculated separately: `menu_lines = 1 + count('\n')`
- Final cursor positioning accounts for menu_lines
- BUT: If menu contains ANSI codes, they might corrupt cursor calculation

## Summary

**screen_buffer's role:**
- 🎯 **Virtual layout engine** that simulates how text will wrap
- 🎯 **Cursor position calculator** accounting for UTF-8, ANSI, wrapping
- 🎯 **Metadata provider** (command_start_row/col, cursor_row/col, num_rows)
- ❌ **NOT used for differential updates** (those functions exist but aren't called)

**Why it works so well:**
- Separates "calculating layout" from "outputting text"
- Single source of truth for cursor positioning
- Handles all edge cases (UTF-8, ANSI, wide chars, tabs) in one place
- Display controller just uses the calculated positions

**The genius:**
- Command text flows naturally with terminal wrapping
- screen_buffer PREDICTS where wrapping will occur
- Cursor positioning uses predictions → always correct
- Prompt-once architecture prevents flicker
- Clear-and-redraw is simple and reliable

This architecture is actually quite elegant for its purpose!
