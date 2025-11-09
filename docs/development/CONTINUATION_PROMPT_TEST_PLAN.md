# Continuation Prompt Layer - Manual Test Plan

**Date Created**: 2025-11-09  
**Status**: Active  
**Related**: CONTINUATION_PROMPT_TEST_TRACKER.md, CONTINUATION_PROMPT_HANDOFF.md

---

## Overview

This document provides systematic manual test procedures for the continuation prompt layer. The layer provides context-aware prompts when entering multi-line shell constructs.

**Current Mode**: Context-Aware (displays `if>`, `loop>`, `func>`, `sh>`, `quote>`)

---

## Prerequisites

- Build lusush: `ninja -C builddir`
- Run lusush: `./builddir/lusush`
- Continuation prompts should appear automatically when pressing Enter in multi-line constructs

---

## Test Categories

### Category 1: If Statement Prompts

**Test 1.1: Simple If Statement**
```bash
# Steps:
1. Type: if [ -f /etc/passwd ]
2. Press: Enter
3. Observe: Should display "if> " prompt on next line
4. Type: then
5. Press: Enter
6. Observe: Should display "if> " prompt
7. Type: echo "file exists"
8. Press: Enter
9. Observe: Should display "if> " prompt
10. Type: fi
11. Press: Enter
12. Observe: Command should execute

# Expected: All continuation lines show "if> "
# Actual: (to be filled during testing)
```

**Test 1.2: If-Else Statement**
```bash
# Steps:
1. Type: if [ -d /tmp ]
2. Press: Enter
3. Observe: "if> " prompt
4. Type: then
5. Press: Enter
6. Type: echo "directory exists"
7. Press: Enter
8. Type: else
9. Press: Enter
10. Observe: Should still show "if> " prompt
11. Type: echo "no directory"
12. Press: Enter
13. Type: fi
14. Press: Enter

# Expected: All lines show "if> "
# Actual: (to be filled during testing)
```

**Test 1.3: Nested If Statements**
```bash
# Steps:
1. Type: if [ -f /etc/passwd ]
2. Press: Enter
3. Type: then
4. Press: Enter
5. Type: if [ -d /tmp ]
6. Press: Enter
7. Observe: Should show "if> " for nested if
8. Type: then
9. Press: Enter
10. Type: echo "both exist"
11. Press: Enter
12. Type: fi
13. Press: Enter
14. Type: fi
15. Press: Enter

# Expected: All lines show "if> " (even nested)
# Actual: (to be filled during testing)
```

---

### Category 2: Loop Prompts

**Test 2.1: For Loop**
```bash
# Steps:
1. Type: for i in 1 2 3
2. Press: Enter
3. Observe: Should display "loop> " prompt
4. Type: do
5. Press: Enter
6. Observe: Should display "loop> " prompt
7. Type: echo $i
8. Press: Enter
9. Type: done
10. Press: Enter
11. Observe: Should execute and print 1, 2, 3

# Expected: All continuation lines show "loop> "
# Actual: (to be filled during testing)
```

**Test 2.2: While Loop**
```bash
# Steps:
1. Type: while true
2. Press: Enter
3. Observe: Should display "loop> " prompt
4. Type: do
5. Press: Enter
6. Type: echo "looping"
7. Press: Enter
8. Type: break
9. Press: Enter
10. Type: done
11. Press: Enter

# Expected: All continuation lines show "loop> "
# Actual: (to be filled during testing)
```

**Test 2.3: Until Loop**
```bash
# Steps:
1. Type: x=0
2. Press: Enter
3. Type: until [ $x -gt 3 ]
4. Press: Enter
5. Observe: Should display "loop> " prompt
6. Type: do
7. Press: Enter
8. Type: echo $x
9. Press: Enter
10. Type: x=$((x+1))
11. Press: Enter
12. Type: done
13. Press: Enter

# Expected: All continuation lines show "loop> "
# Actual: (to be filled during testing)
```

---

### Category 3: Function Definition Prompts

**Test 3.1: Simple Function**
```bash
# Steps:
1. Type: myfunc() {
2. Press: Enter
3. Observe: Should display "func> " prompt
4. Type: echo "hello from function"
5. Press: Enter
6. Type: }
7. Press: Enter
8. Type: myfunc
9. Press: Enter
10. Observe: Should print "hello from function"

# Expected: Lines 3-6 show "func> "
# Actual: (to be filled during testing)
# Note: This test currently fails in unit tests - function detection not working
```

