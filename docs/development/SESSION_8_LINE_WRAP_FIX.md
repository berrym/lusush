# Session 8: Line Wrap Cursor Positioning Fix

**Date**: 2025-11-10  
**Branch**: feature/lle  
**Status**: ✅ COMPLETE - Working implementation with correct architecture  
**Commits**: 4322e58, 34ba284, f4a4261, d045f6a

---

## Executive Summary

**Problem**: Cursor positioning was broken on wrapped lines - cursor would jump to wrong position and consume lines above the prompt when text wrapped at terminal edge.

**Root Cause**: Commits after 31dd662 broke working line wrap implementation by adding continuation prompt logic that incorrectly triggered on wrapped lines (`num_rows > 1`) instead of only on actual multi-line input (lines with `\n` characters).

**Solution**: 
1. Restored all display subsystem files from last working commit (31dd662)
2. Implemented proper prompt-once architecture per MODERN_EDITOR_WRAPPING_RESEARCH.md
3. Verified basic line wrapping works correctly with research-compliant architecture

**Impact**: Establishes correct architectural foundation for future continuation prompt implementation with proper wrapped vs newline detection.

---

## Problem Description

### User Report

When typing long lines that wrap at terminal edge:
- Cursor would jump to wrong position after wrapping
- Lines above the prompt would be "consumed" (cleared)
- Text would appear on wrong terminal lines
- Display would corrupt the shell prompt above LLE's prompt

### Example Behavior

```
Before wrap (terminal width 110, prompt width 62):
[mberry@fedora-xps13.local] ~/Lab/c/lusush $ echo "this is a very long line of tex that shoul

After typing 'd' (wraps to next line):
❯ LLE_ENABLED=1 ./builddir/lusush                             echo "this is a very long line of tex that should
```

The command text appeared on the line with the shell prompt, not on the LLE prompt line.

---

## Investigation Process

### Initial Attempts (Failed)

Multiple attempts to fix the issue failed because they assumed the problem was in cursor positioning logic:

1. **Attempt**: Check for newlines before adding continuation prompt width
   - **Result**: Failed - still consumed lines
   
2. **Attempt**: Remove `\r` usage per research document
   - **Result**: Failed - still consumed lines
   
3. **Attempt**: Implement "prompt-once" architecture
   - **Result**: Failed - still consumed lines
   
4. **Attempt**: Fix order of cursor movements (column first, then up)
   - **Result**: Failed - still consumed lines

**Key Mistake**: Trying to fix code by reasoning about what should work, instead of comparing exact diffs between working and broken versions.

### Critical User Feedback

> "you are running in circles and beating a dead horse over and over again. what do we know? we know the exact research and design that is supposed to work, it only worked when the screen buffer with differential updates was implemented, we know what that commit was, the research is always correct, the code diverged somewhere you are not looking at because you are trying to be clever and not looking at the entire diffs between the working line wrapped commit and the broken ones"

This redirected the investigation to the correct approach: **look at actual diffs**.

### Git Bisect Process

Used git bisect to find the exact commit where line wrapping broke:

```bash
# Start from known good commit
git checkout 3b83534  # User confirmed: "cursor stayed in correct location during line wrap"

# Bisect between good and current
git bisect start
git bisect good 3b83534
git bisect bad 5aef518

# Test commits:
- 9ea4c3e: GOOD
- 31dd662: GOOD ✅ (last working commit)
- 0b07c79: BAD
- 4a65192: BAD  
- eecb87c: BAD ✅ (first broken commit)
```

**Result**: 
- **Last working**: 31dd662 "LLE: Phase 4 - Composition Engine Integration"
- **First broken**: eecb87c "LLE: Implement continuation prompts as real display layer"

### Root Cause Analysis

Examined diff between 31dd662 (working) and eecb87c (broken):

```bash
git diff 31dd662 eecb87c -- src/display/display_controller.c
```

**Breaking changes in eecb87c**:

