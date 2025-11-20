# TAB Completion Debug Test Instructions

**Build Status**: ✅ Enhanced debug build ready  
**Date**: 2025-11-19  
**Purpose**: Identify why TAB completion menu doesn't appear  

---

## Debug Logging Added

This build includes comprehensive debug logging to trace the complete execution flow:

### 1. Completion Generation (keybinding_actions.c)
```
[DEBUG lle_complete] buffer='...', cursor=N, gen_result=N, result=0x...
[DEBUG lle_complete] result->count=N
[DEBUG lle_complete] Multiple completions (N) - showing menu
[DEBUG lle_complete] Got display_controller: 0x...
[DEBUG lle_complete] Got menu from completion_system: 0x...
[DEBUG lle_complete] Setting menu on display_controller
[DEBUG lle_complete] Menu set - NOT calling refresh
```

### 2. Display Refresh (lle_readline.c)
```
[DEBUG refresh_display] Called
[DEBUG refresh_display] Got display_integration: 0x...
[DEBUG refresh_display] About to call lle_render_buffer_content
[DEBUG refresh_display] Got display_bridge: 0x...
[DEBUG refresh_display] Calling lle_display_bridge_send_output
```

### 3. **NEW: Command Layer (command_layer.c)**
```
[DEBUG command_layer_set_command] cmd_changed=0/1, cursor_changed=0/1, first_render=0/1
[DEBUG command_layer_set_command] EARLY RETURN - no changes detected
  OR
[DEBUG command_layer_set_command] Proceeding with update and event publish
[DEBUG command_layer_set_command] Publishing REDRAW_NEEDED event
```

### 4. Display Controller (display_controller.c)
```
[DEBUG dc_handle_redraw] Called from event handler
[DEBUG dc_handle_redraw] Writing menu (NNN chars)
[DEBUG dc_handle_redraw] Menu written
```

---

## Test Procedure

### Run Test:
```bash
cd /home/mberry/Lab/c/lusush
LLE_ENABLED=1 ./builddir/lusush -i 2>&1 | tee /tmp/tab_test_output.txt
```

### Steps:
1. Wait for prompt to appear
2. Type: `ec` (should appear in red)
3. Press: `TAB`
4. Observe: Does menu appear?
5. Press: `Ctrl+D` to exit
6. Review output in `/tmp/tab_test_output.txt`

---

## What to Look For

### Scenario A: refresh_display() NOT CALLED
**Symptom**: No `[DEBUG refresh_display] Called` after TAB

**Debug output will show**:
```
[DEBUG lle_complete] Menu set - NOT calling refresh
(no refresh_display debug)
```

**Meaning**: The keybinding action handler isn't calling refresh_display() after lle_complete() returns. This would indicate a bug in the action execution flow.

---

### Scenario B: command_layer EARLY RETURN (Most Likely)
**Symptom**: `refresh_display` called but `EARLY RETURN` message appears

**Debug output will show**:
```
[DEBUG lle_complete] Menu set - NOT calling refresh
[DEBUG refresh_display] Called
[DEBUG refresh_display] Calling lle_display_bridge_send_output
[DEBUG command_layer_set_command] cmd_changed=0, cursor_changed=0, first_render=0
[DEBUG command_layer_set_command] EARLY RETURN - no changes detected
(no dc_handle_redraw debug)
```

**Meaning**: 
- TAB completion sets menu on display_controller ✅
- refresh_display() is called ✅  
- display_bridge sends to command_layer ✅
- **BUT**: command_layer sees no change in command text or cursor
- Early return optimization prevents REDRAW_NEEDED event from being published ❌
- display_controller never gets notified to render the menu ❌

**This is the smoking gun!** The menu state changed, but command_layer doesn't know about it.

---

### Scenario C: Event Not Reaching Handler
**Symptom**: Event published but not handled

**Debug output will show**:
```
[DEBUG lle_complete] Menu set - NOT calling refresh
[DEBUG refresh_display] Called
[DEBUG refresh_display] Calling lle_display_bridge_send_output
[DEBUG command_layer_set_command] cmd_changed=1, cursor_changed=0, first_render=0
[DEBUG command_layer_set_command] Proceeding with update and event publish
[DEBUG command_layer_set_command] Publishing REDRAW_NEEDED event
(no dc_handle_redraw debug)
```

**Meaning**: Event is published but not received by handler. Event system issue.

---

### Scenario D: Everything Works (Unexpected!)
**Symptom**: Menu appears after TAB

**Debug output will show**:
```
[DEBUG lle_complete] Menu set - NOT calling refresh
[DEBUG refresh_display] Called
[DEBUG refresh_display] Calling lle_display_bridge_send_output
[DEBUG command_layer_set_command] Proceeding with update and event publish
[DEBUG command_layer_set_command] Publishing REDRAW_NEEDED event
[DEBUG dc_handle_redraw] Called from event handler
[DEBUG dc_handle_redraw] Writing menu (NNN chars)
```

**Meaning**: The issue may have been inadvertently fixed by the debug logging changes (timing-related bug).

---

## Expected Result

Based on architectural analysis in `TAB_COMPLETION_DEBUG_ANALYSIS.md`, I predict **Scenario B**: command_layer early return.

### Why This Happens:

When TAB is pressed on "ec":
1. ✅ Completions generated (3 items)
2. ✅ Menu set on display_controller  
3. ✅ lle_complete() returns LLE_SUCCESS
4. ✅ refresh_display(ctx) called by framework
5. ✅ lle_render_buffer_content() renders "ec" (unchanged)
6. ✅ lle_display_bridge_send_output() calls command_layer_set_command()
7. ❌ **command_layer_set_command() sees**:
   - Command text: "ec" → was "ec" → **no change**
   - Cursor position: 2 → was 2 → **no change**
   - Is first render: false → **not first render**
   - **Result**: Early return, no event published!
8. ❌ display_controller never gets REDRAW_NEEDED event
9. ❌ dc_handle_redraw_needed() never called
10. ❌ Menu never rendered to terminal

### Why Ctrl+G "Works":

Ctrl+G sets `abort_requested` which exits readline completely and returns to shell. The shell then starts a **new** prompt cycle:
- New prompt = `is_first_render = true`
- Bypasses early return optimization
- REDRAW_NEEDED event published
- Menu appears!

So Ctrl+G doesn't fix the issue - it just triggers a complete refresh.

---

## Next Steps Based on Results

### If Scenario B (Early Return):
**Root cause confirmed**: Menu state changes don't trigger redraw when command text is unchanged.

**Solution Options**:
1. **Option 1**: Add "menu changed" detection to command_layer
   - Pros: Proper architectural fix
   - Cons: Violates separation of concerns (command_layer shouldn't know about menus)

2. **Option 2**: Make display_controller handle menu rendering independently
   - Pros: Better separation of concerns
   - Cons: Need to refactor menu rendering path

3. **Option 3**: Force a "no-op" command change to bypass early return
   - Pros: Simple fix
   - Cons: Hack that user will reject

4. **Option 4**: Add separate "metadata changed" flag to command_layer
   - Pros: Generic solution for non-text changes
   - Cons: More complex

Will discuss with user and implement proper architectural fix.

### If Other Scenarios:
Will analyze debug output and adapt solution accordingly.

---

## File to Review After Test

```bash
cat /tmp/tab_test_output.txt
```

Look for the debug message sequence and match to scenarios above.

---

## Related Documents

- `TAB_COMPLETION_DEBUG_ANALYSIS.md` - Detailed architectural analysis
- `REGRESSION_TEST_PLAN.md` - Overall regression tracking
- `docs/development/OPTION_C_ARCHITECTURE_REFACTOR.md` - Architecture details
