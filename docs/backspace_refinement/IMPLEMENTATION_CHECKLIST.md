# Backspace Line Wrap Refinement - Implementation Checklist

**Document Version**: 1.0  
**Date**: December 29, 2024  
**Status**: Ready for Immediate Implementation  
**Priority**: CRITICAL - Must be completed before production deployment

## 🚨 CRITICAL PRIORITY IMPLEMENTATION

This checklist provides step-by-step implementation guidance for resolving critical backspace line wrap issues identified through real-world testing. Follow this checklist sequentially for guaranteed success.

## Prerequisites

### ✅ **Before Starting - Verification Steps**

- [ ] Read `docs/backspace_refinement/TECHNICAL_SPECIFICATION.md` completely
- [ ] Understand current issues from terminal session analysis in `AI_CONTEXT.md`
- [ ] Verify development environment: `scripts/lle_build.sh build && scripts/lle_build.sh test`
- [ ] Create development branch: `git checkout -b task/backspace-refinement`
- [ ] Backup current working state: `git commit -am "Save state before backspace refinement"`

## Phase 1: Enhanced Display State Tracking (2-3 hours)

### 📁 **Files to Modify**
- `src/line_editor/display.h`
- `src/line_editor/display.c`

### ✅ **Task 1.1: Expand Display State Structure**

**File**: `src/line_editor/display.h`

- [ ] Add new fields to `lle_display_state_t`:
```c
// ADD these fields to existing lle_display_state_t structure
// Visual footprint tracking
size_t last_visual_rows;           // Number of terminal rows used
size_t last_visual_end_col;        // Column position on last row
size_t last_total_chars;           // Total characters rendered
bool last_had_wrapping;            // Whether content wrapped lines

// Consistency tracking  
uint32_t last_content_hash;        // Hash of last rendered content
bool syntax_highlighting_applied;   // Track highlighting state

// Clearing state
lle_terminal_coordinates_t clear_start;  // Where clearing should begin
lle_terminal_coordinates_t clear_end;    // Where clearing should end
```

- [ ] Add visual footprint structure:
```c
typedef struct {
    size_t rows_used;
    size_t end_column;
    bool wraps_lines;
    size_t total_visual_width;
} lle_visual_footprint_t;
```

- [ ] Add function declaration:
```c
bool lle_calculate_visual_footprint(const char *text, size_t length, 
                                   size_t prompt_width, size_t terminal_width,
                                   lle_visual_footprint_t *footprint);
```

### ✅ **Task 1.2: Implement Visual Footprint Calculation**

**File**: `src/line_editor/display.c`

- [ ] Implement core calculation function:
```c
bool lle_calculate_visual_footprint(const char *text, size_t length, 
                                   size_t prompt_width, size_t terminal_width,
                                   lle_visual_footprint_t *footprint) {
    if (!text || !footprint) return false;
    
    // Initialize footprint
    memset(footprint, 0, sizeof(lle_visual_footprint_t));
    
    size_t current_col = prompt_width;
    size_t rows = 0;
    size_t visual_chars = 0;
    
    for (size_t i = 0; i < length; ) {
        // Handle UTF-8 character width properly
        size_t char_bytes;
        size_t char_width = lle_utf8_char_display_width(text + i, &char_bytes);
        
        if (current_col + char_width >= terminal_width) {
            // Line wrap occurs
            rows++;
            current_col = char_width;
            footprint->wraps_lines = true;
        } else {
            current_col += char_width;
        }
        
        visual_chars += char_width;
        i += char_bytes;
    }
    
    footprint->rows_used = rows;
    footprint->end_column = current_col;
    footprint->total_visual_width = visual_chars;
    
    return true;
}
```

### ✅ **Task 1.3: Update Display State Initialization**

**File**: `src/line_editor/display.c`

- [ ] Find `lle_display_create()` or similar initialization function
- [ ] Add initialization of new fields:
```c
// ADD to display state initialization
display->last_visual_rows = 0;
display->last_visual_end_col = 0;
display->last_total_chars = 0;
display->last_had_wrapping = false;
display->last_content_hash = 0;
display->syntax_highlighting_applied = false;
memset(&display->clear_start, 0, sizeof(lle_terminal_coordinates_t));
memset(&display->clear_end, 0, sizeof(lle_terminal_coordinates_t));
```

### ✅ **Task 1.4: Build and Test Phase 1**

- [ ] Build: `scripts/lle_build.sh build`
- [ ] Test: `scripts/lle_build.sh test`
- [ ] Verify no regressions: All existing tests should pass
- [ ] Commit: `git commit -am "LLE-051: Enhanced display state tracking"`

## Phase 2: Intelligent Clearing Strategy (3-4 hours)

### 📁 **Files to Modify**
- `src/line_editor/terminal_manager.h`
- `src/line_editor/terminal_manager.c`

### ✅ **Task 2.1: Add Clearing Function Declarations**

**File**: `src/line_editor/terminal_manager.h`

- [ ] Add function declarations:
```c
bool lle_clear_visual_region(lle_terminal_manager_t *tm, 
                           const lle_visual_footprint_t *old_footprint,
                           const lle_visual_footprint_t *new_footprint);

bool lle_clear_multi_line_fallback(lle_terminal_manager_t *tm,
                                  const lle_visual_footprint_t *footprint);

bool lle_clear_multi_line_precise(lle_terminal_manager_t *tm,
                                 const lle_terminal_coordinates_t *cursor_pos,
                                 const lle_visual_footprint_t *footprint);
```

### ✅ **Task 2.2: Implement Region-Based Clearing**

**File**: `src/line_editor/terminal_manager.c`

- [ ] Implement main clearing function:
```c
bool lle_clear_visual_region(lle_terminal_manager_t *tm, 
                           const lle_visual_footprint_t *old_footprint,
                           const lle_visual_footprint_t *new_footprint) {
    if (!tm || !old_footprint) return false;
    
    // Strategy selection based on complexity
    if (!old_footprint->wraps_lines && 
        (!new_footprint || !new_footprint->wraps_lines)) {
        // Simple case: single line clearing
        return lle_terminal_clear_to_end_of_line(tm);
    }
    
    // Complex case: multi-line clearing
    lle_terminal_coordinates_t cursor_pos;
    if (lle_terminal_get_cursor_position(tm, &cursor_pos)) {
        // Precise clearing with known position
        return lle_clear_multi_line_precise(tm, &cursor_pos, old_footprint);
    } else {
        // Fallback clearing strategy
        return lle_clear_multi_line_fallback(tm, old_footprint);
    }
}
```

### ✅ **Task 2.3: Implement Fallback Clearing**

**File**: `src/line_editor/terminal_manager.c`

- [ ] Implement fallback strategy:
```c
bool lle_clear_multi_line_fallback(lle_terminal_manager_t *tm,
                                  const lle_visual_footprint_t *footprint) {
    if (!tm || !footprint) return false;
    
    // Strategy: Clear more than needed to ensure completeness
    size_t clear_rows = footprint->rows_used + 1;  // Safety margin
    
    // Clear current line
    if (!lle_terminal_clear_to_end_of_line(tm)) return false;
    
    // Clear additional rows
    for (size_t i = 1; i <= clear_rows; i++) {
        // Move down and clear
        if (!lle_terminal_write_escape(tm, "\n\x1b[K")) return false;
    }
    
    // Return to original position
    for (size_t i = 0; i <= clear_rows; i++) {
        if (!lle_terminal_write_escape(tm, "\x1b[1A")) return false;
    }
    
    return true;
}
```

### ✅ **Task 2.4: Implement Precise Clearing**

**File**: `src/line_editor/terminal_manager.c`

