# Phase 1: Screen Buffer Enhancement Design

**Document**: phase1_screen_buffer_enhancement_design.md  
**Date**: 2025-11-08  
**Phase**: 1 - Detailed Design  
**Status**: Design Complete - Ready for Review  
**Purpose**: Enhance screen_buffer to support per-line prefixes (continuation prompts)

---

## Overview

The screen buffer enhancement adds native support for per-line prefixes (like continuation prompts) that are:
1. **Separate from content**: Prefix tracked independently from line content
2. **Independently dirty**: Can update prefix without touching content
3. **Cursor-aware**: Cursor position accounts for prefix length
4. **Efficient**: Minimal rendering overhead using existing dirty tracking

### Design Goals

1. **Maintain backward compatibility**: Existing code continues to work
2. **Leverage existing architecture**: Build on proven screen_buffer design
3. **Efficient rendering**: Independent dirty tracking for prefix vs content
4. **Natural cursor handling**: Prefix length automatically factored in
5. **Reusable**: Works for continuation prompts AND future features (autosuggestions)

---

## Current Screen Buffer Architecture

### Existing Structure

```c
typedef struct {
    char ch;              // Character (UTF-8 byte)
    bool is_prompt;       // True if this cell is part of the prompt
} screen_cell_t;

typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;           // Number of characters in this line
    bool dirty;           // True if this line changed since last render
} screen_line_t;

typedef struct {
    screen_line_t lines[SCREEN_BUFFER_MAX_ROWS];
    int num_rows;
    int terminal_width;
    int cursor_row;
    int cursor_col;
} screen_buffer_t;
```

### Key Function

```c
void screen_buffer_render(
    screen_buffer_t *buffer,
    const char *prompt_text,     // Single primary prompt
    const char *command_text,    // Command content
    size_t cursor_byte_offset
);
```

**Current Behavior**: Renders prompt once, then command content with wrapping.

---

## Enhanced Screen Buffer Architecture

### Enhanced Structures

```c
/**
 * Per-line prefix information
 * 
 * Stores prefix (e.g., continuation prompt) separately from content
 * for efficient independent updates and cursor position calculation.
 */
typedef struct {
    char *text;               // Prefix text (e.g., "> ", "loop> ")
    size_t length;            // Length in characters (UTF-8 aware)
    size_t visual_width;      // Visual width (excluding ANSI codes)
    bool contains_ansi;       // True if contains ANSI escape sequences
    bool dirty;               // True if prefix changed since last render
} screen_line_prefix_t;

/**
 * Enhanced line structure with prefix support
 * 
 * BACKWARD COMPATIBLE: prefix is optional (NULL = no prefix)
 */
typedef struct {
    screen_cell_t cells[SCREEN_BUFFER_MAX_COLS];
    int length;                        // Number of characters in this line (content only)
    bool dirty;                        // True if CONTENT changed since last render
    
    // NEW: Prefix support
    screen_line_prefix_t *prefix;      // Line prefix (NULL = no prefix)
    bool prefix_dirty;                 // True if PREFIX changed (independent of content)
    
    // Cached calculations
    int total_visual_width;            // prefix_width + content_width (cached)
    bool width_cache_valid;            // True if total_visual_width is valid
} screen_line_t;

/**
 * Screen buffer (main structure remains compatible)
 */
typedef struct {
    screen_line_t lines[SCREEN_BUFFER_MAX_ROWS];
    int num_rows;
    int terminal_width;
    int cursor_row;
    int cursor_col;                    // NOTE: This is DISPLAY position (includes prefix)
    
    // NEW: Cursor position details
    int cursor_buffer_col;             // Cursor position in CONTENT (excluding prefix)
    bool cursor_in_prefix;             // True if cursor is in prefix area (normally false)
} screen_buffer_t;
```

### Key Design Decisions

1. **Prefix is Optional**: `prefix == NULL` means no prefix (backward compatible)
2. **Independent Dirty Tracking**: `dirty` (content) and `prefix_dirty` (prefix) tracked separately
3. **Cursor Distinction**: Two cursor positions:
   - `cursor_col`: Display position (includes prefix)
   - `cursor_buffer_col`: Content position (excludes prefix)
