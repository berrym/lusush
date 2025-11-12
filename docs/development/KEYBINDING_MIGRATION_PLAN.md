# Keybinding Manager Migration Plan

**Date**: 2025-11-11  
**Branch**: `feature/lle`  
**Goal**: Replace hardcoded keybindings in `lle_readline.c` with keybinding manager  
**Status**: Ready to begin

---

## Overview

**Current State**: All key handling hardcoded in `lle_readline.c` (21 keybindings)  
**Target State**: All keys handled through keybinding manager (42 actions available)  
**Reason**: Correct architecture, extensibility, user customization, fixes validated

**Audit Results**: ✅ 100% coverage - All hardcoded keys have corresponding actions

---

## Migration Strategy

### Incremental Groups Approach

Migrate keybindings in 5 groups, testing thoroughly after each group:

1. **Group 1: Navigation** (4 keys) - Lowest risk
2. **Group 2: Deletion** (3 keys) - Medium risk  
3. **Group 3: Kill/Yank** (4 keys) - Higher risk
4. **Group 4: History & Special** (7 keys) - High risk
5. **Group 5: Accept Line** (1 key) - Critical

**Duration**: 1-2 days total (2-3 hours per group with testing)

---

## Group 1: Navigation Keys (LOWEST RISK)

### Keybindings to Migrate
- LEFT arrow → `lle_backward_char`
- RIGHT arrow → `lle_forward_char`
- HOME → `lle_beginning_of_line`
- END → `lle_end_of_line`

### Implementation Steps

**Step 1: Initialize Keybinding Manager** (30 min)
```c
// In lle_readline.c, lle_readline() function
lle_keybinding_manager_t *kb_manager = lle_keybinding_manager_create();
if (!kb_manager) {
    // Handle error
}

// Load Emacs preset (includes all standard keybindings)
lle_keybinding_load_emacs_preset(ctx.editor);
```

**Step 2: Register Keybindings** (15 min)
```c
// Bind arrow keys
lle_keybinding_bind_special(kb_manager, LLE_KEY_LEFT, 0, lle_backward_char);
lle_keybinding_bind_special(kb_manager, LLE_KEY_RIGHT, 0, lle_forward_char);
lle_keybinding_bind_special(kb_manager, LLE_KEY_HOME, 0, lle_beginning_of_line);
lle_keybinding_bind_special(kb_manager, LLE_KEY_END, 0, lle_end_of_line);
```

**Step 3: Route Events to Keybinding Manager** (30 min)
```c
// In input event loop
if (event->type == LLE_INPUT_TYPE_SPECIAL_KEY) {
    if (event->data.special_key.key == LLE_KEY_LEFT ||
        event->data.special_key.key == LLE_KEY_RIGHT ||
        event->data.special_key.key == LLE_KEY_HOME ||
        event->data.special_key.key == LLE_KEY_END) {
        
        // Dispatch through keybinding manager
        lle_result_t result = lle_keybinding_dispatch(kb_manager, event, ctx.editor);
        if (result == LLE_SUCCESS) {
            refresh_display(&ctx);
        }
        continue;  // Skip hardcoded handling
    }
}
// Fallthrough to hardcoded handling for unmigrated keys
```

**Step 4: Test Thoroughly** (1 hour)
- Test each arrow key individually
- Test with all Phase 1 UTF-8 inputs (café, 日本, 🎉)
- Test navigation across grapheme boundaries
- Test HOME/END on multi-byte characters
- Verify cursor position correctness
- Check for memory leaks

### Success Criteria
- ✅ All 4 navigation keys work identically to hardcoded version
- ✅ All Phase 1 UTF-8 tests pass
- ✅ No cursor jump issues
- ✅ No memory leaks
- ✅ Performance acceptable (<100μs per keypress)

### Rollback Procedure
If tests fail:
1. Comment out keybinding manager dispatch code
2. Re-enable hardcoded handling
3. Document the failure in MIGRATION_ISSUES.md
4. Investigate root cause before continuing

---

## Group 2: Deletion Keys (MEDIUM RISK)

### Keybindings to Migrate
- BACKSPACE → `lle_backward_delete_char`
- DELETE → `lle_delete_char`
- Ctrl-D → `lle_delete_char` / `lle_send_eof` (special case)

### Implementation Steps

**Step 1: Bind Deletion Keys** (15 min)
```c
lle_keybinding_bind_special(kb_manager, LLE_KEY_BACKSPACE, 0, lle_backward_delete_char);
lle_keybinding_bind_special(kb_manager, LLE_KEY_DELETE, 0, lle_delete_char);
lle_keybinding_bind_key(kb_manager, 'D', LLE_MOD_CTRL, handle_ctrl_d_wrapper);
```

