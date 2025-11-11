# Phase 1 Quick-Start Testing Guide

**Date**: 2025-11-11  
**Purpose**: Rapid verification of Phase 1 UTF-8/Grapheme functionality  
**Time Required**: 5-10 minutes  
**Audience**: Developer ready to test interactively

---

## Prerequisites

```bash
cd /home/mberry/Lab/c/lusush
git checkout feature/lle-utf8-grapheme
meson compile -C builddir lusush
```

---

## Quick Test Procedure

### Step 1: Launch Shell (30 seconds)

```bash
./builddir/lusush
```

You should see the lusush prompt. If it crashes immediately, Phase 1 has a critical bug.

**Expected**: Clean startup with prompt

---

### Step 2: Basic UTF-8 Test (1 minute)

Type these characters and test cursor movement:

```
é
```
- **Test**: Press LEFT arrow, then RIGHT arrow
- **Expected**: Cursor moves by whole character (not bytes)
- **Test**: Press BACKSPACE
- **Expected**: Entire character deleted

```
日本
```
- **Test**: Press LEFT arrow twice
- **Expected**: Cursor moves back 2 characters (not 6 bytes)
- **Test**: Press HOME, then END
- **Expected**: Cursor at start, then end

```
🎉🎊
```
- **Test**: Press LEFT arrow twice  
- **Expected**: Cursor moves back 2 emoji (not 8 bytes)
- **Test**: BACKSPACE twice
- **Expected**: Both emoji deleted

**Pass Criteria**: Cursor movement is by grapheme, not byte

---

### Step 3: Complex Graphemes (2 minutes)

Test grapheme cluster handling:

```bash
# Family emoji (ZWJ sequence)
👨‍👩‍👧‍👦
```
- **Test**: Press LEFT arrow once
- **Expected**: Cursor moves over ENTIRE family (not individual people)
- **Test**: BACKSPACE
- **Expected**: Entire family deleted, not just one person

```bash
# Flag emoji  
🇺🇸
```
- **Test**: LEFT arrow once
- **Expected**: Cursor moves over entire flag (not half)
- **Test**: BACKSPACE
- **Expected**: Entire flag deleted

```bash
# Combining characters (if terminal supports)
# Type 'e' then combining acute: e + ́ = é
```
- **Test**: BACKSPACE should delete both base and combining mark
- **Note**: May need special input method

**Pass Criteria**: Complex graphemes treated as single units

---

### Step 4: Mixed Content (1 minute)

Type a realistic command with mixed scripts:

```bash
echo "Hello 世界 🌍" | grep 日本
```

- **Test**: Use LEFT/RIGHT arrows to navigate
- **Expected**: Each grapheme is one cursor position
- **Test**: Use Ctrl-A (start) and Ctrl-E (end)
- **Expected**: Cursor jumps correctly
- **Test**: Use Ctrl-W to delete word
- **Expected**: Words deleted properly

**Pass Criteria**: Navigation works smoothly with mixed content

---

### Step 5: Buffer Modifications (1 minute)

Test that index stays synchronized:

```bash
# Type:
test
# Press HOME, then type:
世界
# Result should be:
世界test
```

- **Test**: Navigate with arrows after insertion
- **Expected**: Cursor positions remain accurate
- **Test**: Delete characters with backspace
- **Expected**: Characters delete cleanly, no corruption

**Pass Criteria**: No cursor position bugs after modifications

---

### Step 6: Multiline Commands (1 minute)

Test line structure tracking:

```bash
if true; then
  echo "Hello 世界"
fi
```

- **Test**: Use UP/DOWN arrows to move between lines
- **Expected**: Cursor positions correctly on each line
- **Test**: Edit middle line (add/delete text)
- **Expected**: Cursor tracking remains accurate

**Pass Criteria**: Multiline editing works correctly

---

## Quick Result Summary

| Test | Status | Notes |
|------|--------|-------|
| 1. Shell Launch | ⬜ Pass / ⬜ Fail | |
| 2. Basic UTF-8 | ⬜ Pass / ⬜ Fail | |
| 3. Complex Graphemes | ⬜ Pass / ⬜ Fail | |
| 4. Mixed Content | ⬜ Pass / ⬜ Fail | |
| 5. Buffer Modifications | ⬜ Pass / ⬜ Fail | |
| 6. Multiline Commands | ⬜ Pass / ⬜ Fail | |

**Overall Assessment**: ⬜ PASS - Ready for Phase 2  /  ⬜ FAIL - Needs fixes

---

## Common Issues and Debugging

### Issue: Cursor positions are wrong

**Symptom**: Cursor appears at wrong location after typing UTF-8

**Diagnosis**:
```bash
# Check if index is being used
gdb ./builddir/lusush
break lle_utf8_index_byte_to_codepoint
run
# Type UTF-8 character
# If breakpoint doesn't hit, index isn't being called
```

**Likely Cause**: Index not integrated into cursor_manager.c

---

### Issue: Characters display as � (replacement character)

**Symptom**: UTF-8 characters show as boxes or �

**Diagnosis**: Not a Phase 1 bug - check terminal/locale:
```bash
echo $LANG
locale charmap
```

**Fix**: Ensure UTF-8 locale:
```bash
export LANG=en_US.UTF-8
```

---

### Issue: Backspace deletes partial grapheme

**Symptom**: Family emoji 👨‍👩‍👧‍👦 breaks into individual people when backspacing

**Diagnosis**: Grapheme boundary detection failure

**Likely Cause**: 
- GB11 rule (ZWJ sequences) not implemented correctly in grapheme.c
- Check tests/lle/compliance/spec_03_utf8_index_test.c

---

### Issue: Crash when typing UTF-8

**Symptom**: Shell crashes or corrupts buffer

**Diagnosis**: Critical bug - memory corruption or null pointer

**Debug Steps**:
```bash
gdb ./builddir/lusush
run
# Type problematic input
# Check backtrace with 'bt'
```

**Likely Causes**:
- Buffer reallocation not updating utf8_index pointer
- Index arrays not resized correctly
- Null pointer when index not initialized

---

## Performance Check (Optional)

If you notice slowness with large buffers:

```bash
# Type a long line with mixed UTF-8
echo "日本日本日本..." # (repeat 100 times)
```

- **Expected**: No noticeable lag
- **If slow**: Index rebuild may be O(n²) instead of O(n)

---

## Next Steps After Testing

### If ALL tests PASS:
1. Update PHASE1_TEST_RESULTS.md with "PASS"
2. Proceed to Phase 2: Display Integration
3. See docs/development/PHASE2_PLANNING.md

### If ANY tests FAIL:
1. Document failure in PHASE1_TEST_RESULTS.md
2. Create GitHub issue with reproduction steps
3. Debug using gdb and check implementation
4. Do NOT proceed to Phase 2 until fixed

---

## Automated Test Helper

For byte-level verification:

```bash
cd /home/mberry/Lab/c/lusush
./tests/manual/utf8_test_helper.sh
```

This shows:
- Byte sequences for test inputs
- Expected grapheme counts
- Character decompositions

---

## Contact / Questions

- See comprehensive test strategy: `docs/development/PHASE1_TESTING_STRATEGY.md`
- See full implementation details: `docs/development/PHASE1_UTF8_GRAPHEME_COMPLETE.md`
- Report bugs to: Project issue tracker

---

**Tester Notes**:

Date tested: ___________  
Terminal: ___________  
Locale: ___________  
Overall result: ___________  
Issues found: ___________
