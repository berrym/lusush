# UTF-8 Index API Alignment Specification

**Date**: 2025-11-11  
**Status**: REQUIRED FOR PHASE 1 STEP 5  
**Priority**: CRITICAL - Blocks cursor manager integration  

## Problem Statement

The UTF-8 index implementation (Steps 2-4) is complete and functional, but cannot be integrated into the cursor manager due to API signature conflicts between:

1. **Actual implementation**: `include/lle/utf8_index.h` + `src/lle/utf8_index.c`
2. **Forward declarations**: `include/lle/buffer_management.h` (lines 149, 221-241, 857-949)

These forward declarations were created before implementation and don't match the actual API.

## Current State (Committed)

### Implemented Files (Working)
- `include/lle/grapheme_detector.h` - UAX #29 grapheme boundary detection
- `src/lle/grapheme_detector.c` - Complete implementation
- `include/lle/char_width.h` - Unicode East Asian Width
- `src/lle/char_width.c` - Complete implementation  
- `include/lle/utf8_index.h` - UTF-8 position mapping API
- `src/lle/utf8_index.c` - Complete 5-phase rebuild algorithm

### Actual API Signatures (Implemented)

```c
// Structure definition
struct lle_utf8_index_t {
    size_t *byte_to_codepoint;
    size_t *codepoint_to_byte;
    size_t *grapheme_to_codepoint;
    size_t *codepoint_to_grapheme;
    size_t *grapheme_to_display;      // NEW - not in buffer_management.h
    size_t *display_to_grapheme;      // NEW - not in buffer_management.h
    
    size_t byte_count;
    size_t codepoint_count;
    size_t grapheme_count;
    size_t display_width;              // NEW - not in buffer_management.h
    
    bool index_valid;
    uint32_t buffer_version;
    uint64_t last_update_time;
    size_t rebuild_count;              // NEW - replaces cache_hit_count
    uint64_t total_rebuild_time_ns;    // NEW - replaces cache_miss_count
};

// Lifecycle functions
lle_result_t lle_utf8_index_init(lle_utf8_index_t *index);  
void lle_utf8_index_cleanup(lle_utf8_index_t *index);

// Core rebuild
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length);

// Position conversions (all implemented)
lle_result_t lle_utf8_index_byte_to_codepoint(const lle_utf8_index_t *index,
                                               size_t byte_offset,
                                               size_t *codepoint_index);

lle_result_t lle_utf8_index_codepoint_to_byte(const lle_utf8_index_t *index,
                                               size_t codepoint_index,
                                               size_t *byte_offset);

lle_result_t lle_utf8_index_byte_to_grapheme(const lle_utf8_index_t *index,
                                              size_t byte_offset,
                                              size_t *grapheme_index);

lle_result_t lle_utf8_index_grapheme_to_byte(const lle_utf8_index_t *index,
                                              size_t grapheme_index,
                                              size_t *byte_offset);

lle_result_t lle_utf8_index_codepoint_to_grapheme(const lle_utf8_index_t *index,
                                                   size_t codepoint_index,
                                                   size_t *grapheme_index);

lle_result_t lle_utf8_index_grapheme_to_codepoint(const lle_utf8_index_t *index,
                                                   size_t grapheme_index,
                                                   size_t *codepoint_index);

lle_result_t lle_utf8_index_grapheme_to_display(const lle_utf8_index_t *index,
                                                 size_t grapheme_index,
                                                 size_t *display_column);

lle_result_t lle_utf8_index_display_to_grapheme(const lle_utf8_index_t *index,
                                                 size_t display_column,
                                                 size_t *grapheme_index);

// Utility functions
void lle_utf8_index_invalidate(lle_utf8_index_t *index);
bool lle_utf8_index_is_valid(const lle_utf8_index_t *index);
```

### Conflicting Forward Declarations (buffer_management.h)

