# Research: Line Wrapping in Modern Line Editors

**Date:** 2025-11-04  
**Author:** Research Investigation  
**Purpose:** Document EXACTLY how Replxx, Fish, and ZLE handle line wrapping

## Executive Summary

After analyzing the actual source code of three modern line editors (Replxx, Fish shell, ZLE), I've discovered **the critical mistake in our implementation**:

### The Problem
**We're trying to move the cursor back to the beginning and redraw everything, but we're DESTROYING THE PROMPT in the process.**

The key issue: When text wraps, we move up N rows to get back to the start, then do `\r` (move to column 0), then try to move forward `prompt_width` columns. **This positions us AFTER where the prompt was, but we just cleared everything with `\033[J`, so the prompt is GONE.**

### The Solution (How All Three Editors Do It)

**ALL THREE EDITORS USE THE SAME FUNDAMENTAL APPROACH:**

1. **The prompt and command buffer are SEPARATE entities**
2. **The prompt is written ONCE and NEVER moved/redrawn during editing**
3. **Only the command text portion is updated incrementally**
4. **Cursor position is calculated incrementally during rendering, NOT via division/modulo**

---

## 1. Replxx Implementation

### Source Code Location
- Repository: https://github.com/AmokHuginnsson/replxx
- Key files:
  - `src/replxx_impl.cxx` - Main refresh logic
  - `src/util.cxx` - Virtual render and cursor calculation
  - `src/terminal.cxx` - Terminal control

### How Replxx Works

#### A. Prompt Handling
```cpp
// From replxx_impl.cxx - refresh_line()
// The prompt is written separately by _prompt.write()
// During refresh, the prompt is NOT redrawn:

void Replxx::ReplxxImpl::refresh_line( HINT_ACTION hintAction_ ) {
    render( hintAction_ );
    handle_hints( hintAction_ );
    
    // Calculate desired cursor position
    int xCursorPos( _prompt.indentation() );
    int yCursorPos( 0 );
    virtual_render( _data.get(), _pos, xCursorPos, yCursorPos );
    
    // Position cursor and clear previous content
    _terminal.set_cursor_visible( false );
    _terminal.jump_cursor( _prompt.indentation(), 
                          -( _prompt._cursorRowOffset - _prompt._extraLines ) );
    
    // Write display buffer (command text only, NOT prompt)
    _terminal.write32( _display.data(), static_cast<int>( _display.size() ) );
    
    // Position cursor
    _terminal.jump_cursor( xCursorPos, -( yEndOfInput - yCursorPos ) );
    _terminal.set_cursor_visible( true );
}
```

**Key Points:**
- Prompt is written ONCE by `_prompt.write()` in `repaint()` function
- `refresh_line()` does NOT rewrite the prompt
- `_prompt.indentation()` returns the column where command starts (after prompt)
- Cursor positioning is relative to prompt indentation

#### B. Cursor Position Calculation
```cpp
// From util.cxx - virtual_render()
void virtual_render( /* ... */ ) {
    int x_ = promptLen_;  // Start at prompt position
    int y_ = 0;
    
    auto advance_cursor = [&]( int by_ = 1 ) {
        x_ += by_;
        if ( x_ >= screenColumns_ ) {
            x_ = 0;
            ++y_;
            wrapped = true;
        }
    };
    
    for ( char32_t c : text ) {
        if ( c == '\n' ) {
            ++y_;
            x_ = promptLen_;  // Return to prompt indentation, not column 0!
        } else {
            advance_cursor( mk_wcwidth(c) );
        }
    }
}
```

**Critical Insight:** 
- Cursor tracking is **incremental** - walk through text character by character
- **NOT** calculated via division/modulo
- After newline, x position returns to `promptLen_`, not 0
- Handles UTF-8 properly by using `mk_wcwidth()` for character width