- [ ] Implement precise clearing:
```c
bool lle_clear_multi_line_precise(lle_terminal_manager_t *tm,
                                 const lle_terminal_coordinates_t *cursor_pos,
                                 const lle_visual_footprint_t *footprint) {
    if (!tm || !cursor_pos || !footprint) return false;
    
    // Clear from current position to end of old content
    for (size_t row = 0; row <= footprint->rows_used; row++) {
        if (row == 0) {
            // Current line: clear from cursor to end
            if (!lle_terminal_clear_to_end_of_line(tm)) return false;
        } else {
            // Additional lines: clear entirely
            if (!lle_terminal_move_cursor(tm, cursor_pos->row + row, 0)) return false;
            if (!lle_terminal_clear_to_end_of_line(tm)) return false;
        }
    }
    
    // Return to correct position
    return lle_terminal_move_cursor(tm, cursor_pos->row, cursor_pos->col);
}
```

### ✅ **Task 2.5: Build and Test Phase 2**

- [ ] Build: `scripts/lle_build.sh build`
- [ ] Test: `scripts/lle_build.sh test`
- [ ] Verify no regressions
- [ ] Commit: `git commit -am "LLE-052: Intelligent clearing strategy"`

## Phase 3: Consistent Rendering Behavior (2-3 hours)

### 📁 **Files to Modify**
- `src/line_editor/display.c`

### ✅ **Task 3.1: Add Unified Rendering Function**

**File**: `src/line_editor/display.c`

- [ ] Add function declaration to header if needed
- [ ] Implement unified rendering:
```c
bool lle_display_update_unified(lle_display_state_t *display, 
                               bool force_full_render) {
    if (!display) return false;
    
    // Calculate visual footprints
    lle_visual_footprint_t old_footprint = {
        .rows_used = display->last_visual_rows,
        .end_column = display->last_visual_end_col,
        .wraps_lines = display->last_had_wrapping,
        .total_visual_width = display->last_total_chars
    };
    
    lle_visual_footprint_t new_footprint = {0};
    const char *text = lle_text_buffer_get_text(display->text_buffer);
    size_t length = lle_text_buffer_get_length(display->text_buffer);
    
    if (!lle_calculate_visual_footprint(text, length, 
                                       display->prompt_width,
                                       display->terminal->geometry.width,
                                       &new_footprint)) {
        return false;
    }
    
    // Determine rendering strategy
    bool needs_full_render = force_full_render || 
                           (old_footprint.wraps_lines != new_footprint.wraps_lines) ||
                           (old_footprint.rows_used != new_footprint.rows_used);
    
    // Apply consistent rendering
    bool result;
    if (needs_full_render) {
        result = lle_render_with_consistent_highlighting(display, &old_footprint, &new_footprint);
    } else {
        result = lle_render_incremental_safe(display, &old_footprint, &new_footprint);
    }
    
    // Update display state
    if (result) {
        display->last_visual_rows = new_footprint.rows_used;
        display->last_visual_end_col = new_footprint.end_column;
        display->last_had_wrapping = new_footprint.wraps_lines;
        display->last_total_chars = new_footprint.total_visual_width;
    }
    
    return result;
}
```

### ✅ **Task 3.2: Implement Consistent Highlighting**

**File**: `src/line_editor/display.c`

- [ ] Implement highlighting consistency:
```c
bool lle_render_with_consistent_highlighting(lle_display_state_t *display,
                                           const lle_visual_footprint_t *old_footprint,
                                           const lle_visual_footprint_t *new_footprint) {
    if (!display) return false;
    
    // POLICY: Only apply syntax highlighting if it was applied during normal typing
    bool should_highlight = display->syntax_highlighting_applied && 
                          lle_should_apply_syntax_highlighting(display);
    
    // Clear old content properly
    if (!lle_clear_visual_region(display->terminal, old_footprint, new_footprint)) {
        return false;
    }
    
    // Render new content with consistent highlighting
    if (should_highlight) {
        return lle_render_with_syntax_highlighting(display);
    } else {
        return lle_render_plain_text(display);
    }
}
```

### ✅ **Task 3.3: Implement Safe Incremental Rendering**

**File**: `src/line_editor/display.c`

- [ ] Implement safe incremental rendering:
```c
bool lle_render_incremental_safe(lle_display_state_t *display,
                                const lle_visual_footprint_t *old_footprint,
                                const lle_visual_footprint_t *new_footprint) {
    if (!display) return false;
    
    // Use existing incremental logic but with footprint awareness
    // This should call the existing lle_display_update_incremental() 
    // but with enhanced clearing based on footprints
    
    return lle_display_update_incremental(display);
}
```

### ✅ **Task 3.4: Build and Test Phase 3**

- [ ] Build: `scripts/lle_build.sh build`
- [ ] Test: `scripts/lle_build.sh test`
- [ ] Verify no regressions
- [ ] Commit: `git commit -am "LLE-053: Consistent rendering behavior"`

## Phase 4: Enhanced Backspace Logic (2-3 hours)

### 📁 **Files to Modify**
- `src/line_editor/line_editor.c`
- `src/line_editor/display.c`

### ✅ **Task 4.1: Add Backspace Strategy Types**

**File**: `src/line_editor/display.h` or appropriate header

- [ ] Add strategy enumeration:
```c
typedef enum {
    LLE_BACKSPACE_SIMPLE,      // Single character, same line
    LLE_BACKSPACE_BOUNDARY,    // Crosses line wrap boundary
    LLE_BACKSPACE_COMPLEX      // Multiple line changes
} lle_backspace_strategy_t;
```

### ✅ **Task 4.2: Implement Strategy Selection**

**File**: `src/line_editor/display.c`

- [ ] Implement strategy selection:
```c
lle_backspace_strategy_t lle_determine_backspace_strategy(
    const lle_visual_footprint_t *current,
    const lle_visual_footprint_t *after_backspace) {
    
    if (!current || !after_backspace) return LLE_BACKSPACE_COMPLEX;
    
    if (!current->wraps_lines && !after_backspace->wraps_lines) {
        return LLE_BACKSPACE_SIMPLE;
    }
    
    if (current->rows_used != after_backspace->rows_used) {
        return LLE_BACKSPACE_BOUNDARY;
    }
    
    return LLE_BACKSPACE_COMPLEX;
}
```

### ✅ **Task 4.3: Implement Enhanced Backspace**

**File**: `src/line_editor/line_editor.c`

- [ ] Find existing backspace handler (likely in input processing)
- [ ] Replace with enhanced version:
```c
bool lle_handle_backspace_enhanced(lle_line_editor_t *editor) {
    if (!editor || !editor->display) return false;
    
    lle_text_buffer_t *buffer = editor->display->text_buffer;
    size_t cursor_pos = lle_text_buffer_get_cursor_position(buffer);
    
    if (cursor_pos == 0) return true;  // Nothing to delete
    
    // Calculate footprints before and after backspace
    lle_visual_footprint_t current_footprint = {0};
    lle_visual_footprint_t after_footprint = {0};
    
    const char *current_text = lle_text_buffer_get_text(buffer);
    size_t current_length = lle_text_buffer_get_length(buffer);
    
    // Calculate current footprint
    lle_calculate_visual_footprint(current_text, current_length,
                                 editor->display->prompt_width,
                                 editor->display->terminal->geometry.width,
                                 &current_footprint);
    
    // Simulate backspace for footprint calculation
    // NOTE: This needs proper UTF-8 handling
    size_t char_bytes = 1;  // Simplified - implement lle_utf8_prev_char_bytes()
    size_t new_length = current_length - char_bytes;
    
    lle_calculate_visual_footprint(current_text, new_length,
                                 editor->display->prompt_width,
                                 editor->display->terminal->geometry.width,
                                 &after_footprint);
    
    // Determine strategy
    lle_backspace_strategy_t strategy = lle_determine_backspace_strategy(
        &current_footprint, &after_footprint);
    
    // Perform the backspace operation
    if (!lle_text_buffer_backspace(buffer)) return false;
    
    // Apply appropriate update strategy
    switch (strategy) {
        case LLE_BACKSPACE_SIMPLE:
            return lle_display_update_unified(editor->display, false);
        
        case LLE_BACKSPACE_BOUNDARY:
        case LLE_BACKSPACE_COMPLEX:
            return lle_display_update_unified(editor->display, true);
        
        default:
            return false;
    }
}
```

