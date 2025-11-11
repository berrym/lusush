# Phase 1 UTF-8/Grapheme Testing Strategy

**Date**: 2025-11-11  
**Status**: Testing Required Before Production  
**Priority**: CRITICAL

---

## Overview

Phase 1 introduces fundamental changes to text handling. Comprehensive testing is **mandatory** before production deployment to prevent data corruption, crashes, or incorrect behavior.

**Testing Philosophy**: Trust but verify. The code compiles and passes compliance tests, but real-world Unicode input can expose edge cases.

---

## Test Environment Setup

### Build Test Binary
```bash
cd /home/mberry/Lab/c/lusush
meson compile -C builddir lusush
./builddir/lusush
```

### Enable Debug Logging (if available)
```bash
export LLE_DEBUG=1
export LLE_UTF8_DEBUG=1
```

### Prepare Test Data
Create test files with various Unicode content:
```bash
# test_utf8.txt
echo "Hello 世界 🌍" > test_utf8.txt
echo "Café naïve" >> test_utf8.txt
echo "👨‍👩‍👧‍👦 family" >> test_utf8.txt
```

---

## Test Suite

### Level 1: Basic UTF-8 Input (CRITICAL)

**Purpose**: Verify basic multi-byte character handling

**Test Cases**:

1. **Single Multi-Byte Character**
   ```
   Input: é (2 bytes: 0xC3 0xA9)
   Expected: Cursor moves 1 position
   Verify: Backspace deletes entire character
   ```

2. **3-Byte Character (CJK)**
   ```
   Input: 日 (3 bytes: 0xE6 0x97 0xA5)
   Expected: Cursor moves 1 position, displays as 2 columns wide
   Verify: Left/right arrow moves by grapheme
   ```

3. **4-Byte Character (Emoji)**
   ```
   Input: 🎉 (4 bytes: 0xF0 0x9F 0x8E 0x89)
   Expected: Cursor moves 1 position, displays as 2 columns wide
   Verify: Backspace removes entire emoji
   ```

4. **Mixed ASCII and UTF-8**
   ```
   Input: Hello 世界
   Expected: Cursor at end shows position 8 (not 13 bytes)
   Verify: Ctrl-A moves to start, Ctrl-E to end
   ```

**Pass Criteria**: All characters display correctly, cursor movements are grapheme-based

### Level 2: Combining Characters (HIGH PRIORITY)

**Purpose**: Verify grapheme cluster detection

**Test Cases**:

1. **Single Combining Mark**
   ```
   Input: e + ́ (U+0301 COMBINING ACUTE ACCENT)
   Result: é (single grapheme cluster)
   Expected: Cursor treats as one position
   Verify: Backspace deletes both base and combining mark
   ```

2. **Multiple Combining Marks**
   ```
   Input: e + ̀ + ̂ (grave + circumflex)
   Result: ề (single grapheme cluster)
   Expected: Single cursor position
   Verify: Arrow keys skip over entire cluster
   ```

3. **Combining Marks Mid-Line**
   ```
   Input: café where é = e + ́
   Expected: 4 cursor positions (not 5)
   Verify: Position calculations account for combining marks
   ```

**Pass Criteria**: Combining marks never treated as separate cursor positions

### Level 3: Emoji Sequences (HIGH PRIORITY)

**Purpose**: Verify complex grapheme boundary detection

**Test Cases**:

1. **Emoji with Skin Tone Modifier**
   ```
   Input: 👨🏽 (MAN + MEDIUM SKIN TONE)
   Bytes: F0 9F 91 A8 F0 9F 8F BD (8 bytes)
   Expected: 1 grapheme cluster, 1 cursor position
   Verify: Backspace removes entire sequence
   ```

2. **ZWJ Emoji Sequence**
   ```
   Input: 👨‍👩‍👧‍👦 (FAMILY: MAN, WOMAN, GIRL, BOY)
   Bytes: 25 bytes total
   Expected: 1 grapheme cluster, 1 cursor position
   Verify: Arrow keys skip entire family
   ```

3. **Regional Indicator Pairs (Flags)**
   ```
   Input: 🇺🇸 (US FLAG = Regional Indicator U + S)
   Bytes: 8 bytes (2 regional indicators)
   Expected: 1 grapheme cluster
   Verify: Cannot split flag with cursor
   ```

4. **Mixed Emoji and Text**
   ```
   Input: Hello 🌍 World
   Expected: Cursor position 6 at globe (not byte offset 9)
   Verify: Proper spacing around wide emoji
   ```

**Pass Criteria**: All emoji sequences treated as atomic units

### Level 4: Buffer Modifications (CRITICAL)

**Purpose**: Verify index invalidation and rebuild

**Test Cases**:

1. **Insert at Start**
   ```
   Initial: "world"
   Action: Move to start, type "Hello "
   Expected: "Hello world", cursor at position 6
   Verify: No cursor position corruption
   ```

2. **Insert in Middle**
   ```
   Initial: "Hello world"
   Action: Move to position 6, type "beautiful "
   Expected: "Hello beautiful world"
   Verify: Cursor tracks correctly after insertion
   ```

3. **Delete at End**
   ```
   Initial: "Hello world"
   Action: Move to end, backspace 5 times
   Expected: "Hello "
   Verify: Cursor position updated correctly
   ```

4. **Replace Selection** (if implemented)
   ```
   Initial: "Hello world"
   Action: Select "world", type "universe"
   Expected: "Hello universe"
   Verify: Index rebuild handles replace operation
   ```

5. **Rapid Insert/Delete**
   ```
   Action: Type "test", backspace 4, type "demo", backspace 4
   Expected: Empty buffer
   Verify: No index corruption from rapid invalidation
   ```

**Pass Criteria**: Index always stays synchronized with buffer content

### Level 5: Multiline Commands (HIGH PRIORITY)

**Purpose**: Verify line structure tracking integration

**Test Cases**:

1. **Simple If Statement**
   ```bash
   if true; then
   [ENTER - should show PS2 prompt]
       echo "hello"
   [ENTER]
   fi
   [ENTER - should execute]
   ```
   Verify: 
   - Continuation prompt appears after "then"
   - Line count tracks correctly
   - Command executes when complete

2. **Case Statement**
   ```bash
   case $var in
   [ENTER]
       pattern)
   [ENTER]
           echo "match"
   [ENTER]
           ;;
   [ENTER]
   esac
   [ENTER - should execute]
   ```
   Verify:
   - Multiple continuation prompts
   - Nested construct tracking
   - Proper completion detection

3. **Here Document**
   ```bash
   cat <<EOF
   [ENTER]
   Line 1
   Line 2
   EOF
   [ENTER - should execute]
   ```
   Verify:
   - Here-doc delimiter tracking
   - Line structure includes here-doc content
   - Completion at matching delimiter

4. **Line Editing in Multiline**
   ```
   Initial: Line 1 of if statement
   Action: Up arrow to previous line, edit, down arrow
   Expected: Returns to current line, edits preserved
   Verify: Line structure updates after edit
   ```

**Pass Criteria**: Shell construct detection matches core lusush behavior

### Level 6: Display Width (MEDIUM PRIORITY)

**Purpose**: Verify East Asian Width calculations

**Test Cases**:

1. **CJK Ideographs**
   ```
   Input: 日本語 (3 characters)
   Expected: 6 display columns (2 per character)
   Verify: Cursor position aligns with display
   ```

2. **Halfwidth vs Fullwidth**
   ```
   Input: ｶﾀｶﾅ (halfwidth katakana)
   Expected: 4 display columns (1 per character)
   Compare: カタカナ (fullwidth) = 8 columns
   ```

3. **Mixed Width Line**
   ```
   Input: Hello 日本 World
   Expected: Proper spacing (5 + 1 + 4 + 1 + 5 = 16 columns)
   Verify: Cursor aligns visually
   ```

**Pass Criteria**: Display width matches visual terminal width

### Level 7: Edge Cases (MEDIUM PRIORITY)

**Purpose**: Stress test boundary conditions

**Test Cases**:

1. **Very Long Line**
   ```
   Input: 10,000 character line
   Expected: No performance degradation
   Verify: Index rebuild completes in < 100ms
   ```

2. **Many Short Lines**
   ```
   Input: 100 lines of 10 characters each
   Expected: Line structure handles many lines
   Verify: No memory leaks
   ```

3. **Pathological Unicode**
   ```
   Input: Invalid UTF-8 sequences
   Expected: Validation rejects, no crash
   Verify: Buffer remains in consistent state
   ```

4. **Overlong Encodings** (security)
   ```
   Input: 0xC0 0x80 (overlong encoding of NULL)
   Expected: Rejected as invalid UTF-8
   Verify: No buffer overflow or injection
   ```

5. **Surrogate Pairs** (invalid in UTF-8)
   ```
   Input: 0xED 0xA0 0x80 (high surrogate)
   Expected: Rejected as invalid UTF-8
   Verify: No processing of invalid data
   ```

**Pass Criteria**: All edge cases handled gracefully, no crashes

### Level 8: Performance (LOW PRIORITY)

**Purpose**: Verify no performance regressions

**Test Cases**:

1. **Index Rebuild Time**
   ```
   Setup: 1000 character buffer
   Action: Invalidate and force rebuild
   Expected: < 1ms on modern hardware
   Measure: Check total_rebuild_time_ns
   ```

