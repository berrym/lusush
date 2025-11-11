# Phase 1 Cursor Synchronization Bug - Technical Analysis

**Issue ID**: ISSUE-001  
**Severity**: Medium (UX impact, no data corruption)  
**Status**: Documented - Fix deferred to Phase 2  
**Date**: 2025-11-11  
**Branch**: feature/lle-utf8-grapheme

---

## Executive Summary

Phase 1 testing revealed a cursor synchronization bug affecting paste operations with multi-codepoint grapheme clusters. The bug causes cursor to appear at wrong position after pasting complex emoji (family, flags, skin tones), but does NOT affect:
- Manual typing (works perfectly)
- Simple emoji paste (works perfectly)
- Navigation after paste (works perfectly)
- Data integrity (no corruption)

**Impact**: 3 of 7 manual tests fail due to cursor positioning  
**Workaround**: Press any arrow key after paste to fix cursor  
**Root Cause**: Buffer insert only updates byte_offset, not codepoint_index/grapheme_index  
**Fix Complexity**: Medium (architectural change needed)  
**Recommendation**: Defer to Phase 2 - Display Integration

---

## Technical Details

### The Cursor Structure

Located in `include/lle/buffer_management.h`, the cursor has three synchronized fields:

```c
typedef struct {
    size_t byte_offset;        // Byte position in buffer
    size_t codepoint_index;    // Codepoint index (UTF-8 aware)
    size_t grapheme_index;     // Grapheme cluster index (UAX #29 aware)
    size_t line;               // Current line number
    size_t column;             // Column in current line
} lle_cursor_t;
```

**Critical Requirement**: All three position fields must stay synchronized:
- `byte_offset` = raw byte position
- `codepoint_index` = which Unicode character (accounting for multi-byte)
- `grapheme_index` = which grapheme cluster (accounting for combining/ZWJ/etc.)

### The Bug

**Location**: `src/lle/buffer_management.c:642-644`

```c
/* Step 8: Update cursor if after insertion point */
if (buffer->cursor.byte_offset >= position) {
    buffer->cursor.byte_offset += text_length;  // ← ONLY THIS LINE!
}
```

**Problem**: Only `byte_offset` is updated. The other two fields become stale.

### Reproduction Case

**Example**: Paste family emoji 👨‍👩‍👧‍👦 (MAN + ZWJ + WOMAN + ZWJ + GIRL + ZWJ + BOY)

```
Composition:
- 7 Unicode codepoints (U+1F468, U+200D, U+1F469, U+200D, U+1F467, U+200D, U+1F466)
- 25 bytes total (4+3+4+3+4+3+4)
- 1 grapheme cluster (ZWJ sequence)

After paste at position 0:
- byte_offset: 0 → 25 ✓ (correct)
- codepoint_index: 0 → 0 ✗ (stale! should be 7)
- grapheme_index: 0 → 0 ✗ (stale! should be 1)

Display code uses grapheme_index for positioning:
- Thinks cursor at grapheme 0
- Actually at grapheme 1
- Result: Cursor drawn 25 bytes to the left of actual position
```

### Why Manual Typing Works

When typing characters manually, each character triggers:
1. Key event → `handle_character_input()`
2. `lle_buffer_insert_text()` (updates byte_offset only)
3. `refresh_display()` (renders)
4. For next character, cursor moves via arrow/movement handler
5. Movement handlers use `lle_cursor_manager_move_by_graphemes()`
6. Cursor manager synchronizes all three fields ✓

The key: Movement between characters uses proper synchronization.

### Why Paste Breaks

When pasting multi-codepoint text:
1. Entire text inserted as one operation
2. `byte_offset` increases by full text length
3. `codepoint_index` and `grapheme_index` never updated
4. No movement happens (cursor stays at end of paste)
5. Display uses stale `grapheme_index` → wrong position

### Why Manual Correction Fixes It

After paste, pressing any navigation key (arrows, backspace, etc.):
1. Arrow keys: Call `lle_cursor_manager_move_by_graphemes(+1 or -1)`
2. Backspace: Deletes character, then updates cursor via cursor_manager
3. All navigation handlers use cursor_manager which recalculates all fields:
   ```c
   lle_cursor_manager_move_to_byte_offset(manager, current_byte_offset);
   ```