#### C. Terminal Control
```cpp
// From terminal.cxx
void Terminal::jump_cursor( int xOffset_, int yOffset_ ) {
    // Unix/Linux: Uses ANSI escape sequences
    if ( yOffset_ != 0 ) {
        char seq[16];
        if ( yOffset_ > 0 ) {
            snprintf( seq, sizeof(seq), "\033[%dB", yOffset_ );  // Down
        } else {
            snprintf( seq, sizeof(seq), "\033[%dA", -yOffset_ ); // Up
        }
        write8( seq, strlen(seq) );
    }
    
    if ( xOffset_ != 0 ) {
        // Move to absolute column (1-based)
        char seq[16];
        snprintf( seq, sizeof(seq), "\033[%dG", xOffset_ + 1 );
        write8( seq, strlen(seq) );
    }
}
```

**Key Point:** Uses `\033[{n}G` to position at absolute column, avoiding the `\r` problem

---

## 2. Fish Shell Implementation

### Source Code Location
- Repository: https://github.com/fish-shell/fish-shell
- Key files:
  - `src/screen.rs` - Screen refresh and cursor management
  - `src/reader/mod.rs` - Reader event loop

### How Fish Works

#### A. Differential Screen Updates
```rust
// From screen.rs - update()
fn update(&mut self, vars: &dyn Environment, left_prompt: &wstr, right_prompt: &wstr) {
    // Compare desired vs actual screen state
    let shared_prefix = line_shared_prefix(o_line(self, i), s_line(self, i));
    
    // Only output differences, skipping unchanged portions
    // This avoids redrawing the prompt unless it changed
}
```

**Key Concept:** Fish maintains two representations:
- `self.desired` - what the screen should look like
- `self.actual` - what we think is currently on screen
- Only outputs the **differences** between them

#### B. Prompt Preservation
```rust
// From screen.rs
let prompt_last_line_should_wrap = 
    prompt_last_line_screen_wide && self.should_wrap(prompt_last_line);

if prompt_changed || prompt_last_line_should_wrap {
    self.r#move(0, 0);
    // Redraw prompt with Osc133PromptStart markers
}
```

**Key Points:**
- Prompt is only redrawn when `prompt_changed` is true
- During normal editing, prompt stays untouched
- Uses OSC 133 markers for semantic prompt boundaries

#### C. Soft Wrap Tracking
```rust
// From screen.rs
if Some(self.actual.cursor.x) == self.actual.screen_width {
    self.soft_wrap_location = Some(Cursor {
        x: 0,
        y: self.actual.cursor.y + 1,
    });
}
```

**Key Concept:** Fish tracks where "soft wraps" occur (natural line continuation at terminal edge) to optimize cursor movement

#### D. Cursor Calculation
```rust
// From screen.rs - desired_append_char()
fn desired_append_char(
    &mut self,
    b: char,
    c: HighlightSpec,
    indent: usize,
    prompt_width: usize,
    bwidth: usize,
) -> bool {
    // When character width + current x exceeds screen width:
    if screen_width.is_none_or(|sw| (self.desired.cursor.x + cw) > sw) {
        self.desired.line_mut(self.desired.cursor.y).is_soft_wrapped = true;
        line_no = self.desired.line_count();
        self.desired.add_line();
        self.desired.cursor.y += 1;
        self.desired.cursor.x = 0;
    }
}
```

**Critical Insight:** Cursor position calculated **incrementally** while building display

---

## 3. ZLE (Zsh Line Editor) Implementation

### Source Code Location
- Repository: https://github.com/zsh-users/zsh
- Key file: `Src/Zle/zle_refresh.c`

### How ZLE Works

#### A. Display Buffer Architecture
```c
// From zle_refresh.c - zrefresh()
// ZLE maintains video buffers for old and new state
static char **nbuf;  // New buffer
static char **obuf;  // Old buffer

void zrefresh(void) {
    // Build display buffer INCLUDING prompt space
    resetvideo();  // Pre-allocate space for prompt width
    
    // Main loop constructs display in nbuf
    for (each character in command) {
        // Add to nbuf, handle wrapping via nextline()
    }
    
    // Compare old vs new buffers
    refreshline(line_number);  // Only output differences
}
```

**Key Architecture:**
- Maintains video buffers representing screen state
- Prompt width (`lpromptw`) is reserved in buffer but prompt text kept separate
- Only outputs **differences** between old and new buffers

#### B. Prompt Handling
```c
// From zle_refresh.c
// Prompt is output separately, not regenerated:
if (!clearflag && lpromptbuf[0]) {
    zputs(lpromptbuf, shout);  // Write prompt
}

// Later, when refreshing lines:
if (vln == 0 && i < lpromptw) {
    // Skip regenerating content within prompt area
    continue;
}
```

**Key Point:** Prompt written once at initialization, skipped during refresh

#### C. Cursor Movement
```c
// From zle_refresh.c - moveto()
void moveto(int ln, int cl) {
    if (vcs == winw) {
        vln++, vcs = 0;  // Handle wrap
    }
    
    // Vertical movement
    if (ln < vln) {
        tc_upcurs(vln - ln);  // Move up
        vln = ln;
    }
    
    // Horizontal movement handled separately
}
```

**Key Concept:** 
- Uses termcap functions (`tc_upcurs`, `tc_downcurs`, etc.)
- Tracks virtual cursor position (`vcs`, `vln`)
- Calculates movement as difference between current and target position

#### D. Line Wrapping
```c
// From zle_refresh.c - nextline()
static void nextline() {
    if (rpms.ln != winh - 1)
        rpms.ln++;
    else {
        if (!rpms.canscroll) { /* scrolling logic */ }
        rpms.canscroll--;
        scrollwindow(0);
    }
}
```

**Key Point:** Handles scrolling when reaching bottom of terminal

---

## 4. Common Patterns Across All Three

### Pattern 1: Separate Prompt and Command
All three editors maintain a **clear separation** between:
- **Prompt** - Static, written once
- **Command buffer** - Dynamic, updated incrementally

**None of them redraw the prompt on every keystroke.**

### Pattern 2: Incremental Cursor Tracking
All three calculate cursor position by **walking through text character by character**:

```
x = prompt_width
y = 0

for each character:
    x += character_width
    if x >= terminal_width:
        x = 0
        y++
```

**They do NOT use division/modulo like:** `row = (prompt_width + position) / terminal_width`

**Why?** Division/modulo doesn't handle:
- Multi-byte UTF-8 sequences
- Wide characters (CJK = 2 columns)
- ANSI escape sequences (0 columns)
- Tab expansion

### Pattern 3: Differential Updates
All three maintain state tracking:
- Fish: `desired` vs `actual` screen state
- ZLE: `nbuf` (new) vs `obuf` (old) video buffers
- Replxx: Implicit via cursor position tracking

**Only output what changed**, minimizing terminal I/O.

### Pattern 4: Relative Cursor Movement
All three use **relative cursor movements** (`\033[A`, `\033[B`, `\033[C`, `\033[D`) or absolute column positioning (`\033[{n}G`).

**None of them use the pattern:** "move up N rows, then `\r`, then move right"

**Why?** That pattern clears the prompt area!

---

## 5. Our Current Implementation Problems

### Problem 1: We Move Up, Then `\r`, Then Clear
```c
// From display_controller.c - dc_handle_redraw_needed()
if (last_total_rows > 0) {
    // Move up to row 0
    char move_up[32];
    snprintf(move_up, sizeof(move_up), "\033[%dA", last_total_rows);
    write(STDOUT_FILENO, move_up, len);
}

// CRITICAL ERROR: This moves to column 0, which is BEFORE the prompt!
write(STDOUT_FILENO, "\r", 1);

// Move forward to after prompt
char move_seq[32];
snprintf(move_seq, sizeof(move_seq), "\033[%zuC", prompt_width);
write(STDOUT_FILENO, move_seq, len);

// CRITICAL ERROR: This clears EVERYTHING including the prompt!
write(STDOUT_FILENO, "\033[J", 3);
```

**What happens:**
1. Cursor is at (2, 15) after typing wrapped text
2. Move up 2 rows → cursor at (0, 15)
3. `\r` → cursor at (0, 0) ← **NOW AT START OF PROMPT LINE**
4. Move right `prompt_width` columns → cursor at (0, prompt_width)
5. `\033[J` clears from (0, prompt_width) to end of screen
6. **PROMPT IS DELETED** because it was at columns 0 to prompt_width-1

### Problem 2: We Redraw the Prompt Every Time
```c
if (!is_first_render) {
    // Move up and clear (destroys prompt)
} else {
    // Write the prompt
    write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
}
```

**This is wrong because:**
- First render: Write prompt ✓
- Second render: Try to clear and redraw, but prompt gets destroyed ✗

**Should be:**
- First render: Write prompt
- All subsequent renders: **Leave prompt alone**, only update command text

### Problem 3: We Don't Use Absolute Column Positioning
Instead of:
```c
write(STDOUT_FILENO, "\r", 1);  // Go to column 0
// Move right prompt_width columns
```

Should use:
```c
// Move to absolute column (1-based indexing)
char seq[32];
snprintf(seq, sizeof(seq), "\033[%zuG", prompt_width + 1);
write(STDOUT_FILENO, seq, strlen(seq));
```

This avoids going through column 0 and risking prompt destruction.

---

## 6. The Correct Solution

Based on how Replxx, Fish, and ZLE all work, here's what we need to do:

### Solution A: Replxx-Style Approach (Recommended)

**Architecture:**
1. Prompt is written ONCE at the start of editing
2. Create a display buffer containing ONLY command text (no prompt)
3. On each refresh:
   - Move cursor to prompt indentation position using **absolute positioning**
   - Clear from current position to end of screen
   - Write command text
   - Position cursor at calculated position

**Code pattern:**
```c
// First render only
if (first_time) {
    write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
    first_time = false;
}

// Every render (including first)
// Move to absolute column where command starts
char move_to_col[32];
snprintf(move_to_col, sizeof(move_to_col), "\033[%zuG", prompt_width + 1);
write(STDOUT_FILENO, move_to_col, strlen(move_to_col));

// Move up to row 0 (where command starts)
if (last_cursor_row > 0) {
    char move_up[32];
    snprintf(move_up, sizeof(move_up), "\033[%dA", last_cursor_row);
    write(STDOUT_FILENO, move_up, strlen(move_up));
}

// Clear from here to end of screen
write(STDOUT_FILENO, "\033[J", 3);

// Write command text
write(STDOUT_FILENO, command_buffer, command_len);

// Position cursor (relative movements from current position)
// ... calculate and move ...
```

### Solution B: Fish-Style Approach (Most Sophisticated)

**Architecture:**
1. Maintain `desired` and `actual` screen representations
2. Calculate differences between them
3. Only output the minimal changes needed
4. Track soft wrap locations for optimization

**This is more complex but most efficient.**

### Solution C: ZLE-Style Approach (Middle Ground)

**Architecture:**
1. Maintain video buffers for current and previous screen state
2. Compare buffers line by line
3. Output only changed portions using `refreshline()`
4. Use termcap for terminal control

---

## 7. Specific Recommendations for Our Codebase

### Immediate Fix (Minimal Changes)

**File:** `src/display/display_controller.c`

**Change 1:** Never move to column 0 after moving up
```c
// BEFORE (WRONG):
write(STDOUT_FILENO, "\r", 1);
char move_seq[32];
snprintf(move_seq, sizeof(move_seq), "\033[%zuC", prompt_width);
write(STDOUT_FILENO, move_seq, len);

// AFTER (CORRECT):
// Use absolute column positioning
char move_to_col[32];
snprintf(move_to_col, sizeof(move_to_col), "\033[%dG", (int)(prompt_width + 1));
write(STDOUT_FILENO, move_to_col, strlen(move_to_col));
```