### ✅ **Task 4.4: Integrate with Input Processing**

**File**: `src/line_editor/line_editor.c`

- [ ] Find backspace key handling in input loop
- [ ] Replace existing call with `lle_handle_backspace_enhanced(editor)`
- [ ] Ensure proper error handling

### ✅ **Task 4.5: Build and Test Phase 4**

- [ ] Build: `scripts/lle_build.sh build`
- [ ] Test: `scripts/lle_build.sh test`
- [ ] Manual test with long lines that wrap
- [ ] Verify backspace across boundaries works correctly
- [ ] Commit: `git commit -am "LLE-054: Enhanced backspace logic"`

## Phase 5: Integration and Testing (2-3 hours)

### 📁 **Files to Create**
- `tests/line_editor/test_backspace_boundaries.c`

### ✅ **Task 5.1: Create Comprehensive Test Suite**

**File**: `tests/line_editor/test_backspace_boundaries.c`

- [ ] Create test file with LLE test framework:
```c
#include "lle_test_framework.h"
#include "../src/line_editor/display.h"
#include "../src/line_editor/text_buffer.h"

LLE_TEST(visual_footprint_single_line) {
    printf("Testing visual footprint calculation for single line... ");
    
    lle_visual_footprint_t footprint;
    bool result = lle_calculate_visual_footprint("hello world", 11, 10, 80, &footprint);
    
    LLE_ASSERT(result == true);
    LLE_ASSERT(footprint.rows_used == 0);
    LLE_ASSERT(footprint.wraps_lines == false);
    LLE_ASSERT(footprint.total_visual_width == 11);
    
    printf("✅ PASSED\n");
}

LLE_TEST(visual_footprint_wrapped_lines) {
    printf("Testing visual footprint calculation for wrapped lines... ");
    
    // Create text that will definitely wrap
    char long_text[200];
    memset(long_text, 'a', 150);
    long_text[150] = '\0';
    
    lle_visual_footprint_t footprint;
    bool result = lle_calculate_visual_footprint(long_text, 150, 10, 80, &footprint);
    
    LLE_ASSERT(result == true);
    LLE_ASSERT(footprint.rows_used > 0);
    LLE_ASSERT(footprint.wraps_lines == true);
    
    printf("✅ PASSED\n");
}

LLE_TEST(backspace_strategy_selection) {
    printf("Testing backspace strategy selection... ");
    
    lle_visual_footprint_t single_line = {0, 50, false, 50};
    lle_visual_footprint_t after_single = {0, 49, false, 49};
    
    lle_backspace_strategy_t strategy = lle_determine_backspace_strategy(
        &single_line, &after_single);
    
    LLE_ASSERT(strategy == LLE_BACKSPACE_SIMPLE);
    
    lle_visual_footprint_t wrapped = {1, 20, true, 100};
    lle_visual_footprint_t after_wrapped = {0, 79, false, 99};
    
    strategy = lle_determine_backspace_strategy(&wrapped, &after_wrapped);
    LLE_ASSERT(strategy == LLE_BACKSPACE_BOUNDARY);
    
    printf("✅ PASSED\n");
}

// Add more tests as needed...

int main(void) {
    printf("Running backspace boundary tests...\n\n");
    
    test_visual_footprint_single_line();
    test_visual_footprint_wrapped_lines();
    test_backspace_strategy_selection();
    
    printf("\n✅ All backspace boundary tests passed!\n");
    return 0;
}
```

### ✅ **Task 5.2: Add Test to Build System**

**File**: `tests/line_editor/meson.build`