2. **Cursor Movement**
   ```
   Setup: 1000 character buffer with valid index
   Action: Move cursor from start to end
   Expected: O(1) per movement (< 1μs each)
   Verify: No O(n) scanning
   ```

3. **Incremental Typing**
   ```
   Action: Type 100 characters one at a time
   Expected: Each insert < 100μs
   Verify: No O(n²) behavior
   ```

4. **Memory Usage**
   ```
   Setup: 1000 character buffer
   Expected: ~64KB index overhead
   Verify: No memory leaks on repeated invalidation
   ```

**Pass Criteria**: Performance acceptable for interactive shell use

---

## Test Execution

### Manual Testing

**Recommended Approach**:
1. Start with Level 1 tests (basic UTF-8)
2. Progress to Level 2-3 (combining marks, emoji)
3. Test Level 4 (buffer modifications) thoroughly
4. Verify Level 5 (multiline) matches core shell
5. Check Level 6 (display) if visual issues noticed
6. Run Level 7 (edge cases) before declaring production-ready

**Time Estimate**: 2-4 hours for thorough manual testing

### Automated Testing (Future)

**Test Framework Needed**:
- Mock terminal input/output
- UTF-8 test corpus (Unicode.org test files)
- Grapheme boundary test suite (UAX #29 test data)
- Performance benchmarks

**Priority**: Medium - Manual testing is sufficient for Phase 1

---

## Test Reporting

### What to Record

For each test case:
1. **Input**: Exact bytes or characters typed
2. **Expected**: What should happen
3. **Actual**: What actually happened
4. **Status**: PASS / FAIL / PARTIAL
5. **Notes**: Any unusual behavior

### Example Test Report
```
Test: Level 2, Case 1 - Single Combining Mark
Input: e + U+0301 (COMBINING ACUTE ACCENT)
Expected: Single grapheme cluster, 1 cursor position
Actual: Cursor shows 2 positions, backspace only deletes mark
Status: FAIL
Notes: Grapheme detection not working, need to debug GB9 rule
```

### Failure Triage

**Critical Failures** (block production):
- Cursor position corruption
- Data loss on buffer modifications
- Crashes on valid UTF-8 input
- Security issues (overlong encodings accepted)

**High Priority Failures** (fix soon):
- Emoji sequences not detected
- Combining marks treated separately
- Multiline tracking incorrect

**Medium Priority** (can defer):
- Display width calculations off by 1
- Performance slower than expected

**Low Priority** (cosmetic):
- Minor visual alignment issues

---

## Debugging Aids

### Enable UTF-8 Index Logging

Add to `utf8_index.c`:
```c
#ifdef LLE_UTF8_DEBUG
    fprintf(stderr, "Index rebuild: %zu bytes -> %zu codepoints -> %zu graphemes\n",
            byte_count, codepoint_count, grapheme_count);
#endif
```

### Cursor Position Dump

Add command to dump cursor state:
```bash
# In lusush interactive mode
^X ^D  # Ctrl-X Ctrl-D to dump debug info
```

Should print:
```
Cursor Position:
  byte_offset: 15
  codepoint_index: 10
  grapheme_index: 8
  line: 0, column: 8
  Index valid: true
  Index stats: 3 rebuilds, 2.5ms total
```

### Buffer Hex Dump

For debugging UTF-8 sequences:
```bash
# In lusush
^X ^H  # Ctrl-X Ctrl-H for hex dump
```

Should show:
```
Buffer (15 bytes):
  48 65 6C 6C 6F 20 E4 B8 96 E7 95 8C 21
  H  e  l  l  o     世(   )  界(   )  !
```

---

## Success Criteria

Phase 1 is **ready for production** when:

1. ✅ All Level 1 tests PASS (basic UTF-8)
2. ✅ All Level 2 tests PASS (combining marks)
3. ✅ All Level 3 tests PASS (emoji sequences)
4. ✅ All Level 4 tests PASS (buffer modifications)
5. ✅ All Level 5 tests PASS (multiline)
6. ✅ No critical or high-priority failures
7. ✅ Performance acceptable (< 100ms for typical operations)
8. ✅ No memory leaks detected

**Current Status**: ⚠️ UNTESTED - Manual testing required

---

## Next Actions

1. **Immediate**: Run Level 1-4 tests manually
2. **Within 24h**: Complete Level 5 (multiline) tests
3. **Before merge**: Run all levels, document results
4. **Before production**: Have second person verify tests

**Estimated Effort**: 3-4 hours total testing time

---

**Document Version**: 1.0  
**Author**: AI Assistant (Claude)  
**Status**: Ready for Test Execution