4. **Visual Width Caching**: Cache `total_visual_width` to avoid recalculating

---

## New API Functions

### Prefix Management

```c
/**
 * Set line prefix
 * 
 * Sets or updates the prefix for a specific line. Prefix is copied internally.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number (0-based)
 * @param prefix_text Prefix string (NULL to remove prefix)
 * @return true on success, false on error
 * 
 * @note Marks prefix as dirty if changed
 * @note Invalidates width cache
 * @note Prefix may contain ANSI escape sequences
 */
bool screen_buffer_set_line_prefix(
    screen_buffer_t *buffer,
    int line_num,
    const char *prefix_text
);

/**
 * Get line prefix
 * 
 * Returns pointer to prefix text for a line.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number
 * @return Prefix text, or NULL if no prefix
 * 
 * @note Returned pointer is internal - do not modify or free
 */
const char *screen_buffer_get_line_prefix(
    const screen_buffer_t *buffer,
    int line_num
);

/**
 * Get line prefix visual width
 * 
 * Returns visual width of prefix (ANSI codes excluded).
 * 
 * @param buffer Screen buffer
 * @param line_num Line number
 * @return Visual width in columns, or 0 if no prefix
 */
size_t screen_buffer_get_prefix_width(
    const screen_buffer_t *buffer,
    int line_num
);

/**
 * Clear all line prefixes
 * 
 * Removes prefixes from all lines.
 * 
 * @param buffer Screen buffer
 */
void screen_buffer_clear_prefixes(screen_buffer_t *buffer);
```

### Combined Rendering

```c
/**
 * Render line with separate prefix and content
 * 
 * Renders a line with optional prefix. More efficient than calling
 * screen_buffer_set_line_prefix() then screen_buffer_render().
 * 
 * @param buffer Screen buffer
 * @param line_num Line number
 * @param prefix_text Prefix string (NULL for no prefix)
 * @param content_text Line content
 * @param cursor_in_content Cursor position within content (or -1 if not on this line)
 * @return true on success, false on error
 * 
 * @note Only marks dirty if content/prefix actually changed
 * @note Cursor position is content-relative (0 = first character of content)
 */
bool screen_buffer_render_line_with_prefix(
    screen_buffer_t *buffer,
    int line_num,
    const char *prefix_text,
    const char *content_text,
    int cursor_in_content
);

/**
 * Render multiline command with per-line prefixes
 * 
 * High-level function for rendering multiline commands with continuation prompts.
 * 
 * @param buffer Screen buffer
 * @param lines Array of content strings (one per line)
 * @param prefixes Array of prefix strings (NULL entries = no prefix)
 * @param line_count Number of lines
 * @param cursor_line Line containing cursor (or -1 if none)
 * @param cursor_offset Cursor offset within cursor_line content
 * @return true on success, false on error
 * 
 * Example:
 *   const char *lines[] = {"for i in 1 2 3; do", "echo $i", "done"};
 *   const char *prefixes[] = {NULL, "loop> ", "loop> "};
 *   screen_buffer_render_multiline_with_prefixes(
 *       buffer, lines, prefixes, 3, 1, 5
 *   );
 */
bool screen_buffer_render_multiline_with_prefixes(
    screen_buffer_t *buffer,
    const char **lines,
    const char **prefixes,
    int line_count,
    int cursor_line,
    int cursor_offset
);
```

### Cursor Position Translation

