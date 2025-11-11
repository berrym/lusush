# Display System Grapheme Support Analysis

**Date**: 2025-11-11  
**Author**: AI Assistant (Session 11)  
**Issue**: Display system not grapheme-aware, causing emoji/CJK corruption  
**Severity**: CRITICAL - Blocks Phase 1 completion

---

## Executive Summary

The LLE display system (render pipeline + display bridge + screen buffer) is not grapheme-aware. It calculates cursor positions and renders characters using codepoint-based or byte-based logic, causing:

- Incorrect cursor positioning (appears offset or jumps to column 0)
- Emoji corruption (shows � replacement character)
- Partial grapheme deletion (breaks multi-codepoint sequences)
- Test failure rate: 6/7 tests fail (only simple 2-byte UTF-8 works)

**Root Cause**: Display width calculation and rendering logic assume 1 codepoint = 1 screen position, which is false for:
- CJK characters (2 columns wide)
- Emoji (1-2 columns wide)  
- Combining marks (0 additional columns)
- ZWJ sequences (must render atomically)
- Regional Indicator pairs (2 codepoints = 1 flag)

**Impact**: Users cannot reliably input emoji, CJK text, or any multi-codepoint graphemes.

**Fix Required**: Phase 2 display integration must implement grapheme-aware width calculation and atomic grapheme rendering.

---

## Problem Manifestation

### Test Evidence

| Character Type | Bytes | Codepoints | Graphemes | Display Cols | Current Result |
|----------------|-------|------------|-----------|--------------|----------------|
| café (é) | 5 | 4 | 4 | 4 | ✅ WORKS |
| 日本 | 6 | 2 | 2 | 4 | ❌ Cursor col 0, artifacts |
| 🎉🎊 | 8 | 2 | 2 | 4 | ❌ Cursor col 0, artifacts |
| 👨‍👩‍👧‍👦 | 25 | 7 | 1 | 2 | ❌ Cursor offset, col 0 jump |
| 🇺🇸 | 8 | 2 | 1 | 2 | ❌ Cursor offset, col 0 jump |
| Hello 世界 | 13 | 8 | 8 | 10 | ❌ Cursor col 0, artifacts |
| 👋🏽 | 8 | 2 | 1 | 2 | ❌ Cursor offset, col 0 jump |

### Failure Symptoms

**Symptom 1: Cursor Offset After Paste**
- **Observed**: Cursor appears 1-N positions to the right of where it should be
- **Example**: After pasting 👨‍👩‍👧‍👦, cursor should be at column 2, appears at column 7+
- **Cause**: Display calculates position as 7 codepoints instead of 1 grapheme

**Symptom 2: Cursor Jump to Column 0**
- **Observed**: Single left arrow press moves cursor to start of line
- **Example**: Type 日本, press left arrow → cursor at column 0
- **Cause**: Negative column calculation underflows or position calculation fails

**Symptom 3: � (U+FFFD) Replacement Character Artifacts**
- **Observed**: Backspace or navigation shows � symbol
- **Example**: Type 日本, backspace → shows � instead of 本
- **Cause**: Display system breaks multi-byte UTF-8 sequence, renders partial bytes

**Symptom 4: Two Backspaces Required**
- **Observed**: First backspace corrupts character, second backspace clears it
- **Example**: CJK or emoji requires two backspaces instead of one
- **Cause**: First backspace deletes some bytes, display shows invalid UTF-8, second backspace clears remainder

**Symptom 5: Emoji Display Corruption**
- **Observed**: Multi-codepoint emoji break apart during navigation
- **Example**: 👨‍👩‍👧‍👦 breaks into components or shows � when navigating
- **Cause**: Display renders ZWJ sequence components separately instead of atomically

---

## Architecture Analysis

### Current Display Pipeline

```
┌──────────────────────────────────────────────────────────────┐
│ 1. lle_readline.c                                            │
│    - User input events                                       │
│    - handle_character_input()                                │
│    - Buffer modifications                                    │
│    - refresh_display() called ← Entry point                  │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             v
┌──────────────────────────────────────────────────────────────┐
│ 2. Display Integration (Spec 08)                             │
│    File: src/lle/lle_readline.c:197-296                      │
│    Function: refresh_display()                               │
│    - Gets display_integration (global)                       │
│    - Gets render_controller                                  │
│    - Marks dirty regions in dirty_tracker                    │
│    - Calls lle_render_buffer_content()        ← Issue #1     │
│    - Passes &buffer->cursor                   ← Issue #2     │
│    - Sends to display_bridge                                 │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             v
┌──────────────────────────────────────────────────────────────┐
│ 3. Render Controller                                         │
│    Function: lle_render_buffer_content()                     │
│    - Receives buffer data                                    │
│    - Receives cursor position                                │
│    - Produces render_output                                  │
│    - ❌ LIKELY: Calculates positions by codepoints           │
│    - ❌ LIKELY: Doesn't understand grapheme clusters         │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             v
┌──────────────────────────────────────────────────────────────┐
│ 4. Display Bridge                                            │
│    File: src/lle/display_bridge.c                            │
│    Function: lle_display_bridge_send_output()                │
│    - Receives render_output                                  │
│    - Receives &buffer->cursor                                │
│    - Sends to command_layer                                  │
│    - ❌ LIKELY: Position calculations here too               │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             v
┌──────────────────────────────────────────────────────────────┐
│ 5. Command Layer / Display Controller                        │
│    - Merges prompt_layer content                             │
│    - Sends to terminal I/O                                   │
│    - ❌ POTENTIAL: screen_buffer involvement here            │
└──────────────────────────────────────────────────────────────┘
```

### Critical Issues in Pipeline

**Issue #1: lle_render_buffer_content() Not Grapheme-Aware**
- **Location**: Render controller
- **Problem**: Likely iterates over codepoints, not graphemes
- **Impact**: Produces wrong column positions for cursor
- **Evidence**: Cursor offset symptoms

**Issue #2: Cursor Position Passed Without Grapheme Context**
- **Location**: refresh_display() line 247
- **Code**: `&ctx->buffer->cursor`
- **Problem**: Cursor has byte_offset, codepoint_index, grapheme_index, but renderer may only use one
- **Impact**: If renderer uses codepoint_index for positioning, wrong column
- **Fix needed**: Ensure renderer uses grapheme_index

**Issue #3: Display Bridge May Recalculate Positions**
- **Location**: display_bridge.c:305+ (per error messages in build log)
- **Problem**: Bridge may convert grapheme positions back to codepoint positions
- **Impact**: Loses grapheme awareness
- **Evidence**: Build log shows undefined references to prompt_layer, command_layer functions

**Issue #4: Screen Buffer Legacy Code**
- **Context**: Per commit "screen_buffer developed for broken system"
- **Problem**: May have byte-based or codepoint-based indexing
- **Impact**: Incompatible with grapheme-based navigation
- **Action needed**: Audit screen_buffer implementation

---

## Width Calculation Problem

### The Core Issue

**Current (Broken) Assumption**:
```
cursor_screen_column = count_codepoints(text_from_start_to_cursor)
```

**Why It Fails**:
- 日本: 2 codepoints but 4 screen columns (CJK = 2 cols each)
- 👨‍👩‍👧‍👦: 7 codepoints but 2 screen columns (ZWJ sequence = 1 grapheme)
- é (composed): 1 codepoint, 1 column ✓ (works)
- é (decomposed): 2 codepoints (e + ́), 1 column (combining mark = 0 width)

**Required (Correct) Approach**:
```c
size_t calculate_display_width(const char *text, size_t byte_length) {
    size_t total_width = 0;
    size_t pos = 0;
    
    while (pos < byte_length) {
        // Find next grapheme boundary
        const char *grapheme_start = text + pos;
        const char *grapheme_end = find_next_grapheme_boundary(text, pos, byte_length);
        size_t grapheme_len = grapheme_end - grapheme_start;
        
        // Calculate width of this grapheme cluster
        int width = wcwidth_grapheme(grapheme_start, grapheme_len);
        if (width < 0) width = 1;  // Default for non-printable
        
        total_width += width;
        pos = grapheme_end - text;
    }
    
    return total_width;
}
```

