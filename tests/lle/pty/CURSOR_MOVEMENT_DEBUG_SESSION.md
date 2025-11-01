# LLE Cursor Movement Debugging Session

## Session Goal
Fix the cursor movement bug identified in basic PTY tests where arrow keys caused display corruption.

## Critical Bugs Fixed

### 1. Display Refresh Without Terminal Control Sequences

**Problem**: The original `refresh_display()` function simply did `printf("%s", display_output)` which **appended** output instead of redrawing the line. This caused:
- Multiple prompts appearing on screen
- Display corruption with fragments like "abberry@" 
- No cursor positioning

**Root Cause**: Missing terminal control sequences for line management.

**Fix Applied** (`src/lle/lle_readline.c:157-211`):
```c
/* Clear current line and redraw from beginning */
printf("\r");              /* Carriage return - go to start of line */
printf("\033[K");          /* Clear from cursor to end of line */
printf("%s", display_output);  /* Output prompt + command */

/* Calculate visible character count in prompt (excluding ANSI codes) */
size_t prompt_visible_len = 0;
const char *p = ctx->prompt;
while (*p) {
    if (*p == '\033') {
        /* Skip ANSI escape sequence */
        while (*p && *p != 'm') p++;
        if (*p == 'm') p++;
    } else if (*p == '\x01' || *p == '\x02') {
        /* Skip readline invisible character markers */
        p++;
    } else {
        prompt_visible_len++;
        p++;
    }
}

/* Position cursor at prompt end + buffer cursor position */
size_t target_column = prompt_visible_len;
if (ctx->buffer) {
    target_column += ctx->buffer->cursor.byte_offset;
}

/* Move to target column (1-indexed) */
printf("\033[%zuG", target_column + 1);
fflush(stdout);
```

**Impact**: 
- ✅ Display now properly clears and redraws on each update
- ✅ Cursor is positioned correctly
- ✅ Backspace test now passes (was failing before)
- ✅ No more prompt duplication

### 2. Buffer Size Corruption (from previous session)

**Already fixed**: Composition engine cache was shrinking the 65KB buffer to match cached output size, causing "Buffer too small" errors.

## Diagnostic Test Results

Created `test_pty_cursor_diagnostic.c` with 6 minimal tests to isolate the cursor movement bug.

### Test Results

```
✓ PASS: ANSI analysis - Shows exact escape sequences
✗ FAIL: Just arrow: Type 'test', left arrow (no insert)
✗ FAIL: Single char: Type 'a', left arrow, type 'X' → 'Xa'
✗ FAIL: Minimal: Type 'ab', left arrow, type 'X' → 'aXb'
✗ FAIL: Three chars: Type 'abc', left once, type 'X' → 'abXc'
✗ FAIL: Control: Type 'ab', left, left, right → cursor moves
```

### Basic Test Suite Results

```
✓ PASS: Simple Prompt Display (102ms)
✓ PASS: Command Text Echo (254ms)
✓ PASS: Backspace Deletion (359ms)  ← NOW PASSING!
✗ FAIL: Cursor Movement (Arrow Keys)
✓ PASS: Multiline Command Display (252ms)

Total: 5 tests
Passed: 4 tests (80% pass rate)
Failed: 1 test
```

## Remaining Issue: Arrow Key Event Processing

### Symptom

After pressing left arrow once, the diagnostic output shows **THREE separate redraws**:

```
Sequence after left arrow:
\r\e[K...prompt...ab...\e[81G\r\e[K...prompt...ab...\e[82G\r\e[K...prompt...ab...\e[83G
```

The cursor position **increments** (81→82→83) instead of decrementing.

### Analysis

1. **Arrow key handler is correct**: `handle_arrow_left()` at line 472 correctly does:
   ```c
   ctx->buffer->cursor.byte_offset--;
   refresh_display(ctx);
   ```

2. **Single left arrow (`\033[D`) triggers multiple events**: Either:
   - The terminal abstraction is generating multiple events for one arrow key
   - The escape sequence is being parsed as multiple characters
   - There's event duplication in the event system

3. **Cursor increments suggest wrong interpretation**: The cursor going 81→82→83 suggests:
   - The left arrow is being interpreted as something that moves forward
   - OR each of the 3 bytes in `\033[D` is generating a separate cursor increment
   - OR the event is being queued/replayed multiple times

### Hypothesis

The most likely cause is that **the escape sequence `\033[D` is being parsed character-by-character** instead of being recognized as a single LEFT arrow key event. Each character (`\033`, `[`, `D`) might be:
- Generating a separate KEY_PRESS event
- Each event somehow incrementing cursor position
- Causing a refresh on each

This would explain why we see 3 refreshes (3 characters in escape sequence).

### Next Steps for User Manual Testing

**Test in Real Terminal**:
1. Open lusush in a real terminal (xterm, kitty, etc.) with `LLE_ENABLED=1`
2. Type: `ab`
3. Press left arrow once
4. Observe if:
   - Display corrupts (multiple prompts visible)
   - Cursor moves left correctly
   - Typing a character inserts at correct position

**Expected Behaviors**:
- ✅ Line should clear and redraw (our fix working)
- ✅ Cursor should visibly move left
- ❌ Might see flickering or multiple redraws (PTY-specific issue?)
- ❓ Typing should work correctly if terminal handles events better than PTY

## Files Modified

1. `src/lle/lle_readline.c` (lines 157-211)
   - Added `\r\e[K` to clear line before redraw
   - Added cursor positioning with `\e[G` escape code
   - Added ANSI code skipping for accurate column calculation

2. `tests/lle/pty/test_pty_cursor_diagnostic.c` (new file, 500+ lines)
   - 6 minimal diagnostic tests
   - Raw ANSI sequence capture and display
   - Detailed failure messages

3. `tests/lle/pty/Makefile`
   - Added `test_pty_cursor_diagnostic` target

## Key Insights

1. **\r\e[K is essential**: Without carriage return + clear, output just appends
2. **Cursor positioning must skip ANSI codes**: The `\e[G` escape needs the *visible* column, not byte offset
3. **Readline markers (\x01/\x02) must be skipped**: These mark invisible characters
4. **PTY testing reveals timing issues**: Multiple rapid refreshes might work fine in real terminal but show up as separate frames in PTY capture
5. **Event parsing is critical**: The terminal abstraction layer needs thorough debugging for escape sequence handling

## Success Metrics

### Achieved ✅
- Fixed display refresh to use proper terminal control
- Backspace now works correctly 
- 80% test pass rate (4/5 basic tests)
- Comprehensive diagnostic test infrastructure

### Remaining ❌
- Arrow key event duplication/misinterpretation
- Cursor movement test still failing
- Need to debug escape sequence parsing in terminal abstraction layer

## Recommendation for Next Session

**Priority**: Debug the event generation/parsing for escape sequences

**Approach**:
1. Add logging to `lle_unix_interface` to show raw bytes received
2. Add logging to escape sequence parser to show how `\033[D` is interpreted
3. Check if events are being queued multiple times
4. Verify `handle_arrow_left()` is actually being called (vs other handlers)

**Alternative**: If PTY-specific issue, validate manually in real terminals first before debugging further. The fix might already work correctly in real usage.
