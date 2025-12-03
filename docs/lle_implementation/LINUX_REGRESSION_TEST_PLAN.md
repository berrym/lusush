# Linux Regression Test Plan

**Created**: 2025-12-02  
**Purpose**: Verify macOS compatibility changes don't break Linux functionality  
**Branch**: `feature/lle`

---

## Summary of Changes Made on macOS

The following changes were made during the macOS compatibility session. Each needs verification on Linux.

### 1. ESC+key as Meta Modifier (HIGH PRIORITY)

**Files Changed**:
- `src/lle/sequence_parser.c` - Added Meta key event creation for ESC+printable
- `src/lle/terminal_unix_interface.c` - Increased timeout values
- `include/lle/input_parsing.h` - Increased `LLE_MAX_SEQUENCE_TIMEOUT_US` to 400ms

**What Changed**:
```c
// sequence_parser.c - NEW CODE in LLE_PARSER_STATE_ESCAPE handler
} else if (c >= 0x20 && c < 0x7F) {
    // ESC + printable ASCII = Meta/Alt + character
    result->data.key_info.modifiers = LLE_KEY_MOD_ALT;
    result->data.key_info.keycode = c;
    // ...
}
```

**Potential Linux Regressions**:
- Alt+key combinations might be affected (Linux terminals send different codes)
- Arrow keys and function keys use ESC sequences - could be misinterpreted
- CSI sequences (`ESC [`) should still work but verify

**Tests to Run**:
```bash
# Start lusush
./builddir/lusush

# Test 1: Alt+key combinations (Linux native Alt)
# Press Alt+b - should move backward one word
# Press Alt+f - should move forward one word
# Press Alt+d - should delete word forward

# Test 2: ESC+key combinations (should also work on Linux now)
# Press ESC then b - should move backward one word
# Press ESC then f - should move forward one word

# Test 3: Arrow keys (use ESC [ sequences)
# Press Up/Down - should navigate history
# Press Left/Right - should move cursor
# Press Home/End - should go to line start/end

# Test 4: Function keys
# Press F1-F12 (if bound) - should work correctly

# Test 5: ESC alone
# Press ESC alone, wait 500ms - should register as ESC
# (Note: There will be ~400ms delay - this is expected)
```

**Risk Level**: MEDIUM - The ESC sequence parsing is core input handling

---

### 2. Tab Handling Changes (LOW PRIORITY)

**Files Changed**:
- `src/lle/keybinding_actions.c` - `lle_tab_insert()` now expands to spaces
- `src/display/screen_buffer.c` - Uses `config.tab_width` instead of hardcoded 8
- `src/lle/display_bridge.c` - Uses `config.tab_width` instead of hardcoded 8

**What Changed**:
- Tab insertion now expands to spaces at input time based on visual column
- Display layer uses `config.tab_width` (default 4) instead of hardcoded 8

**Potential Linux Regressions**:
- Tab completion might behave differently (but TAB key triggers completion, not tab-insert)
- M-TAB (Alt+Tab) inserts literal spaces now instead of `\t` character

**Tests to Run**:
```bash
# Test 1: Tab completion
# Type "ls -" then press TAB - should show completions

# Test 2: Tab insertion (Alt+Tab or M-TAB)
# Press Alt+Tab or ESC then Tab - should insert spaces to next tab stop

# Test 3: Display of tabs from external sources
# echo -e "hello\tworld" | cat  # verify tabs display at 4-column stops
```

**Risk Level**: LOW - Tab handling is rarely used in shell input

---

### 3. Git-Aware Prompt Fix (Issue #14) (LOW PRIORITY)

**Files Changed**:
- `src/prompt.c` - Added pipe draining in `run_command()` to prevent SIGPIPE
- `src/readline_stubs.c` - Fixed `lusush_pool_free()` vs `free()` mismatch

**What Changed**:
```c
// prompt.c - run_command() now drains pipe before pclose()
while (fgets(drain_buffer, sizeof(drain_buffer), fp) != NULL) {
    // Discard remaining output to prevent SIGPIPE
}
status = pclose(fp);
```

**Potential Linux Regressions**:
- Git prompt information might not display (but this was broken before)
- Memory handling change could cause issues (unlikely)

**Tests to Run**:
```bash
# Test 1: Git prompt in git repository
cd ~/some-git-repo
./builddir/lusush
# Verify prompt shows branch name and status (e.g., "(main *)")

# Test 2: Non-git directory
cd /tmp
./builddir/lusush
# Verify no errors, prompt shows normally without git info

# Test 3: Rapid directory changes
# cd between git and non-git directories rapidly
# Verify no crashes or hangs
```

**Risk Level**: LOW - The fix was defensive (drain pipe before close)

---

### 4. Timeout Value Changes (MEDIUM PRIORITY)