1. **Added continuation prompt detection based on `num_rows > 1`**:
   ```c
   if (desired_screen.num_rows == 1 || !controller->continuation_layer) {
       /* Simple rendering */
   } else {
       /* Multi-line with continuation prompts */
   }
   ```
   
   **Problem**: `num_rows > 1` is TRUE for both:
   - Actual multi-line input (with `\n` characters)
   - Wrapped lines (terminal edge wrap, NO `\n`)

2. **Added cursor column adjustment for continuation prompts**:
   ```c
   if (cursor_row > 0 && controller->continuation_layer) {
       cursor_col += strlen(prefix);  // Added prompt width
   }
   ```
   
   **Problem**: This triggered on wrapped lines too!

**The Fix**: Restore simple rendering from 31dd662 which didn't distinguish between line types.

---

## Solution: Two-Phase Approach

### Phase 1: Restore Working Code

**Commit**: `4322e58` - "LLE: Fix line wrap cursor positioning by restoring working display code"

**Action**: Restored ALL display subsystem files from commit 31dd662:
- `src/display/composition_engine.c`
- `src/display/continuation_prompt_layer.c`
- `src/display/display_controller.c`
- `include/display/composition_engine.h`
- `include/display/continuation_prompt_layer.h`
- `include/display/display_controller.h`
- `include/display/layer_events.h`

**Key Lesson**: Don't try to restore just one file - restore the entire working state of the subsystem.

**Result**: ✅ Basic line wrapping works correctly

**Working code from 31dd662**:
```c
/* Move up if needed */
if (prompt_rendered && current_screen.cursor_row > 0) {
    char up_seq[16];
    snprintf(up_seq, sizeof(up_seq), "\033[%dA", current_screen.cursor_row);
    write(STDOUT_FILENO, up_seq, up_len);
}

/* Move to beginning of line */
write(STDOUT_FILENO, "\r", 1);

/* Clear from cursor to end */
write(STDOUT_FILENO, "\033[J", 3);

/* Draw prompt */
write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));

/* Draw command */
write(STDOUT_FILENO, command_buffer, strlen(command_buffer));
```

### Phase 2: Implement Correct Architecture

**Commit**: `34ba284` - "LLE: Implement prompt-once architecture per research document"

**Motivation**: The working code from 31dd662 violated research document principles:
- ❌ Redrew prompt on every keystroke
- ❌ Used `\r` (moves to column 0)
- ❌ Cleared from column 0 (could clear prompt)

**User Decision**: Implement correct architecture NOW while we have working code to test against.

**Implementation**: Followed EXACT pattern from MODERN_EDITOR_WRAPPING_RESEARCH.md:

```c
size_t prompt_width = screen_buffer_calculate_visual_width(prompt_buffer, 0);

/* First render only: Draw prompt once */
if (!prompt_rendered) {
    if (prompt_buffer[0]) {
        write(STDOUT_FILENO, prompt_buffer, strlen(prompt_buffer));
    }
    prompt_rendered = true;
}

/* Every render: Position to command start and redraw command */

/* Step 1: Move to absolute column where command starts */
char move_to_col[32];
snprintf(move_to_col, sizeof(move_to_col), "\033[%zuG", prompt_width + 1);
write(STDOUT_FILENO, move_to_col, col_len);

/* Step 2: Move up to row 0 if needed */
if (current_screen.cursor_row > 0) {
    char move_up[32];
    snprintf(move_up, sizeof(move_up), "\033[%dA", current_screen.cursor_row);
    write(STDOUT_FILENO, move_up, up_len);
}

/* Step 3: Clear from current position to end */
write(STDOUT_FILENO, "\033[J", 3);

/* Step 4: Write command text */
write(STDOUT_FILENO, command_buffer, strlen(command_buffer));

/* Step 5: Position cursor using absolute column positioning */
int rows_to_move_up = final_row - cursor_row;
if (rows_to_move_up > 0) {
    char up_seq[16];
    snprintf(up_seq, sizeof(up_seq), "\033[%dA", rows_to_move_up);
    write(STDOUT_FILENO, up_seq, up_len);
}

char col_seq[16];
snprintf(col_seq, sizeof(col_seq), "\033[%dG", cursor_col + 1);
write(STDOUT_FILENO, col_seq, col_seq_len);
```

