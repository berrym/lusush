# Spec 03: Buffer Management - Phased Implementation Plan

**Specification**: `docs/lle_specification/03_buffer_management_complete.md`  
**Total Spec Size**: 1,345 lines  
**Estimated Implementation**: 3,500-4,500 lines of C code  
**Complexity**: **HIGHEST** - Largest and most complex spec  
**Status**: Planning Complete - Ready for Phased Implementation  
**Created**: 2025-10-21

---

## Executive Summary

### Why This Spec is Critical

Buffer Management is the **heart of LLE** - it's the data structure that holds the text being edited. Every other system (input, display, history, completion) interacts with buffers. This is:

- **The largest spec** (1,345 lines specification → 3,500-4,500 lines code)
- **The most complex** (11 major subsystems, 40-50 functions)
- **The most critical** (blocks MVE - Minimal Viable Editor)

### Phasing Strategy Rationale

**Why We MUST Phase This Spec:**

1. **Size**: 3,500-4,500 lines is too large for a single implementation cycle
2. **Complexity**: 11 interconnected subsystems with concurrent state management
3. **Dependencies**: Some subsystems depend on others (UTF-8 → Cursor → Multiline)
4. **Risk**: Implementing everything at once = high error rate, difficult debugging
5. **Testing**: Each phase can be thoroughly tested before proceeding

### Phase Overview

**8 Phases Total** - Each phase is complete, tested, and functional:

| Phase | Focus | Lines | Duration | Risk |
|-------|-------|-------|----------|------|
| 1 | Foundation | 500-600 | 1-2 weeks | LOW |
| 2 | Core Ops | 600-700 | 1 week | MEDIUM |
| 3 | UTF-8 | 600-700 | 1 week | MEDIUM |
| 4 | Cursor | 350-450 | 1 week | LOW-MED |
| 5 | Change Track | 400-500 | 1 week | HIGH |
| 6 | Multiline | 300-400 | 1 week | MEDIUM |
| 7 | Validation | 250-350 | 1 week | LOW |
| 8 | Integration | 200-300 | 1 week | LOW |

**Total**: 3,200-4,000 lines over 7-8 weeks

---

## Critical Dependencies

### External Dependencies (REQUIRED)

1. **Lusush Memory Pool System** - CRITICAL
   - Functions: `lusush_memory_pool_alloc()`, `lusush_memory_pool_realloc()`, `lusush_memory_pool_free()`
   - Status: Must verify these exist in Lusush codebase
   - Fallback: Can use LLE memory pool (Spec 15) if Lusush integration deferred

2. **Spec 16: Error Handling** - ✅ COMPLETE
   - All buffer functions return `lle_result_t`
   - Error context for detailed diagnostics

3. **Spec 15: Memory Management** - ⚠️ PHASE 1 COMPLETE
   - Can use `lle_memory_pool_t` for buffer allocations
   - Phase 1 provides sufficient functionality

4. **Spec 14: Performance Monitoring** - ⚠️ PHASE 1 COMPLETE
   - `lle_performance_monitor_record_operation()` available
   - Can track buffer operation timing

5. **Spec 04: Event System** - ⚠️ PHASE 1 COMPLETE
   - Will need to emit events: `LLE_EVENT_BUFFER_CHANGED`, `LLE_EVENT_CURSOR_MOVED`
   - Phase 1 provides event creation and dispatch

### Internal Dependencies (Initialization Order)

```
Phase 1: Foundation
├── lle_buffer_system_init()
│   ├── lle_performance_monitor_init() [Spec 14]
│   └── lle_buffer_pool_init() [internal]

Phase 3: UTF-8
├── lle_utf8_processor_init()
└── lle_utf8_index_t structures

Phase 4: Cursor
├── lle_cursor_manager_init()
└── depends on: utf8_processor (Phase 3)

Phase 5: Change Tracking
├── lle_change_tracker_init()
└── depends on: cursor_manager (Phase 4)

Phase 6: Multiline
├── lle_multiline_manager_init()
└── depends on: utf8_processor (Phase 3)

Phase 7: Validation
├── lle_buffer_validator_init()
└── depends on: utf8_processor (Phase 3)
```

---

## Phase 1: Foundation (500-600 lines, 1-2 weeks)

### Objective

Establish core data structures, memory management, and system lifecycle. After Phase 1, we can create and destroy buffers, but cannot edit them yet.

### Deliverables

**Files Created**:
- `include/lle/buffer_system.h` - Main buffer system header
- `src/lle/buffer_system.c` - System lifecycle and pool management
- `src/lle/buffer_core.c` - Core buffer structures

**Functions Implemented** (8 functions):

1. **System Lifecycle** (3 functions):
   - `lle_buffer_system_init()` - Initialize buffer management system
   - `lle_buffer_system_destroy()` - Clean up system
   - `lle_buffer_system_validate()` - Validate system state