### Width Rules by Character Type

| Type | Graphemes | Codepoints | Width | wcwidth Behavior |
|------|-----------|------------|-------|------------------|
| ASCII | 1 | 1 | 1 | wcwidth('a') = 1 |
| CJK | 1 | 1 | 2 | wcwidth('世') = 2 |
| Emoji (simple) | 1 | 1 | 2 | wcwidth(🎉) = 2 |
| Emoji (ZWJ) | 1 | N | 2 | Must calculate entire cluster |
| Combining mark | 0 | 1 | 0 | wcwidth(U+0301) = 0 |
| Flag emoji | 1 | 2 | 2 | Must recognize RI pair |
| Skin tone | 0 | 1 | 0 | Modifier adds to base |

**Critical Rules**:
1. **Never call wcwidth() on individual codepoints within a grapheme**
2. **Calculate width for entire grapheme cluster as unit**
3. **Use grapheme_index, not codepoint_index, for position tracking**
4. **Account for zero-width combining characters**

---

## Rendering Problem

### Atomic Grapheme Rendering

**Current (Broken)**:
```c
// Renders each codepoint separately
for (size_t i = 0; i < codepoint_count; i++) {
    render_codepoint(codepoints[i]);  // ← WRONG
}
```

**Why It Breaks ZWJ Sequences**:
- 👨‍👩‍👧‍👦 = MAN(👨) + ZWJ + WOMAN(👩) + ZWJ + GIRL(👧) + ZWJ + BOY(👦)
- Rendering each codepoint separately shows: 👨 👩 👧 👦 (separate)
- Should render as atomic unit: 👨‍👩‍👧‍👦 (combined)
- ZWJ (U+200D) tells renderer "join these into one glyph"

**Required (Correct)**:
```c
// Render grapheme clusters atomically
size_t pos = 0;
while (pos < buffer->length) {
    // Find next grapheme boundary
    size_t grapheme_start = pos;
    size_t grapheme_end = find_next_grapheme_boundary(buffer->data, pos, buffer->length);
    size_t grapheme_len = grapheme_end - grapheme_start;
    
    // Render entire grapheme cluster as atomic unit
    render_grapheme_cluster(buffer->data + grapheme_start, grapheme_len);
    
    pos = grapheme_end;
}
```

### Why Artifacts (�) Appear

**Scenario**: User types 日本, presses backspace

**What Should Happen**:
1. Buffer has: 0xE4 0xB8 0x96 (世) 0xE7 0x95 0x8C (界)
2. Cursor at byte 6 (after both characters)
3. Backspace: Delete bytes 3-5 (界)
4. Buffer now: 0xE4 0xB8 0x96 (世)
5. Display: 世

**What Actually Happens (Broken)**:
1. Buffer has: 0xE4 0xB8 0x96 0xE7 0x95 0x8C
2. Cursor position calculation wrong (thinks cursor at byte 2?)
3. Backspace: Deletes byte 2 (0x96) ← WRONG BYTE
4. Buffer now: 0xE4 0xB8 0xE7 0x95 0x8C ← INVALID UTF-8
5. Display tries to render: 0xE4 0xB8 (incomplete), 0xE7 0x95 0x8C (complete)
6. Shows: � 界 (replacement char + valid char)

**Root Cause**: Position calculation error causes wrong byte deletion

---

## Files Requiring Changes

### Primary Suspects (Need Audit)

**1. Render Controller**
- **File**: Unknown - need to locate lle_render_buffer_content()
- **Issue**: Likely uses codepoint-based iteration
- **Fix**: Convert to grapheme-based iteration
- **Priority**: CRITICAL

**2. Display Bridge**
- **File**: src/lle/display_bridge.c
- **Functions**: lle_display_bridge_send_output() (line 305+)
- **Issue**: May recalculate positions using codepoints
- **Fix**: Use grapheme_index from cursor, don't recalculate
- **Priority**: CRITICAL

**3. Screen Buffer**
- **File**: Unknown - need to locate screen_buffer implementation
- **Issue**: Per commit message, may be byte/codepoint indexed
- **Fix**: Audit, possibly rewrite for grapheme awareness
- **Priority**: HIGH

**4. Refresh Display**
- **File**: src/lle/lle_readline.c:197-296
- **Function**: refresh_display()
- **Issue**: Passes cursor, but renderer may not use grapheme_index
- **Fix**: Ensure render pipeline receives and uses grapheme position
- **Priority**: MEDIUM (interface change)

### Supporting Files (Likely Need Updates)

**5. Cursor Manager**
- **File**: src/lle/cursor_manager.c
- **Status**: ✅ Already grapheme-aware
- **Possible additions**: Helper functions for display system

**6. UTF-8 Index**
- **File**: src/lle/utf8_index.c
- **Status**: ✅ Already has grapheme mappings
- **Possible additions**: Display width lookup functions

**7. Unicode Grapheme**
- **File**: src/lle/unicode_grapheme.c
- **Status**: ✅ Boundary detection working
- **Possible additions**: Width calculation functions

---

## Implementation Plan

### Phase 2 Step 1: Audit and Locate

**Action**: Find the actual implementation files
- [ ] Locate lle_render_buffer_content() implementation
- [ ] Find screen_buffer implementation
- [ ] Identify all position calculation sites
- [ ] Map data flow from buffer to terminal

**Tools**:
- `grep -r "lle_render_buffer_content"` 
- `grep -r "screen_buffer"`
- `grep -r "wcwidth"`
- `grep -r "cursor.*column"`

### Phase 2 Step 2: Add Display Width Functions

**File**: src/lle/unicode_grapheme.c (or new file: display_width.c)

**Functions to Add**:
```c
/**
 * Calculate display width of a single grapheme cluster
 * Returns: Number of screen columns (0, 1, or 2)
 */
int lle_grapheme_display_width(const char *grapheme, size_t length);

/**
 * Calculate total display width from start to byte offset
 * Returns: Screen column position
 */
size_t lle_calculate_display_width(const char *text, size_t byte_offset);

/**
 * Find byte offset that corresponds to target screen column
 * Returns: Byte offset for grapheme starting at/before column
 */
size_t lle_display_column_to_byte(const char *text, size_t target_column);
```

**Implementation Notes**:
- Use `wcwidth()` but call on base character of grapheme
- Handle ZWJ sequences specially
- Cache width calculations in UTF-8 index if possible

### Phase 2 Step 3: Fix Render Controller

**File**: TBD (lle_render_buffer_content implementation)

**Changes**:
```c
// BEFORE (broken)
for (size_t i = 0; i < buffer->codepoint_count; i++) {
    // Process codepoint
}

// AFTER (fixed)
size_t grapheme_idx = 0;
size_t byte_pos = 0;
while (grapheme_idx < buffer->grapheme_count) {
    // Find grapheme byte range
    size_t grapheme_start = byte_pos;
    size_t grapheme_end = find_next_grapheme_boundary(buffer->data, byte_pos, buffer->length);
    
    // Calculate width
    int width = lle_grapheme_display_width(buffer->data + grapheme_start, 
                                           grapheme_end - grapheme_start);
    
    // Render atomically
    render_grapheme(buffer->data + grapheme_start, grapheme_end - grapheme_start, width);
    
    byte_pos = grapheme_end;
    grapheme_idx++;
}
```

### Phase 2 Step 4: Fix Cursor Position Calculation

**File**: Display bridge or render controller

**Changes**:
```c
// BEFORE (broken)
size_t cursor_column = cursor->codepoint_index;  // ← WRONG

// AFTER (fixed)
size_t cursor_column = lle_calculate_display_width(buffer->data, cursor->byte_offset);
```

**Alternative (if UTF-8 index has width cache)**:
```c
size_t cursor_column;
lle_utf8_index_grapheme_to_display(buffer->utf8_index, 
                                   cursor->grapheme_index, 
                                   &cursor_column);
```

### Phase 2 Step 5: Fix Display Bridge

**File**: src/lle/display_bridge.c

**Changes**:
- Don't recalculate positions from codepoints
- Trust the cursor->grapheme_index
- Use display width functions for column calculation
- Pass grapheme-based positions to terminal layer

### Phase 2 Step 6: Test and Validate

**Test Suite**:
```bash
# Run all 7 Phase 1 tests
./builddir/lusush
# Test 1: café ← Should still pass
# Test 2: 日本 ← Should now pass
# Test 3: 🎉🎊 ← Should now pass
# Test 4: 👨‍👩‍👧‍👦 ← Should now pass
# Test 5: 🇺🇸 ← Should now pass
# Test 6: Hello 世界 ← Should now pass
# Test 7: 👋🏽 ← Should now pass
```

**Validation Criteria**:
- [ ] 7/7 tests pass
- [ ] No cursor offset after paste
- [ ] No cursor jump to column 0
- [ ] No � artifacts
- [ ] Single backspace deletes entire grapheme
- [ ] Arrow keys move by grapheme
- [ ] Display shows correct character widths

---

## Screen Buffer Context

### Historical Note

Per commit message: "screen_buffer developed for broken system"

This suggests the screen_buffer was a workaround for terminal issues, possibly:
- Terminal didn't handle UTF-8 correctly
- Terminal had cursor positioning bugs
- Screen buffer implemented manual character tracking

**Implications**:
- Screen buffer may have byte-based or codepoint-based indexing
- May need significant rewrite or replacement
- Could be source of position calculation errors

**Action Required**:
- Locate screen_buffer implementation
- Audit for byte/codepoint assumptions
- Determine if rewrite needed or can be adapted

---

## Performance Considerations

### Width Calculation Cost

**O(n) Per Refresh**: Calculating display width requires scanning graphemes

**Optimization Strategies**:
1. **Cache in UTF-8 Index**: Add display_width field to grapheme cache
2. **Incremental Updates**: Only recalculate changed regions (dirty tracking already exists)
3. **Cursor Position Cache**: Store last known screen position, update incrementally

**Implementation**:
```c
// Add to utf8_index structure
typedef struct {
    size_t codepoint_index;
    size_t grapheme_index;
    size_t display_column;  // ← NEW: cached screen position
} lle_utf8_position_t;
```

### Rendering Optimization

**Current**: Dirty tracker marks changed regions ✓

**Additional**: 
- Only recalculate widths for dirty regions
- Cache grapheme boundaries (already done in utf8_index)
- Avoid redundant wcwidth() calls

---

## Testing Strategy

### Unit Tests to Add

**test_display_width.c**:
```c
void test_ascii_width() {
    assert(lle_grapheme_display_width("a", 1) == 1);
}

void test_cjk_width() {
    assert(lle_grapheme_display_width("世", 3) == 2);
}

void test_emoji_width() {
    assert(lle_grapheme_display_width("🎉", 4) == 2);
}

void test_zwj_sequence_width() {
    assert(lle_grapheme_display_width("👨‍👩‍👧‍👦", 25) == 2);
}

void test_combining_mark_width() {
    assert(lle_grapheme_display_width("e\u0301", 3) == 1);  // é composed
}

void test_buffer_display_width() {
    const char *text = "Hello 世界";
    assert(lle_calculate_display_width(text, strlen(text)) == 10);
    // H=1 e=1 l=1 l=1 o=1 space=1 世=2 界=2 = 10 columns
}
```

### Integration Tests

**test_display_cursor_position.c**:
- Insert various grapheme types
- Verify cursor screen position after each insert
- Test navigation (arrow keys) updates position correctly
- Test backspace updates position correctly

### Regression Tests

**test_phase1_scenarios.c**:
- Automated versions of all 7 manual tests
- Run as part of CI/CD
- Ensures fixes don't break as code evolves

---

## Risk Assessment

### High Risk Areas

**1. Screen Buffer Rewrite**
- **Risk**: May need complete rewrite
- **Impact**: Large code change, potential for new bugs
- **Mitigation**: Thorough testing, incremental changes

**2. Performance Regression**
- **Risk**: Width calculation adds O(n) per refresh
- **Impact**: Slower display updates
- **Mitigation**: Caching, dirty region optimization

**3. Render Pipeline Changes**
- **Risk**: Multiple files need coordinated changes
- **Impact**: Risk of introducing new bugs in other areas
- **Mitigation**: Change one component at a time, test after each

### Medium Risk Areas

**4. wcwidth() Limitations**
- **Risk**: wcwidth() not always correct for emoji
- **Impact**: Some emoji may display at wrong width
- **Mitigation**: Add special cases for known problem characters

**5. Terminal Compatibility**
- **Risk**: Different terminals render emoji differently
- **Impact**: Cursor may appear correct in one terminal, wrong in another
- **Mitigation**: Test on multiple terminals, document compatibility

---

## Success Criteria

### Phase 2 Display Integration Complete When:

- [ ] All 7 Phase 1 tests pass (7/7 PASS)
- [ ] No cursor positioning errors
- [ ] No � replacement character artifacts
- [ ] Emoji render correctly and stay intact during navigation
- [ ] CJK characters display at correct width (2 columns)
- [ ] Width calculation tests pass
- [ ] Rendering integrity tests pass
- [ ] No performance regression (display refresh < 16ms for typical input)
- [ ] Code review approved
- [ ] Documentation updated

### Ready to Merge When:

- [ ] All success criteria met
- [ ] Tested on multiple terminals (at least 3)
- [ ] No regressions in existing functionality
- [ ] Integration tests added to CI/CD
- [ ] PHASE1_TEST_RESULTS.md shows 7/7 PASS
- [ ] AI_ASSISTANT_HANDOFF_DOCUMENT.md updated
- [ ] User approves for merge

---

## References

### Relevant Specifications

- **UAX #29**: Unicode Text Segmentation (Grapheme Cluster Boundaries)
- **UAX #11**: East Asian Width (CJK character widths)
- **Unicode 15.0**: Emoji specifications (ZWJ, modifiers)
- **POSIX wcwidth()**: Wide character width function

### Relevant Files (From Investigation)

- `src/lle/lle_readline.c:197-296` - refresh_display()
- `src/lle/display_bridge.c` - Display bridge implementation
- `src/lle/cursor_manager.c` - Cursor position management (already grapheme-aware)
- `src/lle/unicode_grapheme.c` - Grapheme boundary detection (already working)
- `src/lle/buffer_management.c` - Buffer operations (data correct)

### Related Documents

- `docs/development/PHASE1_TEST_RESULTS.md` - Test execution results
- `docs/development/PHASE1_CURSOR_BUG_ANALYSIS.md` - Previous cursor analysis
- `docs/development/PHASE2_PLANNING.md` - Phase 2 roadmap
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - Project status

---

## Conclusion

The display system requires significant work to achieve grapheme awareness. This is not a simple bug fix but a fundamental architectural change to how the system calculates positions and renders text.

**Estimated Effort**: 8-16 hours

**Critical Path**: 
1. Locate render controller implementation
2. Add display width calculation functions
3. Convert rendering to grapheme-based iteration
4. Fix cursor position calculations
5. Test exhaustively

**Blocker Status**: Phase 1 cannot be considered complete until display system is fixed. Current 1/7 pass rate is not acceptable for production.

**Next Actions**: Begin Phase 2 Step 1 - Audit and locate implementation files.