**Key Changes**:
1. ✅ Prompt drawn ONCE only (not every keystroke)
2. ✅ Using `\033[{n}G` for absolute positioning (not `\r`)
3. ✅ Column positioning BEFORE moving up (per research doc)
4. ✅ Never moving to column 0 after first render
5. ✅ Clearing only command area (not from column 0)

**Result**: ✅ Basic line wrapping still works with correct architecture

---

## Architecture Compliance

### Research Document Requirements

From `MODERN_EDITOR_WRAPPING_RESEARCH.md`:

**What ALL Three Editors Do (Replxx, Fish, ZLE)**:
1. Prompt and command buffer are SEPARATE entities
2. Prompt is written ONCE and NEVER moved/redrawn during editing
3. Only the command text portion is updated incrementally
4. Cursor position is calculated incrementally, NOT via division/modulo

**What NOT To Do**:
1. ❌ Don't redraw prompt on every keystroke
2. ❌ Don't use `\r` after moving up (destroys prompt)
3. ❌ Don't calculate cursor position with division/modulo
4. ❌ Don't clear from column 0 (clears prompt)
5. ❌ Don't assume cursor row is same after wrapping

### Current Implementation Status

| Requirement | Phase 1 (31dd662) | Phase 2 (34ba284) |
|-------------|-------------------|-------------------|
| Prompt drawn once | ❌ Redrawn every time | ✅ Once only |
| Use `\033[{n}G` | ❌ Used `\r` | ✅ Absolute positioning |
| Avoid column 0 | ❌ Used `\r` | ✅ Never goes to column 0 |
| Clear only command | ❌ Cleared from column 0 | ✅ Clears from prompt_width + 1 |
| Incremental cursor | ✅ screen_buffer | ✅ screen_buffer |

**Phase 1**: Working but architecturally incorrect  
**Phase 2**: Working AND architecturally correct ✅

---

## Technical Details

### Screen Buffer Role

`screen_buffer_render()` is called with BOTH prompt and command:
```c
screen_buffer_render(&desired_screen, prompt_buffer, command_buffer, cursor_byte_offset);
```

**Why?** It needs prompt to:
1. Calculate where command starts (after prompt)
2. Determine cursor screen coordinates correctly
3. Handle line wrapping calculations

**Key Point**: screen_buffer needs prompt for CALCULATIONS, but we don't have to WRITE it to terminal every time.

### ANSI Escape Sequences Used

| Sequence | Purpose | Example |
|----------|---------|---------|
| `\033[{n}A` | Move up n rows | `\033[1A` = up 1 row |
| `\033[{n}G` | Move to column n (1-based) | `\033[63G` = column 63 |
| `\033[J` | Clear from cursor to end of screen | Clears below cursor |
| `\r` | Carriage return (column 0) | **NOT USED** in phase 2 |

### Cursor Position Calculation

From `screen_buffer.c`:
- Walks through text character-by-character
- Accounts for UTF-8 multi-byte sequences
- Handles wide characters (2 columns)
- Skips ANSI escape codes (0 columns)
- Tracks wrapping at terminal width

**NOT using**: Division or modulo arithmetic (error-prone for UTF-8)

---

## Testing Verification

### Test Case: Basic Line Wrapping

**Setup**:
- Terminal width: 110 columns
- Prompt width: 62 columns (calculated)
- Available for command: 48 columns on row 0

**Test Input**:
```bash
echo "this is a very long line of text that should wrap and is still working correctly!"
```

**Expected Behavior**:
- First 48 characters on row 0 (after prompt)
- Remaining characters wrap to row 1, column 0
- Cursor follows text correctly
- No line consumption above prompt

**Result with Phase 1**: ✅ Works
**Result with Phase 2**: ✅ Works

### Test Case: Cursor Positioning After Wrap

**Action**: Type long line, cursor wraps to row 1

**Expected**:
- Typing more characters continues on row 1
- Backspace moves cursor back correctly
- Cursor visible at correct position

