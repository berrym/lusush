# LLE Keybinding Test Plan

**Document**: KEYBINDING_TEST_PLAN.md  
**Date**: 2025-11-07  
**Branch**: feature/lle  
**Purpose**: Systematic testing of all LLE keybinding functions  
**Status**: Ready for Tier 1 testing (17 implemented keybindings)

---

## Overview

This test plan provides comprehensive manual testing procedures for all LLE keybinding functions. Tests are organized in tiers based on implementation status, with each test covering multiple scenarios including ASCII, UTF-8, wrapped lines, and edge cases.

### Test Execution Requirements

- **Environment**: Interactive terminal (80+ columns recommended)
- **Shell**: lusush with LLE enabled
- **Test Data**: Various text inputs (ASCII, UTF-8, long lines)
- **Expected Behavior**: Compare against GNU Readline (bash) behavior

---

## Tier 1: Currently Implemented Keybindings (Ready to Test)

These 17 keybindings are implemented and ready for comprehensive testing.

### 1. Basic Character Input

**Function**: Regular character insertion  
**Test Cases**:

#### Test 1.1: ASCII Character Input
**Steps**:
1. Start lusush
2. Type: `hello world`
3. Press Enter

**Expected**: Command executes, characters appear as typed

#### Test 1.2: UTF-8 Character Input
**Steps**:
1. Start lusush
2. Type: `echo "café ☕ 中文 🔥"`
3. Press Enter

**Expected**: All UTF-8 characters display correctly, command executes

#### Test 1.3: Long Line Wrapping
**Steps**:
1. Start lusush
2. Type: `echo "this is a very long line of text that should wrap to the next line when it exceeds the terminal width"`
3. Observe wrapping behavior

**Expected**: Line wraps at terminal edge, continues on next line

---

### 2. Backspace (backward-delete-char)

**Keybinding**: `Backspace` or `DEL`  
**Function**: Delete character before cursor

#### Test 2.1: Delete ASCII Characters
**Steps**:
1. Type: `hello world`
2. Press Backspace 5 times
3. Press Enter

**Expected**: Buffer shows `hello `, command executes

#### Test 2.2: Delete UTF-8 Characters
**Steps**:
1. Type: `café`
2. Press Backspace once
3. Observe buffer

**Expected**: Entire `é` character deleted (both bytes), buffer shows `caf`

#### Test 2.3: Delete at Beginning of Line
**Steps**:
1. Type: `test`
2. Press Ctrl-A (move to beginning)
3. Press Backspace
4. Observe buffer

**Expected**: Nothing happens (already at start), buffer still shows `test`

#### Test 2.4: Delete Across Wrap Boundary
**Steps**:
1. Type a long line that wraps (80+ chars)
2. Press Backspace to delete characters on second line
3. Continue backspacing across wrap boundary

**Expected**: Characters delete correctly, cursor moves to previous line when crossing boundary

---

### 3. Enter (accept-line)

**Keybinding**: `Enter` or `RET`  
**Function**: Execute/accept the current line

#### Test 3.1: Accept at End of Line
**Steps**:
1. Type: `echo "test"`
2. Press Enter

**Expected**: Command executes, output appears on fresh line below

#### Test 3.2: Accept at Beginning of Line
**Steps**:
1. Type: `echo "test"`
2. Press Ctrl-A (move to beginning)
3. Press Enter

**Expected**: Full command executes (not just from cursor position), output on fresh line

#### Test 3.3: Accept at Middle of Line
**Steps**:
1. Type: `echo "hello world"`
2. Press Ctrl-A, then Right arrow 10 times (position at space)
3. Press Enter

**Expected**: Full command executes, output on fresh line

#### Test 3.4: Accept on Wrapped Line
**Steps**:
1. Type long line that wraps
2. Use arrow keys to position cursor on second line
3. Press Enter

**Expected**: Full command executes, output appears below wrapped input

#### Test 3.5: Accept with Complex Edits
**Steps**:
1. Type: `echo "this is a very long line of text that should wrap"`
2. Backspace partway back
3. Retype the rest
4. Left arrow to first line
5. Right arrow to second line
6. Left arrow back to first line
7. Insert word "really" 
8. Press Enter

**Expected**: Full command with "really" inserted executes, output on fresh line (regression test for issue fixed 2025-11-07)

---

### 4. Arrow Keys - Left/Right (backward-char/forward-char)

**Keybindings**: `Left Arrow` / `Ctrl-B` and `Right Arrow` / `Ctrl-F`  
**Function**: Move cursor one character backward/forward

#### Test 4.1: Move Left Through ASCII
**Steps**:
1. Type: `hello world`
2. Press Left arrow 5 times
3. Type: `there `

**Expected**: Buffer shows `hello there world`

#### Test 4.2: Move Right Through ASCII
**Steps**:
1. Type: `hello world`
2. Press Ctrl-A (beginning)
3. Press Right arrow 6 times
4. Type: `beautiful `

**Expected**: Buffer shows `hello beautiful world`

#### Test 4.3: Move Through UTF-8
**Steps**:
1. Type: `café`
2. Press Left arrow once
3. Observe cursor position

**Expected**: Cursor moves before `é` (not in middle of UTF-8 sequence)

#### Test 4.4: Move Across Wrap Boundary
**Steps**:
1. Type long line that wraps
2. Press Left arrow repeatedly to move from second line to first line

**Expected**: Cursor moves smoothly across wrap, no display glitches

#### Test 4.5: Move at Boundaries
**Steps**:
1. Type: `test`
2. Press Left arrow 10 times (more than buffer length)
3. Press Right arrow 10 times

**Expected**: Cursor stops at beginning, then moves to end, doesn't go past boundaries

---

### 5. Home/End Keys (beginning-of-line/end-of-line)

**Keybindings**: `Home` / `Ctrl-A` and `End` / `Ctrl-E`  
**Function**: Move cursor to beginning/end of line

#### Test 5.1: Home Key
**Steps**:
1. Type: `echo "hello world"`
2. Press Home (or Ctrl-A)
3. Type: `# `

**Expected**: Buffer shows `# echo "hello world"`

#### Test 5.2: End Key
**Steps**:
1. Type: `echo "hello`
2. Press Home (or Ctrl-A)
3. Press End (or Ctrl-E)
4. Type: ` world"`

**Expected**: Buffer shows `echo "hello world"`

#### Test 5.3: Home/End with Wrapped Lines
**Steps**:
1. Type long line that wraps to 3 lines
2. Position cursor on line 2
3. Press Home

**Expected**: Cursor moves to beginning of line 2 (not line 1)

#### Test 5.4: Home/End with UTF-8
**Steps**:
1. Type: `echo "café ☕ 中文"`
2. Press Home, then End
3. Observe cursor positions

**Expected**: Cursor at start/end of buffer, no UTF-8 corruption

---

### 6. Delete Key (delete-char)

**Keybinding**: `Delete` or `Ctrl-D`  
**Function**: Delete character at cursor (or EOF if empty)

#### Test 6.1: Delete ASCII Character
**Steps**:
1. Type: `hello world`
2. Press Home
3. Press Delete
4. Observe buffer

**Expected**: Buffer shows `ello world`

#### Test 6.2: Delete UTF-8 Character
**Steps**:
1. Type: `café`
2. Press Home, Right 3 times (position before `é`)
3. Press Delete

**Expected**: Entire `é` deleted, buffer shows `caf`

#### Test 6.3: Delete at End of Line
**Steps**:
1. Type: `test`
2. Press Delete

**Expected**: Nothing happens (already at end)

#### Test 6.4: EOF on Empty Line
**Steps**:
1. Start lusush
2. Press Ctrl-D (or Delete on empty line)

**Expected**: Shell exits (EOF signal)

---

### 7. Ctrl-K (kill-line)

**Keybinding**: `Ctrl-K`  
**Function**: Kill from cursor to end of line

#### Test 7.1: Kill to End
**Steps**:
1. Type: `echo "hello world"`
2. Press Home, Right 5 times (after "hello")
3. Press Ctrl-K
4. Observe buffer

**Expected**: Buffer shows `echo `, killed text stored in kill buffer

#### Test 7.2: Kill at End of Line
**Steps**:
1. Type: `test`
2. Press Ctrl-K

**Expected**: Nothing visible happens (already at end), but newline might be stored

#### Test 7.3: Kill on Wrapped Line
**Steps**:
1. Type long line that wraps to 2 lines
2. Position cursor at start of line 1
3. Press Ctrl-K

**Expected**: Kills to end of line 1 only (not entire buffer)

---

### 8. Ctrl-U (backward-kill-line)

**Keybinding**: `Ctrl-U`  
**Function**: Kill from beginning of line to cursor

#### Test 8.1: Kill from Beginning
**Steps**:
1. Type: `echo "hello world"`
2. Press End
3. Press Ctrl-U
4. Observe buffer

**Expected**: Buffer empty, killed text stored in kill buffer

#### Test 8.2: Kill Partial Line
**Steps**:
1. Type: `echo "hello world"`
2. Position cursor after "hello"
3. Press Ctrl-U

**Expected**: Buffer shows ` "hello world"` (killed `echo`)

---

### 9. Ctrl-W (unix-word-rubout)

**Keybinding**: `Ctrl-W`  
**Function**: Kill previous whitespace-delimited word

#### Test 9.1: Kill Previous Word
**Steps**:
1. Type: `echo hello world`
2. Press Ctrl-W

**Expected**: Buffer shows `echo hello `, "world" killed

#### Test 9.2: Kill Multiple Words
**Steps**:
1. Type: `echo hello world test`
2. Press Ctrl-W twice

**Expected**: Buffer shows `echo hello `, both "test" and "world" killed

#### Test 9.3: Kill at Beginning
**Steps**:
1. Type: `test`
2. Press Home
3. Press Ctrl-W

**Expected**: Nothing happens (no word before cursor)

---

### 10. Ctrl-Y (yank)

**Keybinding**: `Ctrl-Y`  
**Function**: Paste killed text from kill buffer

#### Test 10.1: Yank After Kill
**Steps**:
1. Type: `echo "hello world"`
2. Press Ctrl-A, Right 5 times (after "hello")
3. Press Ctrl-K (kill " world")
4. Type: ` there`
5. Press Ctrl-Y

**Expected**: Buffer shows `echo there "hello world"`

#### Test 10.2: Yank Without Prior Kill
**Steps**:
1. Start fresh prompt
2. Press Ctrl-Y

**Expected**: Nothing happens (kill buffer empty)

#### Test 10.3: Multiple Yanks
**Steps**:
1. Type: `test`
2. Press Ctrl-U (kill "test")
3. Press Ctrl-Y
4. Press Space
5. Press Ctrl-Y again

**Expected**: Buffer shows `test test`

---

### 11. Ctrl-A (beginning-of-line)

**Keybinding**: `Ctrl-A`  
**Function**: Move to beginning of line

*See Test 5.1-5.4 (same as Home key)*

---

### 12. Ctrl-B (backward-char)

**Keybinding**: `Ctrl-B`  
**Function**: Move backward one character

*See Test 4.1-4.5 (same as Left arrow)*

---

### 13. Ctrl-E (end-of-line)

**Keybinding**: `Ctrl-E`  
**Function**: Move to end of line

*See Test 5.1-5.4 (same as End key)*

---

### 14. Ctrl-F (forward-char)

**Keybinding**: `Ctrl-F`  
**Function**: Move forward one character

*See Test 4.1-4.5 (same as Right arrow)*

---

### 15. Ctrl-G (abort-line)

**Keybinding**: `Ctrl-G`  
**Function**: Abort/cancel current line

#### Test 15.1: Abort with Text
**Steps**:
1. Type: `echo "hello world"`
2. Press Ctrl-G

**Expected**: Buffer cleared, fresh prompt appears, no command executed

#### Test 15.2: Abort Empty Line
**Steps**:
1. Start fresh prompt
2. Press Ctrl-G

**Expected**: Fresh prompt appears (essentially no-op)

#### Test 15.3: Abort Incomplete Input
**Steps**:
1. Type: `echo "unclosed quote`
2. Press Enter (should prompt for continuation)
3. Press Ctrl-G

**Expected**: Multiline input cancelled, fresh prompt appears

---

### 16. Ctrl-L (clear-screen)

**Keybinding**: `Ctrl-L`  
**Function**: Clear screen, redraw current line

#### Test 16.1: Clear with Text
**Steps**:
1. Run some commands to fill screen
2. Type: `echo "test"`
3. Press Ctrl-L

**Expected**: Screen clears, prompt with "test" reappears at top

#### Test 16.2: Clear Empty Line
**Steps**:
1. Start fresh prompt
2. Press Ctrl-L

**Expected**: Screen clears, fresh prompt at top

---

### 17. Ctrl-D (EOF)

**Keybinding**: `Ctrl-D`  
**Function**: EOF if line empty, otherwise delete-char

#### Test 17.1: EOF on Empty Line
**Steps**:
1. Start lusush
2. Press Ctrl-D on empty prompt

**Expected**: Shell exits

#### Test 17.2: Delete Character Mid-Line
**Steps**:
1. Type: `hello`
2. Press Home
3. Press Ctrl-D

**Expected**: First character deleted, buffer shows `ello`

---

## Tier 1 Test Summary

**Total Tests**: 17 keybindings × ~3-5 test cases each = **~60 test cases**

**Test Coverage**:
- ✅ ASCII text
- ✅ UTF-8 characters (2-byte, 3-byte, 4-byte)
- ✅ Wrapped lines
- ✅ Edge cases (empty buffer, boundaries)
- ✅ Kill buffer interaction
- ✅ Display regression scenarios

**Testing Method**: Manual interactive testing  
**Success Criteria**: All tests pass, behavior matches GNU Readline (bash)

---

## Tier 2: Next Implementation Priority (Not Yet Implemented)

These keybindings should be implemented and tested next.

### History Navigation
- [ ] Up Arrow / Ctrl-P (previous-history)
- [ ] Down Arrow / Ctrl-N (next-history)
- [ ] Ctrl-R (reverse-search-history)
- [ ] Ctrl-S (forward-search-history)

### Word Movement
- [ ] Meta-F (forward-word)
- [ ] Meta-B (backward-word)
- [ ] Meta-D (kill-word)
- [ ] Meta-DEL (backward-kill-word)

### Additional Editing
- [ ] Ctrl-T (transpose-chars)
- [ ] Ctrl-Q / Ctrl-V (quoted-insert)

**Estimated Test Count**: 10 keybindings × 4 test cases = **40 test cases**

---

## Tier 3: Advanced Features (Future Implementation)

### Completion System
- [ ] Tab (complete)
- [ ] Meta-? (possible-completions)
- [ ] Meta-* (insert-completions)

### Case Conversion
- [ ] Meta-U (upcase-word)
- [ ] Meta-L (downcase-word)
- [ ] Meta-C (capitalize-word)

### Kill Ring Advanced
- [ ] Meta-Y (yank-pop)

### Special Operations
- [ ] Meta-T (transpose-words)
- [ ] Meta-\ (delete-horizontal-space)

**Estimated Test Count**: 10 keybindings × 3 test cases = **30 test cases**

---

## Test Execution Tracking

### Tier 1 Progress

| Keybinding | Test Cases | Status | Date | Notes |
|-----------|-----------|--------|------|-------|
| Character Input | 3 | ⬜ Not Started | | |
| Backspace | 4 | ⬜ Not Started | | |
| Enter | 5 | ⬜ Not Started | | |
| Left/Right | 5 | ⬜ Not Started | | |
| Home/End | 4 | ⬜ Not Started | | |
| Delete | 4 | ⬜ Not Started | | |
| Ctrl-K | 3 | ⬜ Not Started | | |
| Ctrl-U | 2 | ⬜ Not Started | | |
| Ctrl-W | 3 | ⬜ Not Started | | |
| Ctrl-Y | 3 | ⬜ Not Started | | |
| Ctrl-A | 4 | ⬜ Not Started | | |
| Ctrl-B | 5 | ⬜ Not Started | | |
| Ctrl-E | 4 | ⬜ Not Started | | |
| Ctrl-F | 5 | ⬜ Not Started | | |
| Ctrl-G | 3 | ⬜ Not Started | | |
| Ctrl-L | 2 | ⬜ Not Started | | |
| Ctrl-D | 2 | ⬜ Not Started | | |

**Legend**:
- ⬜ Not Started
- 🔄 In Progress
- ✅ Passed
- ❌ Failed
- ⚠️ Partial (some tests passed, some failed)

---

## Bug Tracking

### Known Issues
*(Document any issues found during testing)*

| ID | Keybinding | Issue | Severity | Status | Fix Commit |
|----|-----------|-------|----------|--------|------------|
| - | - | - | - | - | - |

---

## Test Environment Setup

### Prerequisites
1. Build lusush with LLE enabled: 
   ```bash
   cd /home/mberry/Lab/c/lusush
   meson setup builddir --reconfigure  # or just: meson setup builddir
   ninja -C builddir lusush
   ```
2. Run lusush: `./builddir/lusush`
3. Ensure terminal is at least 80 columns wide
4. Have test data ready (UTF-8 strings, long lines)

### Test Data Samples

**ASCII Test String**: `The quick brown fox jumps over the lazy dog`

**UTF-8 Test Strings**:
- 2-byte: `café résumé naïve`
- 3-byte: `中文字符 日本語 한글`
- 4-byte: `🔥 ☕ 🎉 🌟 💻`
- Mixed: `Hello 世界 🌍 café!`

**Long Line (80+ chars)**:
```
echo "This is a very long line of text that should definitely wrap to the next line when displayed in a standard 80-column terminal width"
```

**Long Line (150+ chars)**:
```
echo "This is an extremely long line of text that will wrap multiple times across several lines to test complex wrapping behavior and cursor movement across wrap boundaries in various scenarios"
```

---

## Success Criteria

### Tier 1 Completion Criteria
- ✅ All 60 test cases pass
- ✅ No display corruption with UTF-8
- ✅ No cursor position errors on wrapped lines
- ✅ Kill buffer works correctly across all kill/yank operations
- ✅ Behavior matches GNU Readline (bash)
- ✅ No memory leaks detected
- ✅ No crashes or hangs

### Overall Test Plan Success
- ✅ Tier 1: 100% passing
- ✅ Tier 2: Implemented and tested
- ✅ Tier 3: Implemented and tested
- ✅ All 130+ test cases passing
- ✅ Performance within spec (< 100μs per operation)
- ✅ Ready for production use

---

## Appendix: Comparison with GNU Readline

All keybinding behavior should match GNU Readline as implemented in bash 5.x. When in doubt, test the same operation in bash and verify lusush behaves identically.

### Testing in Bash
```bash
# Start bash in clean state
bash --norc --noprofile

# Test a keybinding
# Type: echo "test"
# Press: Ctrl-A
# Expected: Cursor at beginning
```

### Known Differences
*(Document any intentional differences from GNU Readline)*

None currently - full compatibility is the goal.

---

**Document End**
