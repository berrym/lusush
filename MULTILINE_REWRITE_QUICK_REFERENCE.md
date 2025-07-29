# Multi-Line Architecture Rewrite - Quick Reference Card

**Status**: Phase 1A ✅ COMPLETE | Phase 2A 🚧 READY FOR DEVELOPMENT  
**Current Task**: Rewrite `lle_display_render()` function for absolute positioning  

## 🚨 THE PROBLEM (30-Second Summary)
```c
// BROKEN (Current): Single-line positioning for multi-line content
cursor_pos = lle_calculate_cursor_position(...);  // Returns (row=1, col=0) ✅
lle_terminal_move_cursor_to_column(terminal, 0);  // Sends \x1b[1G ❌ WRONG LINE

// FIXED (Target): Absolute positioning for multi-line content  
cursor_pos = lle_calculate_cursor_position(...);  // Returns (row=1, col=0) ✅
lle_terminal_move_cursor(terminal, 1, 0);         // Sends \x1b[2;1H ✅ CORRECT
```

## 🎯 IMMEDIATE NEXT TASKS (Phase 2A.1)
- [ ] **Week 1**: Rewrite `lle_display_render()` (src/line_editor/display.c:388-392)
- [ ] **Week 1**: Add prompt position tracking to display state
- [ ] **Week 2**: Replace ALL `lle_terminal_move_cursor_to_column()` calls
- [ ] **Week 2**: Update clearing operations for multi-line regions

## 🔧 KEY FUNCTIONS (Phase 1A - READY TO USE)

### Coordinate Conversion
```c
// Convert relative cursor position → absolute terminal coordinates
lle_terminal_coordinates_t terminal_pos = lle_convert_to_terminal_coordinates(
    &cursor_pos, state->prompt_start_row, state->prompt_start_col);

// Validate before use
if (!lle_validate_terminal_coordinates(&terminal_pos, &state->geometry)) {
    return false;
}

// Use absolute positioning
lle_terminal_move_cursor(state->terminal, terminal_pos.terminal_row, terminal_pos.terminal_col);
```

### Multi-Line Clearing
```c
// Clear rectangular region (instead of single-line clearing)
lle_terminal_clear_region(state->terminal, start_row, start_col, end_row, end_col);

// Clear multiple complete lines
lle_terminal_clear_lines(state->terminal, start_row, num_lines);
```

### Position Tracking (Add to display functions)
```c
// Track where prompt/content are positioned on screen
state->prompt_start_row = current_terminal_row;
state->prompt_start_col = current_terminal_col;
state->content_start_row = prompt_end_row;
state->content_start_col = prompt_end_col + prompt_width;
state->position_tracking_valid = true;
```

## 📋 MANDATORY REPLACEMENT PATTERNS

### ❌ OLD (Single-Line) → ✅ NEW (Multi-Line)
```c
// REPLACE THIS:
lle_terminal_move_cursor_to_column(terminal, col);

// WITH THIS:
lle_terminal_coordinates_t pos = lle_convert_to_terminal_coordinates(
    &relative_pos, state->prompt_start_row, state->prompt_start_col);
lle_terminal_move_cursor(terminal, pos.terminal_row, pos.terminal_col);
```

```c
// REPLACE THIS:
lle_terminal_clear_to_eol(terminal);

// WITH THIS:
lle_terminal_clear_from_position_to_eol(terminal, row, col);
```

## 🎯 TARGET FUNCTIONS (Phase 2A)

### Phase 2A.1: Core Display Functions
- `lle_display_render()` - Line 388-392 ⚠️ PRIMARY TARGET
- `lle_display_update_incremental()` - Lines 657, 906  
- All clearing operations in display.c

### Phase 2A.2: Cursor Movement Functions  
- `lle_display_move_cursor_home()` - Line 1612
- `lle_display_move_cursor_end()` - Line 1667
- `lle_display_enter_search_mode()` - Line 1717
- `lle_display_exit_search_mode()` - Line 1747

## 🏗️ DEVELOPMENT WORKFLOW

### 1. Setup
```bash
# Read the complete plan
less MULTILINE_ARCHITECTURE_REWRITE_PLAN.md

# Study infrastructure (Phase 1A complete)
grep -n "lle_convert_to_terminal_coordinates" src/line_editor/cursor_math.c
grep -n "lle_terminal_clear_region" src/line_editor/terminal_manager.c
```