**Result**: ✅ Works correctly

---

## Lessons Learned

### 1. Trust the Research, Not Assumptions

**Wrong Approach**: Try to reason about what should work and implement it  
**Right Approach**: Follow research document exactly, even if you don't understand why

### 2. Compare Actual Diffs, Don't Theorize

**Wrong Approach**: "The bug is probably in cursor positioning logic"  
**Right Approach**: "What exactly changed between working and broken commits?"

### 3. Restore Complete Subsystems, Not Individual Files

**Wrong Approach**: "Just restore display_controller.c"  
**Right Approach**: "Restore all display subsystem files together"

### 4. Fix Architecture While You Have Working Code

**Wrong Approach**: "It works, ship it, fix architecture later"  
**Right Approach**: "Fix architecture NOW while we can verify it still works"

### 5. Git Bisect is Invaluable

When you know a commit worked but current code doesn't:
```bash
git bisect start
git bisect good <working-commit>
git bisect bad HEAD
# Test each commit git gives you
```

This pinpoints EXACTLY where the regression occurred.

---

## Future Work

### Continuation Prompts Re-implementation

**Challenge**: Need to detect actual multi-line input (with `\n`) vs wrapped lines

**Approach**:
1. Check for actual newline characters in command text
2. Only apply continuation prompt logic for lines with `\n`
3. Use screen_buffer's line tracking to identify continuation lines
4. Don't trigger on `num_rows > 1` alone (that includes wrapped lines!)

**Key Insight**: The prompt-once architecture is now in place, providing the correct foundation.

### Proper Line Type Detection

Need to distinguish:
- **Wrapped lines**: Text wraps at terminal edge, no `\n` in buffer
- **Continuation lines**: Explicit `\n` in buffer (for loop, if, etc.)

**Possible approach**:
```c
bool has_newlines = (strchr(command_text, '\n') != NULL);
if (has_newlines && num_rows > 1) {
    // This is actual multi-line input with continuation prompts
} else {
    // This is single-line input, possibly wrapped
}
```

---

## References

### Documentation
- `docs/development/MODERN_EDITOR_WRAPPING_RESEARCH.md` - Research on how Replxx, Fish, ZLE work
- `docs/development/ARCHITECTURE_IMPACT_ANALYSIS.md` - LLE architectural boundaries
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Session 8 summary

### Key Commits
- `3b83534` - Last known good basic input (Nov 4)
- `31dd662` - Last working line wrap with arrows (Nov 8) ✅
- `eecb87c` - First broken commit (Nov 9) ❌
- `4322e58` - Restore working code (Nov 10)
- `34ba284` - Implement prompt-once architecture (Nov 10)

### Code Files Modified
- `src/display/display_controller.c` - Main rendering logic
- `src/display/composition_engine.c` - Layer composition
- `src/display/continuation_prompt_layer.c` - Continuation prompt support
- `include/display/*.h` - Header files

---

## Appendix: Debug Process

### Debug Logging Added

Added file-based logging to avoid terminal interference:
```c
FILE *log = fopen("/tmp/lle_debug.log", "a");
fprintf(log, "RENDER: prompt_rendered=%d, current=(%d,%d), desired=(%d,%d)\n",
        prompt_rendered, current_screen.cursor_row, current_screen.cursor_col,
        desired_screen.cursor_row, desired_screen.cursor_col);
fclose(log);
```

This revealed:
- When cursor position was being saved incorrectly
- Where cursor movements were happening
- That `num_rows` was increasing on wrap (expected)

### User Patience is Critical

The user's redirect to "stop theorizing and look at diffs" was the turning point. When debugging complex issues:

1. ✅ Look at actual changes between working and broken
2. ✅ Use git bisect to find exact regression
3. ✅ Restore known working code first
4. ❌ Don't try to fix by reasoning about what should work
5. ❌ Don't assume you understand the architecture better than research

---

**Document Status**: Complete reference for Session 8 investigation and solution  
**Last Updated**: 2025-11-10  
**Verified**: Line wrapping works correctly with prompt-once architecture
