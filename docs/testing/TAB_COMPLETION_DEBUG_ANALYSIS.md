# TAB Completion Debug Analysis

**Date**: 2025-11-19  
**Issue**: TAB on "ec" doesn't show completion menu  
**Symptom**: Ctrl+G shows menu, but TAB doesn't  

---

## Expected Execution Flow

When TAB is pressed on "ec":

### 1. Keybinding Execution (lle_readline.c)
```
Line 1141: execute_keybinding_action() called with "TAB"
Line 1146: action->type == LLE_ACTION_TYPE_SIMPLE
Line 1147: exec_result = action->func.simple(editor)
           → calls lle_complete(editor)
```

### 2. Completion Generation (keybinding_actions.c:1321)
```c
lle_complete(editor):
  - Generate completions for "ec"
  - Expected: 3 completions (echo, ecryptfs-*, etc.)
  - Create menu state
  - Set menu on display_controller
  - Return LLE_SUCCESS
```

**Debug Output Expected**:
```
[DEBUG lle_complete] buffer='ec', cursor=2, gen_result=0, result=0x...
[DEBUG lle_complete] result->count=3
[DEBUG lle_complete] Multiple completions (3) - showing menu
[DEBUG lle_complete] Got display_controller: 0x...
[DEBUG lle_complete] Got menu from completion_system: 0x...
[DEBUG lle_complete] Setting menu on display_controller
[DEBUG lle_complete] Menu set - NOT calling refresh (let execute_keybinding_action handle it)
```

### 3. Post-Action Refresh (lle_readline.c:1161)
```c
if (exec_result == LLE_SUCCESS) {
    refresh_display(ctx);  // ← THIS SHOULD BE CALLED
}
```

**Debug Output Expected**:
```
[DEBUG refresh_display] Called
[DEBUG refresh_display] Got display_integration: 0x...
[DEBUG refresh_display] About to call lle_render_buffer_content
[DEBUG refresh_display] Got display_bridge: 0x...
[DEBUG refresh_display] Calling lle_display_bridge_send_output
```

### 4. Display Bridge (lle_display_bridge.c)
```
lle_display_bridge_send_output():
  - Convert render_output to command text
  - Call command_layer_set_command()
```

### 5. Command Layer Event (command_layer.c)
```c
command_layer_set_command():
  - Detect command changed (or cursor changed)
  - Publish LAYER_EVENT_REDRAW_NEEDED event
```

### 6. Display Controller Handler (display_controller.c)
```c
dc_handle_redraw_needed():
  - Check if completion_menu_visible
  - Render menu to terminal
```

**Debug Output Expected**:
```
[DEBUG dc_handle_redraw] Called from event handler
[DEBUG dc_handle_redraw] Writing menu (NNN chars)
[DEBUG dc_handle_redraw] Menu written
```

---

## Actual Behavior (Observed)

### What Works:
✅ Completions ARE generated (3 items for "ec")  
✅ Menu IS set on display_controller  
✅ `lle_complete()` returns `LLE_SUCCESS`  

### What Doesn't Work:
❌ Menu does NOT appear on screen after TAB  
❌ NO `[DEBUG dc_handle_redraw]` output after TAB  
❌ Menu DOES appear after Ctrl+G  

### Key Observation:
Ctrl+G triggers menu display, but Ctrl+G exits readline (`*ctx->done = true`) and returns to shell, which starts a fresh prompt. This suggests the menu appears during the NEXT prompt cycle, not during the current TAB handling.

---

## Hypotheses

### Hypothesis 1: refresh_display() Not Being Called
**Test**: Look for `[DEBUG refresh_display] Called` in output after TAB

**Possible causes**:
- `exec_result != LLE_SUCCESS` (unlikely - we see `lle_complete` returns SUCCESS)
- Code path not reaching line 1161
- Different action type registration

**How to verify**: Debug output should show or not show the message

---

### Hypothesis 2: refresh_display() Returning Early
**Test**: Look for early return debug messages

**Possible causes**:
```c
// Line 204-207
if (!ctx || !ctx->buffer) {
    fprintf(stderr, "[DEBUG refresh_display] Early return: ctx or buffer NULL\n");
    return;
}

// Line 211-215
if (!display_integration) {
    fprintf(stderr, "[DEBUG refresh_display] Early return: no display_integration\n");
    return;
}
```

**How to verify**: Debug output would show early return message

---

### Hypothesis 3: Event Chain Broken
**Test**: Look for full debug sequence up to display_bridge call

**Possible causes**:
- `lle_display_bridge_send_output()` not calling `command_layer_set_command()`
- `command_layer_set_command()` not publishing `REDRAW_NEEDED` event
- Event handler not subscribed or not triggering
- Early return in `command_layer_set_command()` preventing event

**How to verify**: 
- If we see `[DEBUG refresh_display] Calling lle_display_bridge_send_output` but NOT `[DEBUG dc_handle_redraw]`, the event chain is broken
- Need to add debug to `command_layer_set_command()` to see if it's publishing event

---

