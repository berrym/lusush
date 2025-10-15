# LLE Known Issues and Technical Debt

**Last Updated:** 2025-10-15  
**Status:** Living Document - Update as issues are discovered or resolved

This document tracks known issues, technical debt, and TODO items discovered during LLE implementation. Issues are categorized by severity and component.

---

## Critical Issues

*Issues that violate core LLE principles or cause crashes/data loss.*

### None Currently

---

## High Priority Issues

*Issues that should be addressed before next phase completion.*

### ISSUE-001: Display System Uses Direct ANSI Escapes
- **Component:** Display System (`src/lle/foundation/display/display.c`)
- **Location:** `lle_display_flush()` line 577
- **Discovered:** Week 10 (2025-10-15)
- **Description:** The display flush function uses direct ANSI escape sequence `\x1b[H` (cursor home) instead of using the capability-based system
- **Violation:** LLE Principle - "No direct ANSI escapes except for DA1 terminal detection"
- **Current Behavior:** Moves cursor to top-left of screen on every render
- **Expected Behavior:** Should use `lle_terminal_capabilities_t` to get cursor positioning capability
- **Impact:** Breaks on non-VT100 terminals, violates abstraction layer
- **TODO:** 
  - Refactor `lle_display_flush()` to use capability system
  - Add capability lookup for cursor positioning
  - Use `term->capabilities->cursor_home` or equivalent
- **Target Week:** Week 15 (Multi-line Editing) - when refining display system for scroll regions

---

## Medium Priority Issues

*Issues that should be addressed eventually but don't block current progress.*

### ISSUE-002: Input Processor Week 10 Scope vs Full Spec 06
- **Component:** Input Processing (`src/lle/foundation/input/`)
- **Discovered:** Week 10 Planning (2025-10-14)
- **Description:** Current implementation covers Week 10 basic scope, but full Spec 06 features are marked as TODO
- **Missing Features:**
  - Mouse input processing
  - Widget hook integration
  - Keybinding engine integration
  - Adaptive terminal integration
  - Event system integration
  - UTF-8 multi-byte sequence handling
  - Complete parser state machine
  - Input stream management
- **Current Status:** All marked with `TODO` comments in code
- **Target Week:** Week 13-14 (Character Operations & Search) - when implementing advanced editing features that need full input system

### ISSUE-003: Test Programs Use getchar() Before Raw Mode
- **Component:** Test Programs (`src/lle/foundation/test/input_processor_tty_test.c`)
- **Location:** Line 66
- **Discovered:** Week 10 Testing (2025-10-15)
- **Description:** TTY test uses `getchar()` for "press any key to start" which causes input buffer pollution
- **Problem:** When user presses arrow keys during startup, `getchar()` only reads ESC, leaving `[D` in buffer
- **Impact:** Confusing test behavior, escape sequences appear as garbage during startup
- **Solution:** Either remove getchar() (use sleep instead) or flush input buffer before starting raw mode
- **Workaround:** `input_processor_minimal_test.c` uses `sleep(2)` instead
- **Priority:** Low - test-only issue, minimal test works correctly

---

## Low Priority / Enhancement Requests

*Nice-to-have improvements that don't affect functionality.*

### ISSUE-004: Terminal Initialization Requires Manual unix_interface Setup
- **Component:** Terminal Abstraction (`src/lle/foundation/terminal/`)
- **Discovered:** Week 10 Testing (2025-10-15)
- **Description:** Test programs must manually initialize `lle_unix_interface_t` and set `output_fd`
- **Current Pattern:**
  ```c
  lle_terminal_abstraction_t term;
  lle_unix_interface_t unix_interface;
  memset(&term, 0, sizeof(term));
  memset(&unix_interface, 0, sizeof(unix_interface));
  unix_interface.output_fd = STDOUT_FILENO;
  term.unix_interface = &unix_interface;
  ```
- **Enhancement:** Could provide `lle_terminal_init_simple()` helper for common case
- **Impact:** Minor - just verbose boilerplate in tests
- **Priority:** Low - not blocking, phase 0 code works this way

---

## Resolved Issues

*Issues that have been fixed - kept for historical reference.*

### ~~ISSUE-R001: Function Naming Inconsistency in Input Processor~~
- **Status:** ✅ RESOLVED (2025-10-15)
- **Component:** Input Processing
- **Description:** Header declared `lle_input_processor_*` but implementation used `lle_simple_input_*`
- **Resolution:** Updated header to match implementation naming convention
- **Commit:** [To be added after Week 10 commit]

### ~~ISSUE-R002: Escape Sequence Parser VMIN/VTIME Settings~~
- **Status:** ✅ RESOLVED (2025-10-15)
- **Component:** Input Processing
- **Description:** Initially used VMIN=0 which caused non-blocking reads and timeout issues
- **Resolution:** Changed to VMIN=1, VTIME=1 for proper blocking with 100ms timeout
- **Result:** Escape sequences now parse correctly
- **Commit:** [To be added after Week 10 commit]

### ~~ISSUE-R003: Display Segfault on NULL unix_interface~~
- **Status:** ✅ RESOLVED (2025-10-15)
- **Component:** Display System
- **Description:** Display tried to access `term->unix_interface->output_fd` when unix_interface was NULL
- **Resolution:** Test programs now properly initialize unix_interface with STDOUT_FILENO
- **Valgrind Output:** Confirmed fixed - no more segfaults on character insert
- **Commit:** [To be added after Week 10 commit]

---

## Issue Tracking Guidelines

### When to Add an Issue
- Discovered violation of LLE specifications or principles
- Found TODO markers in code that need tracking
- Identified technical debt during implementation
- Encountered workarounds that need proper fixes
- Discovered bugs that can't be fixed immediately

### Issue Format
Each issue should include:
- **Component:** Which subsystem is affected
- **Location:** File and line number if applicable
- **Discovered:** When and during which work
- **Description:** Clear explanation of the problem
- **Impact:** How it affects functionality
- **TODO:** Specific steps to resolve
- **Phase:** When it should be addressed

### Priority Levels
- **Critical:** Violates core principles, causes crashes/data loss
- **High:** Should fix before phase completion
- **Medium:** Should fix eventually, doesn't block progress
- **Low:** Nice-to-have, enhancement request

### Resolution Process
When resolving an issue:
1. Move to "Resolved Issues" section with ~~strikethrough~~
2. Add ✅ RESOLVED status with date
3. Add commit reference
4. Keep for historical reference (don't delete)

---

## Related Documents
- [DECISION_LOG.md](./DECISION_LOG.md) - Why we made certain choices
- [AI_ASSISTANT_HANDOFF.md](../AI_ASSISTANT_HANDOFF.md) - Current implementation status
- [CODE_METRICS.md](./CODE_METRICS.md) - Code quality metrics
- [Spec 06 - Input Parsing](../../lle_specification/06_input_parsing_complete.md) - Full input system specification

---

**Maintenance Note:** This document should be updated whenever:
- New issues are discovered during implementation
- Issues are resolved or worked around
- Priorities change based on project needs
- Phase transitions require re-evaluation of technical debt