**Test 3.2: Multi-line Function**
```bash
# Steps:
1. Type: greet() {
2. Press: Enter
3. Observe: "func> " prompt (if working)
4. Type: local name=$1
5. Press: Enter
6. Type: echo "Hello, $name"
7. Press: Enter
8. Type: }
9. Press: Enter

# Expected: Lines show "func> "
# Actual: (to be filled during testing)
# Note: May show "> " if function detection is not working
```

---

### Category 4: Subshell and Command Grouping

**Test 4.1: Subshell**
```bash
# Steps:
1. Type: (
2. Press: Enter
3. Observe: Should display "sh> " prompt
4. Type: cd /tmp
5. Press: Enter
6. Type: pwd
7. Press: Enter
8. Type: )
9. Press: Enter
10. Type: pwd
11. Press: Enter
12. Observe: Second pwd should show original directory

# Expected: Lines 3-8 show "sh> "
# Actual: (to be filled during testing)
# Note: This test currently fails in unit tests - subshell detection not working
```

**Test 4.2: Command Group (curly braces)**
```bash
# Steps:
1. Type: {
2. Press: Enter
3. Observe: Continuation prompt (may vary)
4. Type: echo "group command 1"
5. Press: Enter
6. Type: echo "group command 2"
7. Press: Enter
8. Type: }
9. Press: Enter

# Expected: Shows continuation prompt
# Actual: (to be filled during testing)
```

---

### Category 5: Quote Continuation

**Test 5.1: Double Quote Continuation**
```bash
# Steps:
1. Type: echo "line one
2. Press: Enter (without closing quote)
3. Observe: Should display "quote> " or "> " prompt
4. Type: line two"
5. Press: Enter
6. Observe: Should execute and print both lines

# Expected: Line 3 shows quote continuation prompt
# Actual: (to be filled during testing)
```

**Test 5.2: Single Quote Continuation**
```bash
# Steps:
1. Type: echo 'line one
2. Press: Enter (without closing quote)
3. Observe: Continuation prompt
4. Type: line two'
5. Press: Enter

# Expected: Shows quote continuation prompt
# Actual: (to be filled during testing)
```

**Test 5.3: Command Substitution in Quotes**
```bash
# Steps:
1. Type: echo "Current directory is
2. Press: Enter
3. Type: $(pwd)"
4. Press: Enter

# Expected: Shows quote continuation prompt
# Actual: (to be filled during testing)
```

---

### Category 6: Pipeline and Command Continuation

**Test 6.1: Pipe Continuation**
```bash
# Steps:
1. Type: echo "hello world" |
2. Press: Enter
3. Observe: Continuation prompt
4. Type: tr '[:lower:]' '[:upper:]'
5. Press: Enter
6. Observe: Should print "HELLO WORLD"

# Expected: Shows continuation prompt ("> ")
# Actual: (to be filled during testing)
```

**Test 6.2: Backslash Continuation**
```bash
# Steps:
1. Type: echo "this is a very long line" \
2. Press: Enter
3. Observe: Continuation prompt
4. Type: "that continues here"
5. Press: Enter

# Expected: Shows continuation prompt
# Actual: (to be filled during testing)
```

---

### Category 7: Case Statement

**Test 7.1: Case Statement**
```bash
# Steps:
1. Type: case $USER in
2. Press: Enter
3. Observe: Continuation prompt
4. Type: root)
5. Press: Enter
6. Type: echo "admin user"
7. Press: Enter
8. Type: ;;
9. Press: Enter
10. Type: *)
11. Press: Enter
12. Type: echo "regular user"
13. Press: Enter
14. Type: ;;
15. Press: Enter
16. Type: esac
17. Press: Enter

# Expected: Shows continuation prompts throughout
# Actual: (to be filled during testing)
```

---

### Category 8: Visual Elements and Edge Cases

**Test 8.1: Empty Continuation Lines**
```bash
# Steps:
1. Type: if [ -f /etc/passwd ]
2. Press: Enter
3. Observe: "if> " prompt appears immediately
4. Press: Enter (on empty line)
5. Observe: "if> " should still appear on next line
6. Type: then
7. Press: Enter

# Expected: Continuation prompt appears even on empty lines
# Actual: (to be filled during testing)
```