**Change 2:** Only write prompt on first render
```c
static bool prompt_written = false;

if (!prompt_written) {
    // First render: write prompt
    write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
    prompt_written = true;
}

// Don't use is_first_render for clearing logic
// Always clear from command start position
```

**Change 3:** Save cursor row correctly
```c
// After positioning cursor at target position
last_cursor_row = cursor_row;  // Save for next render
```

### Long-Term Solution (Architecture Improvement)

1. **Separate Prompt from Command Buffer**
   - Prompt rendered once by prompt_layer
   - Command text managed independently
   - Clear separation in data structures

2. **Implement Incremental Cursor Tracking**
   - Already done in `display_bridge.c::calculate_cursor_screen_position()`
   - This is working correctly ✓

3. **Add Display State Tracking**
   - Track what's currently on screen
   - Compare with desired state
   - Output minimal changes

4. **Use Absolute Positioning**
   - Replace `\r` + `\033[{n}C` with `\033[{n}G`
   - Safer and more reliable

---

## 8. Key Takeaways

### What We Learned

1. **Prompt and command are separate** - Never mix them in the same buffer
2. **Incremental cursor calculation** - Walk through text, don't use division
3. **Differential updates** - Only output what changed
4. **Absolute positioning** - Use `\033[{n}G` instead of `\r` + forward movement
5. **State tracking** - Know what's on screen vs what should be

### What Not To Do

1. ❌ Don't redraw prompt on every keystroke
2. ❌ Don't use `\r` after moving up (destroys prompt)
3. ❌ Don't calculate cursor position with division/modulo
4. ❌ Don't clear from column 0 (clears prompt)
5. ❌ Don't assume cursor row is same after wrapping

### The Root Cause

**Our prompt disappears because we're using the sequence:**
```
Move up → \r → Move right → Clear to end
```

**The `\r` moves us to column 0, which is BEFORE the prompt. Then `\033[J` clears everything INCLUDING the prompt.**

**Solution:** Use absolute column positioning (`\033[{n}G`) to go directly to the command start position without passing through column 0.

---

## 9. References

### Source Code Links

**Replxx:**
- Main: https://github.com/AmokHuginnsson/replxx/blob/master/src/replxx_impl.cxx
- Cursor calc: https://github.com/AmokHuginnsson/replxx/blob/master/src/util.cxx
- Terminal: https://github.com/AmokHuginnsson/replxx/blob/master/src/terminal.cxx

**Fish:**
- Screen: https://github.com/fish-shell/fish-shell/blob/master/src/screen.rs
- Reader: https://github.com/fish-shell/fish-shell/blob/master/src/reader/mod.rs

**ZLE:**
- Refresh: https://github.com/zsh-users/zsh/blob/master/Src/Zle/zle_refresh.c

### ANSI Escape Sequences

- `\033[{n}A` - Cursor up n rows
- `\033[{n}B` - Cursor down n rows  
- `\033[{n}C` - Cursor forward n columns
- `\033[{n}D` - Cursor back n columns
- `\033[{n}G` - Cursor to column n (1-based, absolute)
- `\033[J` - Clear from cursor to end of screen
- `\033[K` - Clear from cursor to end of line
- `\r` - Carriage return (column 0)

**Critical:** `\033[{n}G` is ABSOLUTE positioning (goes to column n), while `\033[{n}C` is RELATIVE (moves forward n columns from current position).

---

## 10. Implementation Checklist

- [ ] Replace `\r` + `\033[{n}C` with `\033[{n}G` for absolute positioning
- [ ] Only write prompt on first render, never redraw it
- [ ] Track last cursor row separately from command state
- [ ] Verify cursor position calculation in `calculate_cursor_screen_position()` (already correct)
- [ ] Test with wrapped lines exceeding terminal width
- [ ] Test with multi-byte UTF-8 characters
- [ ] Test with wide characters (CJK, emoji)
- [ ] Test at bottom of terminal (scrolling behavior)
- [ ] Test with very long commands (multiple wrap lines)
- [ ] Verify no prompt flickering or disappearance

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-04  
**Status:** Complete - Ready for Implementation
