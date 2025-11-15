# LLE Known Issues and Blockers

**Date**: 2025-11-14  
**Status**: ⚠️ ACTIVE DEVELOPMENT - Known Issues Tracked  
**Implementation Status**: Phase 1 complete, Groups 1-6 keybindings implemented

---

## Executive Summary

**Current State**: Active development with keybinding manager migration complete

- ⚠️ **3 Active Issues** (display bug, v1.3.0 regressions)
- ✅ **No Blockers** (all issues are non-critical)
- ✅ **Living document enforcement active**
- ✅ **Meta/Alt keybindings working** (Session 14)

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

## Resolved Issues (Session 14)

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

**Last Updated**: 2025-11-14  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS
