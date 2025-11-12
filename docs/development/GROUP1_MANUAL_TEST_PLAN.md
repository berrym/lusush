# Group 1 Manual Test Plan - Navigation Keys
**Date**: 2025-11-11  
**Tester**: (Your name)  
**Branch**: feature/lle  
**Commit**: (Fill in after testing)

## Overview
This document provides step-by-step manual tests for Group 1 keybinding manager migration.
Group 1 migrates 4 navigation keys: LEFT, RIGHT, HOME, END.

**Test Goal**: Verify keybinding manager routes these keys correctly and maintains UTF-8/grapheme support.

---

## Test Environment Setup

1. Build lusush:
   ```bash
   cd /home/mberry/Lab/c/lusush/builddir
   ninja lusush
   ```

2. Start lusush:
   ```bash
   cd /home/mberry/Lab/c/lusush
   ./builddir/lusush
   ```

3. Verify you see the lusush prompt (e.g., `lusush$ `)

---

## Test Suite

### TEST 1: LEFT Arrow - Basic ASCII
**Input**: Type `hello` then press LEFT arrow 3 times  
**Expected**: Cursor moves from position 5 (after 'o') to position 2 (on first 'l', between 'e' and 'l')  
**Visual Check**: Cursor should be on the first 'l'

**Result**: [x] PASS  [ ] FAIL  
**Notes**: Cursor positioned correctly. Text intact. No visual issues.


---

### TEST 2: RIGHT Arrow - Basic ASCII
**Input**: From TEST 1 position, press RIGHT arrow 3 times  
**Expected**: Cursor moves back to end of line  
**Visual Check**: Cursor should be after 'o'

**Result**: [x] PASS  [ ] FAIL  
**Notes**: Cursor returned perfectly to end after 3 RIGHT arrow presses.


---

### TEST 3: HOME Key - Basic ASCII
**Input**: Press HOME key  
**Expected**: Cursor jumps to beginning of line  
**Visual Check**: Cursor should be on the 'h' (position 0)

**Result**: [x] PASS  [ ] FAIL  
**Notes**: HOME key correctly moved cursor to beginning (on top of 'h').


---

### TEST 4: END Key - Basic ASCII
**Input**: Press END key  
**Expected**: Cursor jumps to end of line  
**Visual Check**: Cursor should be after 'o'

**Result**: [x] PASS  [ ] FAIL  
**Notes**: END key correctly moved cursor to end (after 'o').


---

### TEST 5: LEFT Arrow - UTF-8 Multi-byte (2-byte)
**Input**: Clear line (Ctrl-U), type `café` then press LEFT arrow 2 times  
**Expected**: Cursor moves by CHARACTERS not bytes (should be on 'f')  
**Critical**: Should NOT break UTF-8 encoding, NO � replacement characters

**Result**: [x] PASS  [ ] FAIL  
**Notes**: Cursor correctly moved by characters (not bytes) and positioned on 'f'. Text "café" displayed correctly with no corruption.


---

### TEST 6: RIGHT Arrow - UTF-8 Multi-byte (2-byte)
**Input**: From TEST 5 position, press RIGHT arrow 2 times  
**Expected**: Cursor moves to end, passing over 'é' correctly  
**Visual Check**: Cursor should be after 'é'

**Result**: [x] PASS  [ ] FAIL  
**Notes**: RIGHT arrow correctly moved across both characters and placed cursor at end after 'café'.


---

### TEST 7: LEFT Arrow - UTF-8 CJK (3-byte)
**Input**: Clear line, type `北京` then press LEFT arrow once  
**Expected**: Cursor moves by CHARACTER not bytes (should be on '京')  
**Critical**: Should move by CHARACTER not bytes (3 bytes per character)

**Result**: [x] PASS  [ ] FAIL  
**Notes**: Cursor correctly moved by one character (3 bytes) and positioned on '京'. CJK support working properly.


---

### TEST 8: RIGHT Arrow - UTF-8 CJK (3-byte)
**Input**: From TEST 7 position, press RIGHT arrow once  
**Expected**: Cursor moves to end  
**Visual Check**: Cursor should be after 京

**Result**: [x] PASS  [ ] FAIL  
**Notes**: RIGHT arrow successfully moved across the '京' character and cursor placed at end.


---

### TEST 9: LEFT Arrow - Emoji (4-byte)
**Input**: Clear line, type `🎉🎊🎈` then press LEFT arrow twice  
**Expected**: Cursor moves by EMOJI not bytes (should be on 🎊)  
**Critical**: Each emoji is 4 bytes, should move by grapheme