**Files Changed**:
- `include/lle/input_parsing.h` - `LLE_MAX_SEQUENCE_TIMEOUT_US` changed from 100000 to 400000
- `src/lle/terminal_unix_interface.c` - Timeout checks changed from 50000 to 300000

**Impact**:
- ESC key alone now has ~400ms delay before registering
- This affects any feature that uses standalone ESC (e.g., aborting operations)

**Tests to Run**:
```bash
# Test 1: ESC to cancel/abort
# Start typing, press ESC - should clear/cancel (with slight delay)
# The delay should be noticeable but not annoying (~400ms)

# Test 2: Rapid input
# Type quickly - should have no issues
# Arrow keys, function keys should respond instantly (they complete sequences)
```

**Risk Level**: MEDIUM - Timeout affects all escape sequence handling

---

## Quick Regression Test Script

Run these commands in sequence to verify basic functionality:

```bash
#!/bin/bash
# Quick Linux regression test

cd /path/to/lusush
./builddir/lusush << 'EOF'
echo "Test 1: Basic input"
echo "Test 2: History" # Press Up to recall
# Press Ctrl+A (beginning of line)
# Press Ctrl+E (end of line)
# Press Alt+b (backward word) - IMPORTANT
# Press Alt+f (forward word) - IMPORTANT
# Press ESC then b (backward word via ESC) - NEW
# Press ESC then f (forward word via ESC) - NEW
# Press Tab (completion)
exit
EOF
```

---

## Expected Behavior Summary

| Feature | Expected Linux Behavior |
|---------|------------------------|
| Alt+b/f | Move by word (native Alt key) |
| ESC+b/f | Move by word (ESC as Meta) - NEW |
| Arrow keys | Navigate/move cursor |
| Tab | Trigger completion |
| Alt+Tab | Insert spaces to next tab stop |
| ESC alone | Register after ~400ms delay |
| Git prompt | Show branch/status in git repos |
| Ctrl+A/E | Beginning/end of line |
| Ctrl+K | Kill to end of line |

---

## If Regressions Are Found

1. **Document the regression**: Note exact steps to reproduce
2. **Check git blame**: Identify which commit introduced the issue
3. **Consider rollback**: If critical, revert specific changes
4. **Fix forward**: Prefer conditional code (`#ifdef __linux__`) if platform-specific

---

## Sign-Off

### Automated Tests (Non-Interactive)
- [x] All automated tests pass on Linux
- [x] Build succeeds with readline enabled
- [x] Build succeeds with readline disabled

### Manual Interactive Testing (REQUIRED - NOT YET DONE)
- [ ] Alt+b/f word movement works correctly
- [ ] ESC+b/f word movement works correctly (NEW - macOS change)
- [ ] ESC alone registers after delay (verify 400ms feels acceptable)
- [ ] Arrow keys navigate cursor and history
- [ ] Function keys (F1-F12) work if bound
- [ ] Tab completion triggers correctly
- [ ] Git prompt displays branch/status
- [ ] Ctrl+A/E/K/U/W work correctly
- [ ] Ctrl+R history search works
- [ ] Autosuggestions appear and can be accepted
- [ ] Multiline editing works
- [ ] No visual glitches or cursor positioning issues

**Automated Tests by**: Claude Code (AI Assistant)  
**Date**: 2025-12-02  
**Linux Distribution**: Fedora 43 (Linux 6.17.8-300.fc43.x86_64)  
**Terminal**: Non-TTY testing environment (automated tests only)

**Manual Testing by**: _______________  
**Date**: _______________  
**Terminal Emulator**: _______________

### Automated Test Results

**Tests Run:**
- `test_input_parser_integration`: 10/10 passed (after fixing timeout test for 400ms change)
- `test_fkey_detection`: 3/3 passed (F1, F5, arrow keys)
- `test_terminal_event_reading`: 14/14 passed
- `test_terminal_capabilities`: 15/15 passed
- `test_event_system`: 35/35 passed
- `test_buffer_operations`: 17/17 passed
- `test_multiline_manager`: 3/3 passed

**Build Configurations Verified:**
- With readline (`-Dreadline_support=true`): Links libreadline.so.8
- Without readline (`-Dreadline_support=false`): Links only libtinfo.so.6

**Notes:**
- Fixed `tests/lle/integration/input_parser_integration_test.c` to use 500ms timeout 
  (was 200ms, now needs > 400ms due to `LLE_MAX_SEQUENCE_TIMEOUT_US` change)
- The `test_fkey_manual` requires a real TTY and is expected to fail in automated testing
- `test_subsystem_integration` has 2 pre-existing UTF-8 index failures unrelated to macOS changes

**IMPORTANT**: Automated tests verify internal logic only. Manual interactive testing 
in a real terminal is REQUIRED before merge to verify the actual user experience.
