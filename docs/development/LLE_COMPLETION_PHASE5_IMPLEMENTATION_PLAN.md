# LLE Completion System - Phase 5 Implementation Plan
**Date**: 2025-11-17  
**Phase**: Display Layer Integration  
**Approach**: Start simple, iterate based on learnings  
**Priority**: Architecturally correct + "just works"

---

## Executive Summary

**Goal**: Integrate completion menu rendering with display system

**Strategy**: Start with Approach A (append menu to command text), iterate if needed

**Key Constraints**:
- Menu limited to one screenful at a time
- Must support scrolling/cycling through entire completion set
- Menu updates should be incremental via diff system
- Cursor stays in command, menu selection separate
- Follow proven continuation prompt pattern

---

## Phase 5 Overview

### What We Have (Phases 1-4 Complete)

**Phase 1-3: Completion Logic**
- ✅ Type classification (completion_types)
- ✅ Data sources (completion_sources)
- ✅ Generation orchestration (completion_generator)

**Phase 4: Menu State**
- ✅ Menu state management (completion_menu_state)
- ✅ Navigation logic (completion_menu_logic)
- ✅ Scrolling support (`first_visible`, `visible_count`)
- ✅ Category tracking

**Phase 5: Display Integration (THIS PHASE)**
- ⏳ Menu text generation
- ⏳ Display system integration
- ⏳ Event-driven updates
- ⏳ Clearing mechanism

---

## Implementation Strategy

### Approach A: Append Menu to Command Text

**Concept**: Treat menu as additional lines after command

**Benefits**:
- Simplest to implement
- Reuses existing screen_buffer infrastructure
- Known to work (similar to multiline commands)
- Minimal API changes

**Flow**:
```
Command text: "ls"
Menu text:    "\ncompleting external command\nls  lsar  lsattr\n..."
Combined:     "ls\ncompleting external command\nls  lsar  lsattr\n..."
                ↓
        screen_buffer_render()
                ↓
        Diff/Apply as normal
```

**Risks**:
- Menu becomes part of "command" from screen_buffer perspective
- Clearing might need special handling
- Cursor byte offset might need adjustment

**Mitigation**:
- Use marker to separate command from menu
- Track menu start position
- Adjust cursor offset to stay in command portion

---

## Detailed Implementation Plan

### Part 1: Menu Text Renderer (New Component)

**File**: `src/lle/completion/completion_menu_renderer.c/h`

**Why separate file?**
- Keeps menu logic in LLE (where state lives)
- Separates formatting from state management
- Can be unit tested independently

**API**:
```c
/**
 * Render completion menu to text buffer
 * 
 * Generates formatted menu text from menu state.
 * Includes category headers, item formatting, selection highlighting.
 * Respects visible_count for scrolling.
 * 
 * @param state Menu state (contains results, selection, visible range)
 * @param output Buffer for rendered menu text
 * @param output_size Size of output buffer
 * @param terminal_width Terminal width for wrapping
 * @param max_rows Maximum rows to render (screen limit)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_completion_menu_render(
    const lle_completion_menu_state_t *state,
    char *output,
    size_t output_size,
    size_t terminal_width,
    size_t max_rows
);
```

**What it does**:

1. **Calculate visible items** from `state->first_visible` and `state->visible_count`
2. **Format header** (e.g., "completing external command")
3. **Format each visible item**:
   - Category headers when type changes
   - Selection highlight for selected item
   - Type indicators if enabled
   - Column wrapping within terminal width
4. **Return formatted text** with `\n` separating rows

**Selection Highlighting**:
```c
// Option: ANSI reverse video for selected item
if (i == state->selected_index) {
    len += snprintf(output + len, output_size - len, "\033[7m%s\033[0m ", item->text);
} else {
    len += snprintf(output + len, output_size - len, "%s ", item->text);
}
```

**Column Layout**:
```c
// Multi-column layout like zsh
size_t col_width = calculate_column_width(visible_items);
size_t cols_per_row = terminal_width / col_width;
// Format items into columns
```

**Category Headers**:
```c
// When item type changes
if (i > 0 && items[i].type != items[i-1].type) {
    len += snprintf(output + len, output_size - len, 
                    "\ncompleting %s\n", get_type_name(items[i].type));
}
```

---

### Part 2: Command Layer Extension

**File**: `src/display/command_layer.c/h`

**Add to command_layer_t**:
```c
typedef struct {
    // Existing fields
    char *command_text;
    size_t command_length;
    char *highlighted_text;
    size_t highlighted_length;
    size_t cursor_position;
    
    // NEW: Completion menu support
    bool completion_menu_active;
    lle_completion_menu_state_t *menu_state;
    char *menu_text;                    // Rendered menu
    size_t menu_text_length;
    size_t command_only_length;         // Length of command without menu
    
    // ... rest of existing fields
} command_layer_t;
```

**New Functions**:

```c
/**
 * Set completion menu state
 * 
 * Associates a completion menu with the command layer.
 * Triggers menu rendering and display update.
 * 
 * @param layer Command layer
 * @param menu_state Menu state from LLE completion system (can be NULL to clear)
 * @return Error code
 */
command_layer_error_t command_layer_set_completion_menu(
    command_layer_t *layer,
    lle_completion_menu_state_t *menu_state
);

/**
 * Update completion menu
 * 
 * Re-renders menu (e.g., after selection changed).
 * Called when menu state changes but command text unchanged.
 * 
 * @param layer Command layer
 * @return Error code
 */
command_layer_error_t command_layer_update_completion_menu(
    command_layer_t *layer
);

/**
 * Clear completion menu
 * 
 * Removes menu from display.
 * 
 * @param layer Command layer
 * @return Error code
 */
command_layer_error_t command_layer_clear_completion_menu(
    command_layer_t *layer
);
```

**Implementation of set_completion_menu**:
```c
command_layer_error_t command_layer_set_completion_menu(
    command_layer_t *layer,
    lle_completion_menu_state_t *menu_state
) {
    if (!layer) return COMMAND_LAYER_ERROR_NULL_POINTER;
    
    // Clear existing menu if present
    if (layer->menu_text) {
        free(layer->menu_text);
        layer->menu_text = NULL;
        layer->menu_text_length = 0;
    }
    
    // Store menu state reference (NOT owned by command_layer)
    layer->menu_state = menu_state;
    layer->completion_menu_active = (menu_state != NULL && menu_state->menu_active);
    
    if (layer->completion_menu_active) {
        // Render menu to text
        char menu_buffer[16384];  // Large buffer for menu
        lle_result_t result = lle_completion_menu_render(
            menu_state,
            menu_buffer,
            sizeof(menu_buffer),
            layer->terminal_width,
            20  // Max 20 rows for menu
        );
        
        if (result == LLE_SUCCESS) {
            layer->menu_text = strdup(menu_buffer);
            layer->menu_text_length = strlen(menu_buffer);
        }
        
        // Rebuild highlighted_text with menu appended
        rebuild_highlighted_text_with_menu(layer);
    }
    
    // Fire event to trigger display update
    fire_command_text_changed_event(layer);
    
    return COMMAND_LAYER_SUCCESS;
}
```

**Implementation of rebuild_highlighted_text_with_menu**:
```c
static void rebuild_highlighted_text_with_menu(command_layer_t *layer) {
    if (!layer->highlighted_text || !layer->menu_text) return;
    
    // Save command-only length (for cursor offset preservation)
    layer->command_only_length = layer->highlighted_length;
    
    // Allocate new buffer for command + menu
    size_t total_size = layer->highlighted_length + 1 + layer->menu_text_length + 1;
    char *new_highlighted = malloc(total_size);
    
    if (new_highlighted) {
        // Copy command text
        strcpy(new_highlighted, layer->highlighted_text);
        
        // Append newline separator
        strcat(new_highlighted, "\n");
        
        // Append menu text
        strcat(new_highlighted, layer->menu_text);
        
        // Replace highlighted_text
        free(layer->highlighted_text);
        layer->highlighted_text = new_highlighted;
        layer->highlighted_length = strlen(new_highlighted);
    }
}
```

---

### Part 3: Display Controller Integration

**File**: `src/display/display_controller.c`

**Modify handle_redraw_event()**:

**Current flow** (Line 212-218):
```c
char command_buffer[COMMAND_LAYER_MAX_HIGHLIGHTED_SIZE];
command_layer_error_t cmd_result = command_layer_get_highlighted_text(
    cmd_layer, command_buffer, sizeof(command_buffer));
```

**No changes needed!** The menu is already in `highlighted_text` if active.

**Cursor offset adjustment** (Line 309):
```c
size_t cursor_byte_offset = cmd_layer->cursor_position;

// NEW: If menu is active, cursor_position is still valid
// because it refers to position in command-only text
// screen_buffer_render will calculate screen position correctly
```

**The beauty of this approach**: Display controller doesn't need to know about menu!

**Potential issue**: If `highlighted_text` now includes menu, continuation prompt detection needs adjustment:

```c
// Line 229: Count newlines in command ONLY, not menu
int newline_count = count_newlines_in_range(
    command_buffer, 
    0, 
    cmd_layer->command_only_length  // NEW: Only count command portion
);
```

---

### Part 4: LLE Integration (Entry Point)

**Where completion is triggered**: Likely in `lle_readline.c` or keybinding handler

**Completion Flow**:

```c
// 1. User presses TAB
// 2. Generate completions (Phases 1-3)
lle_completion_result_t *result = NULL;
lle_completion_generate(memory_pool, buffer, cursor_pos, &result);

if (result && result->count > 0) {
    // 3. Create menu state (Phase 4)
    lle_completion_menu_state_t *menu_state = NULL;
    lle_completion_menu_state_create(memory_pool, result, &config, &menu_state);
    
    // 4. Set menu on command layer (Phase 5 - NEW)
    command_layer_set_completion_menu(cmd_layer, menu_state);
    
    // 5. Display update happens automatically via event system
}
```

**Menu Navigation** (Arrow keys):

```c
// User presses Down arrow while menu active
if (menu_state && menu_state->menu_active) {
    // Update menu state
    lle_completion_menu_move_down(menu_state);
    
    // Re-render menu
    command_layer_update_completion_menu(cmd_layer);
    
    // Display update happens automatically
}
```

**Menu Accept** (Enter key):

```c
// User presses Enter while menu active
if (menu_state && menu_state->menu_active) {
    const lle_completion_item_t *selected = NULL;
    lle_completion_menu_accept(menu_state, &selected);
    
    if (selected) {
        // Replace current word with selected completion
        replace_word_at_cursor(buffer, cursor_pos, selected->text);
        
        // Clear menu
        command_layer_clear_completion_menu(cmd_layer);
        
        // Update command display
        command_layer_update_command(cmd_layer, buffer, new_cursor_pos);
    }
}
```

---

### Part 5: Clearing Mechanism

**When to clear menu**:
- User accepts completion (Enter)
- User cancels (Escape)
- User types character (dismisses menu)
- Command changes significantly

**How clearing works**:

```c
command_layer_error_t command_layer_clear_completion_menu(
    command_layer_t *layer
) {
    if (!layer) return COMMAND_LAYER_ERROR_NULL_POINTER;
    
    if (!layer->completion_menu_active) {
        return COMMAND_LAYER_SUCCESS;  // Nothing to clear
    }
    
    // Free menu text
    if (layer->menu_text) {
        free(layer->menu_text);
        layer->menu_text = NULL;
        layer->menu_text_length = 0;
    }
    
    // Clear menu reference
    layer->menu_state = NULL;
    layer->completion_menu_active = false;
    
    // Rebuild highlighted_text without menu
    // Just restore original highlighted command
    rebuild_syntax_highlighting(layer);
    
    // Fire event to update display
    fire_command_text_changed_event(layer);
    
    return COMMAND_LAYER_SUCCESS;
}
```

**Diff system handles the rest**:
- Old screen: 10 rows (command + menu)
- New screen: 1 row (command only)
- Diff generates: CLEAR_TO_EOS from row 1
- Terminal: Menu rows erased

---

### Part 6: Scrolling Support

**Menu scrolling is handled in Phase 4** (already complete):
- `first_visible` tracks scroll position
- `visible_count` limits displayed items
- Navigation functions update `first_visible` as needed

**Rendering respects scroll state**:
```c
// In lle_completion_menu_render()
size_t start_idx = state->first_visible;
size_t end_idx = min(start_idx + state->visible_count, state->result->count);

for (size_t i = start_idx; i < end_idx; i++) {
    // Render items[i]
}
```

**Example with 100 completions, 10 visible**:
- Initial: items 0-9 visible
- User presses Down 10 times: selected = 9, first_visible = 0
- User presses Down again: selected = 10, first_visible = 1
  - Now showing items 1-10
  - Screen diff: row changed, not full re-render
- User presses Page Down: selected = 20, first_visible = 11
  - Now showing items 11-20
  - Screen diff: many rows changed

**Cycling**:
```c
// In lle_completion_menu_move_down() (already implemented)
if (state->selected_index >= total_items) {
    state->selected_index = 0;  // Wrap to beginning
    state->first_visible = 0;
}
```

---

## File Structure

### New Files to Create

```
src/lle/completion/completion_menu_renderer.c    (~400 lines)
src/lle/completion/completion_menu_renderer.h    (~100 lines)
```

### Files to Modify

```
src/display/command_layer.c                      (~200 lines added)
include/display/command_layer.h                  (~50 lines added)
src/display/display_controller.c                 (~20 lines modified)
```

### Test Files to Create

```
tests/lle/unit/test_completion_menu_renderer.c   (~300 lines)
```

---

## Implementation Phases

### Phase 5.1: Menu Renderer (Week 1)

**Tasks**:
1. Create `completion_menu_renderer.c/h`
2. Implement `lle_completion_menu_render()`
   - Basic text formatting
   - Selection highlighting (ANSI reverse video)
   - Category headers
   - Column layout
3. Write unit tests
4. Compile and test renderer standalone

**Success Criteria**:
- Renderer produces correct text output
- Selection highlighting works
- Handles edge cases (empty menu, single item, etc.)
- Unit tests pass

### Phase 5.2: Command Layer Integration (Week 2)

**Tasks**:
1. Extend `command_layer_t` structure
2. Implement menu setter/updater/clearer functions
3. Implement `rebuild_highlighted_text_with_menu()`
4. Update event firing
5. Write integration tests

**Success Criteria**:
- Menu appears in `highlighted_text`
- Command portion unchanged
- Events fire correctly
- Menu clearing works

### Phase 5.3: Display Controller Adjustments (Week 2)

**Tasks**:
1. Adjust newline counting for continuation prompts
2. Test cursor positioning with menu active
3. Verify diff system handles menu rows correctly
4. Test clearing mechanism

**Success Criteria**:
- Menu renders below command
- Cursor stays in command
- Menu clears cleanly
- No display corruption

### Phase 5.4: LLE Integration (Week 3)

**Tasks**:
1. Wire up TAB key to trigger completion + menu
2. Wire up arrow keys to navigate menu
3. Wire up Enter to accept selection
4. Wire up Escape to cancel
5. Implement word replacement on accept
6. Add event handlers

**Success Criteria**:
- TAB shows menu
- Arrows navigate
- Enter accepts and fills command
- Escape dismisses
- Character input dismisses menu

### Phase 5.5: Testing and Refinement (Week 3-4)

**Tasks**:
1. Test with various completion scenarios
2. Test scrolling with large result sets
3. Test performance (100+ items)
4. Test edge cases
5. Fix bugs
6. Optimize rendering if needed

**Success Criteria**:
- Menu works like zsh example
- Performance acceptable (<50ms updates)
- No visual glitches
- Handles all edge cases

---

## Potential Issues and Mitigations

### Issue 1: Menu in highlighted_text confuses other systems

**Symptom**: Continuation prompt analyzer sees menu text as command

**Mitigation**: Use `command_only_length` field to limit analysis range

**Code**:
```c
// Only analyze command portion
int newlines = count_newlines_in_range(text, 0, cmd_layer->command_only_length);
```

### Issue 2: Cursor position becomes invalid

**Symptom**: Cursor jumps into menu area

**Mitigation**: `cursor_position` always refers to command-only text, even when menu appended

**Code**:
```c
// cursor_position is byte offset in command text only
// screen_buffer_render uses this offset correctly
// Menu rows are AFTER cursor calculation
```

### Issue 3: Large menus cause performance issues

**Symptom**: Slow rendering with 100+ items

**Mitigation**: 
- Limit `visible_count` to screen height minus command rows
- Only render visible items
- Use column layout to fit more per row

**Code**:
```c
size_t max_menu_rows = terminal_height - command_rows - 2;  // -2 for margins
size_t visible_count = calculate_visible_count(max_menu_rows, col_width);
```

### Issue 4: Diff system inefficient with menu changes

**Symptom**: Full re-render on selection change

**Mitigation**: 
- ANSI reverse video changes only one line
- Diff should detect minimal change
- If not, may need separate menu buffer (Approach B)

**Monitor**: Measure diff performance, iterate if needed

### Issue 5: Menu doesn't clear cleanly

**Symptom**: Menu rows remain after dismissal

**Mitigation**:
- Ensure `highlighted_text` rebuilt without menu
- Verify diff generates CLEAR_TO_EOS
- Test clearing in all scenarios

**Code**:
```c
// When clearing, highlighted_text must NOT contain menu
assert(!strstr(layer->highlighted_text, "completing"));
```

---

## Testing Strategy

### Unit Tests

1. **Menu Renderer**:
   - Empty menu
   - Single item
   - Multiple items, single column
   - Multiple items, multi-column
   - Selection highlighting
   - Category headers
   - Scrolling (first_visible offset)
   - Max rows limit

2. **Command Layer**:
   - Set menu (menu appears in highlighted_text)
   - Update menu (selection change updates text)
   - Clear menu (menu removed from highlighted_text)
   - Command update with menu active
   - cursor_position preservation

### Integration Tests

1. **Display System**:
   - Menu appears below command
   - Cursor stays in command
   - Menu clears cleanly
   - Continuation prompts work with menu
   - Multiline command with menu

2. **End-to-End**:
   - TAB triggers menu
   - Arrow navigation
   - Selection acceptance
   - Menu dismissal
   - Scrolling large menus
   - Cycling wrap-around

### Performance Tests

1. **Rendering Speed**:
   - 10 items: <10ms
   - 100 items: <50ms
   - 1000 items (scrolled): <50ms

2. **Navigation Speed**:
   - Arrow key: <10ms update
   - Page up/down: <20ms update
   - Scroll to end: <30ms update

### Visual Tests

1. **Manual Testing Scenarios**:
   - `ls<TAB>` - shows file completions
   - `cd<TAB>` - shows directory completions
   - `echo $<TAB>` - shows variable completions
   - Long command with menu
   - Multiline command with menu
   - Wrapped command with menu

---

## Success Criteria for Phase 5

### Functional Requirements

- [  ] TAB key triggers completion menu
- [  ] Menu appears below command line
- [  ] Arrow keys navigate menu
- [  ] Enter accepts selected completion
- [  ] Escape dismisses menu
- [  ] Character input dismisses menu
- [  ] Selection fills command buffer correctly
- [  ] Menu scrolls/cycles through all items
- [  ] Category headers display correctly
- [  ] Type indicators work (if enabled)

### Architectural Requirements

- [  ] LLE remains source of truth
- [  ] All updates through screen_buffer diff system
- [  ] No direct terminal I/O from completion code
- [  ] Cursor stays in command area
- [  ] Menu state owned by LLE
- [  ] Display queries, never modifies state
- [  ] Event-driven updates
- [  ] Clean separation of concerns

### Performance Requirements

- [  ] Menu display <50ms
- [  ] Navigation update <10ms
- [  ] Scroll update <20ms
- [  ] Works with 100+ completions
- [  ] No visible lag or flicker

### Quality Requirements

- [  ] No display corruption
- [  ] No cursor jumping
- [  ] Clean menu clearing
- [  ] Handles edge cases gracefully
- [  ] Works with multiline commands
- [  ] Works with continuation prompts
- [  ] Memory leaks check passes
- [  ] All tests pass

---

## Iteration Plan

### If Approach A Works

**Great!** Ship it and move on.

### If Approach A Has Issues

**Iterate to Approach B**: Separate menu buffer

**Changes**:
- Don't append menu to command text
- Add menu as separate screen_buffer region
- Render menu independently
- Compose during diff/apply

**Benefits**:
- Cleaner separation
- Easier to optimize
- More flexible layout

**Costs**:
- More complex implementation
- More API changes
- More testing needed

### If Approach B Has Issues

**Iterate to Approach C**: Terminal-level menu

**Changes**:
- Use alternate screen buffer
- Render menu with direct terminal codes
- Switch between main/menu screens

**Benefits**:
- Maximum control
- No screen_buffer complexity

**Costs**:
- Breaks diff system paradigm
- More terminal-specific code
- Harder to maintain

---

## Next Immediate Steps

1. **Create branch**: `feature/lle-completion-phase5`
2. **Create renderer files**: Start with basic structure
3. **Write first test**: Empty menu rendering
4. **Implement basic renderer**: Get something working
5. **Test manually**: Verify output looks right
6. **Iterate**: Add features incrementally

---

## Open Questions

1. **Column layout algorithm**: Fixed width or dynamic?
2. **Category header styling**: ANSI colors or plain text?
3. **Selection marker**: Reverse video, arrow, or both?
4. **Max menu height**: Fixed (20) or dynamic (screen height - 5)?
5. **Scroll indicators**: Show "more above/below"?

These will be answered during implementation.

---

## Conclusion

Phase 5 starts with the simplest approach that could work: append menu text to command text, let existing infrastructure handle it. This follows the "architecturally correct + just works" principle while acknowledging we'll learn and iterate.

The plan is detailed enough to start implementation, flexible enough to adapt, and based on proven patterns from continuation prompts.

**Ready to begin implementation on user approval.**