### 2. Find Single-Line Positioning
```bash
# Find all single-line positioning calls (THESE NEED TO BE REPLACED)
grep -n "lle_terminal_move_cursor_to_column" src/line_editor/display.c
grep -n "lle_terminal_clear_to_eol" src/line_editor/display.c
```

### 3. Development Pattern
```c
// For each function that needs updating:

// 1. Add position tracking
state->prompt_start_row = /* calculate current prompt row */;
state->prompt_start_col = /* calculate current prompt col */;

// 2. Convert relative → absolute coordinates  
lle_terminal_coordinates_t abs_pos = lle_convert_to_terminal_coordinates(
    &relative_cursor_pos, state->prompt_start_row, state->prompt_start_col);

// 3. Use absolute positioning
lle_terminal_move_cursor(state->terminal, abs_pos.terminal_row, abs_pos.terminal_col);

// 4. Use multi-line clearing
lle_terminal_clear_region(state->terminal, start_row, start_col, end_row, end_col);
```

### 4. Testing
```bash
# Test new functionality
meson test -C builddir test_multiline_architecture_rewrite -v

# Test existing functionality still works
meson test -C builddir test_lle_018_multiline_input_display -v

# Test complete suite
meson test -C builddir --no-rebuild | tail -10
```

## 📊 PROGRESS TRACKING

### ✅ Phase 1A: Infrastructure (COMPLETE)
- [x] Position tracking in display state
- [x] Coordinate conversion functions  
- [x] Multi-line terminal operations
- [x] Comprehensive test suite (15 tests)
- [x] 497+ existing tests still pass

### 🚧 Phase 2A: Core Display Rewrite (IN PROGRESS)
- [ ] `lle_display_render()` rewrite
- [ ] Prompt position tracking
- [ ] Replace all single-line positioning
- [ ] Multi-line clearing integration

## 🚨 CRITICAL FILES TO MODIFY

### Primary Target: `src/line_editor/display.c`
```c
// Lines to focus on:
// 388-392: lle_display_render() cursor positioning
// 657: lle_display_update_incremental() wrap boundary 
// 906: lle_display_update_incremental() cursor positioning
// 1612: lle_display_move_cursor_home() 
// 1667: lle_display_move_cursor_end()
```

### Supporting Infrastructure (READY TO USE):
- `src/line_editor/cursor_math.h/c` - Coordinate conversion functions
- `src/line_editor/terminal_manager.h/c` - Multi-line operations
- `tests/line_editor/test_multiline_architecture_rewrite.c` - Working examples

## 🏆 SUCCESS CRITERIA (Phase 2A)
- [ ] **No `lle_terminal_move_cursor_to_column()` calls** in display.c
- [ ] **All cursor positioning uses absolute coordinates**
- [ ] **Multi-line clearing for all clear operations**
- [ ] **Position tracking maintained consistently**
- [ ] **All tests pass** (existing + new multi-line tests)
- [ ] **Performance maintained** (sub-millisecond response)

## 💡 DEBUGGING TIPS

### Common Issues
1. **Segfaults**: Check display state initialization (`memset(&state, 0, sizeof(state))`)
2. **Wrong positioning**: Verify coordinate conversion and prompt tracking
3. **Clear failures**: Ensure multi-line clearing with correct bounds
4. **Performance**: Profile absolute vs column positioning differences

### Test With Multi-Line Content
```c
// Create long text that wraps multiple lines
char long_text[200];
memset(long_text, 'x', 199);
long_text[199] = '\0';
// Test all operations with this content
```

## 📞 EMERGENCY CONTACTS
- **Architecture Questions**: Read `DEFINITIVE_DEVELOPMENT_PATH.md`
- **Implementation Examples**: Study `tests/line_editor/test_multiline_architecture_rewrite.c`  
- **Rollback Plan**: All changes isolated to display.c - can revert if needed
- **Performance Issues**: Use existing performance test framework

---

**🎯 START HERE**: Focus on `lle_display_render()` function in `src/line_editor/display.c` line 388-392. Replace single-line positioning with coordinate conversion + absolute positioning pattern shown above.**

**The infrastructure is ready. Phase 2A development can begin immediately.**