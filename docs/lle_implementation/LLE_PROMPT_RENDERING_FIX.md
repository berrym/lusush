# LLE Prompt Rendering Fix

**Date**: 2025-10-31  
**Status**: Code complete, requires interactive testing  
**Branch**: feature/lle

---

## Problem Identified

User reported that with LLE enabled, prompt and input were not visible, but commands were being executed:

```
[DISPLAY] Rendering 1 lines, cursor at (0, 0)
[DISPLAY] Line 0: len=0, content=''
```

The display_generator was generating **empty content** because it wasn't including the prompt text.

---

## Root Cause

The `display_generator` in `src/lle/terminal_display_generator.c` was only processing the command buffer, not the prompt. It had:

```c
size_t current_column = state->prompt_width;  // Tracked width
// But never actually added the prompt text!
```

---

## Solution Implemented

### 1. Added Prompt to Generation Parameters

**File**: `include/lle/terminal_abstraction.h`

```c
typedef struct lle_generation_params {
    bool force_full_refresh;
    bool optimize_for_speed;
    size_t max_display_lines;
    const char *prompt;              /* Prompt text to display */
} lle_generation_params_t;
```

### 2. Updated Display Generator to Use Prompt

**File**: `src/lle/terminal_display_generator.c` (lines 279-290)

```c
/* Initialize first line and add prompt */
new_content->lines[0].length = 0;

/* Add prompt to first line */
if (generator->params.prompt && generator->params.prompt[0] != '\0') {
    size_t prompt_len = strlen(generator->params.prompt);
    result = append_to_line(&new_content->lines[0], generator->params.prompt, prompt_len);
    if (result != LLE_SUCCESS) {
        lle_display_content_destroy(new_content);
        return result;
    }
    current_column = prompt_len;
}
```

### 3. Set Prompt Before Content Generation

**File**: `src/lle/lle_readline.c` (line 127)

```c
/* Set prompt in generation params before generating content */
ctx->term->display_generator->params.prompt = ctx->prompt;
```

---

## Expected Behavior After Fix

When LLE is enabled, the debug output should now show:

```
[DISPLAY] Rendering 1 lines, cursor at (0, 45)
[DISPLAY] Line 0: len=45, content='[user@host] ~/path $ '
```

And the actual terminal should display:
- ✅ Prompt text
- ✅ User input characters as they type
- ✅ Cursor positioned correctly

---

## Testing Required

### Interactive Terminal Test

**CRITICAL**: LLE requires a real TTY. Piped input will fail with:
```
[LLE] FAILED: enter raw mode failed, result=1200
```

**Test Procedure**:

```bash
# 1. Start lusush interactively
./builddir/lusush -i

# 2. Enable LLE
config set editor.use_lle true

# 3. Verify display
# You should now see:
# - Prompt appears
# - Characters echo as you type
# - Cursor moves correctly
# - Backspace works
# - Enter executes commands

# 4. Test basic editing
echo hello world
# Use arrow keys to move cursor
# Use backspace to delete
# Verify everything is visible

# 5. Exit
exit
```

---

## Files Modified

1. `include/lle/terminal_abstraction.h` - Added prompt to generation_params
2. `src/lle/terminal_display_generator.c` - Render prompt to first line
3. `src/lle/lle_readline.c` - Set prompt before content generation

---

## Debug Output Cleanup

Before final testing, remove debug output:

```bash
# Remove debug fprintf statements
sed -i '/fprintf(stderr, "\[LLE\]/d' src/lle/*.c
sed -i '/fprintf(stderr, "\[DISPLAY\]/d' src/lle/*.c
sed -i '/fprintf(stderr, "\[GNU\]/d' src/*.c

# Rebuild
ninja -C builddir
```

---

## Architecture Validation

This fix maintains architectural compliance:

- ✅ **Separation of concerns**: Editing logic separate from rendering
- ✅ **Single source of truth**: Buffer + prompt = authoritative state
- ✅ **Display generation**: Converts state to display content
- ✅ **Terminal abstraction**: Rendering through display client

---

## Next Steps

1. **Test interactively** - Verify prompt and input are visible
2. **Remove debug output** - Clean up all fprintf statements
3. **Test all features**:
   - Character insertion
   - Backspace/delete
   - Cursor movement (arrows, Home, End)
   - Line editing (Ctrl-A, Ctrl-E, Ctrl-K, etc.)
   - Command execution (Enter)
   - Multiline input
4. **Commit changes** with message: "LLE: Fix display_generator to render prompt text"

---

## Summary

The display rendering architecture was correct - the issue was simply that the prompt text wasn't being included in the generated display content. With this fix:

- Display generator now receives prompt via generation params
- Prompt is prepended to first display line
- Cursor positioning accounts for prompt length
- Complete line (prompt + buffer) is rendered to terminal

The fix is minimal, targeted, and maintains all architectural principles.