- [ ] Add new test to build configuration:
```meson
test_backspace_boundaries = executable(
    'test_backspace_boundaries',
    'test_backspace_boundaries.c',
    dependencies: [line_editor_dep],
    include_directories: [line_editor_inc, test_inc]
)

test('backspace boundaries', test_backspace_boundaries)
```

### ✅ **Task 5.3: Performance Testing**

- [ ] Manual performance test with debug timing
- [ ] Test with very long lines (200+ characters)
- [ ] Test rapid backspace operations
- [ ] Verify memory usage remains stable

### ✅ **Task 5.4: Integration Testing**

- [ ] Test full editing workflow
- [ ] Test with syntax highlighting enabled/disabled
- [ ] Test terminal resize during editing
- [ ] Test with various terminal widths

### ✅ **Task 5.5: Build and Test Phase 5**

- [ ] Build: `scripts/lle_build.sh build`
- [ ] Test: `scripts/lle_build.sh test`
- [ ] Run specific test: `meson test -C builddir test_backspace_boundaries`
- [ ] Verify all tests pass
- [ ] Commit: `git commit -am "LLE-055: Comprehensive backspace refinement testing"`

## Final Integration and Validation

### ✅ **Final Testing Checklist**

- [ ] All unit tests pass: `scripts/lle_build.sh test`
- [ ] Manual testing with long commands that wrap
- [ ] Backspace across line boundaries works without artifacts
- [ ] Syntax highlighting is consistent
- [ ] No performance regressions
- [ ] Memory usage stable (run with valgrind if available)

### ✅ **Real-World Validation**

- [ ] Test the exact scenario from original bug report:
  - Type very long echo command that wraps
  - Backspace across the wrap boundary
  - Verify complete clearing without highlighted remnants
  - Verify consistent visual appearance

- [ ] Test additional scenarios:
  - Multiple wrapped lines
  - Backspace from middle of wrapped content
  - Terminal resize during editing
  - UTF-8 characters in wrapped content

### ✅ **Documentation Updates**

- [ ] Update `AI_CONTEXT.md` status to reflect completion
- [ ] Update `LLE_PROGRESS.md` with completion status
- [ ] Update `LLE_STABLE_FUNCTIONALITY.md` to mark backspace as stable

### ✅ **Final Commit and Merge**

- [ ] Final commit: `git commit -am "Complete backspace line wrap refinement - all issues resolved"`
- [ ] Push branch: `git push origin task/backspace-refinement`
- [ ] Create pull request or merge to main development branch
- [ ] Update documentation to reflect production readiness

## Success Criteria Validation

Upon completion, verify these success criteria are met:

- ✅ **Complete clearing** across line boundaries - no visual remnants
- ✅ **Consistent syntax highlighting** - same appearance regardless of edit path
- ✅ **No visual artifacts** - clean display after backspace operations
- ✅ **Proper cursor positioning** - correct position after boundary crossings
- ✅ **Robust fallback** - graceful handling of edge cases
- ✅ **Performance maintained** - no noticeable lag during editing
- ✅ **Zero regressions** - all existing functionality still works

## Emergency Rollback Plan

If critical issues arise during implementation:

1. **Immediate rollback**: `git checkout feature/lusush-line-editor && git reset --hard HEAD~n`
2. **Identify specific issue**: Use `git bisect` to find problematic commit
3. **Selective revert**: Revert specific problematic changes while keeping working parts
4. **Fallback to safe state**: Return to last known good configuration

## Implementation Notes

- **Estimated Total Time**: 12-16 hours across 5 phases
- **Critical Dependencies**: Each phase builds on the previous, complete sequentially
- **Testing Strategy**: Build and test after each phase to catch issues early
- **Performance Focus**: Monitor performance impact throughout implementation
- **Documentation**: Update relevant docs immediately upon completion

This checklist ensures systematic, reliable implementation of the backspace line wrap refinement with minimal risk and maximum confidence in the result.