4. All three fields synchronized ✓
5. Cursor appears in correct position

**User Observation**: Backspace was used to "manually correct" the cursor, which worked because backspace operations properly synchronize cursor fields through the cursor_manager.

---

## Proposed Solutions

### Option 1: Pass cursor_manager to buffer_insert_text() [REJECTED]

**Approach**: Change function signature to include cursor_manager

```c
lle_result_t lle_buffer_insert_text(lle_buffer_t *buffer,
                                   lle_cursor_manager_t *cursor_mgr,  // NEW
                                   size_t position,
                                   const char *text,
                                   size_t text_length);
```

**Pros**:
- Direct access to proper synchronization
- Clean solution

**Cons**:
- Changes core API signature
- Breaks existing code
- Buffer layer shouldn't depend on cursor layer (architectural layering)

**Decision**: REJECTED - violates separation of concerns

---

### Option 2: Caller Synchronizes Cursor [RECOMMENDED]

**Approach**: Make cursor synchronization the caller's responsibility

```c
// In lle_readline.c::handle_character_input()
lle_result_t result = lle_buffer_insert_text(ctx->buffer, 
                                            ctx->buffer->cursor.byte_offset,
                                            utf8_char, char_len);
if (result == LLE_SUCCESS) {
    // Synchronize cursor after insert
    lle_cursor_manager_move_to_byte_offset(ctx->cursor_manager,
                                          ctx->buffer->cursor.byte_offset);
    refresh_display(ctx);
}
```

**Pros**:
- No API changes
- Clean separation: buffer handles data, caller handles cursor
- Follows existing pattern (caller owns coordination)

**Cons**:
- Easy to forget in new code
- Callers must know to do this

**Decision**: RECOMMENDED for Phase 2

---

### Option 3: Add Buffer-Level Sync Function [ALTERNATIVE]

**Approach**: Add convenience function to buffer layer

```c
// In buffer_management.h
void lle_buffer_sync_cursor_indices(lle_buffer_t *buffer);

// Implementation
void lle_buffer_sync_cursor_indices(lle_buffer_t *buffer) {
    size_t byte_off = buffer->cursor.byte_offset;
    buffer->cursor.codepoint_index = 
        lle_utf8_count_codepoints(buffer->data, byte_off);
    buffer->cursor.grapheme_index = 
        lle_utf8_count_graphemes(buffer->data, byte_off);
}
```

**Pros**:
- No architectural violations
- Self-contained solution
- Easy for callers to use

**Cons**:
- O(n) cost on every sync (walks buffer from start)
- Duplicates logic that cursor_manager already has
- Still requires caller to remember

**Decision**: Could work as interim solution

---

### Option 4: Automatic Sync After Insert [COMPLEX]

**Approach**: Always sync after buffer modifications

**Pros**:
- Transparent to callers
- Never forgets

**Cons**:
- O(n) cost on every insert
- Performance impact for rapid typing
- Still doesn't have access to cursor_manager for proper sync

**Decision**: REJECTED - performance concerns

---

## Recommended Implementation Plan (Phase 2)

### Step 1: Add Cursor Sync to Insert/Delete Handlers

**Files to Modify**:
- `src/lle/lle_readline.c` - Character input handler
- `src/lle/lle_readline.c` - Paste handler (if separate)
- `src/lle/keybinding_actions.c` - Yank/paste actions

**Pattern**:
```c
// After EVERY lle_buffer_insert_text() or lle_buffer_delete_text()
if (result == LLE_SUCCESS && editor->cursor_manager) {
    lle_cursor_manager_move_to_byte_offset(
        editor->cursor_manager,
        buffer->cursor.byte_offset
    );
}
```

### Step 2: Test All Paste Scenarios

- Single character paste
- Multi-byte character paste  
- Complex grapheme paste (ZWJ, RI, modifiers)
- Large text paste (performance)
- Paste at start/middle/end of buffer

### Step 3: Performance Validation

- Measure sync overhead
- Ensure typing remains responsive
- Consider lazy sync (only on display refresh)

### Step 4: Integration Testing

- Test with all 7 Phase 1 test cases
- Verify 7/7 pass rate
- Test edge cases (empty buffer, etc.)

---

## Alternative: Lazy Synchronization

Instead of syncing immediately after insert, sync only when needed:

**Trigger Points**:
1. Before display refresh (sync just before rendering)
2. Before cursor query operations
3. On navigation commands

**Advantage**: Amortizes cost across multiple inserts (e.g., rapid typing)

**Implementation**:
```c
// In refresh_display()
if (cursor_needs_sync(ctx)) {
    lle_cursor_manager_move_to_byte_offset(...);
    mark_cursor_synced(ctx);
}
```

---

## Testing Requirements

### Unit Tests Needed

1. **Test: Single char insert syncs cursor**
   - Insert 'a', verify all cursor fields correct

2. **Test: Multi-byte insert syncs cursor**
   - Insert 'é' (2 bytes), verify codepoint_index = 1

3. **Test: Complex grapheme insert syncs cursor**
   - Insert '👨‍👩‍👧‍👦', verify grapheme_index = 1, codepoint_index = 7

4. **Test: Multiple inserts sync correctly**
   - Insert 3 different emoji, verify cursor at grapheme 3

5. **Test: Insert in middle syncs cursor**
   - Insert at position 5, verify cursor fields account for insertion

### Integration Tests Needed

1. Paste all 7 Phase 1 test cases
2. Verify cursor appears at correct position
3. Verify navigation works after paste
4. Verify backspace works from end position

---

## Performance Considerations

**Current Cost** (per `lle_cursor_manager_move_to_byte_offset`):
- If UTF-8 index valid: O(1) lookups ✓
- If UTF-8 index invalid: O(n) rebuild then O(1) lookup

**After Fix**:
- Every insert triggers cursor sync
- First sync after insert: O(n) (index rebuild)
- Subsequent syncs: O(1) (index cached)

**Mitigation**:
- UTF-8 index already designed for this use case
- Lazy invalidation minimizes rebuilds
- Cost is acceptable for typical input rates

---

## Impact Analysis

### User Experience

**Before Fix**:
- Paste complex emoji → cursor wrong
- Press arrow key → cursor fixed
- Annoying but not blocking

**After Fix**:
- Paste complex emoji → cursor correct
- All operations work immediately
- Seamless experience

### Code Complexity

**Before Fix**:
- Buffer layer: Simple (just update byte_offset)
- Caller: Must remember to sync (easy to forget)

**After Fix**:
- Buffer layer: Still simple
- Caller: Explicit sync pattern (harder to forget)
- Slight increase in complexity for correctness

### Performance

**Before Fix**:
- Insert: Fast (no cursor work)
- Arrow key after: Pays sync cost

**After Fix**:
- Insert: Slightly slower (sync cost)
- Arrow key after: Already synced
- Net: Similar cost, better UX

---

## Decision & Timeline

**Decision**: Fix in Phase 2 - Display Integration

**Rationale**:
1. Phase 1 is UTF-8 foundation - this is display/coordination issue
2. Workaround available (arrow key after paste)
3. No data corruption risk
4. Proper fix requires design consideration
5. Better to group with other display work

**Timeline**:
- Phase 2 Step 0: Fix cursor synchronization (2-4 hours)
- Phase 2 Step 1-6: Display integration per plan
- Total Phase 2: 2-4 days

**Acceptance Criteria**:
- All 7 Phase 1 tests pass (including paste)
- No performance regression
- Clean architectural solution

---

## Conclusion

The cursor synchronization bug is well-understood with clear fix path. Impact is limited (paste-only, UX not correctness), workaround exists, and proper fix integrates cleanly with Phase 2 display work.

**Recommendation**: Document, defer to Phase 2, proceed with current Phase 1 foundation.

---

**Document Status**: FINAL  
**Author**: AI Assistant (Session 10)  
**Reviewed**: Pending  
**Phase 2 Reference**: This document guides Phase 2 Step 0 implementation