```c
// Line 149 - Forward declaration
typedef struct lle_utf8_index_t lle_utf8_index_t;

// Lines 221-241 - Structure definition (CONFLICTS WITH ACTUAL)
struct lle_utf8_index_t {
    size_t *byte_to_codepoint;
    size_t *codepoint_to_byte;
    size_t *grapheme_to_codepoint;
    size_t *codepoint_to_grapheme;
    // MISSING: grapheme_to_display, display_to_grapheme
    
    size_t byte_count;
    size_t codepoint_count;
    size_t grapheme_count;
    // MISSING: display_width
    
    bool index_valid;
    uint32_t buffer_version;
    uint64_t last_update_time;
    
    size_t cache_hit_count;        // WRONG - should be rebuild_count
    size_t cache_miss_count;       // WRONG - should be total_rebuild_time_ns
};

// Lines 857-949 - Function declarations (CONFLICTS)
lle_result_t lle_utf8_index_init(lle_utf8_index_t **index);  // WRONG - should be *index
lle_result_t lle_utf8_index_destroy(lle_utf8_index_t *index); // WRONG NAME - should be cleanup
// ... missing byte_to_grapheme, grapheme_to_byte
// ... missing grapheme_to_display, display_to_grapheme
lle_result_t lle_utf8_index_invalidate(lle_utf8_index_t *index); // WRONG - should return void
```

## Required Changes for Step 5

### Change 1: Update buffer_management.h Structure Definition

**File**: `include/lle/buffer_management.h`  
**Lines**: 221-241

Replace structure definition with:

```c
struct lle_utf8_index_t {
    /* Fast position mapping arrays */
    size_t *byte_to_codepoint;
    size_t *codepoint_to_byte;
    size_t *grapheme_to_codepoint;
    size_t *codepoint_to_grapheme;
    size_t *grapheme_to_display;           // ADD THIS
    size_t *display_to_grapheme;           // ADD THIS
    
    /* Index metadata */
    size_t byte_count;
    size_t codepoint_count;
    size_t grapheme_count;
    size_t display_width;                  // ADD THIS
    
    /* Index validity and versioning */
    bool index_valid;
    uint32_t buffer_version;
    uint64_t last_update_time;
    
    /* Performance tracking */
    size_t rebuild_count;                  // CHANGE FROM cache_hit_count
    uint64_t total_rebuild_time_ns;        // CHANGE FROM cache_miss_count
};
```

### Change 2: Update buffer_management.h Function Declarations

**File**: `include/lle/buffer_management.h`  
**Lines**: 857-949

Replace ALL lle_utf8_index_* function declarations with:

```c
/* UTF-8 Index API - Implementation in utf8_index.c */

lle_result_t lle_utf8_index_init(lle_utf8_index_t *index);  // CHANGE ** to *
void lle_utf8_index_cleanup(lle_utf8_index_t *index);       // RENAME from destroy
lle_result_t lle_utf8_index_rebuild(lle_utf8_index_t *index,
                                    const char *text,
                                    size_t text_length);

/* Byte <-> Codepoint conversions */
lle_result_t lle_utf8_index_byte_to_codepoint(const lle_utf8_index_t *index,
                                               size_t byte_offset,
                                               size_t *codepoint_index);
lle_result_t lle_utf8_index_codepoint_to_byte(const lle_utf8_index_t *index,
                                               size_t codepoint_index,
                                               size_t *byte_offset);

/* Byte <-> Grapheme conversions (ADD THESE) */
lle_result_t lle_utf8_index_byte_to_grapheme(const lle_utf8_index_t *index,
                                              size_t byte_offset,
                                              size_t *grapheme_index);
lle_result_t lle_utf8_index_grapheme_to_byte(const lle_utf8_index_t *index,
                                              size_t grapheme_index,
                                              size_t *byte_offset);

/* Codepoint <-> Grapheme conversions */
lle_result_t lle_utf8_index_codepoint_to_grapheme(const lle_utf8_index_t *index,
                                                   size_t codepoint_index,
                                                   size_t *grapheme_index);
lle_result_t lle_utf8_index_grapheme_to_codepoint(const lle_utf8_index_t *index,
                                                   size_t grapheme_index,
                                                   size_t *codepoint_index);

/* Grapheme <-> Display conversions (ADD THESE) */
lle_result_t lle_utf8_index_grapheme_to_display(const lle_utf8_index_t *index,
                                                 size_t grapheme_index,
                                                 size_t *display_column);
lle_result_t lle_utf8_index_display_to_grapheme(const lle_utf8_index_t *index,
                                                 size_t display_column,
                                                 size_t *grapheme_index);

/* Utility functions */
void lle_utf8_index_invalidate(lle_utf8_index_t *index);  // CHANGE return type to void
bool lle_utf8_index_is_valid(const lle_utf8_index_t *index);
```

