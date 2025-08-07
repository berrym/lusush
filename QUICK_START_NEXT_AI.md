# QUICK START - NEXT AI ASSISTANT
================================================================================

**TASK**: Fix cursor positioning for line boundary crossing in multiline backspace
**TIME**: 30-60 minutes  
**STATUS**: 95% complete, one targeted fix needed

================================================================================
## IMMEDIATE ACTION REQUIRED
================================================================================

### **WHAT TO DO**
1. Open `src/line_editor/edit_commands.c`
2. Find lines ~350-365 (cross-line operation branch)
3. Replace simple backspace with proper cursor positioning
4. Test with "echo Hello" + backspace

### **EXACT PROBLEM**
When backspace crosses from wrapped line back to prompt line, cursor doesn't move up.

**Current debug output**:
```
[MATH_DEBUG] Cross-line operation: from line 1 to line 0
[MATH_DEBUG] Using simple backspace for line boundary crossing
```

**Need**: Cursor should move up to line 0 at calculated column position.

### **SOLUTION PATTERN**
Replace this code block (lines ~350-365):
```c
// FOR line boundary crossings, use simple backspace approach to avoid corruption
fprintf(stderr, "[MATH_DEBUG] Using simple backspace for line boundary crossing\n");
if (lle_termcap_move_cursor_left(1) == LLE_TERMCAP_OK) {
    // simple backspace logic...
}
```

**With this**:
```c
// Move cursor to target position using mathematical calculation
if (current_line > target_line) {
    size_t lines_to_move_up = current_line - target_line;
    if (lle_termcap_move_cursor_up((int)lines_to_move_up) == LLE_TERMCAP_OK) {
        fprintf(stderr, "[MATH_DEBUG] SUCCESS: Moved up %zu lines\n", lines_to_move_up);
        
        if (lle_termcap_cursor_to_column((int)target_col) == LLE_TERMCAP_OK) {
            fprintf(stderr, "[MATH_DEBUG] SUCCESS: Positioned at target column %zu\n", target_col);
            
            if (lle_termcap_clear_to_eol() == LLE_TERMCAP_OK) {
                fprintf(stderr, "[MATH_DEBUG] SUCCESS: Cleared deleted characters\n");
            }
        }
    }
}
```

================================================================================
## CONTEXT
================================================================================

### **What's Working (DON'T CHANGE)**
- ✅ Mathematical coordinate calculations  
- ✅ Same-line backspace operations
- ✅ State synchronization
- ✅ Termcap function integration
- ✅ Buffer management

### **Variables Available**
- `current_line` - Where cursor is now
- `target_line` - Where cursor should go  
- `target_col` - Target column position
- All termcap functions: `lle_termcap_move_cursor_up()`, `lle_termcap_cursor_to_column()`, etc.

### **Test Case**
```bash
scripts/lle_build.sh build
LLE_DEBUG=1 ./builddir/lusush
# Type: echo Hello
# Press backspace repeatedly  
# OBSERVE: Cursor should move up when crossing line boundary
```

### **Success Indicators**
```
[MATH_DEBUG] SUCCESS: Moved up 1 lines
[MATH_DEBUG] SUCCESS: Positioned at target column 80
[MATH_DEBUG] SUCCESS: Cleared deleted characters
```

================================================================================
## BUILD AND TEST
================================================================================

```bash
# Build
scripts/lle_build.sh build

# Test interactively  
LLE_DEBUG=1 ./builddir/lusush

# Look for successful cross-line cursor movement
```

**That's it! Simple targeted fix to complete the multiline backspace implementation.**

Read `NEXT_AI_ASSISTANT_FINAL_HANDOFF.md` for complete details if needed.