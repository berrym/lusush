# LLE Known Issues and Blockers

**Date**: 2025-11-15  
**Status**: ⚠️ ACTIVE DEVELOPMENT - Known Issues Tracked  
**Implementation Status**: Phase 1 complete, Groups 1-6 keybindings implemented, UTF-8 cell storage complete

---

## Executive Summary

**Current State**: Active development with keybinding manager migration complete

- ⚠️ **2 Active Issues** (multiline ENTER display, v1.3.0 regressions)
- ✅ **No Blockers** (all issues are non-critical)
- ✅ **Living document enforcement active**
- ✅ **Meta/Alt keybindings working** (Session 14)
- ✅ **Multi-line prompts working** (Session 14)

---

## Active Issues

### Issue #1: Multiline ENTER Display Bug
**Severity**: MEDIUM  
**Discovered**: 2025-11-14 (Session 14, Meta/Alt testing)  
**Component**: Display finalization / lle_readline.c  

**Description**:
When pressing ENTER on a non-final line of multiline input, the output appears on the line below where ENTER was pressed, rather than after the complete multiline input.

**Reproduction**:
```bash
# Type this multiline input:
if true; then
echo done
fi

# Press ENTER on line 1 "if true; then":
# Expected: Output appears after all 3 lines
# Actual: Output appears on line 2, clearing "echo done" and "fi"

# Press ENTER on line 2 "echo done":
# Expected: Output appears after all 3 lines
# Actual: Output appears on line 3, clearing "fi"

# Press ENTER on line 3 "fi":
# Correct: Output appears after all 3 lines
```

**Root Cause**: Unknown - appears to be in display finalization logic when ENTER context-aware action completes on non-final line.

**Impact**:
- Command executes correctly
- Visual display is confusing/incorrect
- Does not affect single-line input
- No data loss

**Priority**: MEDIUM (functional but poor UX)  
**Workaround**: Press ENTER on the final line only  
**Resolution Plan**: Investigate display refresh logic in `lle_accept_line_context()` and `refresh_display()`  
**Assigned**: Not assigned  

---

### Issue #2: break Statement Inside Loops Broken
**Severity**: HIGH  
**Discovered**: Pre-2025-11-14 (existed in v1.3.0, recently discovered)  
**Component**: Shell interpreter / loop execution  

**Description**:
The `break` statement does not work correctly inside `for`, `while`, or `until` loops. Instead of breaking out of the loop, it causes unexpected behavior.

**Reproduction**:
```bash
for i in 1 2 3 4 5; do
    echo $i
    if [ $i -eq 3 ]; then
        break
    fi
done
# Expected: Prints 1, 2, 3 then stops
# Actual: Unknown (needs testing to document exact behavior)
```

**Root Cause**: Unknown - likely in loop control flow implementation in shell interpreter

**Impact**:
- Core shell functionality broken
- Scripts using `break` will fail
- Pre-existed in v1.3.0 (not a regression from LLE work)

**Priority**: HIGH (core feature broken)  
**Workaround**: Avoid using `break`, use conditionals to skip loop body  
**Resolution Plan**: Investigate shell interpreter loop control flow  
**Assigned**: Not assigned  
**Note**: This is a shell interpreter bug, not an LLE bug

---

### Issue #3: Pipe Character Does Not Trigger Continuation
**Severity**: MEDIUM  
**Discovered**: Pre-2025-11-14  
**Component**: Input continuation parser  

**Description**:
When a line ends with a pipe character `|`, the continuation parser does not recognize it as incomplete input and does not enter multiline editing mode. Other incomplete constructs (unclosed quotes, unclosed braces, etc.) work correctly.

**Reproduction**:
```bash
# Type this and press ENTER:
echo hello |

# Expected: Continuation prompt appears, multiline editing mode
# Actual: Executes immediately, likely causing error

# Compare to working continuation:
if true; then
# Correctly shows continuation prompt
```

**Root Cause**: Pipe character not handled in continuation detection logic (likely in `is_input_incomplete()` or related parser)

**Impact**:
- Cannot build pipelines across multiple lines interactively
- Workaround requires typing entire pipeline on one line
- Single-line pipes work correctly

**Priority**: MEDIUM (useful feature missing)  
**Workaround**: Type entire pipeline on single line  
**Resolution Plan**: Add pipe handling to continuation detection parser  
**Assigned**: Not assigned  

---

## Resolved Issues

### ✅ Screen Buffer Single-Byte Cell Limitation (Session 15)
**Resolved**: 2025-11-15  
**Severity**: MEDIUM  
**Component**: Screen buffer cell storage

**Description**: Screen buffer cells stored only the first byte of UTF-8 sequences, limiting internal representation to ASCII or first byte of multi-byte characters. While prompts and commands displayed correctly (direct STDOUT write), this limitation affected:
- Future diff-based rendering
- Prefix rendering for continuation prompts
- Any feature needing to reconstruct text from cells