**Result**: [x] PASS  [ ] FAIL  
**Notes**: LEFT arrow successfully moved two characters (8 bytes total). Cursor positioned on middle emoji '🎊'. All three emoji displayed correctly.


---

### TEST 10: RIGHT Arrow - Emoji (4-byte)
**Input**: From TEST 9 position, press RIGHT arrow twice  
**Expected**: Cursor moves to end  
**Visual Check**: Cursor should be after 🎈

**Result**: [x] PASS  [ ] FAIL  
**Notes**: RIGHT arrow successfully moved one character per press (two total) and placed cursor at end after '🎈'.


---

### TEST 11: LEFT Arrow - Complex Grapheme Cluster
**Input**: Clear line, type `👨‍👩‍👧‍👦` (family emoji, multi-codepoint) then press LEFT arrow once  
**Expected**: Cursor moves over ENTIRE grapheme cluster as one unit  
**Critical**: This is a complex emoji made of multiple codepoints with ZWJ joiners

**Result**: [x] PASS  [ ] FAIL  
**Notes**: LEFT arrow correctly moved over entire family emoji as ONE unit. Cursor jumped from end to beginning (on top of emoji). Visual cursor offset is a known terminal emulator rendering issue, not an LLE issue. Complex grapheme cluster handled correctly.


---

### TEST 12: HOME/END - UTF-8 Mixed
**Input**: Clear line, type `hello café 北京 🎉`  
**Action 1**: Press HOME  
**Expected 1**: Cursor at beginning (on 'h')  
**Action 2**: Press END  
**Expected 2**: Cursor at end (after 🎉)

**Result**: [x] PASS  [ ] FAIL  
**Notes**: HOME correctly jumped to beginning, END correctly jumped to end. Mixed UTF-8 content handled properly.


---

### TEST 13: Navigation Combination
**Input**: Clear line, type `test 测试 🎉`  
**Sequence**:
1. Press HOME (cursor at start)
2. Press RIGHT 5 times (should be at space before 测)
3. Press LEFT 2 times (should be on 't' in "test")
4. Press END (should be at end)

**Expected**: All movements work correctly with mixed ASCII/UTF-8/emoji

**Result**: [x] PASS  [ ] FAIL  
**Notes**: All navigation worked correctly through mixed ASCII/CJK/emoji. Visual cursor rendering issues observed but these are terminal emulator problems, not LLE issues. Logical cursor positions were correct throughout the test sequence.


---

### TEST 14: Empty Line Navigation
**Input**: Clear line (should be empty)  
**Actions**:
1. Press LEFT arrow (should do nothing or stay at position 0)
2. Press RIGHT arrow (should do nothing or stay at position 0)
3. Press HOME (should stay at position 0)
4. Press END (should stay at position 0)

**Expected**: No crashes, no errors, cursor stays at position 0

**Result**: [x] PASS  [ ] FAIL  
**Notes**: Empty line handled perfectly. All navigation keys worked without errors or crashes.


---

### TEST 15: Rapid Navigation
**Input**: Type `hello world`  
**Action**: Rapidly press LEFT arrow 11 times (one per character)  
**Expected**: Cursor reaches beginning, doesn't go past it, no crashes

**Result**: [x] PASS  [ ] FAIL  
**Notes**: Cursor reached beginning after exactly 11 LEFT arrow presses. Boundary handled correctly, did not go past position 0. Flawless pass.


---

## Test Summary

**Total Tests**: 15  
**Passed**: 15  
**Failed**: 0  
**Pass Rate**: 100%

**Critical Issues Found**:
None

**Minor Issues Found**:
- Visual cursor rendering with complex grapheme clusters and mixed CJK/emoji - Known terminal emulator rendering issue, NOT an LLE bug
- LLE logical cursor positions are correct in all cases

**Overall Assessment**: [x] Group 1 APPROVED  [ ] Group 1 NEEDS FIXES

---

## Notes for Next Session

**Key Findings**:
- All 4 Group 1 navigation keys (LEFT, RIGHT, HOME, END) work flawlessly through keybinding manager
- UTF-8 support fully intact (2-byte, 3-byte, 4-byte characters)
- Complex grapheme cluster support working correctly (family emoji with ZWJ)
- No regressions from hardcoded implementation
- Keybinding manager routing adds no noticeable latency
- Architecture is sound and ready for Group 2 expansion

**Recommendations for Group 2**:
- Continue with same incremental approach
- Test deletion operations thoroughly with UTF-8 to ensure no corruption
- Pay special attention to Ctrl-D EOF vs delete behavior

---

## Sign-off

**Tested By**: mberry  
**Date**: 2025-11-11  
**Ready for Group 2**: [x] YES  [ ] NO