```c
/**
 * Translate buffer position to display position
 * 
 * Converts cursor position in content (excluding prefix) to
 * display position (including prefix).
 * 
 * @param buffer Screen buffer
 * @param line_num Line number
 * @param buffer_col Cursor position in content (0-based)
 * @return Display column position (includes prefix width)
 * 
 * Example:
 *   Prefix: "loop> " (6 characters)
 *   buffer_col = 0 → returns 6 (first char of content)
 *   buffer_col = 5 → returns 11 (sixth char of content)
 */
int screen_buffer_translate_buffer_to_display_col(
    const screen_buffer_t *buffer,
    int line_num,
    int buffer_col
);

/**
 * Translate display position to buffer position
 * 
 * Converts display position to content position.
 * 
 * @param buffer Screen buffer
 * @param line_num Line number
 * @param display_col Display column position
 * @param out_buffer_col Output: Content position (may be negative if in prefix)
 * @return true if position is in content, false if in prefix
 * 
 * Example:
 *   Prefix: "loop> " (6 characters)
 *   display_col = 3 → returns false, out_buffer_col = -3 (in prefix)
 *   display_col = 6 → returns true, out_buffer_col = 0 (start of content)
 *   display_col = 10 → returns true, out_buffer_col = 4 (in content)
 */
bool screen_buffer_translate_display_to_buffer_col(
    const screen_buffer_t *buffer,
    int line_num,
    int display_col,
    int *out_buffer_col
);
```

### Enhanced Existing Functions

```c
/**
 * Enhanced render function (backward compatible)
 * 
 * Behavior:
 * - If no line prefixes set: Works exactly as before
 * - If line prefixes set: Renders first line with prompt_text,
 *   subsequent lines with their prefixes
 * 
 * @param buffer Buffer to render into
 * @param prompt_text Primary prompt (line 0)
 * @param command_text Command content (may span multiple lines)
 * @param cursor_byte_offset Cursor position in command_text
 * 
 * @note Fully backward compatible - existing callers unaffected
 */
void screen_buffer_render(
    screen_buffer_t *buffer,
    const char *prompt_text,
    const char *command_text,
    size_t cursor_byte_offset
);

/**
 * Enhanced diff generation
 * 
 * Behavior:
 * - Detects prefix-only changes (content unchanged)
 * - Generates efficient updates for prefix-only changes
 * - Only updates changed portions (prefix OR content)
 * 
 * @param old_buffer Previous state
 * @param new_buffer New state
 * @param diff Output diff
 * 
 * @note Leverages independent dirty tracking
 */
void screen_buffer_diff(
    const screen_buffer_t *old_buffer,
    const screen_buffer_t *new_buffer,
    screen_diff_t *diff
);
```

---

## Implementation Details

### Memory Management

**Prefix Allocation**:
```c
// When setting prefix
if (line->prefix == NULL) {
    line->prefix = malloc(sizeof(screen_line_prefix_t));
}
free(line->prefix->text);  // Free old
line->prefix->text = strdup(prefix_text);  // Allocate new
line->prefix->length = strlen_utf8(prefix_text);
line->prefix->visual_width = visual_width_no_ansi(prefix_text);
line->prefix_dirty = true;
```

**Cleanup**:
```c
// When clearing or destroying
if (line->prefix) {
    free(line->prefix->text);
    free(line->prefix);
    line->prefix = NULL;
}
```

### Dirty Tracking Logic

**Prefix Change**:
```c
if (line->prefix && strcmp(line->prefix->text, new_prefix) != 0) {
    free(line->prefix->text);
    line->prefix->text = strdup(new_prefix);
    line->prefix_dirty = true;  // Mark prefix dirty
    // line->dirty unchanged (content not affected)
}
```

**Content Change**:
```c
if (line_content_changed(line, new_content)) {
    update_line_content(line, new_content);
    line->dirty = true;         // Mark content dirty
    // line->prefix_dirty unchanged (prefix not affected)
}
```

**Diff Generation**:
```c
// Optimize: Only update changed portions
if (new_line->prefix_dirty && !new_line->dirty) {
    // Only prefix changed - update prefix region only
    generate_prefix_update(diff, line_num, new_line->prefix);
} else if (new_line->dirty && !new_line->prefix_dirty) {
    // Only content changed - update content region only
    generate_content_update(diff, line_num, new_line);
} else if (new_line->prefix_dirty && new_line->dirty) {
    // Both changed - update entire line
    generate_full_line_update(diff, line_num, new_line);
}
```

### Cursor Position Handling