**Test 8.2: Syntax Highlighting Preservation**
```bash
# Steps:
1. Type: if [ -f /etc/passwd ]
2. Press: Enter
3. Type: then echo "exists"
4. Observe: Check if "echo" is highlighted (if syntax highlighting enabled)
5. Press: Enter
6. Type: fi
7. Press: Enter

# Expected: Syntax highlighting works with continuation prompts
# Actual: (to be filled during testing)
```

**Test 8.3: Cursor Positioning**
```bash
# Steps:
1. Type: for i in 1 2 3
2. Press: Enter
3. Observe: Cursor should be positioned after "loop> " prompt
4. Type: do
5. Press: Left Arrow multiple times
6. Observe: Cursor should not move into the "loop> " prompt area
7. Press: Home
8. Observe: Cursor should move to start of editable area (after prompt)

# Expected: Cursor confined to editable area, not prompt
# Actual: (to be filled during testing)
```

**Test 8.4: Terminal Resize During Multi-line**
```bash
# Steps:
1. Type: if [ -f /etc/passwd ]
2. Press: Enter
3. Observe: "if> " prompt
4. Resize terminal window (make narrower/wider)
5. Observe: Display should redraw correctly
6. Type: then
7. Press: Enter
8. Type: fi
9. Press: Enter

# Expected: Display remains correct after resize
# Actual: (to be filled during testing)
```

---

### Category 9: Editing Multi-line Constructs

**Test 9.1: Up Arrow to Previous Line**
```bash
# Steps:
1. Type: for i in 1 2 3
2. Press: Enter
3. Type: do
4. Press: Enter
5. Press: Up Arrow
6. Observe: Should show previous line "do" for editing
7. Press: Up Arrow again
8. Observe: Should show "for i in 1 2 3"

# Expected: Arrow navigation works across multi-line construct
# Actual: (to be filled during testing)
```

**Test 9.2: Editing and Re-entering Line**
```bash
# Steps:
1. Type: if [ -f /etc/passwd ]
2. Press: Enter
3. Type: then
4. Press: Enter
5. Press: Up Arrow
6. Edit to: then echo "editing"
7. Press: Enter
8. Observe: New line appears with "if> " prompt

# Expected: Editing preserves multi-line state
# Actual: (to be filled during testing)
```

---

### Category 10: Mode Switching (if implemented)

**Test 10.1: Switch to Simple Mode**
```bash
# If there's a command to switch modes (check implementation):
# Steps would depend on how mode switching is exposed to user
# May require configuration or runtime command

# Expected: All continuation prompts show "> " instead of context-aware
# Actual: (to be filled during testing)
```

---

## Test Result Format

For each test, record results in CONTINUATION_PROMPT_TEST_TRACKER.md using:

```
Status: [ ] PASS / [ ] FAIL / [ ] PARTIAL / [ ] SKIP
Expected: [what should happen]
Actual: [what actually happened]
Notes: [any observations, issues, or additional context]
```

---

## Known Limitations

Based on unit test results, the following features may not work:

1. **Function Detection**: `myfunc() { ... }` may show `>` instead of `func>`
2. **Subshell Detection**: `( ... )` may show `>` instead of `sh>`
3. **Quote Handling**: May show `quote>` when simple `>` expected, or vice versa

These are limitations in the underlying `input_continuation.c` detection logic.

---

## Test Environment Info

Record the following when testing:

- **Date**: 
- **Lusush Version**: 1.3.0
- **Build**: `git rev-parse --short HEAD`
- **Terminal**: (e.g., xterm, gnome-terminal, kitty)
- **Terminal Size**: (e.g., 80x24, 120x40)
- **Shell Config**: (any custom PS1, PS2, or lusush config)

---

## Success Criteria

- **Critical (must pass)**: 
  - If statement prompts (1.1, 1.2)
  - Loop prompts (2.1, 2.2)
  - Quote continuation (5.1, 5.2)
  - Cursor positioning (8.3)
  - Empty line prompts (8.1)

- **Important (should pass)**:
  - Pipeline continuation (6.1)
  - Terminal resize (8.4)
  - Arrow navigation (9.1)
  - Syntax highlighting (8.2)

- **Nice to have**:
  - Function prompts (3.1, 3.2) - known limitation
  - Subshell prompts (4.1) - known limitation
  - Case statements (7.1)