**Fix Applied**:
- Upgraded `screen_cell_t` structure from single byte to full UTF-8 sequence storage
- Changed from `char ch` to `char utf8_bytes[4]` with metadata (`byte_len`, `visual_width`)
- Updated all code paths that read/write cells
- Memory increased from 2 bytes/cell to 8 bytes/cell (~410 KB max)

**Capabilities Now Supported**:
- ASCII (1 byte, 1 column)
- Extended Latin (2 bytes, 1 column)
- CJK ideographs (3 bytes, 2 columns)
- Emoji (4 bytes, 2 columns)
- Box-drawing characters (3 bytes, 1 column)

**Files Changed**:
- `include/display/screen_buffer.h` - Updated structure, added stdint.h
- `src/display/screen_buffer.c` - Updated all cell operations

**Testing Results**:
- ✅ Zero regressions in baseline testing
- ✅ Emoji rendering perfect (🚀 💻)
- ✅ Cursor positioning accurate
- ✅ All editing operations natural
- ✅ Multi-line input working
- ✅ Alt keybindings working

---

### ✅ Multi-line Prompt Cursor Positioning Bug (Session 14)
**Resolved**: 2025-11-14  
**Severity**: HIGH  
**Component**: Screen buffer prompt rendering / display_controller.c

**Description**: Multi-line prompts (e.g., dark theme with 2-line prompt) had incorrect cursor positioning and display corruption on character input.

**Root Causes**:
1. `screen_buffer_render()` didn't handle `\n` in prompt text (only in command text)
2. `display_controller.c` always moved to row 0 before clearing (assumed single-line prompt)
3. `display_controller.c` used total prompt width instead of actual command start column

**Fixes Applied**:
1. Added `\n`, `\r`, `\t` handling to prompt rendering loop (screen_buffer.c:231-261)
2. Added `command_start_row` and `command_start_col` fields to `screen_buffer_t`
3. Updated `display_controller.c` to use `command_start_row` instead of hardcoded row 0
4. Updated `display_controller.c` to use `command_start_col` instead of calculated prompt width

**Files Changed**:
- `include/display/screen_buffer.h` - Added command_start_row/col fields
- `src/display/screen_buffer.c` - Handle newlines in prompts, set command start position
- `src/display/display_controller.c` - Use actual command start position for clearing

**Testing Results**:
- ✅ Cursor positioned correctly on second prompt line
- ✅ Character input without display corruption
- ✅ Line wrapping and navigation across boundaries working
- ✅ UTF-8 box-drawing characters rendering correctly

---

### ✅ Meta/Alt Key Detection Not Implemented
**Resolved**: 2025-11-14  
**Resolution**: Implemented ESC+character detection in key_detector.c and event routing in lle_readline.c

**Changes**:
- Added Meta/Alt sequences to key_mappings table
- Fixed hex escape sequence bug (`"\x1Bf"` → `"\x1B" "f"`)
- Fixed missing keycode field in event conversion
- All Group 6 keybindings now functional (M-f, M-b, M-<, M->)

---

## Prevention Measures

To prevent future issues:

1. ✅ **Living Document Enforcement** - Pre-commit hooks enforce document updates
2. ✅ **Known Issues Tracking** - This document updated with all discovered bugs
3. ✅ **Spec Compliance Mandate** - Only implement exact spec APIs
4. ✅ **Comprehensive Testing** - All features tested before commit
5. ⚠️ **Issue Documentation** - **MUST update this file when bugs discovered**

---

## Issue Reporting Protocol

**When new issues are discovered**:

1. **Document in this file immediately** with:
   - Severity (BLOCKER, CRITICAL, HIGH, MEDIUM, LOW)
   - Description with reproduction steps
   - Root cause (if known, otherwise "Unknown")
   - Impact on users
   - Priority
   - Workaround (if available)
   - Resolution plan
   - Discovered date

2. **Update living documents**:
   - AI_ASSISTANT_HANDOFF_DOCUMENT.md (note issue in Known Issues section)
   - Mark as blocker in status if severity is BLOCKER

3. **DO NOT let issues be forgotten**:
   - Check this file before each session
   - Reference issue numbers in commits related to bugs
   - Update status when work progresses on issue

---

## Severity Definitions

- **BLOCKER**: Prevents all work, must fix immediately
- **CRITICAL**: Core functionality broken, high priority
- **HIGH**: Important feature broken or major bug
- **MEDIUM**: Functionality works but with issues, or useful feature missing
- **LOW**: Minor issue, cosmetic, or edge case

---

## Current Status

**Active Issues**: 3  
**Blockers**: 0  
**High Priority**: 1 (break statement)  
**Medium Priority**: 2 (multiline display, pipe continuation)  
**Implementation Status**: Groups 1-6 complete, Meta/Alt working  
**Next Action**: Continue development, address issues when prioritized

---

**Last Updated**: 2025-11-15  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS
