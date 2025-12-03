# macOS LLE Manual Testing Checklist

**Date**: 2025-12-02  
**Tester**: _______________  
**macOS Version**: _______________  
**Terminal**: _______________  

## How to Use This Checklist

1. Run the shell: `LLE_ENABLED=1 ./builddir/lusush`
2. For each test, mark: PASS, FAIL, or PARTIAL
3. For failures, note the observed behavior
4. After testing, share results for fixes

---

## Phase 1: Basic Input (Start Here)

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 1.1 | Type `echo hello` + Enter | Prints "hello" | | |
| 1.2 | Type `ls` + Enter | Lists directory | | |
| 1.3 | Type `pwd` + Enter | Shows current directory | | |

**Phase 1 Status**: _______________

---

## Phase 2: Cursor Navigation

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 2.1 | Type `echo test`, Left arrow x4 | Cursor moves left to 'e' in echo | | |
| 2.2 | Right arrow x2 | Cursor moves right | | |
| 2.3 | Home or Ctrl+A | Cursor jumps to start of line | | |
| 2.4 | End or Ctrl+E | Cursor jumps to end of line | | |
| 2.5 | Alt+F (or Esc then F) | Forward one word | | |
| 2.6 | Alt+B (or Esc then B) | Backward one word | | |

**Phase 2 Status**: _______________

---

## Phase 3: Text Editing

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 3.1 | Type `hello`, Backspace x2 | "hel" remains | | |
| 3.2 | Type `hello`, Ctrl+A, Delete | "ello" remains | | |
| 3.3 | Type `hello world`, Ctrl+A, Ctrl+K | Line cleared, "hello world" in kill ring | | |
| 3.4 | Ctrl+Y | Yanks back "hello world" | | |
| 3.5 | Type `hello world`, Ctrl+U | Entire line cleared | | |
| 3.6 | Type `hello world`, Ctrl+W | "world" deleted, "hello " remains | | |
| 3.7 | Type text, Ctrl+_ | Undo last change | | |
| 3.8 | Ctrl+^ (Ctrl+Shift+6) | Redo | | |

**Phase 3 Status**: _______________

---

## Phase 4: History Navigation

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 4.1 | Run `echo one`, `echo two`, `echo three` | Commands execute | | |
| 4.2 | Up arrow | Shows "echo three" | | |
| 4.3 | Up arrow again | Shows "echo two" | | |
| 4.4 | Down arrow | Shows "echo three" | | |
| 4.5 | Down arrow at end | Empty line or current input | | |
| 4.6 | Ctrl+P | Same as Up arrow | | |
| 4.7 | Ctrl+N | Same as Down arrow | | |

**Phase 4 Status**: _______________

---

## Phase 5: History Search (Ctrl+R)

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 5.1 | Ctrl+R | Prompt changes to search mode | | |
| 5.2 | Type `echo` | Finds matching history entry | | |
| 5.3 | Ctrl+R again | Finds next match | | |
| 5.4 | Enter | Executes found command | | |
| 5.5 | Ctrl+G during search | Aborts search, returns to normal | | |

**Phase 5 Status**: _______________

---

## Phase 6: Tab Completion

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 6.1 | Type `ech` + Tab | Completes to `echo ` | | |
| 6.2 | Type `cd /` + Tab | Shows directory completions | | |
| 6.3 | Type `display ` + Tab | Shows display subcommands | | |
| 6.4 | Type `display lle ` + Tab | Shows lle subcommands | | |
| 6.5 | Type `history ` + Tab | Shows history subcommands | | |

**Phase 6 Status**: _______________

---

## Phase 7: Signal Handling

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 7.1 | Type partial command, Ctrl+C | Shows ^C, new prompt, line cleared | | |
| 7.2 | Empty line, Ctrl+D | Shell exits | | |
| 7.3 | Non-empty line, Ctrl+D | Deletes char under cursor OR does nothing | | |
| 7.4 | Ctrl+L | Screen clears, prompt redrawn | | |
| 7.5 | Ctrl+Z during command | Suspends if foreground job running | | |

**Phase 7 Status**: _______________

---

## Phase 8: Autosuggestions (if enabled)

First enable: `display lle autosuggestions on`

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 8.1 | Type `ec` (after running echo before) | Ghost text suggests completion | | |
| 8.2 | Right arrow | Accepts full suggestion | | |
| 8.3 | Ctrl+Right | Accepts one word of suggestion | | |
| 8.4 | Any other key | Suggestion dismissed, typing continues | | |

**Phase 8 Status**: _______________

---

## Phase 9: Multiline Input

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 9.1 | Type `echo "hello` + Enter | Continuation prompt (PS2) appears | | |
| 9.2 | Type `world"` + Enter | Prints "hello\nworld" | | |
| 9.3 | Type `echo \` + Enter | Continuation for backslash | | |

**Phase 9 Status**: _______________

---

## Phase 10: Display Commands

| Test | Action | Expected | Result | Notes |
|------|--------|----------|--------|-------|
| 10.1 | `display lle` | Shows LLE status | | |
| 10.2 | `display lle keybindings` | Shows keybindings table | | |
| 10.3 | `display lle diagnostics` | Shows diagnostics info | | |
| 10.4 | `display lle autosuggestions on` | Enables autosuggestions | | |
| 10.5 | `display lle syntax on` | Enables syntax highlighting | | |

**Phase 10 Status**: _______________

---

## Summary

| Phase | Status | Critical Issues |
|-------|--------|-----------------|
| 1. Basic Input | | |
| 2. Cursor Navigation | | |
| 3. Text Editing | | |
| 4. History Navigation | | |
| 5. History Search | | |
| 6. Tab Completion | | |
| 7. Signal Handling | | |
| 8. Autosuggestions | | |
| 9. Multiline Input | | |
| 10. Display Commands | | |

**Overall Status**: _______________

---

## Issue Tracking

### Issue 1
- **Phase/Test**: 
- **Expected**: 
- **Actual**: 
- **Reproducible**: Yes / No / Sometimes
- **Severity**: Critical / Major / Minor

### Issue 2
- **Phase/Test**: 
- **Expected**: 
- **Actual**: 
- **Reproducible**: Yes / No / Sometimes
- **Severity**: Critical / Major / Minor

### Issue 3
- **Phase/Test**: 
- **Expected**: 
- **Actual**: 
- **Reproducible**: Yes / No / Sometimes
- **Severity**: Critical / Major / Minor

---

## Quick Reference

**Start shell**: `LLE_ENABLED=1 ./builddir/lusush`

**Key notation**:
- `Ctrl+X` = Hold Control, press X
- `Alt+X` = Hold Alt/Option, press X (may need terminal config)
- `Esc X` = Press Escape, release, then press X (alternative to Alt)

**If Alt doesn't work**: macOS Terminal.app may need:
Preferences -> Profiles -> Keyboard -> "Use Option as Meta key"