### Hypothesis 4: Command Layer Early Return (MOST LIKELY)
**Test**: Check if command_layer_set_command() is hitting early return

**Root cause theory**:
```c
// command_layer.c lines 294-300
bool command_changed = (strcmp(layer->command_text, command_text) != 0);
bool cursor_changed = (layer->cursor_position != cursor_pos);
bool is_first_render = (layer->update_sequence_number == 0);

if (!command_changed && !cursor_changed && !is_first_render) {
    // Early return - NO EVENT PUBLISHED
    return COMMAND_LAYER_SUCCESS;
}
```

When TAB is pressed on "ec":
- Command text is still "ec" (no change)
- Cursor is still at position 2 (no change)
- NOT first render
- **Result**: Early return, no REDRAW_NEEDED event published!

But when Ctrl+G is pressed:
- Exits readline completely
- Returns to shell
- Shell starts NEW prompt cycle
- NEW prompt = first_render = bypasses early return
- Menu appears!

**This is the smoking gun!**

The menu is SET on display_controller, but the display_controller never gets the REDRAW_NEEDED event because command_layer thinks nothing changed.

---

## Solution

The completion menu state change should trigger a redraw EVEN IF the command text and cursor haven't changed.

### Option A: Detect Menu Change in command_layer
Track whether completion menu state changed and avoid early return if menu visible/invisible state changed.

**Problem**: Violates separation of concerns - command_layer shouldn't know about completion menus

### Option B: Force REDRAW_NEEDED Event from display_controller
When `display_controller_set_completion_menu()` is called, directly publish REDRAW_NEEDED event instead of relying on command_layer.

**Problem**: We already tried this - event IS published but not processed?

### Option C: Temporarily Modify Command Text (HACK)
Add invisible change to command text to force command_layer to see a change.

**Problem**: User explicitly rejected "hacks"

### Option D: Separate Menu Rendering Path
Menu should render independently of command_layer refresh.

**Problem**: Breaks architecture - we need unified display controller

### Option E: Explicit Menu Render Call
After setting menu, explicitly call `dc_handle_redraw_needed()` directly instead of going through events.

**Problem**: Bypasses event system, but might be acceptable for menu-only updates?

### Option F: Fix Event Subscription/Processing
Verify the REDRAW_NEEDED event from display_controller_set_completion_menu() is actually being processed by the event handler.

**Current implementation** (display_controller.c:1941):
```c
layer_event_t event = {
    .type = LAYER_EVENT_REDRAW_NEEDED,
    .source_layer = LAYER_ID_DISPLAY_CONTROLLER,
    .timestamp = 0
};
layer_event_publish(&event);
```

Need to verify this event is being received by `dc_handle_redraw_needed()`.

---

## Next Steps

1. **Run Test**: Execute with debug build and capture full output:
   ```bash
   LLE_ENABLED=1 ./builddir/lusush -i 2>&1 | tee tab_test_output.txt
   # Type: ec<TAB>
   ```

2. **Analyze Output**: Look for which hypothesis matches:
   - No `[DEBUG refresh_display] Called` → Hypothesis 1
   - Early return message → Hypothesis 2  
   - No `[DEBUG dc_handle_redraw]` after bridge call → Hypothesis 3 or 4

3. **Add More Debug** if needed:
   - Add debug to `command_layer_set_command()` to show early return reason
   - Add debug to event publishing/handling
   - Add debug to verify event handler subscription

4. **Implement Fix**: Based on analysis, likely need to:
   - Either: Make menu changes trigger redraw independently of command changes
   - Or: Fix event handling so REDRAW_NEEDED from set_completion_menu() works
   - Or: Add "menu_changed" flag to bypass command_layer early return

---

## Why Ctrl+G Works

Ctrl+G works because it triggers `abort_requested`:

```c
// lle_readline.c line 1153-1158
if (ctx->editor->abort_requested) {
    *ctx->done = true;
    *ctx->final_line = strdup("");
    return exec_result;
}
```

This exits the readline loop, returns to shell, which then:
1. Starts a NEW readline prompt
2. New prompt = first_render = true
3. Bypasses early return in command_layer
4. REDRAW_NEEDED event published
5. `dc_handle_redraw_needed()` called
6. Menu appears!

So Ctrl+G isn't "fixing" the issue - it's just triggering a complete display refresh by starting over.

---

## Test Results

**Test 1: TAB on 'ec'**
- Date: 2025-11-19
- Build: Debug build with refresh_display logging
- Command: `LLE_ENABLED=1 ./builddir/lusush -i 2>&1`
- Steps: Type 'ec', press TAB
- Expected: Menu appears
- Actual: [PENDING USER TEST]

Debug output analysis: [TO BE FILLED IN]

---

## Conclusion

Based on architectural analysis, the most likely issue is **Hypothesis 4**: command_layer early return prevents REDRAW_NEEDED event when only menu state changes but command text/cursor don't change.

The fix should involve ensuring menu state changes trigger display refresh independently of command text changes, while maintaining proper architectural separation of concerns.
