# LLE Spec 12 Phase 5.4 - Manual Test Plan

**Document**: COMPLETION_PHASE54_MANUAL_TEST_PLAN.md  
**Date**: 2025-11-18  
**Phase**: 5.4 - Keyboard Event Wiring  
**Status**: Ready for Testing  
**Tester**: User (live terminal testing required)

---

## Overview

This document provides a structured test plan for manually testing the Phase 5.4 completion system keyboard integration in a live terminal. The completion system is now fully integrated - this testing validates it works correctly in real-world use.

**What We're Testing**:
- TAB key completion generation and menu display
- Arrow key navigation through completion menu
- Enter key completion acceptance
- Escape key menu cancellation
- Menu auto-dismiss on typing
- Cursor positioning after completions
- Display rendering of completion menu

---

## Test Environment Setup

**Prerequisites**:
1. Build lusush: `meson compile -C builddir`
2. Run lusush: `./builddir/lusush`
3. Terminal with at least 80 columns width
4. Clean environment (or known test files/commands)

**Expected Behavior**:
- Menu appears below command line
- Menu shows completion items with categories
- Selection highlighted
- Cursor stays in correct position

---

## Test Cases

### Test 1: Basic TAB Completion - Single Match

**Objective**: Verify single completion inserts directly

**Steps**:
1. Type: `ec` (partial command)
2. Press: TAB
3. Observe: Should auto-complete to `echo` (if it's the only match)

**Expected Result**:
- ✅ Command completes to `echo`
- ✅ Cursor at end of `echo`
- ✅ No menu displayed (single match)
- ✅ Ready to type arguments

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 2: TAB Completion - Multiple Matches (Menu Display)

**Objective**: Verify menu appears for multiple completions

**Steps**:
1. Type: `l` (partial command)
2. Press: TAB
3. Observe menu appearance

**Expected Result**:
- ✅ Menu appears below command line
- ✅ Shows commands starting with 'l' (ls, ln, last, etc.)
- ✅ First item highlighted/selected
- ✅ Menu has category headers (COMMANDS, etc.)
- ✅ Original command `l` still visible
- ✅ Cursor position unchanged

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 3: TAB Key Cycling Through Menu

**Objective**: Verify TAB cycles through menu items

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Press: TAB again (should move to next item)
4. Press: TAB again (should move to next item)
5. Observe selection changes

**Expected Result**:
- ✅ Each TAB moves selection down
- ✅ Selection wraps to top after last item
- ✅ Menu re-renders with new selection
- ✅ Command text unchanged

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 4: Down Arrow Menu Navigation

**Objective**: Verify down arrow moves through menu

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Press: Down Arrow (should move to next item)
4. Press: Down Arrow again
5. Observe selection changes

**Expected Result**:
- ✅ Down arrow moves selection down
- ✅ Wraps to top after last item
- ✅ Menu updates correctly
- ✅ Smooth navigation

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 5: Up Arrow Menu Navigation

**Objective**: Verify up arrow moves through menu

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Press: Down Arrow twice
4. Press: Up Arrow (should move back up)
5. Observe selection changes

**Expected Result**:
- ✅ Up arrow moves selection up
- ✅ Wraps to bottom when at top
- ✅ Menu updates correctly
- ✅ Can navigate both directions

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 6: Enter Key Accepts Completion

**Objective**: Verify Enter accepts selected completion

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Press: Down Arrow to select `ls`
4. Press: Enter
5. Observe result

**Expected Result**:
- ✅ Command replaced with `ls`
- ✅ Menu disappears
- ✅ Cursor at end of `ls`
- ✅ Ready to type arguments or execute
- ✅ Display clean (no menu artifacts)

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 7: Escape Key Cancels Menu

**Objective**: Verify Escape dismisses menu without completing

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Press: Escape
4. Observe result

**Expected Result**:
- ✅ Menu disappears
- ✅ Original command `l` unchanged
- ✅ Cursor position unchanged
- ✅ Can continue typing normally
- ✅ Display clean

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 8: Character Input Dismisses Menu

**Objective**: Verify typing dismisses menu automatically

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Type: `s` (character input)
4. Observe result

**Expected Result**:
- ✅ Menu disappears immediately
- ✅ Character `s` inserted → command becomes `ls`
- ✅ Cursor after `ls`
- ✅ Can continue typing normally

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 9: File Path Completion

**Objective**: Verify file/directory completion works

**Steps**:
1. Type: `ls /u` (partial path)
2. Press: TAB
3. Observe completions

**Expected Result**:
- ✅ Menu shows directories under /u (like /usr, /usr/local, etc.)
- ✅ Directories marked with indicator
- ✅ Can navigate and select
- ✅ Completion inserts correctly

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 10: Variable Completion

**Objective**: Verify variable completion works

**Steps**:
1. Type: `echo $HO` (partial variable)
2. Press: TAB
3. Observe completions

**Expected Result**:
- ✅ Menu shows variables starting with HO (HOME, etc.)
- ✅ Variables categorized correctly
- ✅ Can select and complete
- ✅ Variable inserted with $

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 11: Menu Display in Narrow Terminal

**Objective**: Verify menu adapts to terminal width

**Steps**:
1. Resize terminal to ~40 columns
2. Type: `l`
3. Press: TAB
4. Observe menu layout

**Expected Result**:
- ✅ Menu adjusts to terminal width
- ✅ Single column layout if needed
- ✅ No overflow or corruption
- ✅ Still navigable

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 12: Menu Display in Wide Terminal

**Objective**: Verify multi-column menu layout

**Steps**:
1. Resize terminal to 120+ columns
2. Type: `l`
3. Press: TAB
4. Observe menu layout

**Expected Result**:
- ✅ Menu uses multi-column layout
- ✅ Items organized efficiently
- ✅ Categories grouped correctly
- ✅ Easy to read

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 13: Completion After Pipe

**Objective**: Verify completion works after shell operators

**Steps**:
1. Type: `echo test | l` (command after pipe)
2. Press: TAB
3. Observe completions

**Expected Result**:
- ✅ Menu shows commands (context detected correctly)
- ✅ Completion works normally
- ✅ Previous part of command unchanged

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 14: Cursor Position After Completion

**Objective**: Verify cursor positioning is correct

**Steps**:
1. Type: `ec`
2. Press: TAB (completes to `echo`)
3. Type: ` test` (add argument)
4. Move cursor back to between `echo` and ` test`
5. Observe cursor position

**Expected Result**:
- ✅ Cursor at end of completed word
- ✅ Can type arguments immediately
- ✅ Cursor movement works correctly
- ✅ No cursor position bugs

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 15: Rapid TAB Presses

**Objective**: Verify system handles rapid input

**Steps**:
1. Type: `l`
2. Press: TAB rapidly 5-10 times
3. Observe behavior

**Expected Result**:
- ✅ System doesn't crash
- ✅ Menu updates correctly
- ✅ Selection cycles properly
- ✅ No visual artifacts
- ✅ Responsive

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 16: Menu with Long Item Names

**Objective**: Verify long completion names display correctly

**Steps**:
1. Create a file with a very long name (50+ chars)
2. Type partial name
3. Press: TAB
4. Observe menu rendering

**Expected Result**:
- ✅ Long names truncated or wrapped appropriately
- ✅ Menu doesn't overflow terminal
- ✅ Still readable and usable
- ✅ No display corruption

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 17: Empty Completion (No Matches)

**Objective**: Verify behavior when no completions exist

**Steps**:
1. Type: `xyzabc` (non-existent command prefix)
2. Press: TAB
3. Observe result

**Expected Result**:
- ✅ No menu appears
- ✅ No error messages
- ✅ Command unchanged
- ✅ Can continue typing or delete

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 18: Multiline Buffer Interaction

**Objective**: Verify completion doesn't interfere with multiline editing

**Steps**:
1. Type: `if true; then`
2. Press: Enter (start multiline)
3. Type: `  ec`
4. Press: TAB
5. Observe behavior

**Expected Result**:
- ✅ Completion works in multiline context
- ✅ Menu appears correctly
- ✅ Multiline prompt preserved
- ✅ Can complete and continue

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 19: History Navigation After Menu Dismiss

**Objective**: Verify arrow keys return to history after menu closes

**Steps**:
1. Execute some commands to populate history
2. Type: `l`
3. Press: TAB (menu appears)
4. Press: Escape (menu closes)
5. Press: Up Arrow
6. Observe result

**Expected Result**:
- ✅ After Escape, up arrow navigates history
- ✅ Previous commands appear
- ✅ Down arrow works for history too
- ✅ Normal history navigation restored

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

### Test 20: Display Refresh and Screen Integrity

**Objective**: Verify display system handles menu correctly

**Steps**:
1. Type: `l`
2. Press: TAB (menu appears)
3. Navigate with arrows
4. Press: Enter to accept
5. Execute command
6. Check for any display artifacts

**Expected Result**:
- ✅ No leftover menu text
- ✅ Prompt appears correctly after execution
- ✅ Screen clean and readable
- ✅ No ANSI escape sequence artifacts
- ✅ Terminal state normal

**Notes**: ___________________________________________________________

**Status**: ⬜ Pass  ⬜ Fail  ⬜ Partial  ⬜ Blocked

---

## Summary Template

**Total Tests**: 20  
**Passed**: _____  
**Failed**: _____  
**Partial**: _____  
**Blocked**: _____

**Critical Issues Found**:
1. ___________________________________________________________
2. ___________________________________________________________
3. ___________________________________________________________

**Minor Issues Found**:
1. ___________________________________________________________
2. ___________________________________________________________
3. ___________________________________________________________

**Overall Assessment**:
⬜ Ready for production  
⬜ Needs minor fixes  
⬜ Needs major fixes  
⬜ Requires redesign

**Notes**:
________________________________________________________________
________________________________________________________________
________________________________________________________________

---

## Issue Reporting Format

If you find issues, please report them in this format:

```
**Issue #X**: [Brief Description]

**Test Case**: [Test number and name]

**Steps to Reproduce**:
1. [Step 1]
2. [Step 2]
3. [Step 3]

**Expected Behavior**:
[What should happen]

**Actual Behavior**:
[What actually happened]

**Severity**: ⬜ Critical  ⬜ High  ⬜ Medium  ⬜ Low

**Screenshots/Logs**: [If available]

**Additional Notes**: [Any other relevant information]
```

---

## Next Steps After Testing

Based on test results:

1. **All Pass**: Proceed to Phase 5.5 (refinement and optimization)
2. **Minor Issues**: Fix and re-test affected cases
3. **Major Issues**: Investigate, fix, and full re-test
4. **Blocked**: Resolve blockers and continue testing

**Testing Date**: _______________  
**Tested By**: _______________  
**Completion Time**: _______________
