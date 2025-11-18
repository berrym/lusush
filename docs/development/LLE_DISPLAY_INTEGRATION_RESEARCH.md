# LLE Display Integration Research

**Date**: 2025-11-17  
**Purpose**: Document existing LLE↔Display integration before completion menu integration  
**Status**: Research Complete  

---

## Critical Understanding: The Flow Must Not Be Violated

### The Sacred Path: LLE → Command Layer → Composition Engine → Screen Buffer → Terminal

```
┌─────────────────────────────────────────────────────────────────────┐
│ LLE EDITING SYSTEM                                                  │
│                                                                     │
│  [Buffer Changes] → [Render System] → lle_render_output_t          │
│        ↓                                                            │
│  lle_display_submission()  ← THIS IS THE ONLY EXIT POINT            │
└────────────────────────────────┬────────────────────────────────────┘
                                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ DISPLAY BRIDGE (src/lle/display_bridge.c)                          │
│                                                                     │
│  lle_display_bridge_submit_render()                                │
│    1. Extract command_text from render_output                      │
│    2. Calculate cursor screen position (NO division/modulo!)       │
│    3. Update command_layer with text + cursor                      │
│    4. command_layer publishes REDRAW_NEEDED event                  │
└────────────────────────────────┬────────────────────────────────────┘
                                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ LUSUSH DISPLAY CONTROLLER (src/display/)                           │
│                                                                     │
│  Composition Engine:                                                │
│    - Receives REDRAW_NEEDED event                                  │
│    - Composes all layers (prompt + command + suggestions + etc)    │
│    - Renders to screen_buffer                                      │
│                                                                     │
│  Screen Buffer:                                                     │
│    - Double buffering                                              │
│    - Dirty region tracking                                         │
│    - Minimal diff calculation                                      │
│    - Only updates changed cells                                    │
└────────────────────────────────┬────────────────────────────────────┘
                                 ↓
                          TERMINAL OUTPUT
```

---

## What We CANNOT Do

### ❌ FORBIDDEN: Direct Terminal I/O from LLE

**WRONG**:
```c
// In LLE completion menu code
printf("\033[2J");  // Clear screen
write(STDOUT_FILENO, menu_text, len);
```

**WHY FORBIDDEN**: 
- Bypasses screen_buffer dirty tracking
- Causes display artifacts and flickering
- Violates terminal state abstraction
- Breaks the composition engine
- No way to restore previous display state

---

### ❌ FORBIDDEN: Bypassing Display Bridge

**WRONG**:
```c
// In LLE completion code
command_layer_update(layer, text);  // Direct layer access
```

**WHY FORBIDDEN**:
- Display bridge tracks state synchronization
- Bridge manages render request queuing
- Bridge handles error recovery
- Direct access violates encapsulation

---

### ❌ FORBIDDEN: Division/Modulo for Cursor Calculation

**WRONG**:
```c
cursor_row = cursor_offset / terminal_width;
cursor_col = cursor_offset % terminal_width;
```

**WHY FORBIDDEN**:
- Doesn't account for grapheme clusters (multi-codepoint emojis)
- Doesn't account for zero-width combining characters
- Doesn't account for wide characters (CJK)
- Doesn't account for prompt width
- Research-validated failure mode (all line editors that used this approach had bugs)

**CORRECT**: Incremental tracking during render (like Replxx, Fish, ZLE)

---

## What We CAN and MUST Do

### ✅ CORRECT: Use Display Bridge for All Updates

```c
// In LLE completion widget
lle_result_t lle_complete_widget(lle_editor_t *editor, void *user_data) {
    // 1. Generate completions
    completion_result_t *results = generate_completions(editor);
    
    // 2. Update buffer with selected completion
    lle_buffer_insert(editor->buffer, completion_text);
    
    // 3. Let normal render flow handle display
    // The buffer change triggers:
    //   - LLE render system creates lle_render_output_t
    //   - lle_display_submission() sends to bridge
    //   - Bridge updates command_layer
    //   - Display controller re-renders
    
    return LLE_SUCCESS;
}
```

**This works because the normal flow handles everything.**

---

### ✅ CORRECT: Extending Command Layer for Overlays

If we need a completion menu overlay, we can:

**Option A: Use Existing Autosuggestions Layer Pattern**

The autosuggestions layer already exists and shows overlaid text:

```
src/display/autosuggestions_layer.c  ← Study this!
```

**How it works**:
1. Layer renders grayed-out text after cursor
2. Composition engine composes it with command layer
3. Screen buffer handles the overlay rendering
4. No terminal I/O in layer code

**We can create**:
```
src/display/completion_menu_layer.c  ← New layer
```

Following the exact same pattern:
- Implement layer_vtable functions
- Register with composition engine
- Publish REDRAW_NEEDED when menu state changes
- Let composition engine + screen buffer do the rendering

---

**Option B: Extend Command Layer with Menu Support**

Add menu rendering capability to command_layer.c:

```c
// In command_layer.c
typedef struct {
    bool menu_active;
    completion_result_t *menu_items;
    size_t selected_index;
} command_layer_menu_state_t;

// When rendering command layer
static void render_with_menu(command_layer_t *layer, screen_buffer_t *screen) {
    // 1. Render command text normally
    render_command_text(layer, screen);
    
    // 2. If menu active, render menu below command
    if (layer->menu_state.menu_active) {
        render_completion_menu(layer->menu_state, screen);
    }
}
```

**This keeps everything within the layer architecture.**

---

### ✅ CORRECT: Menu State Management

**Where menu state lives**:
```c
// In src/lle/completion/completion_menu.c
typedef struct {
    completion_result_t *items;
    size_t selected_index;
    size_t first_visible;
    bool active;
} lle_completion_menu_state_t;

// Store in editor
struct lle_editor {
    // ... existing fields ...
    lle_completion_menu_state_t *completion_menu;
};
```

**When Tab is pressed**:
```c
lle_result_t lle_complete_widget(lle_editor_t *editor, void *user_data) {
    // 1. Generate completions
    completion_result_t *items = generate_completions(editor);
    
    // 2. Activate menu
    editor->completion_menu->items = items;
    editor->completion_menu->active = true;
    editor->completion_menu->selected_index = 0;
    
    // 3. Trigger display update through PROPER FLOW
    // Option A: Set flag that render system checks
    editor->needs_menu_render = true;
    
    // Option B: Publish event that command_layer listens to
    publish_event(editor->event_system, LLE_EVENT_COMPLETION_MENU_OPENED);
    
    // Either way, the DISPLAY LAYER renders the menu, not LLE
}
```

---

## Research-Validated Principles

From `LLE_TERMINAL_STATE_MANAGEMENT_RESEARCH.md`:

### 1. Internal State Authority
✅ LLE buffer and cursor state is authoritative  
✅ Display system renders LLE state  
❌ Never query terminal for state  

### 2. Terminal Abstraction Layer
✅ All terminal differences handled by adapter pattern  
✅ LLE code is terminal-agnostic  
❌ No termcap/terminfo calls in LLE  

### 3. Atomic Display Operations
✅ All display updates coordinated through composition engine  
✅ Screen buffer ensures atomicity  
❌ No partial updates to terminal  

---

## Completion Menu Integration Strategy

### Phase 1: Menu State in LLE
Create menu state structures in `src/lle/completion/`:
- `completion_menu_state.h` - Menu state structure
- `completion_menu_logic.c` - Menu navigation logic (arrow keys)
- `completion_widget.c` - Widget that activates menu

**No display code here. Pure logic.**

---

### Phase 2: Display Layer Integration

**Option A: New Layer (Clean Separation)**

Create `src/display/completion_menu_layer.c`:
```c
// Layer vtable implementation
static layer_vtable_t completion_menu_layer_vtable = {
    .init = completion_menu_layer_init,
    .cleanup = completion_menu_layer_cleanup,
    .render = completion_menu_layer_render,  // ← ONLY place that renders
    .handle_event = completion_menu_layer_handle_event,
    .z_index = 10  // Above command layer, below status line
};
```

**Benefits**:
- Clean separation of concerns
- Follows existing layer pattern
- Easy to enable/disable
- No changes to command_layer

**Option B: Extend Command Layer (Simpler)**

