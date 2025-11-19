# Completion System Phase 5.4 - Event Wiring Implementation Guide
**Date**: 2025-11-18  
**Session**: 21 → 22 Handoff  
**Status**: Ready to implement  
**Previous Work**: Phases 5.1-5.2 complete (renderer + command layer integration)

---

## Executive Summary

**What's Done:**
- ✅ Phase 5.1: Menu renderer (formats menu state to text)
- ✅ Phase 5.2: Command layer integration (appends menu to command output)
- ✅ Phases 1-4: Completion logic, sources, generator, menu state/navigation

**What's Next:**
- Phase 5.4: Wire up keyboard events (TAB, Arrow keys, Enter, Escape)
- Connect user input → completion generation → menu display → selection

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