2. **Buffer Pool** (3 functions):
   - `lle_buffer_pool_init()` - Initialize buffer memory pool
   - `lle_buffer_pool_destroy()` - Clean up pool
   - `lle_buffer_pool_allocate()` - Allocate buffer from pool

3. **Core Buffer** (2 functions):
   - `lle_buffer_create()` - Create empty buffer
   - `lle_buffer_destroy()` - Destroy buffer and free memory

**Data Structures Implemented**:

```c
// Main system structure
typedef struct lle_buffer_system {
    lle_buffer_t *current_buffer;
    lle_buffer_pool_t *buffer_pool;
    lle_performance_monitor_t *perf_monitor;
    lle_memory_pool_t *memory_pool;  // LLE or Lusush pool
} lle_buffer_system_t;

// Buffer pool (simplified Phase 1)
typedef struct lle_buffer_pool {
    lle_buffer_t **buffers;
    size_t capacity;
    size_t count;
    lle_memory_pool_t *memory_pool;
} lle_buffer_pool_t;

// Core buffer structure (Phase 1 - minimal)
typedef struct lle_buffer {
    uint32_t buffer_id;
    char name[LLE_BUFFER_NAME_MAX];
    
    char *data;
    size_t capacity;
    size_t length;
    
    lle_buffer_pool_t *pool;
    lle_memory_pool_t *memory_pool;
} lle_buffer_t;
```

### What's Deferred

- Line structure tracking (Phase 2)
- UTF-8 indexing (Phase 3)
- Cursor management (Phase 4)
- Change tracking (Phase 5)
- Multiline support (Phase 6)
- Validation system (Phase 7)

### Testing

```c
test_buffer_system_init_destroy()
test_buffer_pool_init_destroy()
test_buffer_create_destroy()
test_buffer_allocation_limits()
test_memory_integration()
```

### Success Criteria

- ✅ System initializes without leaks
- ✅ Can create multiple buffers
- ✅ Buffers are destroyed cleanly
- ✅ Memory pool integration works
- ✅ Performance monitoring records operations
- ✅ All functions compile cleanly
- ✅ Zero stubs

### Build Integration

```python
# src/lle/meson.build additions
if fs.exists('buffer_system.c')
  lle_sources += files('buffer_system.c')
endif
if fs.exists('buffer_core.c')
  lle_sources += files('buffer_core.c')
endif
```

### Estimated Effort

**Lines of Code**: 500-600  
**Duration**: 1-2 weeks  
**Risk Level**: LOW  
**Dependencies**: Specs 15, 14 (Phase 1 sufficient)

---

## Phase 2: Core Buffer Operations (600-700 lines, 1 week)

### Objective

Implement basic text insertion and deletion operations with line structure tracking (NO UTF-8 yet - treat as bytes). After Phase 2, can edit buffers as byte arrays.

### Deliverables

**Files Created/Modified**:
- `src/lle/buffer_operations.c` - Insert, delete, line tracking

**Functions Implemented** (6 functions):

1. **Buffer Operations** (4 functions):
   - `lle_buffer_insert_text()` - Insert bytes at position
   - `lle_buffer_delete_text()` - Delete byte range
   - `lle_buffer_expand()` - Grow buffer capacity
   - `lle_buffer_get_content()` - Read buffer content

2. **Line Structure** (2 functions):
   - `lle_buffer_rebuild_line_structure()` - Build line array
   - `lle_buffer_get_line_info()` - Get line metadata

**Data Structures Enhanced**:

```c
// Buffer structure with line tracking
typedef struct lle_buffer {
    // Phase 1 fields...
    
    // Phase 2 additions:
    lle_line_info_t *lines;      // Line array
    size_t line_count;            // Number of lines
    size_t line_capacity;         // Allocated line capacity
    uint64_t modification_count;  // Edit counter
    uint64_t last_modified_time;  // Last edit timestamp
} lle_buffer_t;

// Line information (Phase 2 - simplified)
typedef struct lle_line_info {
    size_t start_offset;   // Byte offset
    size_t end_offset;     // Byte offset
    size_t length;         // Line length in bytes
} lle_line_info_t;
```

### Implementation Notes

**Phase 2 Simplifications**:
- Treat buffer as byte array (no UTF-8 awareness)
- Line breaks = `\n` character only
- No cursor tracking yet
- No change tracking/undo
- No multiline parsing
- No validation/checksums

**Insert Algorithm** (simplified):
```c
lle_result_t lle_buffer_insert_text(lle_buffer_t *buffer, 
                                    size_t position, 
                                    const char *text, 
                                    size_t text_length) {
    // 1. Check capacity, expand if needed
    // 2. Make space: memmove(data + pos + len, data + pos, old_len - pos)
    // 3. Copy text: memcpy(data + pos, text, len)
    // 4. Update length
    // 5. Rebuild line structure
    // 6. Update timestamps
}
```

**Delete Algorithm** (simplified):
```c
lle_result_t lle_buffer_delete_text(lle_buffer_t *buffer,
                                    size_t start_position,
                                    size_t delete_length) {
    // 1. Validate range
    // 2. Compact: memmove(data + start, data + start + len, old_len - start - len)
    // 3. Update length
    // 4. Rebuild line structure
    // 5. Update timestamps
}
```

### What's Deferred

- UTF-8 validation and indexing (Phase 3)
- Cursor position updates (Phase 4)
- Change tracking for undo (Phase 5)
- Multiline context parsing (Phase 6)
- Buffer validation/checksums (Phase 7)

### Testing

```c
test_buffer_insert_at_start()
test_buffer_insert_at_end()
test_buffer_insert_in_middle()
test_buffer_delete_range()
test_buffer_expand_capacity()
test_line_structure_rebuild()
test_multiline_content()
test_performance_benchmarks()
```

### Success Criteria

- ✅ Can insert text at any position
- ✅ Can delete text ranges
- ✅ Line structure tracks newlines correctly
- ✅ Buffer capacity expands automatically
- ✅ Operations are sub-millisecond
- ✅ No memory leaks
- ✅ Zero stubs

### Estimated Effort

**Lines of Code**: 600-700  
**Duration**: 1 week  
**Risk Level**: MEDIUM (mutable state management)  
**Dependencies**: Phase 1 complete

---

## Phase 3: UTF-8 Support (600-700 lines, 1 week)

### Objective

Add complete UTF-8 Unicode support with codepoint/grapheme indexing. After Phase 3, buffer operations are UTF-8 aware and can handle multi-byte characters correctly.

### Deliverables

**Files Created**:
- `src/lle/utf8_processor.c` - UTF-8 validation and processing
- `src/lle/utf8_index.c` - UTF-8 index building and maintenance

**Functions Implemented** (7 functions):

1. **UTF-8 Processor** (4 functions):
   - `lle_utf8_processor_init()` - Initialize processor
   - `lle_utf8_processor_destroy()` - Clean up
   - `lle_utf8_is_valid()` - Validate UTF-8 sequence
   - `lle_utf8_validate_buffer()` - Validate entire buffer

2. **UTF-8 Counting** (3 functions):
   - `lle_utf8_count_codepoints()` - Count Unicode codepoints
   - `lle_utf8_count_graphemes()` - Count grapheme clusters
   - `lle_utf8_get_byte_offset()` - Map codepoint → byte offset

**Data Structures Enhanced**:

```c
// UTF-8 processor
typedef struct lle_utf8_processor {
    // UTF-8 validation tables
    uint8_t *state_table;
    
    // Grapheme break properties
    // (simplified in Phase 3, full Unicode later)
} lle_utf8_processor_t;

// Buffer with UTF-8 metadata
typedef struct lle_buffer {
    // Phase 1, 2 fields...
    
    // Phase 3 additions:
    lle_utf8_processor_t *utf8_processor;
    size_t codepoint_count;   // Unicode codepoints
    size_t grapheme_count;    // Grapheme clusters
    lle_utf8_index_t *utf8_index;  // Fast position lookup
    bool utf8_index_valid;    // Index status
} lle_buffer_t;

// UTF-8 index (Phase 3 - simple)
typedef struct lle_utf8_index {
    size_t *byte_offsets;     // byte offset for each codepoint
    size_t codepoint_capacity;
    size_t codepoint_count;
} lle_utf8_index_t;

// Enhanced line info with UTF-8
typedef struct lle_line_info {
    size_t start_offset;      // Byte offset
    size_t end_offset;        // Byte offset
    size_t length;            // Bytes
    size_t codepoint_count;   // Phase 3: codepoints in line
    size_t grapheme_count;    // Phase 3: graphemes in line
    size_t visual_width;      // Phase 3: display width
} lle_line_info_t;
```

### Implementation Notes

**UTF-8 Validation** (DFA-based):
```c
bool lle_utf8_is_valid(const char *text, size_t length) {
    // Use state table for efficient validation
    // Detect:
    // - Invalid byte sequences
    // - Overlong encodings
    // - Surrogate pairs in UTF-8
    // - Out-of-range codepoints
}
```

**UTF-8 Index Building**:
```c
lle_result_t lle_utf8_index_rebuild(lle_buffer_t *buffer) {
    // Walk buffer byte-by-byte
    // For each UTF-8 sequence:
    //   - Validate sequence
    //   - Record byte offset
    //   - Increment codepoint counter
    // Build fast lookup table: codepoint → byte offset
}
```

**Integration with Insert/Delete**:
- Insert: Validate UTF-8 BEFORE accepting input
- Insert: Update UTF-8 index incrementally
- Delete: Ensure deletion on character boundaries
- Both: Update codepoint/grapheme counts

### What's Deferred

- Full Unicode grapheme cluster rules (simplified for Phase 3)
- Bidirectional text support
- Normalization (NFC/NFD)
- Complex emoji/ZWJ sequences (basic support only)

### Testing

```c
test_utf8_validation()
test_utf8_codepoint_counting()
test_utf8_grapheme_counting()
test_utf8_index_building()
test_utf8_boundary_detection()
test_multibyte_insert()
test_multibyte_delete()
test_emoji_handling()  // Basic
test_invalid_utf8_rejection()
```

### Success Criteria

- ✅ Validates UTF-8 correctly
- ✅ Counts codepoints accurately
- ✅ Counts graphemes (simplified rules)
- ✅ UTF-8 index provides O(1) lookups
- ✅ Insert/delete preserve UTF-8 validity
- ✅ Handles multi-byte characters (2, 3, 4 bytes)
- ✅ Rejects invalid UTF-8
- ✅ Zero stubs

### Estimated Effort

**Lines of Code**: 600-700  
**Duration**: 1 week  
**Risk Level**: MEDIUM (Unicode complexity)  
**Dependencies**: Phase 2 complete

---

## Phase 4: Cursor Management (350-450 lines, 1 week)

### Objective

Implement cursor position tracking with logical (byte/codepoint/grapheme) and visual (column/row) coordinate synchronization. After Phase 4, cursor movement is UTF-8 aware.

### Deliverables

**Files Created**:
- `src/lle/cursor_manager.c` - Cursor position management

**Functions Implemented** (7 functions):

1. **Cursor Manager** (3 functions):
   - `lle_cursor_manager_init()` - Initialize manager
   - `lle_cursor_manager_destroy()` - Clean up
   - `lle_cursor_manager_set_buffer()` - Associate with buffer

2. **Cursor Movement** (4 functions):
   - `lle_cursor_manager_move_to_byte_offset()` - Absolute positioning
   - `lle_cursor_manager_move_by_graphemes()` - Relative movement
   - `lle_cursor_manager_adjust_for_insert()` - Update after insert
   - `lle_cursor_manager_validate_and_correct()` - Validate position

**Data Structures**:

```c
// Cursor position (all coordinate systems)
typedef struct lle_cursor_position {
    // Logical positions
    size_t byte_offset;       // Byte offset in buffer
    size_t codepoint_index;   // Unicode codepoint index
    size_t grapheme_index;    // Grapheme cluster index
    
    // Visual positions
    size_t line_number;       // Line number (0-based)
    size_t column;            // Visual column (accounts for tabs, wide chars)
    
    // Validity
    bool valid;               // Position is valid
} lle_cursor_position_t;

// Cursor manager
typedef struct lle_cursor_manager {
    lle_buffer_t *buffer;             // Associated buffer
    lle_cursor_position_t position;   // Current cursor position
    lle_utf8_processor_t *utf8_proc;  // UTF-8 processor
    
    // Movement history (for smart movement)
    size_t preferred_column;  // Sticky column for up/down
} lle_cursor_manager_t;

// Buffer with cursor
typedef struct lle_buffer {
    // Phase 1, 2, 3 fields...
    
    // Phase 4 additions:
    lle_cursor_manager_t *cursor_mgr;
    lle_cursor_position_t cursor;  // Current cursor
} lle_buffer_t;
```

### Implementation Notes

**Coordinate Synchronization**:
```c
lle_result_t lle_cursor_manager_move_to_byte_offset(
    lle_cursor_manager_t *mgr,
    size_t byte_offset
) {
    // 1. Validate byte_offset <= buffer->length
    // 2. Ensure byte_offset is on UTF-8 boundary
    // 3. Use UTF-8 index to find codepoint_index
    // 4. Count graphemes from line start to find grapheme_index
    // 5. Find line_number by scanning line array
    // 6. Calculate visual column (account for tabs, wide chars)
    // 7. Update all position fields atomically
}
```

**Insert/Delete Adjustment**:
```c
lle_result_t lle_cursor_manager_adjust_for_insert(
    lle_buffer_t *buffer,
    size_t insert_position,
    size_t insert_length
) {
    // If cursor is after insert position:
    //   - Shift byte_offset by insert_length
    //   - Recalculate codepoint_index
    //   - Recalculate grapheme_index
    //   - Recalculate line_number/column
}
```

### What's Deferred

- Selection ranges (Phase 5 or later)
- Multiple cursors
- Virtual column (beyond line end)
- Smart column preservation across long/short lines (simplified)

### Testing

```c
test_cursor_init_destroy()
test_cursor_move_to_byte_offset()
test_cursor_move_by_graphemes()
test_cursor_adjust_after_insert()
test_cursor_adjust_after_delete()
test_cursor_validation()
test_cursor_multibyte_navigation()
test_cursor_multiline_navigation()
```

### Success Criteria

- ✅ Cursor tracks all coordinate systems
- ✅ Movement operations update all fields
- ✅ Insert/delete adjusts cursor correctly
- ✅ Cursor always on valid UTF-8 boundary
- ✅ Visual column accounts for wide characters
- ✅ Line number tracks correctly
- ✅ Zero stubs

### Estimated Effort

**Lines of Code**: 350-450  
**Duration**: 1 week  
**Risk Level**: LOW-MEDIUM  
**Dependencies**: Phase 3 complete

---

## Phase 5: Change Tracking & Undo/Redo (400-500 lines, 1 week)

### Objective

Implement complete undo/redo system with atomic operation tracking. After Phase 5, all buffer edits can be undone and redone.

### Deliverables

**Files Created**:
- `src/lle/change_tracker.c` - Undo/redo system

**Functions Implemented** (6 functions):

1. **Change Tracker** (3 functions):
   - `lle_change_tracker_init()` - Initialize tracker
   - `lle_change_tracker_destroy()` - Clean up
   - `lle_change_tracker_clear()` - Clear history

2. **Undo/Redo** (3 functions):
   - `lle_change_tracker_undo()` - Undo last operation
   - `lle_change_tracker_redo()` - Redo undone operation
   - `lle_change_tracker_can_undo()` / `_can_redo()` - Query capability

**Data Structures**:

```c
// Change operation types
typedef enum {
    LLE_CHANGE_TYPE_INSERT,
    LLE_CHANGE_TYPE_DELETE,
    LLE_CHANGE_TYPE_REPLACE,
} lle_change_type_t;

// Single change operation
typedef struct lle_change_operation {
    lle_change_type_t type;
    size_t position;          // Byte offset
    size_t length;            // Operation length
    char *saved_text;         // Deleted text (for undo)
    lle_cursor_position_t saved_cursor;  // Cursor before op
    uint64_t timestamp;       // Operation timestamp
} lle_change_operation_t;

// Change tracker
typedef struct lle_change_tracker {
    lle_change_operation_t *undo_stack;
    size_t undo_stack_size;
    size_t undo_stack_capacity;
    
    lle_change_operation_t *redo_stack;
    size_t redo_stack_size;
    size_t redo_stack_capacity;
    
    lle_memory_pool_t *memory_pool;
    bool enabled;
} lle_change_tracker_t;

// Buffer with change tracking
typedef struct lle_buffer {
    // Phase 1-4 fields...
    
    // Phase 5 additions:
    lle_change_tracker_t *change_tracker;
    bool change_tracking_enabled;
} lle_buffer_t;
```

### Implementation Notes

**Recording Changes**:
```c
// In lle_buffer_insert_text():
if (buffer->change_tracking_enabled) {
    lle_change_operation_t op = {
        .type = LLE_CHANGE_TYPE_INSERT,
        .position = position,
        .length = text_length,
        .saved_text = NULL,  // No saved text for insert
        .saved_cursor = buffer->cursor,
        .timestamp = lle_get_current_time_us()
    };
    lle_change_tracker_push_undo(buffer->change_tracker, &op);
    lle_change_tracker_clear_redo(buffer->change_tracker);
}

// In lle_buffer_delete_text():
if (buffer->change_tracking_enabled) {
    // Save deleted text for undo
    char *saved = malloc(delete_length + 1);
    memcpy(saved, buffer->data + start_position, delete_length);
    
    lle_change_operation_t op = {
        .type = LLE_CHANGE_TYPE_DELETE,
        .position = start_position,
        .length = delete_length,
        .saved_text = saved,  // CRITICAL: Save for undo
        .saved_cursor = buffer->cursor,
        .timestamp = lle_get_current_time_us()
    };
    lle_change_tracker_push_undo(buffer->change_tracker, &op);
    lle_change_tracker_clear_redo(buffer->change_tracker);
}
```

**Undo Logic**:
```c
lle_result_t lle_change_tracker_undo(lle_change_tracker_t *tracker,
                                     lle_buffer_t *buffer) {
    if (tracker->undo_stack_size == 0) {
        return LLE_ERROR_NOTHING_TO_UNDO;
    }
    
    // Pop operation from undo stack
    lle_change_operation_t *op = &tracker->undo_stack[--tracker->undo_stack_size];
    
    // Apply inverse operation
    switch (op->type) {
        case LLE_CHANGE_TYPE_INSERT:
            // Undo insert = delete the inserted text
            lle_buffer_delete_text(buffer, op->position, op->length);
            break;
            
        case LLE_CHANGE_TYPE_DELETE:
            // Undo delete = re-insert the saved text
            lle_buffer_insert_text(buffer, op->position, 
                                   op->saved_text, op->length);
            break;
    }
    
    // Restore cursor position
    buffer->cursor = op->saved_cursor;
    
    // Move operation to redo stack
    lle_change_tracker_push_redo(tracker, op);
    
    return LLE_SUCCESS;
}
```

### What's Deferred

- Change sequences (grouping multiple ops)
- Transaction support
- Selective undo
- Change persistence to disk
- Conflict resolution

### Testing

```c
test_change_tracker_init()
test_undo_insert()
test_undo_delete()
test_redo_operations()
test_undo_redo_chain()
test_cursor_restoration()
test_undo_stack_limits()
test_memory_cleanup()
```

### Success Criteria

- ✅ All inserts are undoable
- ✅ All deletes are undoable
- ✅ Redo works after undo
- ✅ Cursor restores to correct position
- ✅ Deleted text is preserved for undo
- ✅ Memory is freed correctly
- ✅ Zero stubs

### Estimated Effort

**Lines of Code**: 400-500  
**Duration**: 1 week  
**Risk Level**: HIGH (state machine complexity)  
**Dependencies**: Phase 4 complete

---

## Phase 6: Multiline Support (300-400 lines, 1 week)

### Objective

Add shell-specific multiline construct parsing (quotes, here-docs, continuations). After Phase 6, buffer understands shell syntax structure.

### Deliverables

**Files Created**:
- `src/lle/multiline_manager.c` - Multiline parsing

**Functions Implemented** (5 functions):

1. **Multiline Manager** (2 functions):
   - `lle_multiline_manager_init()` - Initialize manager
   - `lle_multiline_manager_destroy()` - Clean up

2. **Multiline Analysis** (3 functions):
   - `lle_determine_line_type()` - Classify line type
   - `lle_update_multiline_context()` - Update parsing state
   - `lle_multiline_is_complete()` - Check if command is complete

**Data Structures**:

```c
// Line types
typedef enum {
    LLE_LINE_TYPE_COMMAND,      // Normal command line
    LLE_LINE_TYPE_CONTINUATION, // Line continuation (\)
    LLE_LINE_TYPE_QUOTE_OPEN,   // Quote opened but not closed
    LLE_LINE_TYPE_HEREDOC,      // Here-document
    LLE_LINE_TYPE_SUBSHELL,     // Subshell/brace not closed
} lle_line_type_t;

// Multiline state
typedef enum {
    LLE_ML_STATE_NORMAL,        // Not in multiline
    LLE_ML_STATE_BACKSLASH,     // Continuation with \
    LLE_ML_STATE_SINGLE_QUOTE,  // Inside '...'
    LLE_ML_STATE_DOUBLE_QUOTE,  // Inside "..."
    LLE_ML_STATE_HEREDOC,       // Inside <<EOF
    LLE_ML_STATE_SUBSHELL,      // Inside $(...) or (...)
    LLE_ML_STATE_BRACE,         // Inside {...}
} lle_multiline_state_t;

// Multiline context
typedef struct lle_multiline_context {
    lle_multiline_state_t state;
    int nesting_level;          // For (), {}, etc.
    char quote_char;            // Active quote char
    char *heredoc_delimiter;    // Heredoc delimiter
    size_t continuation_lines;  // Number of continuation lines
} lle_multiline_context_t;

// Multiline manager
typedef struct lle_multiline_manager {
    lle_utf8_processor_t *utf8_proc;
    lle_multiline_context_t context;
} lle_multiline_manager_t;

// Buffer with multiline
typedef struct lle_buffer {
    // Phase 1-5 fields...
    
    // Phase 6 additions:
    lle_multiline_manager_t *multiline_mgr;
    lle_multiline_context_t *multiline_ctx;
    bool multiline_active;
} lle_buffer_t;

// Enhanced line info
typedef struct lle_line_info {
    // Phase 1-3 fields...
    
    // Phase 6 additions:
    lle_line_type_t type;
    lle_multiline_state_t ml_state;
    uint8_t indent_level;
} lle_line_info_t;
```

### Implementation Notes

**Multiline Parsing** (simplified shell syntax):
```c
lle_result_t lle_determine_line_type(lle_buffer_t *buffer, 
                                     lle_line_info_t *line) {
    // Parse line character by character
    // Track:
    // - Quote state (', ", `)
    // - Escape sequences (\)
    // - Subshell depth ( $(... ), $( ... )
    // - Brace depth { ... }
    // - Heredoc markers (<<EOF)
    // - Line continuation (\n at end)
    
    // Set line->type based on parsing result
    // Update buffer->multiline_ctx state
}
```

**Command Completion Check**:
```c
bool lle_multiline_is_complete(lle_buffer_t *buffer) {
    // Check multiline context:
    // - All quotes closed?
    // - All parentheses balanced?
    // - All braces balanced?
    // - Heredoc completed?
    // - No trailing backslash?
    
    return (buffer->multiline_ctx->state == LLE_ML_STATE_NORMAL &&
            buffer->multiline_ctx->nesting_level == 0);
}
```

### What's Deferred

- Full POSIX shell grammar parsing
- Syntax error detection
- Smart indentation
- Syntax highlighting integration (Spec 11)
- Complex nested structures (advanced)

### Testing

```c
test_simple_command()
test_backslash_continuation()
test_single_quote_multiline()
test_double_quote_multiline()
test_heredoc_parsing()
test_subshell_nesting()
test_brace_nesting()
test_mixed_multiline()
test_completion_detection()
```

### Success Criteria

- ✅ Detects line continuations
- ✅ Tracks quote state correctly
- ✅ Balances parentheses/braces
- ✅ Parses heredocs
- ✅ Determines command completeness
- ✅ Handles nested structures (basic)
- ✅ Zero stubs

### Estimated Effort

**Lines of Code**: 300-400  
**Duration**: 1 week  
**Risk Level**: MEDIUM  
**Dependencies**: Phase 3 complete (UTF-8 for parsing)

---

## Phase 7: Validation & Monitoring (250-350 lines, 1 week)

### Objective

Add buffer integrity validation, checksum calculation, and cache management. After Phase 7, buffer corruption can be detected.

### Deliverables

**Files Created**:
- `src/lle/buffer_validator.c` - Validation and integrity

**Functions Implemented** (6 functions):

1. **Validator** (2 functions):
   - `lle_buffer_validator_init()` - Initialize validator
   - `lle_buffer_validator_destroy()` - Clean up

2. **Validation** (4 functions):
   - `lle_buffer_validate_complete()` - Full validation
   - `lle_buffer_update_checksum()` - Calculate checksum
   - `lle_buffer_invalidate_cache()` - Invalidate caches
   - `lle_buffer_verify_integrity()` - Integrity check

**Data Structures**:

```c
// Buffer flags
typedef enum {
    LLE_BUFFER_FLAG_MODIFIED     = 0x01,
    LLE_BUFFER_FLAG_UTF8_VALID   = 0x02,
    LLE_BUFFER_FLAG_INDEX_VALID  = 0x04,
    LLE_BUFFER_FLAG_CACHE_VALID  = 0x08,
} lle_buffer_flags_t;

// Cache types
typedef enum {
    LLE_CACHE_RENDER         = 0x01,
    LLE_CACHE_LAYOUT         = 0x02,
    LLE_CACHE_LINE_STRUCTURE = 0x04,
} lle_cache_type_t;

// Validator
typedef struct lle_buffer_validator {
    lle_utf8_processor_t *utf8_proc;
    uint32_t validation_count;
    uint32_t error_count;
} lle_buffer_validator_t;

// Buffer with validation
typedef struct lle_buffer {
    // Phase 1-6 fields...
    
    // Phase 7 additions:
    lle_buffer_validator_t *validator;
    uint32_t checksum;
    bool integrity_valid;
    lle_buffer_flags_t flags;
    uint32_t cache_version;
    bool cache_dirty;
} lle_buffer_t;
```

### Implementation Notes

**Validation**:
```c
lle_result_t lle_buffer_validate_complete(lle_buffer_t *buffer) {
    // 1. Verify UTF-8 validity
    if (!lle_utf8_validate_buffer(buffer)) {
        return LLE_ERROR_INVALID_UTF8;
    }
    
    // 2. Verify length consistency
    if (buffer->length > buffer->capacity) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }
    
    // 3. Verify line structure
    size_t expected_lines = count_newlines(buffer->data);
    if (buffer->line_count != expected_lines) {
        return LLE_ERROR_LINE_COUNT_MISMATCH;
    }
    
    // 4. Verify cursor position
    if (buffer->cursor.byte_offset > buffer->length) {
        return LLE_ERROR_CURSOR_OUT_OF_BOUNDS;
    }
    
    // 5. Verify UTF-8 index
    if (buffer->utf8_index_valid) {
        // Spot-check index entries
    }
    
    return LLE_SUCCESS;
}
```

**Checksum**:
```c
lle_result_t lle_buffer_update_checksum(lle_buffer_t *buffer) {
    // Simple CRC32 or hash
    buffer->checksum = crc32(buffer->data, buffer->length);
    buffer->integrity_valid = true;
    return LLE_SUCCESS;
}
```

### What's Deferred

- Cryptographic checksums
- Automatic corruption recovery
- Persistent validation logs
- Advanced cache strategies

### Testing

```c
test_buffer_validation()
test_checksum_calculation()
test_cache_invalidation()
test_integrity_verification()
test_utf8_validation_integration()
test_corruption_detection()
```

### Success Criteria

- ✅ Detects buffer corruption
- ✅ Validates UTF-8 integrity
- ✅ Verifies line structure consistency
- ✅ Checksum calculation works
- ✅ Cache invalidation correct
- ✅ Zero stubs

### Estimated Effort

**Lines of Code**: 250-350  
**Duration**: 1 week  
**Risk Level**: LOW  
**Dependencies**: Phase 3 complete

---

## Phase 8: Integration & Testing (200-300 lines, 1 week)

### Objective

Complete system integration, event emission, comprehensive testing, and documentation. After Phase 8, Spec 03 is production-ready.

### Deliverables

**Integration**:
- Event emission on buffer changes
- Performance metric collection
- Memory pool integration verification

**Testing**:
- Comprehensive unit test suite
- Integration tests
- Performance benchmarks
- Fuzzing tests

**Documentation**:
- Compliance audit document
- API documentation
- Performance characteristics
- Migration guide

**Functions Implemented** (3 functions):

1. **Event Integration** (2 functions):
   - `lle_buffer_emit_change_event()` - Emit buffer changed event
   - `lle_buffer_emit_cursor_event()` - Emit cursor moved event

2. **Statistics** (1 function):
   - `lle_buffer_get_statistics()` - Collect buffer statistics

### Event Integration

```c
// In lle_buffer_insert_text() and lle_buffer_delete_text():
if (buffer->event_system) {
    lle_event_t *event = NULL;
    lle_buffer_change_data_t change_data = {
        .buffer_id = buffer->buffer_id,
        .position = position,
        .length = text_length,
        .change_type = LLE_CHANGE_TYPE_INSERT
    };
    
    lle_event_create(buffer->event_system,
                     LLE_EVENT_BUFFER_CHANGED,
                     &change_data,
                     sizeof(change_data),
                     &event);
    lle_event_enqueue(buffer->event_system, event);
}

// In cursor movement:
if (buffer->event_system) {
    lle_event_t *event = NULL;
    lle_cursor_change_data_t cursor_data = {
        .buffer_id = buffer->buffer_id,
        .old_position = old_cursor,
        .new_position = buffer->cursor
    };
    
    lle_event_create(buffer->event_system,
                     LLE_EVENT_CURSOR_MOVED,
                     &cursor_data,
                     sizeof(cursor_data),
                     &event);
    lle_event_enqueue(buffer->event_system, event);
}
```

### Comprehensive Testing

```c
// Unit tests (each phase)
test_phase1_foundation()
test_phase2_operations()
test_phase3_utf8()
test_phase4_cursor()
test_phase5_undo_redo()
test_phase6_multiline()
test_phase7_validation()

// Integration tests
test_full_editing_workflow()
test_multiline_editing_with_undo()
test_utf8_multibyte_editing()
test_event_emission()
test_performance_benchmarks()

// Stress tests
test_large_buffer_operations()
test_rapid_insert_delete()
test_deep_undo_stack()
test_memory_limits()

// Fuzzing
test_random_operations()
test_invalid_utf8_sequences()
test_malformed_input()
```

### Compliance Audit

Create `spec_03_compliance_audit.md` documenting:
- All 40-50 functions implemented
- All data structures defined
- All subsystems operational
- Performance benchmarks
- Memory usage analysis
- Integration verification

### Success Criteria

- ✅ All phases complete
- ✅ All tests passing
- ✅ Events emit correctly
- ✅ Performance meets targets
- ✅ Memory leaks eliminated
- ✅ Documentation complete
- ✅ Compliance audit passes

### Estimated Effort

**Lines of Code**: 200-300 (integration + tests)  
**Duration**: 1 week  
**Risk Level**: LOW  
**Dependencies**: Phases 1-7 complete

---

## Summary: Complete Implementation Timeline

### Phase Dependency Graph

```
Phase 1: Foundation (500-600 LOC, 1-2 weeks)
    ↓
Phase 2: Core Ops (600-700 LOC, 1 week)
    ↓
Phase 3: UTF-8 (600-700 LOC, 1 week)
    ↓
    ├─→ Phase 4: Cursor (350-450 LOC, 1 week)
    │       ↓
    │   Phase 5: Change Track (400-500 LOC, 1 week)
    │
    └─→ Phase 6: Multiline (300-400 LOC, 1 week)
    
    ↓
Phase 7: Validation (250-350 LOC, 1 week)
    ↓
Phase 8: Integration (200-300 LOC, 1 week)
```

### Total Implementation Stats

| Metric | Value |
|--------|-------|
| **Total Lines of Code** | 3,200-4,000 |
| **Total Functions** | 40-50 |
| **Total Duration** | 7-8 weeks |
| **Phases** | 8 |
| **Files Created** | 8-10 source files |
| **Data Structures** | 14+ structs, enums |

### Critical Success Factors

1. ✅ **Complete each phase fully** - No stubs, no TODOs
2. ✅ **Test each phase thoroughly** - Before proceeding
3. ✅ **Maintain UTF-8 correctness** - All operations UTF-8 aware
4. ✅ **Preserve memory safety** - Zero leaks, bounds checking
5. ✅ **Meet performance targets** - Sub-millisecond operations
6. ✅ **Document as you go** - Living documentation

### Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| UTF-8 edge cases | Extensive test corpus, fuzzing |
| Memory leaks | Valgrind on every commit |
| Undo/redo bugs | Transaction logging, state validation |
| Performance regression | Continuous benchmarking |
| Integration failures | Incremental integration tests |

---

## Next Steps

1. **Verify Lusush memory pool API** - Check if `lusush_memory_pool_*` functions exist
2. **Begin Phase 1 implementation** - Foundation and lifecycle
3. **Create compliance audit template** - For tracking progress
4. **Set up performance benchmarking** - Baseline measurements

**Ready to begin implementation when approved.**

---

**Document Status**: ✅ COMPLETE  
**Next Update**: After Phase 1 implementation  
**Owner**: AI Assistant  
**Reviewed**: Pending user approval