**Step 2: Handle Ctrl-D Special Case** (30 min)
```c
// Wrapper function for Ctrl-D that checks buffer state
static lle_result_t handle_ctrl_d_wrapper(lle_editor_t *editor) {
    if (editor->buffer->length == 0) {
        return lle_send_eof(editor);
    } else {
        return lle_delete_char(editor);
    }
}
```

**Step 3: Route Events** (15 min)
- Add BACKSPACE, DELETE, Ctrl-D to keybinding manager dispatch

**Step 4: Test Thoroughly** (1 hour)
- Test deletion on ASCII characters
- Test deletion on multi-byte UTF-8 (café → caf, not caf�)
- Test deletion on grapheme clusters (👨‍👩‍👧‍👦 deletes entirely)
- Test Ctrl-D on empty buffer (should send EOF)
- Test Ctrl-D on non-empty buffer (should delete)
- Run all Phase 1 tests

### Success Criteria
- ✅ UTF-8/grapheme deletion works correctly
- ✅ No corruption (no U+FFFD)
- ✅ Ctrl-D EOF handling works
- ✅ All Phase 1 tests pass

---

## Group 3: Kill/Yank (HIGHER RISK)

### Keybindings to Migrate
- Ctrl-K → `lle_kill_line`
- Ctrl-U → `lle_unix_line_discard`
- Ctrl-W → `lle_unix_word_rubout`
- Ctrl-Y → `lle_yank`

### Implementation Steps

**Step 1: Bind Kill/Yank Keys** (15 min)
```c
lle_keybinding_bind_key(kb_manager, 'K', LLE_MOD_CTRL, lle_kill_line);
lle_keybinding_bind_key(kb_manager, 'U', LLE_MOD_CTRL, lle_unix_line_discard);
lle_keybinding_bind_key(kb_manager, 'W', LLE_MOD_CTRL, lle_unix_word_rubout);
lle_keybinding_bind_key(kb_manager, 'Y', LLE_MOD_CTRL, lle_yank);
```

**Step 2: Test Kill Ring Integration** (1.5 hours)
- Test Ctrl-K kills to end correctly
- Test Ctrl-U kills entire line
- Test Ctrl-W kills word backward
- Test Ctrl-Y yanks correctly
- Test kill → yank → kill → yank (kill ring cycling)
- Test with UTF-8 text

### Success Criteria
- ✅ Kill operations work correctly
- ✅ Yank restores killed text
- ✅ Kill ring maintains history
- ✅ UTF-8 boundaries respected

---

## Group 4: History & Special Keys (HIGH RISK)

### Keybindings to Migrate
- Ctrl-N → `lle_history_next`
- Ctrl-P → `lle_history_previous`
- UP arrow → `lle_smart_up_arrow`
- DOWN arrow → `lle_smart_down_arrow`
- Ctrl-G → `lle_abort_line`
- Ctrl-L → `lle_clear_screen`
- Ctrl-A, Ctrl-B, Ctrl-E, Ctrl-F (Ctrl+letter aliases for navigation)

### Implementation Steps

**Step 1: Bind Keys** (20 min)
```c
lle_keybinding_bind_key(kb_manager, 'N', LLE_MOD_CTRL, lle_history_next);
lle_keybinding_bind_key(kb_manager, 'P', LLE_MOD_CTRL, lle_history_previous);
lle_keybinding_bind_special(kb_manager, LLE_KEY_UP, 0, lle_smart_up_arrow);
lle_keybinding_bind_special(kb_manager, LLE_KEY_DOWN, 0, lle_smart_down_arrow);
lle_keybinding_bind_key(kb_manager, 'G', LLE_MOD_CTRL, lle_abort_line);
lle_keybinding_bind_key(kb_manager, 'L', LLE_MOD_CTRL, lle_clear_screen);
// Ctrl+letter navigation aliases
lle_keybinding_bind_key(kb_manager, 'A', LLE_MOD_CTRL, lle_beginning_of_line);
lle_keybinding_bind_key(kb_manager, 'B', LLE_MOD_CTRL, lle_backward_char);
lle_keybinding_bind_key(kb_manager, 'E', LLE_MOD_CTRL, lle_end_of_line);
lle_keybinding_bind_key(kb_manager, 'F', LLE_MOD_CTRL, lle_forward_char);
```

**Step 2: Test History** (1 hour)
- Test Ctrl-P/Ctrl-N navigate history
- Test UP/DOWN arrows in single-line mode (history)
- Test UP/DOWN arrows in multi-line mode (buffer navigation)
- Test with UTF-8 commands in history

