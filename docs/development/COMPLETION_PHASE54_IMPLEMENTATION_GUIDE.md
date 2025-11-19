# Completion System Phase 5.4 - Event Wiring Implementation Guide
**Date**: 2025-11-18 (Updated 2025-11-19)
**Session**: 21 → 22 Handoff  
**Status**: PARTIAL - Menu Display Working, Navigation/Dismissal Pending  
**Previous Work**: Phases 5.1-5.3 complete (renderer + command layer + display controller integration)

---

## Executive Summary

**What's Done:**
- ✅ Phase 5.1: Menu renderer (formats menu state to text)
- ✅ Phase 5.2: Command layer integration (appends menu to command output)
- ✅ Phase 5.3: Display controller integration (menu separation, cursor positioning)
- ✅ Phase 5.4 (Partial): TAB key wiring, basic menu display working
- ✅ Phases 1-4: Completion logic, sources, generator, menu state/navigation

**What's Working (Session 21 Results):**
- ✅ TAB key triggers completion and displays menu
- ✅ Menu renders below command without continuation prompts
- ✅ Cursor positioned correctly accounting for menu lines
- ✅ Multi-line prompt support

**What's Still Needed:**
- ❌ Arrow key navigation updating command line
- ❌ Enter key accepting completion
- ❌ Escape key dismissing menu
- ❌ Typing dismissing menu
- ❌ Completion deduplication
- ❌ TAB cycling through completions

**Where to Implement:**
- File: `src/lle/keybinding_actions.c`
- Functions: `lle_complete()`, `lle_possible_completions()`, `lle_insert_completions()`
- Location: Lines 1185-1215 (currently stubs)

---

## Architecture Overview

### Current State

**Keybinding System:**
- TAB key is bound to `lle_complete()` function
- Arrow keys (UP/DOWN) are bound to `lle_smart_up_arrow()` / `lle_smart_down_arrow()`
- These are "simple" keybinding actions: `lle_result_t action(lle_editor_t *editor)`

**Context-Aware Alternative:**
- Some actions have context-aware versions: `lle_result_t action_context(readline_context_t *ctx)`
- Example: `lle_accept_line_context(readline_context_t *ctx)` at line 475
- Context-aware actions have access to full readline context including display

### Data Flow

```
User presses TAB
    ↓
lle_complete(editor) called by keybinding system
    ↓
Generate completions (Phases 1-3)
    lle_completion_generate(pool, buffer, cursor, &result)
    ↓
Create menu state (Phase 4)
    lle_completion_menu_state_create(pool, result, &config, &menu_state)
    ↓
Set menu on command layer (Phase 5.2)
    command_layer_set_completion_menu(cmd_layer, menu_state, terminal_width)
    ↓
Display updates automatically (event system)
    ↓
Menu appears below command
```

### Key Data Structures

**lle_editor_t** (include/lle/lle_editor.h):
```c
typedef struct lle_editor {
    lle_buffer_t *buffer;                         // Text buffer
    lle_cursor_manager_t *cursor_manager;         // Cursor position
    lle_display_controller_t *display_controller; // Display (has command_layer)
    lle_completion_system_t *completion_system;   // PLACEHOLDER - we implement this!
    lle_memory_pool_t *lle_pool;                  // Memory management
    // ... other fields
} lle_editor_t;
```

**readline_context_t** (src/lle/lle_readline.c line 140):
```c
typedef struct readline_context {
    lle_buffer_t *buffer;
    lle_editor_t *editor;                    // Full editor context
    lle_terminal_abstraction_t *term;
    const char *prompt;
    // ... other fields
} readline_context_t;
```

---

## Implementation Plan

### Step 1: Add Completion State to Editor

**Problem**: Where do we store the active completion menu state?

**Options:**
1. Add field to `lle_editor_t` structure
2. Use the existing `completion_system` field (currently just a forward declaration)
3. Store in a static variable in keybinding_actions.c

**Recommendation**: Option 2 - Define `lle_completion_system_t` structure

**File**: Create `include/lle/completion_system.h` or extend existing completion headers

**Structure**:
```c
typedef struct lle_completion_system {
    lle_completion_result_t *current_result;      // Current completions
    lle_completion_menu_state_t *current_menu;    // Active menu (NULL if no menu)
    lle_completion_menu_config_t menu_config;     // Menu configuration
    lle_memory_pool_t *memory_pool;               // Memory for allocations
    bool menu_active;                             // Quick check
} lle_completion_system_t;
```

### Step 2: Initialize Completion System

**File**: `src/lle/lle_editor.c` (or wherever `lle_editor_create()` is)

**In `lle_editor_create()`:**
```c
// Initialize completion system
editor->completion_system = lle_pool_alloc(sizeof(lle_completion_system_t));
if (editor->completion_system) {
    memset(editor->completion_system, 0, sizeof(lle_completion_system_t));
    editor->completion_system->memory_pool = editor->lle_pool;
    editor->completion_system->menu_config = lle_completion_menu_default_config();
    editor->completion_system->menu_active = false;
}
```

### Step 3: Implement lle_complete() - TAB Key Handler

**File**: `src/lle/keybinding_actions.c` line 1188

**Current Code:**
```c
lle_result_t lle_complete(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Completion system requires Spec 12 implementation */
    return LLE_SUCCESS;
}
```

**New Implementation:**
```c
lle_result_t lle_complete(lle_editor_t *editor) {
    if (!editor || !editor->buffer || !editor->completion_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_completion_system_t *comp = editor->completion_system;
    
    // If menu is already active, cycle through completions
    if (comp->menu_active && comp->current_menu) {
        lle_completion_menu_move_down(comp->current_menu);
        
        // Update display
        if (editor->display_controller) {
            command_layer_t *cmd_layer = get_command_layer_from_display(editor->display_controller);
            if (cmd_layer) {
                // Get terminal width
                size_t term_width = 80; // TODO: Get from terminal abstraction
                command_layer_update_completion_menu(cmd_layer, term_width);
            }
        }
        return LLE_SUCCESS;
    }
    
    // Generate completions
    size_t cursor_pos = lle_buffer_get_cursor(editor->buffer);
    const char *buffer_text = editor->buffer->data;
    
    lle_completion_result_t *result = NULL;
    lle_result_t res = lle_completion_generate(
        comp->memory_pool,
        buffer_text,
        cursor_pos,
        &result
    );
    
    if (res != LLE_SUCCESS || !result || result->count == 0) {
        // No completions available
        if (result) {
            lle_completion_result_free(result);
        }
        return LLE_SUCCESS;
    }
    
    // Store result
    if (comp->current_result) {
        lle_completion_result_free(comp->current_result);
    }
    comp->current_result = result;
    
    // Single completion - insert directly
    if (result->count == 1) {
        // TODO: Insert completion into buffer
        // lle_buffer_replace_word_at_cursor(editor->buffer, result->items[0].text);
        lle_completion_result_free(comp->current_result);
        comp->current_result = NULL;
        return LLE_SUCCESS;
    }
    
    // Multiple completions - show menu
    lle_completion_menu_state_t *menu_state = NULL;
    res = lle_completion_menu_state_create(
        comp->memory_pool,
        result,
        &comp->menu_config,
        &menu_state
    );
    
    if (res != LLE_SUCCESS || !menu_state) {
        return res;
    }
    
    // Store menu state
    if (comp->current_menu) {
        lle_completion_menu_state_free(comp->current_menu);
    }
    comp->current_menu = menu_state;
    comp->menu_active = true;
    
    // Display menu
    if (editor->display_controller) {
        command_layer_t *cmd_layer = get_command_layer_from_display(editor->display_controller);
        if (cmd_layer) {
            size_t term_width = 80; // TODO: Get from terminal abstraction
            command_layer_set_completion_menu(cmd_layer, menu_state, term_width);
        }
    }
    
    return LLE_SUCCESS;
}
```

### Step 4: Handle Arrow Keys for Menu Navigation

**Problem**: Arrow keys are currently bound to `lle_smart_up_arrow()` and `lle_smart_down_arrow()`

**Solution**: Extend these functions to check for active completion menu first

