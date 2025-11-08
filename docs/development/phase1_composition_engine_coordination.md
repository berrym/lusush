# Phase 1: Composition Engine Coordination Design
## Continuation Prompt Integration Architecture

**Document Status**: Phase 1 Design - Detailed Specification  
**Created**: 2025-11-08  
**Purpose**: Define how the composition engine coordinates continuation_prompt_layer and screen_buffer

---

## Overview

The composition engine serves as the orchestrator for the hybrid continuation prompt architecture. It coordinates three key components:

1. **continuation_prompt_layer**: Generates context-aware prompts for each line
2. **screen_buffer**: Renders multiline content with per-line prefixes
3. **Cursor translation**: Maps buffer positions to display positions

This document specifies the detailed coordination logic, data flow, and integration points.

---

## Current Composition Engine Architecture

### Existing Responsibilities

The composition engine currently combines:
- **prompt_layer**: Primary prompt (single occurrence at start)
- **command_layer**: Command text with syntax highlighting

### Existing Composition Flow

```
1. Get prompt content from prompt_layer
2. Get command content from command_layer
3. Analyze prompt structure (single/multiline)
4. Calculate positioning (where command starts)
5. Combine into single composed output
6. Return composed output to display controller
```

### Key Existing Features to Leverage

- **Prompt structure analysis**: Already analyzes multiline prompts
- **Cursor tracking**: Already calculates cursor positions (incremental tracking)
- **Performance caching**: Already caches composition results
- **Event-driven updates**: Already subscribes to layer change events

---

## Enhanced Architecture for Continuation Prompts

### New Responsibilities

The composition engine will be enhanced to:

1. **Split command into lines**: Parse command content by newlines
2. **Request continuation prompts**: Query continuation_prompt_layer for each line
3. **Coordinate screen_buffer**: Pass prompts and content to screen_buffer with prefix API
4. **Translate cursor position**: Map buffer cursor to display cursor accounting for prompts
5. **Handle multiline editing**: Support cursor movement across lines with prompts

### Integration Points

```
composition_engine_t
    |
    +-- prompt_layer (existing)
    |
    +-- command_layer (existing)
    |
    +-- continuation_prompt_layer (NEW)
    |
    +-- screen_buffer (enhanced with prefix support)
```

---

## Detailed Coordination Flow

### Phase A: Initialization Enhancement

**New Initialization Requirements**:

```c
typedef struct {
    // Existing fields...
    prompt_layer_t *prompt_layer;
    command_layer_t *command_layer;
    
    // NEW: Continuation prompt support
    continuation_prompt_layer_t *continuation_prompt_layer;
    bool continuation_prompts_enabled;
    
    // Enhanced screen buffer (will support prefixes in Phase 2)
    screen_buffer_t *screen_buffer;
    
} composition_engine_t;
```

**New Initialization Function**:

```c
composition_engine_error_t composition_engine_set_continuation_layer(
    composition_engine_t *engine,
    continuation_prompt_layer_t *continuation_layer
);

// Usage:
composition_engine_set_continuation_layer(engine, cont_prompt_layer);
```

**Initialization Checklist**:
- [ ] Add continuation_prompt_layer field to composition_engine_t
- [ ] Add continuation_prompts_enabled flag (default: false)
- [ ] Create setter function for continuation layer
- [ ] Subscribe to continuation layer events (CONTINUATION_PROMPT_CHANGED)
- [ ] Add cache invalidation on continuation settings change

---

### Phase B: Content Splitting Logic

**Line Splitting Algorithm**:

The composition engine must split command content into logical lines for prompt assignment.

```c
/**
 * Split command content into lines for continuation prompt processing.
 * 
 * This function splits the command buffer by newlines while preserving
 * the byte offsets of each line start. This is critical for cursor
 * position translation.
 * 
 * @param command_content The full command text (may contain \n)
 * @param line_info Output array of line information
 * @param max_lines Maximum number of lines to process
 * @param out_line_count Output: number of lines found
 * @return COMPOSITION_ENGINE_SUCCESS on success
 */
typedef struct {
    size_t byte_offset;          // Byte offset of line start in buffer
    size_t length;                // Length of line (excluding \n)
    const char *content;          // Pointer to line content
} command_line_info_t;

composition_engine_error_t composition_engine_split_command_lines(
    const char *command_content,
    command_line_info_t *line_info,
    size_t max_lines,
    size_t *out_line_count
);
```

**Implementation Strategy**:

```c
// Pseudo-code for line splitting:
size_t byte_offset = 0;
size_t line_count = 0;
const char *line_start = command_content;

for (const char *p = command_content; *p != '\0'; p++) {
    if (*p == '\n') {
        // Found line boundary
        line_info[line_count].byte_offset = byte_offset;
        line_info[line_count].length = p - line_start;
        line_info[line_count].content = line_start;
        line_count++;
        
        // Move to next line
        line_start = p + 1;
        byte_offset = (p + 1) - command_content;
    }
}

// Handle last line (no trailing \n)
if (line_start < command_content + strlen(command_content)) {
    line_info[line_count].byte_offset = byte_offset;
    line_info[line_count].length = strlen(line_start);
    line_info[line_count].content = line_start;
    line_count++;
}
```

**Edge Cases**:
- Empty lines (preserve as empty with prompt)
- Lines with only whitespace (preserve)
- Very long lines exceeding COMPOSITION_ENGINE_MAX_LINE_WIDTH
- Commands with no newlines (single line, no continuation prompt)

---

### Phase C: Continuation Prompt Coordination

**Prompt Request Logic**:

For each line after the first, the composition engine requests a continuation prompt.

```c
/**
 * Build continuation prompts for all command lines.
 * 
 * For line 0: Use primary prompt from prompt_layer
 * For line 1+: Request continuation prompt from continuation_prompt_layer
 * 
 * @param engine The composition engine
 * @param line_info Array of line information
 * @param line_count Number of lines
 * @param prompts Output array of prompt strings
 * @param prompt_buffer_size Size of each prompt buffer
 * @return COMPOSITION_ENGINE_SUCCESS on success
 */
composition_engine_error_t composition_engine_build_continuation_prompts(
    composition_engine_t *engine,
    const command_line_info_t *line_info,
    size_t line_count,
    char prompts[][CONTINUATION_PROMPT_MAX_LENGTH],
    size_t prompt_buffer_size
);
```

**Implementation Strategy**:

```c
// Line 0: Use primary prompt (from prompt_layer)
// NOTE: For line 0, we don't use continuation prompt because
// the primary prompt is already handled by existing logic.
// Continuation prompts only apply to lines 1+.

for (size_t i = 1; i < line_count; i++) {
    if (engine->continuation_prompts_enabled) {
        // Request prompt from continuation_prompt_layer
        continuation_prompt_error_t result = 
            continuation_prompt_layer_get_prompt_for_line(
                engine->continuation_prompt_layer,
                i,                              // Line number
                full_command_content,           // Full command for context
                prompts[i],                     // Output buffer
                prompt_buffer_size              // Buffer size
            );
        
        if (result != CONTINUATION_PROMPT_SUCCESS) {
            // Fallback to simple "> " prompt
            snprintf(prompts[i], prompt_buffer_size, "> ");
        }
    } else {
        // Continuation prompts disabled, use empty string
        prompts[i][0] = '\0';
    }
}
```

**Context Passing**:

The continuation_prompt_layer needs the full command content to analyze context:

```c
// Example: For a command like:
// "for i in 1 2 3\n"
// "do\n"
// "    echo $i\n"
// "done"

// Line 1: continuation_prompt_layer receives full command
//         Detects "for" keyword, returns "for> "
// Line 2: continuation_prompt_layer receives full command
//         Detects we're inside for loop body, returns "for> "
// Line 3: continuation_prompt_layer receives full command
//         Detects "done" closes loop, returns "> "
```

**Performance Considerations**:

- **Caching**: Continuation prompts can be cached per command content hash
- **Incremental updates**: Only re-request prompts if command content changed
- **Batch requests**: Request all prompts in single API call (future optimization)

---

### Phase D: Screen Buffer Coordination

**Screen Buffer Integration**:

The composition engine passes line content and prompts to screen_buffer for rendering.

```c
/**
 * Render command content to screen buffer with continuation prompts.
 * 
 * This function coordinates the screen_buffer to render multiline command
 * content with appropriate continuation prompts as line prefixes.
 * 
 * @param engine The composition engine
 * @param line_info Array of line information
 * @param line_count Number of lines
 * @param prompts Array of continuation prompts
 * @param primary_prompt The primary prompt (for line 0)
 * @return COMPOSITION_ENGINE_SUCCESS on success
 */
composition_engine_error_t composition_engine_render_to_screen_buffer(
    composition_engine_t *engine,
    const command_line_info_t *line_info,
    size_t line_count,
    const char prompts[][CONTINUATION_PROMPT_MAX_LENGTH],
    const char *primary_prompt
);
```

**Rendering Strategy**:

```c
// Line 0: Primary prompt + first line of command
screen_buffer_set_line_prefix(screen_buffer, 0, primary_prompt);
screen_buffer_set_line_content(screen_buffer, 0, line_info[0].content);

// Line 1+: Continuation prompt + subsequent lines
for (size_t i = 1; i < line_count; i++) {
    screen_buffer_set_line_prefix(screen_buffer, i, prompts[i]);
    screen_buffer_set_line_content(screen_buffer, i, line_info[i].content);
}

// Render entire buffer
screen_buffer_render_multiline_with_prefixes(
    screen_buffer,
    0,                  // Start line
    line_count          // Number of lines
);
```

**Screen Buffer API Requirements** (Phase 2 Implementation):

```c
// These functions will be implemented in Phase 2: Screen Buffer Enhancement

bool screen_buffer_set_line_prefix(
    screen_buffer_t *buffer,
    int line_num,
    const char *prefix_text
);

bool screen_buffer_set_line_content(
    screen_buffer_t *buffer,
    int line_num,
    const char *content
);

bool screen_buffer_render_multiline_with_prefixes(
    screen_buffer_t *buffer,
    int start_line,
    int num_lines
);
```

**Dirty Tracking Coordination**:

The screen_buffer tracks prefix and content dirty flags independently:

```c
// When continuation prompt changes:
// - Mark line.prefix_dirty = true
// - Keep line.dirty = false (content unchanged)
// - Render only updates prefix

// When command content changes:
// - Mark line.dirty = true
// - Keep line.prefix_dirty = false (prompt unchanged)
// - Render only updates content

// When both change:
// - Mark both flags = true
// - Render updates both prefix and content
```

**Performance Benefit**: Independent dirty tracking minimizes terminal updates.

---

### Phase E: Cursor Position Translation

**Translation Algorithm**:

The composition engine must translate cursor positions between two spaces:

1. **Buffer space**: Cursor as byte offset in command text (no prompts)
2. **Display space**: Cursor as (row, column) on terminal (includes prompts)

```c
/**
 * Translate buffer cursor position to display cursor position.
 * 
 * This function maps a cursor byte offset in the command buffer to
 * a (row, column) position on the terminal display, accounting for:
 * - Primary prompt on line 0
 * - Continuation prompts on lines 1+
 * - UTF-8 multi-byte characters
 * - Wide characters (CJK - 2 columns)
 * - ANSI escape sequences (0 columns)
 * - Tab expansion
 * - Line wrapping (when content exceeds terminal width)
 * 
 * @param engine The composition engine
 * @param cursor_byte_offset Cursor position in command buffer
 * @param terminal_width Terminal width for wrapping
 * @param out_row Output: cursor row (0-based)
 * @param out_col Output: cursor column (0-based)
 * @return COMPOSITION_ENGINE_SUCCESS on success
 */
composition_engine_error_t composition_engine_translate_cursor_buffer_to_display(
    composition_engine_t *engine,
    size_t cursor_byte_offset,
    int terminal_width,
    size_t *out_row,
    size_t *out_col
);
```

**Translation Strategy** (Incremental Tracking):

This uses the **proven incremental tracking approach** from Replxx/Fish/ZLE:

```c
// CORRECT APPROACH: Walk character-by-character
size_t byte_pos = 0;
size_t display_row = 0;
size_t display_col = 0;

// Start after primary prompt
display_col = visual_width(primary_prompt);

for (size_t line_idx = 0; line_idx < line_count; line_idx++) {
    const char *line = line_info[line_idx].content;
    size_t line_len = line_info[line_idx].length;
    
    // If this is not the first line, account for continuation prompt
    if (line_idx > 0) {
        display_row++;
        display_col = visual_width(prompts[line_idx]);
    }
    
    // Walk through line character-by-character
    for (size_t i = 0; i < line_len; ) {
        // Check if this is the cursor position
        if (byte_pos == cursor_byte_offset) {
            *out_row = display_row;
            *out_col = display_col;
            return COMPOSITION_ENGINE_SUCCESS;
        }
        
        // Decode next character
        uint32_t codepoint;
        size_t char_bytes = utf8_decode(line + i, &codepoint);
        
        // Calculate visual width
        int visual_width = 0;
        if (codepoint == '\t') {
            // Tab expands to next multiple of 8
            visual_width = 8 - (display_col % 8);
        } else if (is_ansi_escape(line + i)) {
            // ANSI escape sequences have 0 visual width
            visual_width = 0;
            char_bytes = ansi_sequence_length(line + i);
        } else if (is_wide_character(codepoint)) {
            // CJK characters take 2 columns
            visual_width = 2;
        } else {
            // Normal characters take 1 column
            visual_width = 1;
        }
        
        // Check for line wrapping
        if (display_col + visual_width > terminal_width) {
            display_row++;
            display_col = 0;
        }
        
        // Advance position
        display_col += visual_width;
        byte_pos += char_bytes;
        i += char_bytes;
    }
    
    // Account for newline character
    byte_pos++;  // The \n itself
}

// Cursor is at end of buffer
if (byte_pos == cursor_byte_offset) {
    *out_row = display_row;
    *out_col = display_col;
    return COMPOSITION_ENGINE_SUCCESS;
}

return COMPOSITION_ENGINE_ERROR_INVALID_PARAM;
```

**Why Incremental Tracking**:

This is the **architecturally correct approach** because:

1. **Handles UTF-8**: Correctly decodes multi-byte characters
2. **Handles wide chars**: CJK characters count as 2 columns
3. **Handles ANSI codes**: Escape sequences count as 0 columns
4. **Handles tabs**: Expands to next multiple of 8
5. **Handles wrapping**: Detects when content exceeds terminal width
6. **Never fails**: Works for all possible inputs

**Alternative Rejected**: Simple division/modulo formulas

```c
// WRONG APPROACH (does not work):
row = cursor_offset / terminal_width;
col = cursor_offset % terminal_width;

// This breaks on:
// - UTF-8 multi-byte characters
// - Wide characters (2 columns)
// - ANSI escape sequences
// - Tab expansion
// - Continuation prompts (different widths)
```

**Reverse Translation** (Display to Buffer):

For mouse clicks or cursor positioning commands:

```c
/**
 * Translate display cursor position to buffer cursor position.
 * 
 * This is the reverse operation: given a (row, column) on the terminal,
 * find the corresponding byte offset in the command buffer.
 * 
 * @param engine The composition engine
 * @param display_row Display row (0-based)
 * @param display_col Display column (0-based)
 * @param terminal_width Terminal width
 * @param out_byte_offset Output: cursor position in command buffer
 * @return COMPOSITION_ENGINE_SUCCESS on success
 */
composition_engine_error_t composition_engine_translate_cursor_display_to_buffer(
    composition_engine_t *engine,
    size_t display_row,
    size_t display_col,
    int terminal_width,
    size_t *out_byte_offset
);
```

This uses the same incremental tracking, but stops when (row, col) matches.

---

### Phase F: Modified Composition Flow

**Enhanced composition_engine_compose()**:

The main compose function gains new logic for continuation prompts:

```c
composition_engine_error_t composition_engine_compose(composition_engine_t *engine) {
    // Existing: Get prompt content
    const char *primary_prompt = prompt_layer_get_content(engine->prompt_layer);
    
    // Existing: Get command content
    const char *command_content = command_layer_get_content(engine->command_layer);
    
    // NEW: Check if continuation prompts are needed
    bool has_newlines = strchr(command_content, '\n') != NULL;
    bool use_continuation = engine->continuation_prompts_enabled && has_newlines;
    
    if (use_continuation) {
        // NEW PATH: Multiline with continuation prompts
        
        // Step 1: Split command into lines
        command_line_info_t line_info[COMPOSITION_ENGINE_MAX_COMMAND_LINES];
        size_t line_count;
        composition_engine_split_command_lines(
            command_content, line_info, 
            COMPOSITION_ENGINE_MAX_COMMAND_LINES, &line_count
        );
        
        // Step 2: Build continuation prompts
        char prompts[COMPOSITION_ENGINE_MAX_COMMAND_LINES][CONTINUATION_PROMPT_MAX_LENGTH];
        composition_engine_build_continuation_prompts(
            engine, line_info, line_count, prompts, CONTINUATION_PROMPT_MAX_LENGTH
        );
        
        // Step 3: Render to screen buffer with prefixes
        composition_engine_render_to_screen_buffer(
            engine, line_info, line_count, prompts, primary_prompt
        );
        
        // Step 4: Get rendered output from screen buffer
        screen_buffer_get_rendered_output(
            engine->screen_buffer,
            engine->composed_output,
            COMPOSITION_ENGINE_MAX_OUTPUT_SIZE
        );
        
    } else {
        // EXISTING PATH: Single line or continuation prompts disabled
        // (existing composition logic unchanged)
        snprintf(engine->composed_output, COMPOSITION_ENGINE_MAX_OUTPUT_SIZE,
                 "%s%s", primary_prompt, command_content);
    }
    
    return COMPOSITION_ENGINE_SUCCESS;
}
```

**Key Design Principle**: Existing single-line behavior is **100% unchanged**.

Continuation prompt logic is **additive only**, activated when:
- `continuation_prompts_enabled == true`
- Command contains newlines

---

## Event Handling and Updates

### Event Subscription

The composition engine subscribes to these events:

```c
// Existing subscriptions:
LAYER_EVENT_PROMPT_CHANGED      // Primary prompt changed
LAYER_EVENT_COMMAND_CHANGED     // Command content changed
LAYER_EVENT_THEME_CHANGED       // Theme changed
LAYER_EVENT_TERMINAL_RESIZE     // Terminal resized

// NEW subscriptions:
LAYER_EVENT_CONTINUATION_PROMPT_CHANGED  // Continuation prompt settings changed
```

### Cache Invalidation Strategy

When to invalidate composition cache:

```c
// Invalidate when:
- Primary prompt changes (existing)
- Command content changes (existing)
- Continuation prompt settings change (NEW)
- Continuation prompt mode changes (NEW)
- Terminal width changes (existing)

// Do NOT invalidate when:
- Cursor position changes (cursor tracking is separate)
- Theme colors change (doesn't affect layout)
```

### Incremental Updates

For efficient updates:

```c
// Change: User types a character on line 2
// Result: Only line 2 content is dirty
// Action: Re-render only line 2 (prompt unchanged)

// Change: Command becomes complete/incomplete
// Result: All continuation prompts may change
// Action: Re-request all prompts, mark all prefix_dirty
```

---

## API Summary

### New Composition Engine Functions

```c
// Initialization
composition_engine_error_t composition_engine_set_continuation_layer(
    composition_engine_t *engine,
    continuation_prompt_layer_t *continuation_layer
);

// Control
composition_engine_error_t composition_engine_enable_continuation_prompts(
    composition_engine_t *engine,
    bool enable
);

// Line processing
composition_engine_error_t composition_engine_split_command_lines(
    const char *command_content,
    command_line_info_t *line_info,
    size_t max_lines,
    size_t *out_line_count
);

// Prompt coordination
composition_engine_error_t composition_engine_build_continuation_prompts(
    composition_engine_t *engine,
    const command_line_info_t *line_info,
    size_t line_count,
    char prompts[][CONTINUATION_PROMPT_MAX_LENGTH],
    size_t prompt_buffer_size
);

// Screen buffer coordination
composition_engine_error_t composition_engine_render_to_screen_buffer(
    composition_engine_t *engine,
    const command_line_info_t *line_info,
    size_t line_count,
    const char prompts[][CONTINUATION_PROMPT_MAX_LENGTH],
    const char *primary_prompt
);

// Cursor translation
composition_engine_error_t composition_engine_translate_cursor_buffer_to_display(
    composition_engine_t *engine,
    size_t cursor_byte_offset,
    int terminal_width,
    size_t *out_row,
    size_t *out_col
);

composition_engine_error_t composition_engine_translate_cursor_display_to_buffer(
    composition_engine_t *engine,
    size_t display_row,
    size_t display_col,
    int terminal_width,
    size_t *out_byte_offset
);

// Enhanced compose (existing function, new behavior)
composition_engine_error_t composition_engine_compose(composition_engine_t *engine);

// Enhanced compose with cursor tracking (existing function, new behavior)
composition_engine_error_t composition_engine_compose_with_cursor(
    composition_engine_t *engine,
    size_t cursor_byte_offset,
    int terminal_width,
    composition_with_cursor_t *result
);
```

---

## Integration with Other Components

### Display Controller Integration

The display controller uses the composition engine:

```c
// Existing flow:
1. display_controller calls composition_engine_compose()
2. composition_engine returns composed output
3. display_controller calls display_integration_draw()

// Enhanced flow (no change from display controller perspective):
1. display_controller calls composition_engine_compose()
2. composition_engine internally:
   - Splits command into lines
   - Requests continuation prompts
   - Renders to screen_buffer with prefixes
   - Returns final output
3. display_controller calls display_integration_draw()
```

**Key Point**: Display controller **does not need to change**. The composition engine handles all continuation prompt complexity internally.

### LLE Integration

LLE (line editor) integration:

```c
// LLE provides:
- Command content (may contain \n)
- Cursor position (byte offset in command buffer)

// Composition engine provides:
- Composed output (with continuation prompts)
- Translated cursor position (row, col for terminal)

// LLE never needs to know about:
- Prompt widths
- Visual layout
- Terminal positioning
- Cursor translation
```

**Architectural Purity Maintained**: LLE remains purely focused on text editing, with no terminal knowledge.

---

## Testing Strategy

### Unit Tests

```c
// Test: Line splitting
- Single line (no newlines)
- Multiple lines with \n
- Empty lines
- Lines with only whitespace
- Very long lines

// Test: Cursor translation
- Cursor at start
- Cursor at end
- Cursor in middle of line
- Cursor at line boundary (\n)
- Cursor with UTF-8 characters
- Cursor with wide characters
- Cursor with ANSI escape sequences
- Cursor with tabs

// Test: Prompt coordination
- Simple mode (all "> ")
- Context-aware mode (loop>, if>, etc.)
- Fallback on error
- Disabled continuation prompts

// Test: Incremental updates
- Type character (content dirty)
- Change continuation mode (prefix dirty)
- Both change (both dirty)
```

### Integration Tests

```c
// Test: Full composition flow
- Single line command
- Multiline command with continuation prompts
- Multiline command with prompts disabled
- Command with syntax highlighting + prompts
- Very long multiline command (>8 lines)

// Test: Event handling
- Prompt change invalidates cache
- Command change invalidates cache
- Continuation settings change invalidates cache
- Terminal resize recalculates layout

// Test: Performance
- Composition time <5ms (existing target)
- Cache hit rate >80% during typing
- Minimal redraws (dirty tracking works)
```

### Regression Tests

Ensure existing behavior is unaffected:

```c
// Test: Single-line commands
- Primary prompt only
- No continuation prompts involved
- Cursor positioning unchanged
- Performance unchanged

// Test: Multiline commands with prompts disabled
- Behaves exactly as before
- No performance regression
- No visual changes
```

---

## Performance Analysis

### Expected Performance

```c
// Composition with continuation prompts:
- Line splitting:              < 50 microseconds
- Prompt requests:             < 100 microseconds (per line)
- Screen buffer rendering:     < 200 microseconds
- Cursor translation:          < 100 microseconds
// Total:                       < 1 millisecond

// With caching (95% of cases):
- Cache lookup:                < 10 microseconds
- Dirty check:                 < 5 microseconds
- Incremental update:          < 100 microseconds
// Total:                       < 150 microseconds
```

### Memory Overhead

```c
// Per composition_engine_t:
sizeof(continuation_prompt_layer_t*)        8 bytes
sizeof(bool continuation_prompts_enabled)   1 byte
sizeof(command_line_info_t) * 8            192 bytes (max 8 lines)
sizeof(prompts) * 8                        512 bytes (64 bytes per prompt)
// Total:                                   ~713 bytes

// Acceptable overhead for feature value
```

---

## Implementation Checklist

Phase 3 (Composition Engine) will implement:

- [ ] Add continuation_prompt_layer field to composition_engine_t
- [ ] Add continuation_prompts_enabled flag
- [ ] Implement composition_engine_set_continuation_layer()
- [ ] Implement composition_engine_enable_continuation_prompts()
- [ ] Implement composition_engine_split_command_lines()
- [ ] Implement composition_engine_build_continuation_prompts()
- [ ] Implement composition_engine_render_to_screen_buffer()
- [ ] Implement composition_engine_translate_cursor_buffer_to_display()
- [ ] Implement composition_engine_translate_cursor_display_to_buffer()
- [ ] Enhance composition_engine_compose() with continuation logic
- [ ] Enhance composition_engine_compose_with_cursor() with continuation logic
- [ ] Add event subscription for LAYER_EVENT_CONTINUATION_PROMPT_CHANGED
- [ ] Add cache invalidation for continuation prompt changes
- [ ] Write unit tests for all new functions
- [ ] Write integration tests for full flow
- [ ] Write regression tests for existing behavior
- [ ] Performance profiling and optimization

---

## Open Questions and Future Considerations

### 1. Batch Prompt Requests

**Question**: Should we request all continuation prompts in a single batch API call?

**Current Design**: Loop calling get_prompt_for_line() for each line

**Alternative**: Single batch API:
```c
continuation_prompt_layer_get_prompts_for_lines(
    layer, 0, line_count, full_command, prompts_array
);
```

**Decision**: Start with per-line API (simpler), optimize with batch API in Phase 5 if needed.

### 2. Partial Cursor Translation

**Question**: Can we optimize cursor translation by caching partial results?

**Idea**: If cursor moves within same line, reuse previous translation + offset

**Challenge**: Must still handle wrapping, UTF-8, etc. within line

**Decision**: Start with full translation (simpler, correct), optimize if profiling shows bottleneck.

### 3. Multi-cursor Support

**Question**: Should composition engine support multiple cursors?

**Current Design**: Single cursor (cursor_byte_offset)

**Future**: Some advanced editors support multiple cursors for parallel editing

**Decision**: Out of scope for Phase 1-5. Revisit if LLE adds multi-cursor support.

### 4. Autosuggestions Integration

**Question**: How will autosuggestions layer integrate with this design?

**Design**: Autosuggestions layer will use screen_buffer prefix API similarly:
```c
// Autosuggestion rendered as prefix on line below command
screen_buffer_set_line_prefix(buffer, command_line_count, "");
screen_buffer_set_line_content(buffer, command_line_count, autosuggestion_text);
```

**Decision**: Screen buffer prefix API is designed for both continuation prompts and autosuggestions.

---

## Conclusion

This design establishes the composition engine as the central orchestrator for the hybrid continuation prompt architecture. Key principles:

1. **Non-invasive**: Existing behavior 100% unchanged
2. **Additive**: New functionality only activates when needed
3. **Efficient**: Leverages caching and dirty tracking
4. **Correct**: Uses incremental cursor tracking (proven approach)
5. **Extensible**: Foundation for autosuggestions and future features

The composition engine coordinates continuation_prompt_layer and screen_buffer while maintaining architectural purity and performance targets.

**Next Phase**: Implement cursor position translation algorithm specification (detailed character-by-character walkthrough with edge cases).
