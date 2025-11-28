# Fish-Style Autosuggestions - Design & Implementation Document

**Version**: 2.0  
**Date**: 2025-11-28  
**Author**: AI Assistant  
**Status**: Implemented & Tested

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Implementation Summary](#2-implementation-summary)
3. [Architecture](#3-architecture)
4. [Key Components](#4-key-components)
5. [Data Flow](#5-data-flow)
6. [Configuration & Behavior](#6-configuration--behavior)
7. [Testing](#7-testing)
8. [Future Enhancements](#8-future-enhancements)
9. [File Change Summary](#9-file-change-summary)

---

## 1. Executive Summary

### 1.1 Objective

Fish shell-style inline autosuggestions for Lusush. As users type commands, predicted completions appear as dimmed "ghost text" after the cursor. Users accept suggestions with Right Arrow, End, Ctrl-F, or Ctrl-E.

### 1.2 Key Features Implemented

- **Ghost Text Display**: Suggestions shown as dimmed (BRIGHT_BLACK) text after cursor
- **LLE History-Based Prediction**: Uses LLE history system (not GNU readline history)
- **Instant Acceptance**: Right Arrow, End, Ctrl-F, Ctrl-E accept full suggestion
- **Non-Intrusive**: Suggestions don't interfere with normal typing or editing
- **UTF-8/Grapheme Support**: Proper display width calculation for Unicode
- **Line Wrapping Support**: Correct cursor positioning with wrapped suggestions

### 1.3 Design Philosophy

1. **Use LLE History**: Direct integration with LLE history system for suggestions
2. **Context-Aware Actions**: Keybindings that adapt behavior based on state
3. **Centralized Rendering**: All display through display_controller - no direct terminal writes
4. **Clean Separation**: Suggestion generation in lle_readline, rendering in display_controller

---

## 2. Implementation Summary

### 2.1 What Was Built

The implementation bypasses the legacy `autosuggestions.c` (which uses GNU readline history) and implements a direct LLE history search in `lle_readline.c`. Suggestions are passed to `display_controller` for rendering.

### 2.2 Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Store suggestion in `readline_context_t` | Allows context-aware actions to access it directly |
| Use `lle_utf8_string_width()` for display | Proper handling of wide chars (CJK, emoji) |
| Skip multiline history entries | Prevents suggesting complex constructs that would corrupt display |
| Disable suggestions in multiline input | Simplifies initial implementation, prevents display complexity |

---

## 3. Architecture

### 3.1 High-Level Flow

```
User types character
        |
        v
lle_readline.c: handle_character_input()
        |
        v
lle_readline.c: refresh_display()
        |
        +---> update_autosuggestion()
        |         |
        |         v
        |     Search LLE history for prefix match
        |     Store result in ctx->current_suggestion
        |
        +---> display_controller_set_autosuggestion()
        |         |
        |         v
        |     autosuggestions_layer_set_suggestion()
        |
        v
display_controller.c: dc_handle_redraw_needed()
        |
        +---> Render command text
        +---> Render ghost text (BRIGHT_BLACK)
        +---> Calculate ghost_text_extra_rows for wrapping
        +---> Position cursor correctly
        |
        v
Terminal Output
```

### 3.2 Suggestion Acceptance Flow

```
User presses Right Arrow (or End/Ctrl-F/Ctrl-E)
        |
        v
lle_readline.c: execute_keybinding_action("RIGHT", ...)
        |
        v
lle_forward_char_or_accept_suggestion(ctx)  [context-aware action]
        |
        +---> Check: cursor at end AND has_autosuggestion(ctx)?
        |         |
        |         YES: accept_autosuggestion(ctx)
        |         |    - Insert ctx->current_suggestion into buffer
        |         |    - Clear suggestion
        |         |    - refresh_display()
        |         |
        |         NO: Normal cursor movement
        |
        v
Done
```

---

## 4. Key Components

### 4.1 lle_readline.c - Suggestion Generation

```c
// Fields added to readline_context_t
char *current_suggestion;      // Current suggestion text
size_t suggestion_alloc_size;  // Allocated buffer size

// Core functions
static void update_autosuggestion(readline_context_t *ctx);
static bool has_autosuggestion(readline_context_t *ctx);
static bool accept_autosuggestion(readline_context_t *ctx);

// Context-aware actions (bound via keybinding manager)
lle_result_t lle_forward_char_or_accept_suggestion(readline_context_t *ctx);
lle_result_t lle_end_of_line_or_accept_suggestion(readline_context_t *ctx);
```

### 4.2 update_autosuggestion() Logic

```c
// Bail conditions (no suggestion generated):
// - cursor not at end of buffer
// - buffer length < 2 characters
// - buffer ends with space
// - buffer contains newline (multiline input)

// History search:
// - Search LLE history most-recent-first
// - Skip entries containing newlines (multiline commands)
// - Find first prefix match
// - Store remaining text (after user input) as suggestion
```

### 4.3 display_controller.c - Ghost Text Rendering

```c
// In dc_handle_redraw_needed(), Step 4a:
if (autosuggestions_enabled && !completion_menu_visible && !is_multiline) {
    const char *suggestion = autosuggestions_layer_get_current_suggestion(...);
    if (suggestion && *suggestion) {
        write(STDOUT_FILENO, "\033[90m", 5);   // BRIGHT_BLACK
        write(STDOUT_FILENO, suggestion, strlen(suggestion));
        write(STDOUT_FILENO, "\033[0m", 4);    // Reset
    }
}

// In Step 5, cursor positioning accounts for ghost text wrapping:
size_t suggestion_width = lle_utf8_string_width(suggestion, strlen(suggestion));
int total_cols_needed = command_end_col + (int)suggestion_width;
if (total_cols_needed > term_width) {
    ghost_text_extra_rows = (total_cols_needed - 1) / term_width;
}
int current_terminal_row = final_row + ghost_text_extra_rows + menu_lines;
```

### 4.4 autosuggestions_layer.c - Direct Setter

```c
// New function that bypasses legacy lusush_get_suggestion()
autosuggestions_layer_error_t autosuggestions_layer_set_suggestion(
    autosuggestions_layer_t *layer,
    const char *suggestion);
```

---

## 5. Data Flow

### 5.1 Suggestion Generation Example

```
User types: "git sta"
History contains: "git status", "git stash", "for i in 1 2; do\necho $i\ndone"

1. update_autosuggestion() called with buffer="git sta"
2. Cursor at end? YES (7 == 7)
3. Length >= 2? YES
4. Ends with space? NO
5. Contains newline? NO
6. Search history backwards:
   - Entry "for i in 1 2; do\necho $i\ndone" - SKIP (contains \n)
   - Entry "git stash" - prefix match! remaining = "sh"
7. Store "sh" in ctx->current_suggestion
8. display_controller_set_autosuggestion(dc, "sh")
9. dc_handle_redraw_needed() renders: "git sta" + gray "sh"
```

### 5.2 Acceptance Example

```
Buffer: "git sta", Suggestion: "sh"
User presses Right Arrow

1. execute_keybinding_action(&ctx, "RIGHT", ...)
2. Keybinding manager finds: RIGHT -> lle_forward_char_or_accept_suggestion
3. Action type is CONTEXT, so call with readline_context_t
4. lle_forward_char_or_accept_suggestion(ctx):
   - cursor at end? YES
   - has_autosuggestion(ctx)? YES ("sh" != "")
   - accept_autosuggestion(ctx):
     - Insert "sh" at cursor position
     - Sync cursor manager
     - Clear ctx->current_suggestion
     - return true
   - refresh_display(ctx)
5. Buffer now: "git stash", cursor at position 9
```

---

## 6. Configuration & Behavior

### 6.1 Current Defaults

| Setting | Value | Notes |
|---------|-------|-------|
| Minimum input length | 2 chars | Don't suggest for single character |
| Multiline suggestions | Disabled | History entries with `\n` or `\\n` skipped |
| Suggestions in multiline input | Disabled | When buffer contains newlines |
| Ghost text color | BRIGHT_BLACK | `\033[90m` - works on all terminals |

### 6.2 Acceptance Keys

| Key | Behavior |
|-----|----------|
| Right Arrow | Accept suggestion if at end, else move right |
| End | Accept suggestion if at end, else move to end |
| Ctrl-F | Same as Right Arrow |
| Ctrl-E | Same as End |

### 6.3 Conditions That Clear Suggestions

- Cursor moves away from end of buffer
- User types a character that doesn't continue the match
- Buffer becomes empty or too short
- Completion menu becomes visible
- Buffer becomes multiline (user presses Enter in incomplete construct)

---

## 7. Testing

### 7.1 Test Cases

| Test | Steps | Expected |
|------|-------|----------|
| Basic display | Type "git" (with "git status" in history) | Ghost text " status" appears |
| Right acceptance | Type "git", press Right | Buffer becomes "git status" |
| End acceptance | Type "git", press End | Buffer becomes "git status" |
| Ctrl-F acceptance | Type "git", press Ctrl-F | Buffer becomes "git status" |
| Ctrl-E acceptance | Type "git", press Ctrl-E | Buffer becomes "git status" |
| No multiline suggestions | Type "for" (with multiline for-loop in history) | No suggestion (entry contains \n) |
| Wrapped suggestion | Type short text with long suggestion | Cursor stays on input line, ghost wraps |
| Accept wrapped | Accept long wrapped suggestion | No display corruption |

### 7.2 Verified Working

- Basic suggestion display
- All acceptance keys (Right, End, Ctrl-F, Ctrl-E)
- Multiline history filtering
- Line-wrapped suggestions with correct cursor positioning
- UTF-8/grapheme cluster display width calculation

---

## 8. Future Enhancements

### 8.1 Configuration Options (Recommended)

These options align with Lusush's philosophy of maximum configurability:

```c
// Potential config.h additions
bool lle_autosuggestions_enabled;           // Master enable/disable (default: true)
bool lle_autosuggestions_in_multiline;      // Allow in multiline input (default: false)
int lle_autosuggestions_min_chars;          // Minimum chars before suggesting (default: 2)
char *lle_autosuggestions_color;            // Ghost text color (default: "bright_black")
```

**Rationale for `lle_autosuggestions_in_multiline`:**
- Default `false` is correct for most users (reduces complexity/confusion)
- Power users who frequently re-type similar multiline constructs might want `true`
- Implementing this requires additional display complexity (continuation prompts + ghost text)

### 8.2 Partial Acceptance (Medium Priority)

```
Feature: Ctrl+Right accepts one word at a time
Example: 
  Buffer: "git"
  Suggestion: " commit --amend"
  Ctrl+Right: accepts " commit" only
  Another Ctrl+Right: accepts " --amend"
```

### 8.3 Word-Based Suggestions (Lower Priority)

Currently suggestions are purely prefix-based. Could add:
- Fuzzy matching for typo tolerance
- Word-based matching (match "git com" to "git commit")

### 8.4 Context-Aware Suggestions (Future)

Enhance suggestions based on:
- Current directory (suggest `make` in directories with Makefile)
- Git status (suggest `git add` when files are modified)
- Recent command patterns

### 8.5 Performance Optimization (If Needed)

Current implementation is O(n) history search. If performance becomes an issue:
- Add prefix trie for O(m) lookups where m = query length
- Implement background suggestion generation
- Add configurable history search depth limit

---

## 9. File Change Summary

### 9.1 Files Modified

| File | Changes |
|------|---------|
| `include/display/autosuggestions_layer.h` | Added `autosuggestions_layer_set_suggestion()` declaration |
| `src/display/autosuggestions_layer.c` | Implemented `autosuggestions_layer_set_suggestion()` |
| `include/display/display_controller.h` | Added `display_controller_set_autosuggestion()` declaration |
| `src/display/display_controller.c` | Added direct setter, fixed cursor positioning for wrapped ghost text |
| `src/lle/lle_readline.c` | Added suggestion generation, context-aware actions, keybindings |

### 9.2 Lines of Code

| Component | Approximate LOC |
|-----------|-----------------|
| Suggestion generation (`update_autosuggestion`) | ~80 |
| Helper functions (`has_autosuggestion`, `accept_autosuggestion`) | ~40 |
| Context-aware actions | ~60 |
| Keybinding updates | ~10 |
| Display controller changes | ~60 |
| Autosuggestions layer setter | ~40 |
| **Total** | **~290** |

---

## Appendix A: Troubleshooting

### A.1 Suggestions Not Appearing

1. Check LLE is enabled: `LUSUSH_USE_LLE=1 ./builddir/lusush`
2. Check history has entries: `cat ~/.lusush_history_lle`
3. Ensure cursor is at end of buffer
4. Ensure input is at least 2 characters
5. Ensure input doesn't end with space

### A.2 Acceptance Not Working

1. Ensure cursor is at end of buffer (suggestion only accepts at end)
2. Check that suggestion is actually displayed (ghost text visible)
3. Verify keybindings are registered (RIGHT, END should be context-aware)

### A.3 Display Corruption

1. Check for multiline history entries being suggested (should be filtered)
2. Verify `lle_utf8_string_width()` is being used (not `strlen()`)
3. Check terminal width is being read correctly

---

**Document End**