Add menu rendering to `src/display/command_layer.c`:
```c
// In command_layer_render()
static void command_layer_render(layer_t *layer, screen_buffer_t *screen) {
    // Existing command rendering
    render_command_text(layer, screen);
    
    // New menu rendering
    if (needs_completion_menu(layer)) {
        render_completion_menu(layer, screen);
    }
}
```

**Benefits**:
- No new layer registration needed
- Menu is always part of command display
- Simpler architecture

**I recommend Option B for pragmatism.**

---

### Phase 3: Event Flow

```
User presses Tab
    ↓
Keybinding system invokes "complete" widget
    ↓
lle_complete_widget(editor)
    ├─ Generate completions
    ├─ Populate editor->completion_menu state
    └─ Set editor->completion_menu->active = true
    ↓
Normal LLE render cycle
    ├─ lle_render_output_t includes menu_active flag
    └─ lle_display_submission() sends to bridge
    ↓
Display bridge → command_layer
    ├─ command_layer sees menu_active flag
    └─ Includes menu in render
    ↓
Composition engine composes layers
    ↓
Screen buffer renders minimal diff
    ↓
Terminal shows menu
```

**User presses Arrow Down**:
```
Keybinding system invokes "menu-next" widget
    ↓
lle_menu_next_widget(editor)
    ├─ editor->completion_menu->selected_index++
    └─ Trigger re-render (normal flow)
    ↓
[Same render flow as above]
```

**User presses Enter**:
```
Keybinding system invokes "menu-accept" widget
    ↓
lle_menu_accept_widget(editor)
    ├─ Get selected completion
    ├─ Insert into buffer
    ├─ Set completion_menu->active = false
    └─ Trigger re-render (normal flow)
```

---

## Critical Implementation Rules

### Rule 1: LLE Code Never Touches Terminal
**All terminal I/O goes through display layers.**

### Rule 2: Display Layers Never Store LLE State
**LLE state lives in LLE. Layers render what LLE tells them.**

### Rule 3: Screen Buffer is Sacred
**Never bypass screen_buffer. It prevents flicker and artifacts.**

### Rule 4: Event-Driven Updates
**Changes trigger events, events trigger renders. No polling.**

### Rule 5: Incremental Cursor Calculation
**Track cursor position during rendering. Never use division/modulo.**

---

## Recommended Implementation Path

### Step 1: Menu Logic (Pure LLE)
- `src/lle/completion/completion_menu_state.h`
- `src/lle/completion/completion_menu_logic.c`
- Navigation functions (next, prev, accept, cancel)
- **No display code**

### Step 2: Menu Widgets (LLE)
- `src/lle/completion/completion_widgets.c`
- `lle_complete_widget` - Activate menu
- `lle_menu_next_widget` - Navigate down
- `lle_menu_prev_widget` - Navigate up  
- `lle_menu_accept_widget` - Accept selection
- `lle_menu_cancel_widget` - Cancel menu
- Register widgets with widget system

### Step 3: Render Output Extension (LLE)
- Extend `lle_render_output_t` to include menu state
- Modify render system to populate menu info

### Step 4: Display Layer Rendering (Lusush Display)
- Add menu rendering to `src/display/command_layer.c`
- `render_completion_menu()` function
- Use screen_buffer for all drawing
- Check `render_output->menu_active` flag

### Step 5: Integration Testing
- Test Tab → menu appears
- Test arrow keys → navigation works
- Test Enter → completion inserted
- Test Escape → menu dismissed
- Verify no flicker (screen_buffer working correctly)

---

## Success Criteria

✅ Menu renders through proper display layer flow  
✅ No direct terminal I/O from LLE code  
✅ Screen buffer dirty tracking intact  
✅ Cursor position calculated incrementally  
✅ Menu state lives in LLE  
✅ Display code lives in display layer  
✅ No flicker or artifacts  
✅ Works with all existing prompts and themes  

---

## Conclusion

**The architecture is sound. We just need to work within it.**

The display integration already provides everything we need:
- Event-driven updates
- Layer composition
- Screen buffer with dirty tracking
- Terminal abstraction

**We must NOT**:
- Add terminal I/O to LLE code
- Bypass the display bridge
- Break the render flow

**We MUST**:
- Use widget system for interaction
- Store menu state in LLE
- Render menu through display layers
- Let screen_buffer handle the actual output

**This is the path forward.**