**File**: `src/lle/keybinding_actions.c` line 476

**Modified lle_smart_up_arrow():**
```c
lle_result_t lle_smart_up_arrow(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Check for active completion menu first
    if (editor->completion_system && editor->completion_system->menu_active) {
        lle_completion_menu_move_up(editor->completion_system->current_menu);
        
        // Update display
        if (editor->display_controller) {
            command_layer_t *cmd_layer = get_command_layer_from_display(editor->display_controller);
            if (cmd_layer) {
                size_t term_width = 80;
                command_layer_update_completion_menu(cmd_layer, term_width);
            }
        }
        return LLE_SUCCESS;
    }
    
    // Original logic: buffer navigation vs history
    bool is_multiline = (editor->buffer->length > 0 && 
                         memchr(editor->buffer->data, '\n', editor->buffer->length) != NULL);
    
    if (is_multiline) {
        return lle_previous_line(editor);
    } else {
        return lle_history_previous(editor);
    }
}
```

**Similar modification for lle_smart_down_arrow()**

### Step 5: Handle Enter Key - Accept Completion

**File**: src/lle/keybinding_actions.c line 1224 (lle_accept_line)

**Modify lle_accept_line():**
```c
lle_result_t lle_accept_line(lle_editor_t *editor) {
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Check for active completion menu
    if (editor->completion_system && editor->completion_system->menu_active) {
        const lle_completion_item_t *selected = NULL;
        lle_completion_menu_accept(editor->completion_system->current_menu, &selected);
        
        if (selected) {
            // TODO: Insert selected completion into buffer at cursor
            // lle_buffer_replace_word_at_cursor(editor->buffer, selected->text);
        }
        
        // Clear menu
        if (editor->display_controller) {
            command_layer_t *cmd_layer = get_command_layer_from_display(editor->display_controller);
            if (cmd_layer) {
                command_layer_clear_completion_menu(cmd_layer);
            }
        }
        
        // Cleanup completion state
        lle_completion_menu_state_free(editor->completion_system->current_menu);
        editor->completion_system->current_menu = NULL;
        editor->completion_system->menu_active = false;
        
        return LLE_SUCCESS;
    }
    
    // Original accept line logic
    editor->eof_requested = false;
    // Signal line is accepted
    return LLE_SUCCESS;
}
```

### Step 6: Handle Escape Key - Cancel Completion

**File**: src/lle/keybinding_actions.c

**Find or create lle_abort() function**, modify it:
```c
lle_result_t lle_abort(lle_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Clear completion menu if active
    if (editor->completion_system && editor->completion_system->menu_active) {
        if (editor->display_controller) {
            command_layer_t *cmd_layer = get_command_layer_from_display(editor->display_controller);
            if (cmd_layer) {
                command_layer_clear_completion_menu(cmd_layer);
            }
        }
        
        // Cleanup
        if (editor->completion_system->current_menu) {
            lle_completion_menu_state_free(editor->completion_system->current_menu);
            editor->completion_system->current_menu = NULL;
        }
        editor->completion_system->menu_active = false;
        
        return LLE_SUCCESS;
    }
    
    // Original abort logic
    editor->abort_requested = true;
    return LLE_SUCCESS;
}
```

### Step 7: Handle Character Input - Dismiss Menu

**Problem**: When user types a character, menu should be dismissed

**Solution**: Add check in character insertion functions

**File**: `src/lle/keybinding_actions.c` (find `lle_self_insert` or similar)

**Add at beginning of character insertion:**
```c
// Clear completion menu on any character input
if (editor->completion_system && editor->completion_system->menu_active) {
    // Clear menu from display
    if (editor->display_controller) {
        command_layer_t *cmd_layer = get_command_layer_from_display(editor->display_controller);
        if (cmd_layer) {
            command_layer_clear_completion_menu(cmd_layer);
        }
    }
    
    // Cleanup state
    if (editor->completion_system->current_menu) {
        lle_completion_menu_state_free(editor->completion_system->current_menu);
        editor->completion_system->current_menu = NULL;
    }
    editor->completion_system->menu_active = false;
}
```

---

## Missing Pieces / TODOs

### 1. Get command_layer from display_controller

**Need helper function:**
```c
command_layer_t* get_command_layer_from_display(lle_display_controller_t *display) {
    // TODO: Implement based on display_controller structure
    // May need to check display_controller.h for actual structure
    return NULL;
}
```

### 2. Get terminal width

**Currently hardcoded to 80, should get from terminal:**
```c
size_t term_width = 80;
// TODO: Get from editor->term or display_controller
```

### 3. Buffer word replacement

**Need function to replace word at cursor:**
```c
lle_result_t lle_buffer_replace_word_at_cursor(
    lle_buffer_t *buffer,
    const char *new_text
);
```

This should:
- Find start of current word (scan back to whitespace/separator)
- Find end of current word (scan forward to whitespace/separator)
- Delete the word
- Insert new_text at that position

### 4. Include headers

**Need to add to keybinding_actions.c:**
```c
#include "lle/completion/completion_types.h"
#include "lle/completion/completion_sources.h"
#include "lle/completion/completion_generator.h"
#include "lle/completion/completion_menu_state.h"
#include "lle/completion/completion_menu_logic.h"
#include "display/command_layer.h"
```

---

## Testing Strategy

### Manual Testing Steps

1. **Basic TAB completion:**
   - Type `l` and press TAB
   - Should see menu with `ls`, `less`, `ln`, etc.

2. **Arrow key navigation:**
   - With menu open, press DOWN arrow
   - Selection should move down
   - Press UP arrow
   - Selection should move up

3. **Accept completion:**
   - Navigate to desired item
   - Press ENTER
   - Item should be inserted into buffer
   - Menu should disappear

4. **Cancel completion:**
   - Open menu with TAB
   - Press ESC
   - Menu should disappear

5. **Type to dismiss:**
   - Open menu with TAB
   - Type any character
   - Menu should disappear
   - Character should be inserted

### Integration Testing

- Test with multiline commands
- Test with empty buffer
- Test with cursor in middle of word
- Test with no available completions
- Test with single completion (should auto-insert)

---

## Next Session Checklist

1. [ ] Read this document
2. [ ] Define `lle_completion_system_t` structure
3. [ ] Initialize completion system in editor creation
4. [ ] Implement `lle_complete()` TAB handler
5. [ ] Modify `lle_smart_up_arrow()` and `lle_smart_down_arrow()`
6. [ ] Modify `lle_accept_line()` for Enter handling
7. [ ] Modify `lle_abort()` for Escape handling
8. [ ] Add menu dismissal to character insertion
9. [ ] Implement helper: get_command_layer_from_display()
10. [ ] Implement helper: lle_buffer_replace_word_at_cursor()
11. [ ] Add necessary #includes
12. [ ] Compile and test
13. [ ] Debug and refine
14. [ ] Commit Phase 5.4

---

## Files to Modify

1. **include/lle/lle_editor.h** - Define `lle_completion_system_t` or include completion_system.h
2. **src/lle/lle_editor.c** - Initialize completion_system in editor_create()
3. **src/lle/keybinding_actions.c** - Implement all completion event handlers
4. **src/lle/buffer_management.c** - Add word replacement function (if not exists)

## Files to Reference

1. **include/lle/completion/*.h** - All completion APIs
2. **include/display/command_layer.h** - Command layer API
3. **src/display/command_layer.c** - Command layer implementation
4. **docs/development/LLE_COMPLETION_PHASE5_IMPLEMENTATION_PLAN.md** - Original plan

---

## Estimated Complexity

- **Lines of code**: ~300-400 new lines
- **Files modified**: 3-4 files
- **Time estimate**: 2-3 hours implementation + 1-2 hours testing
- **Risk level**: Medium (touching input handling, needs careful testing)

---

**Good luck with Phase 5.4!** 🚀

---

## ACTUAL IMPLEMENTATION (Session 21)

### What Was Actually Implemented

The implementation deviated from the planned approach in several critical ways. Here's what actually happened:

#### 1. No Completion System Structure Added to Editor

**Planned**: Add `lle_completion_system_t` to `lle_editor_t`

**Actual**: Implementation worked WITHOUT adding a completion system structure. State is managed differently:
- Menu state is passed directly to command layer
- No persistent completion state in editor
- Simpler but less stateful approach

#### 2. TAB Key Handler - Simplified Implementation

**Location**: `src/lle/lle_readline.c` lines 1477-1480, 804-826

**Actual Code**:
```c
// In input processing loop:
if (codepoint == '\t' || codepoint == 9) {
    execute_keybinding_action(&ctx, "TAB", handle_tab);
    break;
}

// TAB handler:
static lle_result_t handle_tab(lle_event_t *event, void *user_data)
{
    (void)event;
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (!ctx || !ctx->editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Call completion function to set up menu */
    lle_result_t result = lle_complete(ctx->editor);
    
    /* Refresh display to render buffer content with menu appended */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}
```

**Key Difference**: Uses existing `refresh_display()` instead of manual display controller calls

#### 3. lle_complete() Implementation

**Location**: `src/lle/keybinding_actions.c` lines 1345-1450

**Actual Implementation** (simplified from plan):
```c
lle_result_t lle_complete(lle_editor_t *editor)
{
    if (!editor || !editor->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Get completion context
    size_t cursor_pos = lle_cursor_manager_get_byte_offset(editor->cursor_manager);
    const char *buffer_text = editor->buffer->data;
    
    // Generate completions
    lle_completion_result_t *result = NULL;
    lle_result_t res = lle_completion_generate(
        editor->lle_pool,
        buffer_text,
        cursor_pos,
        &result
    );
    
    if (res != LLE_SUCCESS || !result || result->count == 0) {
        return LLE_SUCCESS;  // No completions
    }
    
    // Single completion - auto insert (NOT IMPLEMENTED YET)
    if (result->count == 1) {
        // TODO: Insert directly
        lle_completion_result_free(result);
        return LLE_SUCCESS;
    }
    
    // Multiple completions - create menu
    lle_completion_menu_config_t config = lle_completion_menu_default_config();
    lle_completion_menu_state_t *menu_state = NULL;
    
    res = lle_completion_menu_state_create(
        editor->lle_pool,
        result,
        &config,
        &menu_state
    );
    
    if (res != LLE_SUCCESS) {
        lle_completion_result_free(result);
        return res;
    }
    
    // Get command layer and set menu
    command_layer_t *cmd_layer = 
        lle_display_controller_get_command_layer(editor->display_controller);
    
    if (cmd_layer) {
        size_t term_width = 80;  // Hardcoded for now
        command_layer_set_completion_menu(cmd_layer, menu_state, term_width);
    }
    
    return LLE_SUCCESS;
}
```

**Key Differences**:
- No completion system state tracking
- Memory pool from editor, not separate system
- Hardcoded terminal width (80)
- Display refresh happens in `handle_tab()`, not here

#### 4. Critical Architecture Discoveries

##### The Double Re-Append Hack

**Problem**: Menu was being wiped during display refresh

**Root Cause**: BOTH `command_layer_set_command()` AND `command_layer_update()` call `perform_syntax_highlighting()`, which overwrites `highlighted_text`

**Solution** (`src/display/command_layer.c`):
```c
// In BOTH set_command() and update():
if (layer->completion_menu_active && layer->menu_state) {
    layer->highlighted_base_length = strlen(layer->highlighted_text);
    size_t term_width = 80;
    append_menu_to_highlighted_text(layer, term_width);
}
```

**Also added force_refresh flag** (lines 305-307):
```c
bool force_refresh = (layer->completion_menu_active && layer->menu_state);

if (!command_changed && !cursor_changed && !force_refresh) {
    return COMMAND_LAYER_SUCCESS;  // Early return skipped when menu active
}
```

##### Menu Separation Architecture

**Problem**: Continuation prompts being added to menu lines

**User's Critical Direction**: "screen_buffer_render() should handle the menu... do it right"

**Solution** (`src/display/display_controller.c` lines 219-227, 413-418):
```c
// Separate menu from command BEFORE screen_buffer_render()
char *menu_text = NULL;
if (cmd_layer->completion_menu_active && cmd_layer->highlighted_base_length > 0) {
    size_t base_len = cmd_layer->highlighted_base_length;
    if (base_len < strlen(command_buffer) && command_buffer[base_len] == '\n') {
        menu_text = &command_buffer[base_len + 1];
        command_buffer[base_len] = '\0';  // Terminate command
    }
}

// ... render command only with continuation prompts ...

// Write menu separately WITHOUT continuation prompts
if (menu_text && *menu_text) {
    write(STDOUT_FILENO, "\n", 1);
    write(STDOUT_FILENO, menu_text, strlen(menu_text));
}
```

##### Cursor Positioning with Menu

**Problem**: Cursor one line below correct position

**Solution** (lines 431-441):
```c
// Count menu lines
int menu_lines = 0;
if (menu_text && *menu_text) {
    menu_lines = 1;  // Newline before menu
    for (const char *p = menu_text; *p; p++) {
        if (*p == '\n') menu_lines++;
    }
}

// Account for menu when moving cursor up
int rows_to_move_up = (final_row - cursor_row) + menu_lines;
```

#### 5. What Was NOT Implemented

- ❌ Arrow key navigation updating command (navigation works but doesn't update buffer)
- ❌ Enter key acceptance
- ❌ Escape key dismissal
- ❌ Typing dismissal
- ❌ Word replacement function
- ❌ Completion system state structure
- ❌ TAB cycling (partially implemented in `lle_complete()` but untested)
- ❌ Single completion auto-insert

### Files Actually Modified

1. **src/lle/completion/completion_menu_state.c** (line 132)
   - Set `menu_active = true` on creation

2. **src/display/command_layer.c** (lines 127, 305-370, 470-476)
   - Added forward declaration for helper
   - Force refresh when menu active
   - Re-append menu in `set_command()` after highlighting
   - Re-append menu in `update()` after highlighting

3. **src/display/display_controller.c** (lines 219-227, 413-418, 431-441)
   - Separate menu from command before rendering
   - Write menu directly after command
   - Account for menu lines in cursor positioning

4. **src/lle/completion/completion_types.c** (lines 45, 52, 60, 68, 76, 84, 92, 100)
   - Set all indicators to empty strings (no emojis)

5. **src/lle/lle_readline.c** (lines 804-826, 1477-1480)
   - Added `handle_tab()` event handler
   - Added TAB detection in input loop

6. **src/lle/keybinding_actions.c** (lines 1345-1450)
   - Implemented `lle_complete()` function

### Critical Lessons Learned

1. **"Do it right, not intermediary hacks"** - User stopped wrong approaches immediately
2. **Screen buffer is the "real" display layer** - Despite architecture, it handles actual rendering
3. **Test in live terminal early** - Many issues invisible in theory
4. **Separation at render time** - Command gets continuation prompts, menu does not
5. **Manual cursor sync critical** - Every buffer change needs explicit sync

### Next Session Priorities

For the next session to complete Phase 5.4:

1. **Implement arrow key navigation** that updates command buffer
2. **Implement Enter key acceptance** with word replacement
3. **Implement Escape key dismissal**
4. **Implement typing dismissal** (clear menu on character input)
5. **Fix duplicate completions** (deduplication logic)
6. **Test TAB cycling** (currently in code but untested)
7. **Implement single completion auto-insert**

### Working Test Command

```bash
LLE_ENABLED=1 ./builddir/lusush
# Type: ec
# Press: TAB
# Expected: Menu shows with echo, ecryptfs-* commands
# Press: Arrow keys (navigation changes selection - confirmed working)
# Issue: Command line doesn't update with selection
```

### Reference Documents

- `docs/development/COMPLETION_REMAINING_TASKS.md` - Detailed task breakdown
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Session 21 summary
- `docs/development/LLE_COMPLETION_PHASE5_IMPLEMENTATION_PLAN.md` - Updated with actual results

---

**Session 21 Status**: Menu display working. Navigation, dismissal, and acceptance still needed.
