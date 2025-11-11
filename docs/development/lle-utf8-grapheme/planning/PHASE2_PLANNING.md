# Phase 2 Planning: Building on UTF-8/Grapheme Foundation

**Date**: 2025-11-11  
**Status**: PLANNING - Not Yet Started  
**Prerequisites**: Phase 1 Complete + Tested

---

## Overview

Phase 2 builds on the UTF-8/grapheme foundation to deliver user-visible improvements. The goal is to leverage Phase 1's infrastructure for better editing experience.

**Key Principle**: Phase 1 gave us the tools, Phase 2 makes them useful to users.

---

## Option Analysis

### Option A: Display Integration ⭐ RECOMMENDED

**Goal**: Make grapheme-aware rendering visible to users

**What It Delivers**:
- Wide characters (CJK, emoji) display correctly
- Cursor aligns with visual position
- Wrapping respects grapheme boundaries
- Proper column calculations for prompts

**Why This First**:
- Directly leverages `grapheme_to_display` mappings from Phase 1
- High user impact (visible improvement)
- Foundation for all future display work
- Relatively low risk (rendering only, no data changes)

**Estimated Effort**: 3-5 days

**Components**:
1. Update render controller to use display width
2. Implement grapheme-aware wrapping
3. Fix cursor rendering for wide characters
4. Test with CJK and emoji content

**Dependencies**:
- Phase 1 `char_width.c` (already done)
- Display controller refactoring (may be needed)

**Risk**: Medium - Display code is complex, but changes are localized

---

### Option B: Advanced Editing Operations

**Goal**: Grapheme-aware editing commands

**What It Delivers**:
- Backspace deletes entire grapheme cluster (not just last byte)
- Delete key respects grapheme boundaries
- Word movement (Ctrl-Left/Right) by grapheme
- Selection treats graphemes as atoms

**Why Defer**:
- Requires more invasive changes to keybinding handlers
- Less visible than display improvements
- Can build on display work from Option A

**Estimated Effort**: 4-6 days

**Components**:
1. Update delete operations to use grapheme boundaries
2. Implement grapheme-aware word detection
3. Update selection logic
4. Add grapheme-aware kill/yank

**Dependencies**:
- Phase 1 index (done)
- Keybinding system refactoring (may be needed)

**Risk**: Medium-High - Editing operations are critical, bugs cause data loss

---

### Option C: Performance Optimization

**Goal**: Reduce index rebuild overhead

**What It Delivers**:
- Incremental index updates (don't rebuild entire index)
- Partial line re-analysis (only modified lines)
- Index caching across buffer versions
- Faster response during rapid typing

**Why Defer**:
- Current performance likely acceptable for shell input
- Optimization without profiling data is premature
- Need real-world usage patterns first

**Estimated Effort**: 5-7 days

**Components**:
1. Implement incremental index updates
2. Add modification range tracking
3. Optimize grapheme boundary detection hot paths
4. Add performance instrumentation

**Dependencies**:
- Profiling data from production use
- Performance testing infrastructure

**Risk**: Medium - Optimization can introduce subtle bugs

---

### Option D: Autosuggestions Integration

**Goal**: UTF-8 aware command suggestions

**What It Delivers**:
- Suggestions consider grapheme positions
- Proper display of wide-character suggestions
- Cursor positioning in suggested text
- Match highlighting respects boundaries

**Why Defer**:
- Autosuggestions system not yet implemented
- Requires display integration (Option A) first
- Benefits are secondary to core editing

**Estimated Effort**: 6-8 days (including autosuggestion system)

**Components**:
1. Design autosuggestion API
2. Implement suggestion matching with grapheme awareness
3. Integrate with display system
4. Add suggestion rendering

**Dependencies**:
- Option A (Display Integration) completed
- History system working
- Completion database

**Risk**: High - Large new feature with many edge cases

---

## Recommended Phase 2: Display Integration

### Detailed Plan

#### Step 1: Audit Current Display Code

**Goal**: Understand what needs to change

**Actions**:
1. Review `src/lle/display_integration.c`
2. Find all byte-based width calculations
3. Identify cursor rendering logic
4. Check wrapping implementation

**Deliverables**:
- Document: `DISPLAY_AUDIT.md` listing all changes needed
- Estimate: Updated effort estimate based on audit

**Time**: 2-4 hours

#### Step 2: Implement Display Width Functions

**Goal**: Replace byte-based with grapheme-based width

**Actions**:
1. Add `lle_buffer_get_display_width(buffer, start, end)`
2. Update line rendering to use display width
3. Fix prompt alignment calculations
4. Handle wide character boundaries

**Deliverables**:
- Functions in `buffer_management.c` or new `display_utils.c`
- Unit tests for width calculations

**Time**: 1 day

#### Step 3: Fix Cursor Rendering

**Goal**: Cursor appears at correct visual position

**Actions**:
1. Use `grapheme_to_display` for cursor position
2. Handle wide characters (cursor before/after)
3. Test with CJK and emoji
4. Fix any off-by-one errors

**Deliverables**:
- Updated cursor rendering code
- Test cases for wide characters

**Time**: 1-2 days

#### Step 4: Implement Grapheme-Aware Wrapping

**Goal**: Long lines wrap at grapheme boundaries

**Actions**:
1. Find wrap points using grapheme index
2. Never wrap in middle of grapheme cluster
3. Handle wide characters at wrap boundary
4. Test with various terminal widths

**Deliverables**:
- Wrapping logic in display controller
- Test cases for boundary conditions

**Time**: 1-2 days

#### Step 5: Testing & Refinement

**Goal**: Verify display works correctly

**Actions**:
1. Run display test suite
2. Manual testing with CJK/emoji content
3. Test on different terminal emulators
4. Fix visual glitches

**Deliverables**:
- Test report documenting all cases
- Bug fixes for discovered issues

**Time**: 1 day

#### Step 6: Documentation

**Goal**: Document Phase 2 completion

**Actions**:
1. Write `PHASE2_DISPLAY_COMPLETE.md`
2. Update handoff document
3. Document known limitations
4. Plan Phase 3 (if needed)

**Deliverables**:
- Complete documentation
- Recommendations for next phase

**Time**: 2-3 hours

---

## Success Criteria

Phase 2 (Display Integration) is complete when:

1. ✅ Wide characters (CJK, emoji) display at correct width
2. ✅ Cursor aligns visually with text
3. ✅ Wrapping respects grapheme boundaries
4. ✅ Prompt alignment correct for all character types
5. ✅ No visual glitches with multi-byte characters
6. ✅ All display tests PASS
7. ✅ Performance acceptable (no visible lag)

---

## Alternative Phase 2 Options (If Display Not Needed)

### Quick Wins

If display integration isn't urgent, consider these smaller improvements:

1. **Grapheme-Aware Backspace** (1-2 days)
   - Single focused improvement
   - High user visibility
   - Low risk

2. **Index Statistics Monitoring** (1 day)
   - Add `/stats` command to show index performance
   - Useful for debugging and optimization
   - Helps identify real bottlenecks

3. **Extended Unicode Testing** (2 days)
   - Build comprehensive test suite
   - Use Unicode.org test data
   - Catch edge cases before users do

4. **Performance Profiling** (1-2 days)
   - Measure actual index rebuild times
   - Profile cursor movement operations
   - Identify optimization opportunities

---

## Phase 3 Possibilities (Future)

### After Phase 2 Display

1. **Advanced Editing** (Option B above)
2. **Autosuggestions** (Option D above)
3. **Syntax Highlighting** (needs grapheme awareness)
4. **Inline Previews** (command expansion, file previews)

### After Phase 2 Performance

1. **Large File Handling** (>10MB buffers)
2. **Real-Time Syntax Validation** (as you type)
3. **Advanced History Search** (fuzzy matching)

---

## Risk Mitigation

### Technical Risks

1. **Display Code Complexity**
   - Mitigation: Thorough audit before starting
   - Fallback: Keep old display code, add feature flag

2. **Terminal Compatibility**
   - Mitigation: Test on multiple terminals (xterm, gnome-terminal, kitty)
   - Fallback: Detect terminal capabilities, graceful degradation

3. **Performance Regression**
   - Mitigation: Benchmark before/after
   - Fallback: Profile and optimize hot paths

### Process Risks

1. **Scope Creep**
   - Mitigation: Stick to defined steps, defer "nice-to-haves"
   - Strategy: Incremental delivery, test after each step

2. **Testing Burden**
   - Mitigation: Automate what we can, manual test critical paths
   - Strategy: Prioritize high-impact test cases

3. **Context Loss**
   - Mitigation: Document as we go, update handoff doc
   - Strategy: This planning document prevents rediscovery

---

## Resource Requirements

### Time Investment

- **Phase 2 (Display)**: 3-5 days focused work
- **Testing**: 1-2 days
- **Documentation**: 0.5 days
- **Total**: 4.5-7.5 days

### Skills Needed

- Understanding of terminal escape sequences
- Knowledge of display rendering (Phase 1 helps)
- Debugging visual issues
- Testing across platforms

### Dependencies

- Phase 1 complete and tested ✅
- Display controller accessible
- Test environment available

---

## Decision Framework

### Choose Display Integration (Option A) If:
- Users report visual issues with UTF-8 characters
- CJK or emoji support is important
- Building foundation for future display features
- Want high-impact visible improvement

### Choose Advanced Editing (Option B) If:
- Users complain about editing multi-byte characters
- Data integrity is top concern
- Want to complete UTF-8 editing story
- Display already works adequately

### Choose Performance (Option C) If:
- Profiling shows index rebuild bottleneck
- Users report lag during typing
- Have specific performance targets to meet
- Other features working well

### Choose Autosuggestions (Option D) If:
- Autosuggestion system already exists
- Display integration complete
- Want high-value user feature
- Have 2+ weeks for implementation

---

## Recommendation

**Start with Option A: Display Integration**

**Rationale**:
1. Highest user visibility (everyone sees display)
2. Builds naturally on Phase 1 work
3. Foundation for future features
4. Lower risk than editing operations
5. Manageable scope (4-7 days)

**Next Step**: Run Phase 1 tests, then begin Display Audit (Step 1)

---

**Document Version**: 1.0  
**Author**: AI Assistant (Claude)  
**Review Status**: Pending user approval and Phase 1 test results
