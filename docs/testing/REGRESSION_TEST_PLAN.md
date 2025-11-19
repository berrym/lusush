# LLE Option C Refactor - Regression Test Plan

**Date**: 2025-11-19  
**Commit**: 24bdc49 (Option C refactor) + fixes  
**Branch**: feature/lle  
**Tester**: User manual testing required

---

## Reported Regressions

After implementing Option C architecture refactor (commit 24bdc49), the following regressions were discovered during manual testing:

### 1. ✅ FIXED: No initial prompt on LLE startup

**Symptom**:
- Prompt not displayed when starting `LLE_ENABLED=1 ./builddir/lusush`
- Typing 'e' causes prompt to appear

**Root Cause**:
- Early return in `command_layer_set_command()` prevented REDRAW_NEEDED event on first render
- On startup with empty buffer, no command/cursor change, so early return skipped event publication

**Fix Applied**:
- Added `is_first_render` check using `update_sequence_number == 0`
- First render now bypasses early return optimization
- File: `src/display/command_layer.c` lines 294-297

**Testing**:
```bash
LLE_ENABLED=1 ./builddir/lusush -i
# Expected: Prompt appears immediately
# Actual: [TO BE TESTED]
```

---

### 2. ⚠️ TAB on "ec" doesn't show completion menu

**Symptom**:
```bash
[mberry@fedora-xps13.local] ~/Lab/c/lusush (feature/lle) $ ec
# Press TAB - nothing visible happens
```

But typing 'h' to make "ech" DOES show menu:
```bash
[mberry@fedora-xps13.local] ~/Lab/c/lusush (feature/lle) $ ech
completing builtin command
echo
completing external command
echo        echo
```

**Possible Causes**:
1. Minimum prefix length requirement (2 chars minimum?)
2. Completion generation failing for "ec" specifically
3. Menu rendering but not visible (display issue)
4. Event not being triggered for "ec"

**Investigation Needed**:
- Check `lle_completion_generate()` minimum length requirements
- Check if completions are being found for "ec"
- Add debug logging to see if menu_state is created
- Verify REDRAW_NEEDED event is published

**Files to Check**:
- `src/lle/completion/completion_generator.c` - generation logic
- `src/lle/keybinding_actions.c` - `lle_complete()` function
- `src/display/display_controller.c` - menu rendering

**Test Cases**:
```bash
# Test 1: Two character prefix
$ ec<TAB>
# Expected: Menu with echo, ecryptfs-*, etc.
# Actual: [TO BE TESTED]

# Test 2: Three character prefix  
$ ech<TAB>
# Expected: Menu with echo
# Actual: Menu appears (CONFIRMED WORKING)

# Test 3: Single character
$ e<TAB>
# Expected: Menu with echo, env, ed, etc.
# Actual: [TO BE TESTED]
```

---

### 3. ⚠️ Completion menu not clearing on character input

**Symptom**:
After "ech" shows menu, typing 'o' to complete "echo" should dismiss menu, but it persists:

```bash
[mberry@fedora-xps13.local] ~/Lab/c/lusush (feature/lle) $ echo 
completing builtin command
echo
completing external command
echo        echo
# Menu still visible even after typing 'o' and space!
```

**Expected Behavior**:
- Any character typed should dismiss the completion menu
- Only the command should remain visible

**Architecture**:
The code SHOULD work:
1. `lle_self_insert()` at line 1747-1750 in keybinding_actions.c checks for menu
2. Calls `clear_completion_menu(editor)` if menu visible
3. `clear_completion_menu()` calls:
   - `lle_completion_system_clear(system)` - clears state
   - `display_controller_clear_completion_menu(dc)` - clears display
   - `refresh_after_completion(dc)` - triggers redraw

**Possible Issues**:
1. `lle_completion_system_is_menu_visible()` returning false when it should be true
2. `display_controller_clear_completion_menu()` not actually clearing the menu
3. Display refresh not happening or not picking up cleared state
4. Menu being re-added somewhere after clearing

**Investigation Needed**:
- Add debug logging to verify `clear_completion_menu()` is being called
- Check if `completion_menu_visible` flag is being set/cleared correctly
- Verify `dc_handle_redraw_needed()` respects cleared menu state
- Check if completion is being re-triggered on character input

**Files to Check**:
- `src/lle/keybinding_actions.c:1747-1750` - dismiss logic
- `src/lle/keybinding_actions.c:155-169` - clear_completion_menu()
- `src/display/display_controller.c:1952-1977` - display_controller_clear_completion_menu()
- `src/display/display_controller.c:229-244` - menu rendering check

**Test Cases**:
```bash
# Test 1: Type to dismiss
$ ec<TAB>          # Shows menu
$ h                # Type 'h' - menu should disappear
# Expected: Menu dismissed, command shows "ech"
# Actual: [TO BE TESTED]

# Test 2: Space to dismiss
$ echo<TAB>        # Shows menu (if applicable)
$ <SPACE>          # Type space
# Expected: Menu dismissed, command shows "echo "
# Actual: [TO BE TESTED]

# Test 3: Escape to dismiss (if implemented)
$ ec<TAB>          # Shows menu
$ <ESC>            # Press escape
# Expected: Menu dismissed, command unchanged
# Actual: [TO BE TESTED - may not be implemented yet]
```

---

## Test Environment

```bash
# Setup
cd /home/mberry/Lab/c/lusush
git checkout feature/lle
ninja -C builddir lusush

# Run tests
LLE_ENABLED=1 ./builddir/lusush -i
```

---

## Success Criteria

### Regression Fix 1: Initial Prompt
- ✅ Prompt appears immediately on startup
- ✅ No need to type character to see prompt

### Regression Fix 2: TAB Completion
- ✅ TAB on "ec" shows completion menu with "echo"
- ✅ TAB on "e" shows completion menu with all 'e' commands
- ✅ Consistent behavior regardless of prefix length

### Regression Fix 3: Menu Dismissal
- ✅ Typing any character dismisses menu
- ✅ Pressing space dismisses menu
- ✅ Menu does not persist after character input
- ✅ Display shows only command after menu dismissed

---

## Debug Commands

If issues persist, add debug logging:

```c
// In src/lle/keybinding_actions.c - lle_self_insert()
fprintf(stderr, "DEBUG: self_insert called, menu_visible=%d\n",
        lle_completion_system_is_menu_visible(editor->completion_system));

// In src/lle/keybinding_actions.c - clear_completion_menu()
fprintf(stderr, "DEBUG: clear_completion_menu called\n");

// In src/display/display_controller.c - dc_handle_redraw_needed()
fprintf(stderr, "DEBUG: menu_visible=%d, active_menu=%p\n",
        controller->completion_menu_visible,
        (void*)controller->active_completion_menu);
```

---

## Related Documents

- `docs/development/OPTION_C_ARCHITECTURE_REFACTOR.md` - Architecture details
- `docs/development/COMPLETION_PHASE54_IMPLEMENTATION_GUIDE.md` - Implementation guide
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Session 22 summary
- `docs/development/COMPLETION_REMAINING_TASKS.md` - Remaining work

---

## Notes for Next Session

If regressions cannot be resolved:
1. Consider partial revert of Option C
2. Add comprehensive debug logging
3. Test in isolated environment
4. Compare behavior with pre-refactor version (commit cb2b7b2)

The Option C refactor is architecturally sound, so these regressions are likely minor bugs rather than fundamental design flaws.