### Change 3: Remove Duplicate Structure from utf8_index.h

**File**: `include/lle/utf8_index.h`  
**Lines**: 20-44

Replace the structure definition with just a forward declaration:

```c
/* Forward declaration - full definition in buffer_management.h */
typedef struct lle_utf8_index_t lle_utf8_index_t;
```

**Rationale**: Avoid duplicate structure definitions. buffer_management.h is the authoritative source since lle_buffer_t references it.

### Change 4: Integrate UTF-8 Index into Cursor Manager

**File**: `src/lle/cursor_manager.c`

Add include:
```c
#include "lle/utf8_index.h"
```

Update `calculate_line_column()` function to use index when available:

```c
static lle_result_t calculate_line_column(lle_cursor_manager_t *manager) {
    // ... existing line finding logic ...
    
    /* Calculate column in codepoints - use index if available */
    size_t column_codepoint = 0;
    if (column_offset > 0) {
        if (buffer->utf8_index && buffer->utf8_index_valid) {
            /* O(1) lookup using index */
            size_t total_codepoints, line_start_codepoints;
            if (lle_utf8_index_byte_to_codepoint(buffer->utf8_index, byte_offset, &total_codepoints) == LLE_SUCCESS &&
                lle_utf8_index_byte_to_codepoint(buffer->utf8_index, line_start, &line_start_codepoints) == LLE_SUCCESS) {
                column_codepoint = total_codepoints - line_start_codepoints;
            } else {
                /* Fallback to O(n) counting */
                column_codepoint = lle_utf8_count_codepoints(buffer->data + line_start, column_offset);
            }
        } else {
            column_codepoint = lle_utf8_count_codepoints(buffer->data + line_start, column_offset);
        }
    }
    
    /* Calculate column in graphemes - use index if available */
    size_t column_grapheme = 0;
    if (column_offset > 0) {
        if (buffer->utf8_index && buffer->utf8_index_valid) {
            /* O(1) lookup using index */
            size_t total_graphemes, line_start_graphemes;
            if (lle_utf8_index_byte_to_grapheme(buffer->utf8_index, byte_offset, &total_graphemes) == LLE_SUCCESS &&
                lle_utf8_index_byte_to_grapheme(buffer->utf8_index, line_start, &line_start_graphemes) == LLE_SUCCESS) {
                column_grapheme = total_graphemes - line_start_graphemes;
            } else {
                /* Fallback to O(n) counting */
                column_grapheme = lle_utf8_count_graphemes(buffer->data + line_start, column_offset);
            }
        } else {
            column_grapheme = lle_utf8_count_graphemes(buffer->data + line_start, column_offset);
        }
    }
    
    // ... rest of function ...
}
```

Update `grapheme_index_to_byte_offset()` helper:

```c
static lle_result_t grapheme_index_to_byte_offset(lle_buffer_t *buffer,
                                                  size_t grapheme_index,
                                                  size_t *byte_offset) {
    if (!buffer || !buffer->data || !byte_offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (grapheme_index == 0) {
        *byte_offset = 0;
        return LLE_SUCCESS;
    }
    
    /* Use UTF-8 index if available for O(1) lookup */
    if (buffer->utf8_index && buffer->utf8_index_valid) {
        return lle_utf8_index_grapheme_to_byte(buffer->utf8_index, grapheme_index, byte_offset);
    }
    
    /* Fallback to O(n) scan */
    // ... existing scan logic ...
}
```

Update `codepoint_index_to_byte_offset()` helper:

```c
static lle_result_t codepoint_index_to_byte_offset(lle_buffer_t *buffer,
                                                   size_t codepoint_index,
                                                   size_t *byte_offset) {
    if (!buffer || !buffer->data || !byte_offset) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (codepoint_index == 0) {
        *byte_offset = 0;
        return LLE_SUCCESS;
    }
    
    /* Use UTF-8 index if available for O(1) lookup */
    if (buffer->utf8_index && buffer->utf8_index_valid) {
        return lle_utf8_index_codepoint_to_byte(buffer->utf8_index, codepoint_index, byte_offset);
    }
    
    /* Fallback to O(n) scan */
    // ... existing scan logic ...
}
```

Update `lle_cursor_manager_move_to_byte_offset()`:

```c
lle_result_t lle_cursor_manager_move_to_byte_offset(lle_cursor_manager_t *manager,
                                                    size_t byte_offset) {
    // ... validation ...
    
    lle_buffer_t *buffer = manager->buffer;
    
    /* Step 2: Calculate codepoint index - use UTF-8 index if available */
    if (byte_offset == 0) {
        manager->position.codepoint_index = 0;
    } else if (buffer->utf8_index && buffer->utf8_index_valid) {
        /* O(1) lookup */
        if (lle_utf8_index_byte_to_codepoint(buffer->utf8_index, byte_offset,
                                             &manager->position.codepoint_index) != LLE_SUCCESS) {
            /* Fallback to O(n) counting */
            manager->position.codepoint_index = lle_utf8_count_codepoints(buffer->data, byte_offset);
        }
    } else {
        manager->position.codepoint_index = lle_utf8_count_codepoints(buffer->data, byte_offset);
    }
    
    /* Step 3: Calculate grapheme index - use UTF-8 index if available */
    if (byte_offset == 0) {
        manager->position.grapheme_index = 0;
    } else if (buffer->utf8_index && buffer->utf8_index_valid) {
        /* O(1) lookup */
        if (lle_utf8_index_byte_to_grapheme(buffer->utf8_index, byte_offset,
                                            &manager->position.grapheme_index) != LLE_SUCCESS) {
            /* Fallback to O(n) counting */
            manager->position.grapheme_index = lle_utf8_count_graphemes(buffer->data, byte_offset);
        }
    } else {
        manager->position.grapheme_index = lle_utf8_count_graphemes(buffer->data, byte_offset);
    }
    
    // ... rest of function ...
}
```

## Implementation Order (Step 5)

1. **Update buffer_management.h structure** (lines 221-241)
2. **Update buffer_management.h function declarations** (lines 857-949)  
3. **Update utf8_index.h** - remove duplicate structure, use forward declaration
4. **Verify compilation** - `meson compile -C builddir lle`
5. **Integrate into cursor_manager.c** - add all the updates above
6. **Verify compilation** - `meson compile -C builddir lle lusush`
7. **Commit** with message starting with "LLE Phase 1 Step 5:"

## Performance Impact

After integration, cursor manager will automatically get:
- **O(1) position conversions** when UTF-8 index is valid
- **Graceful degradation** to O(n) scanning when index is invalid
- **No behavioral changes** - same results, just faster

## Testing Strategy

The UTF-8 index rebuild algorithm has already been tested during Steps 2-4 implementation. Step 5 integration testing should verify:

1. Cursor movement with ASCII text (simple case)
2. Cursor movement with multi-byte UTF-8 (CJK characters)
3. Cursor movement with combining marks
4. Cursor movement with emoji + ZWJ sequences
5. Cursor movement with invalid/missing index (fallback path)

## Next Steps After Step 5

Once API alignment is complete and cursor manager integration works:

- **Step 6**: Update display system to use `display_to_grapheme` / `grapheme_to_display` for proper cursor rendering
- **Step 7**: Update editing operations (backspace/delete) to respect grapheme boundaries using `lle_is_grapheme_boundary()`
- **Step 8**: Comprehensive testing with all Unicode edge cases

## Notes

- The UTF-8 index implementation is **COMPLETE and CORRECT**
- The blocker is purely **API contract alignment**
- This is **NOT algorithm work** - just signature fixes
- Estimated time for Step 5: **30-45 minutes** with this document
- The graceful fallback ensures **no breakage** if index is unavailable