**Setting Cursor**:
```c
// Compositor provides content-relative position
int buffer_col = 5;  // 5th character in content
int line_num = 1;

// Screen buffer translates to display position
buffer->cursor_buffer_col = buffer_col;
buffer->cursor_col = screen_buffer_translate_buffer_to_display_col(
    buffer, line_num, buffer_col
);
buffer->cursor_row = line_num;
```

**Cursor Movement**:
```c
// User presses LEFT arrow (in compositor/LLE)
cursor_in_buffer--;  // Move left in buffer space

// Screen buffer translates for display
display_col = buffer_to_display_col(cursor_in_buffer);
move_terminal_cursor(display_col);
```

### Visual Width Caching

```c
// Cache total line width for performance
void update_line_width_cache(screen_line_t *line) {
    if (line->width_cache_valid) return;
    
    line->total_visual_width = 0;
    
    if (line->prefix) {
        line->total_visual_width += line->prefix->visual_width;
    }
    
    line->total_visual_width += calculate_content_width(line);
    line->width_cache_valid = true;
}

// Invalidate on changes
void invalidate_width_cache(screen_line_t *line) {
    line->width_cache_valid = false;
}
```

---

## Backward Compatibility

### Existing Code Continues Working

**Before Enhancement**:
```c
screen_buffer_render(buffer, "$ ", "echo hello", 0);
// Works as before
```

**After Enhancement**:
```c
// Same call works identically
screen_buffer_render(buffer, "$ ", "echo hello", 0);
// No prefixes set, behaves exactly as before
```

### Migration Path

**Phase 1**: Add new structures/functions (backward compatible)
```c
// Old code: Still works
screen_buffer_render(buffer, prompt, command, cursor);

// New code: Can use prefixes
screen_buffer_set_line_prefix(buffer, 1, "loop> ");
```

**Phase 2**: Existing callers can gradually adopt new API
```c
// Gradually migrate to new rendering
screen_buffer_render_multiline_with_prefixes(...);
```

**Phase 3**: Eventually deprecate old single-prompt API (far future)

---

## Performance Considerations

### Efficient Updates

**Scenario 1**: User types character (content changes)
- Only `line->dirty = true`
- Prefix not re-rendered
- **Saves**: 1-6 characters of terminal output per keystroke

**Scenario 2**: Mode switches simple→context-aware (prefix changes)
- Only `line->prefix_dirty = true`
- Content not re-rendered
- **Saves**: Entire line content redraw (could be 50+ characters)

**Scenario 3**: Both change (new line added)
- Both flags set
- Full line render
- **No overhead**: Same as without prefixes

### Memory Overhead

**Per Line**:
- `screen_line_prefix_t*`: 8 bytes (pointer)
- `bool prefix_dirty`: 1 byte
- `prefix structure`: ~32 bytes when allocated
- **Total**: ~9 bytes per line (when no prefix), ~41 bytes (with prefix)

**For 100 lines**: ~4KB additional memory (negligible)

### Cache Benefits

Width caching avoids recalculating on every cursor movement:
- **Without cache**: Calculate width every cursor update (~1000x/sec)
- **With cache**: Calculate width on content change only (~10x/sec)
- **Savings**: 99% reduction in width calculations

---

## Integration with Continuation Prompt Layer

### Composition Flow

```c
// 1. Compositor gets command content
const char *command = command_layer_get_content(layer);

// 2. Split into lines
char **lines = split_by_newline(command, &line_count);

// 3. Get prompts from continuation layer
char prompts[line_count][64];
for (int i = 1; i < line_count; i++) {
    continuation_prompt_layer_get_prompt_for_line(
        cont_layer, i, command, prompts[i], 64
    );
}

// 4. Render to screen buffer with prefixes
screen_buffer_render_multiline_with_prefixes(
    screen_buffer,
    lines,
    prompts,  // prompts[0] = NULL (use primary prompt)
    line_count,
    cursor_line,
    cursor_offset
);

// 5. Generate diff and apply
screen_buffer_diff(old_buffer, screen_buffer, &diff);
screen_buffer_apply_diff(&diff, STDOUT_FILENO);
```

### Cursor Translation Example