**Step 3: Test Special Operations** (30 min)
- Test Ctrl-G aborts correctly
- Test Ctrl-L clears screen and redraws

### Success Criteria
- ✅ History navigation works
- ✅ Smart arrows context-aware
- ✅ Abort clears state correctly
- ✅ Clear screen works

---

## Group 5: Accept Line (CRITICAL)

### Keybinding to Migrate
- ENTER → `lle_accept_line`

### Implementation Steps

**Step 1: Bind ENTER** (10 min)
```c
lle_keybinding_bind_special(kb_manager, LLE_KEY_ENTER, 0, lle_accept_line);
```

**Step 2: Test Exhaustively** (2 hours)
- Test simple commands execute
- Test multi-line commands execute
- Test incomplete commands (continuation handling)
- Test with UTF-8 commands
- Test empty input
- Test whitespace-only input
- Regression test ALL previous functionality

### Success Criteria
- ✅ Commands execute correctly
- ✅ Multi-line works
- ✅ UTF-8 commands execute
- ✅ NO REGRESSIONS in any previous functionality

---

## Testing Protocol

### After Each Group

**1. Unit Testing**
- Test each migrated keybinding individually
- Verify behavior matches hardcoded version exactly

**2. UTF-8 Testing**
- Run all Phase 1 UTF-8 tests (café, 日本, 🎉, etc.)
- Test grapheme boundary handling
- Test multi-codepoint graphemes (flags, skin tones)

**3. Integration Testing**
- Test combinations (navigate + edit, kill + yank, etc.)
- Test with history
- Test with wrapped lines
- Test with multi-line input

**4. Performance Testing**
- Measure keypress latency (<100μs target)
- Check for memory leaks (valgrind)
- Profile if performance degrades

**5. Regression Testing**
- Verify all previously migrated groups still work
- No new bugs introduced

### Final Validation (After Group 5)

- ✅ Run complete Phase 1 UTF-8 test suite
- ✅ Test all 21 keybindings work correctly
- ✅ Performance acceptable
- ✅ No memory leaks
- ✅ No regressions
- ✅ Code is cleaner (hardcoded switch removed)

---

## Rollback Plan

### If Any Group Fails

**Immediate Actions**:
1. STOP migration
2. Document failure in `KEYBINDING_MIGRATION_ISSUES.md`
3. Revert to hardcoded handling for failed group
4. Keep successfully migrated groups active

**Investigation**:
1. Reproduce the failure
2. Check if issue is in keybinding manager or action function
3. Review Session 12 cursor sync fixes
4. Check for missing buffer/cursor synchronization

**Resolution**:
1. Fix the root cause
2. Re-test the fix
3. Document the solution
4. Resume migration

---

## Success Metrics

### Completion Criteria

- ✅ All 21 hardcoded keybindings migrated
- ✅ All 21 keybindings work identically to before
- ✅ All Phase 1 UTF-8 tests pass
- ✅ No performance degradation
- ✅ No memory leaks
- ✅ Code is cleaner (hardcoded switch statement removed)

### Benefits Achieved

- ✅ Correct architecture (as you noted)
- ✅ User customization enabled
- ✅ 21 bonus features available for binding
- ✅ Vi mode preset available
- ✅ Easier to maintain and extend
- ✅ Session 12 cursor sync fixes validated

---

## Timeline Estimate

| Group | Implementation | Testing | Total |
|-------|---------------|---------|-------|
| Group 1 (Navigation) | 2 hours | 1 hour | 3 hours |
| Group 2 (Deletion) | 1 hour | 1 hour | 2 hours |
| Group 3 (Kill/Yank) | 1 hour | 1.5 hours | 2.5 hours |
| Group 4 (History/Special) | 1.5 hours | 1.5 hours | 3 hours |
| Group 5 (Accept Line) | 30 min | 2 hours | 2.5 hours |
| **TOTAL** | **6 hours** | **7 hours** | **13 hours** |

Spread over 2-3 days with breaks.

---

## Dependencies

### Prerequisites
- ✅ Keybinding manager compiles
- ✅ All actions implemented
- ✅ Session 12 cursor sync fixes applied
- ✅ UTF-8 support complete (tokenizer + LLE)
- ✅ Audit complete (this plan)

### No Blockers
All dependencies satisfied. Ready to begin immediately.

---

## Next Step

**Begin Group 1 Migration**: Navigation keys (LEFT, RIGHT, HOME, END)

See `KEYBINDING_MIGRATION_TRACKER.md` for execution checklist.