```c
// User has cursor at position 5 in "echo $i" (second line)
// Line 1: "loop> echo $i"
//         ^^^^^^ prefix (6 chars)
//               ^^^^^   content (cursor at position 5)

// Compositor knows:
int buffer_line = 1;
int buffer_col = 5;

// Screen buffer translates:
int display_col = screen_buffer_translate_buffer_to_display_col(
    screen_buffer,
    buffer_line,
    buffer_col
);
// display_col = 11 (6 for prefix + 5 for position)

// Terminal cursor positioned at column 11
```

---

## Testing Strategy

### Unit Tests

1. **Prefix Management**:
   - Set prefix on line
   - Get prefix from line
   - Remove prefix (set NULL)
   - Update existing prefix
   - Prefix with ANSI codes
   - Visual width calculation

2. **Dirty Tracking**:
   - Prefix change sets prefix_dirty only
   - Content change sets dirty only
   - Both changes set both flags
   - Clear resets flags

3. **Cursor Translation**:
   - Buffer to display (with/without prefix)
   - Display to buffer (in prefix vs content)
   - Edge cases (cursor at 0, at boundary)

4. **Rendering**:
   - Single line with prefix
   - Multiline with prefixes
   - Mixed (some lines with, some without)
   - Line wrapping with prefixes

5. **Performance**:
   - Width cache effectiveness
   - Diff generation with prefixes
   - Memory usage

6. **Backward Compatibility**:
   - Old render calls work unchanged
   - No prefixes = identical behavior
   - Existing diff generation unaffected

### Integration Tests

1. With continuation_prompt_layer
2. With composition engine
3. Cursor movement across prefix boundaries
4. Mode switching (simple ↔ context-aware)
5. Terminal resize with prefixes

---

## Open Questions for Review

1. **Prefix Ownership**: Should screen_buffer own prefix memory or just reference it?
   - Current design: Owns (copies string)
   - Alternative: Reference (faster but requires external lifetime management)

2. **Width Cache Granularity**: Cache per-line or per-buffer?
   - Current: Per-line (more granular invalidation)
   - Alternative: Per-buffer (simpler but less efficient)

3. **Cursor in Prefix**: Should cursor ever be IN prefix area?
   - Current: No (cursor_in_prefix tracked but normally false)
   - Use case: Editing continuation prompt itself (future feature?)

4. **ANSI Stripping**: Reuse existing utilities or implement internally?
   - Current: Use screen_buffer_visual_width() (existing)
   - Benefits: Consistency with rest of screen_buffer

---

## API Summary

### New Functions (14 total)

**Prefix Management (4)**:
- `screen_buffer_set_line_prefix()`
- `screen_buffer_get_line_prefix()`
- `screen_buffer_get_prefix_width()`
- `screen_buffer_clear_prefixes()`

**Rendering (2)**:
- `screen_buffer_render_line_with_prefix()`
- `screen_buffer_render_multiline_with_prefixes()`

**Cursor Translation (2)**:
- `screen_buffer_translate_buffer_to_display_col()`
- `screen_buffer_translate_display_to_buffer_col()`

**Enhanced Existing (2)**:
- `screen_buffer_render()` - enhanced but backward compatible
- `screen_buffer_diff()` - enhanced with prefix awareness

### Data Structure Changes

**Enhanced `screen_line_t`**:
- Added: `screen_line_prefix_t *prefix`
- Added: `bool prefix_dirty`
- Added: `int total_visual_width`
- Added: `bool width_cache_valid`

**Enhanced `screen_buffer_t`**:
- Added: `int cursor_buffer_col`
- Added: `bool cursor_in_prefix`

**New `screen_line_prefix_t`**: Complete structure for prefix info

---

## Next Steps

1. ✅ Design complete
2. ⏳ Review and approval
3. ⏳ Update screen_buffer.h header
4. ⏳ Design composition engine coordination (next document)

---

**Design Status**: Complete - Ready for implementation  
**Estimated Implementation Time**: 2 days  
**Backward Compatibility**: 100% - all existing code continues working  
**Memory Overhead**: ~40 bytes per line with prefix, ~9 bytes without  
**Author**: AI Assistant with user collaboration
