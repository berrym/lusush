# Pre-Commit Hook Improvements - Phase 1 Implementation Ready

**Date**: 2025-10-30  
**Purpose**: Document exact changes needed for Phase 1 critical gap fixes  
**Status**: READY TO IMPLEMENT - Specific line numbers and code provided

---

## Overview

This document provides the exact changes needed to implement Phase 1 critical gap fixes in `.git/hooks/pre-commit`. Each improvement is documented with:
- Exact line number where to insert
- Complete code to add
- Explanation of what it fixes
- Test case to verify it works

**Backup Created**: `.git/hooks/pre-commit.backup-20251030`

---

## Gap 1: New Compliance Tests Must Compile and Pass

**Problem**: Compliance tests can be added to a commit but never verified to actually compile and pass.

**Fix Location**: Insert after line 230 (after existing compliance test check)

**Code to Add**:
```bash
    # ============================================================================
    # GAP 1 FIX: Verify NEW compliance tests compile and pass (BLOCKING)
    # ============================================================================
    
    # Check if any NEW compliance tests are being added in this commit
    NEW_COMPLIANCE_TESTS=$(git diff --cached --name-only --diff-filter=A | grep "tests/lle/compliance/.*\.c$" || true)
    
    if [ -n "$NEW_COMPLIANCE_TESTS" ]; then
        echo "New compliance test(s) detected - verifying they compile and pass..."
        
        for test_file in $NEW_COMPLIANCE_TESTS; do
            test_name=$(basename "$test_file" .c)
            echo "  Compiling $test_name..."
            
            # Compile the test
            if ! gcc -o "/tmp/$test_name" "$test_file" \
                -I./include -std=c11 -D_POSIX_C_SOURCE=200809L \
                -Wall -Wextra -Werror -pthread -g 2>/tmp/compile_error.txt; then
                echo "❌ BLOCKER: New compliance test $test_file FAILS TO COMPILE"
                echo ""
                echo "COMPILATION ERRORS:"
                echo "========================================================"
                cat /tmp/compile_error.txt
                echo "========================================================"
                echo ""
                echo "FORBIDDEN: Cannot commit a compliance test that doesn't compile."
                echo "This usually means you made assumptions about the API without reading"
                echo "the actual header file. See SPEC_04_COMPLIANCE_TEST_LESSONS.md"
                echo ""
                rm -f /tmp/compile_error.txt
                CHECKS_FAILED=1
                continue
            fi
            
            echo "  Running $test_name..."
            
            # Run the test
            if ! /tmp/$test_name > /tmp/test_output.txt 2>&1; then
                echo "❌ BLOCKER: New compliance test $test_file FAILS"
                echo ""
                echo "TEST OUTPUT:"
                echo "========================================================"
                cat /tmp/test_output.txt
                echo "========================================================"
                echo ""
                echo "FORBIDDEN: Cannot commit a failing compliance test."
                echo "Fix the test to match the actual API before committing."
                echo ""
                rm -f /tmp/test_output.txt /tmp/$test_name
                CHECKS_FAILED=1
            else
                echo "  ✓ $test_name compiles and passes"
                rm -f /tmp/test_output.txt /tmp/$test_name
            fi
        done
        
        if [ $CHECKS_FAILED -eq 0 ]; then
            echo "✓ All new compliance tests compile and pass"
        fi
    fi
```

**Test Case**:
```bash
# Create a broken compliance test
echo 'int main() { undefined_function(); return 0; }' > tests/lle/compliance/test_broken.c
git add tests/lle/compliance/test_broken.c
git commit -m "LLE: Test Gap 1 fix"
# Expected: BLOCKED with compilation error

# Create a failing compliance test
echo '#include <stdio.h>
int main() { 
    if (1 != 2) { 
        printf("FAIL\\n"); 
        return 1; 
    }
    return 0;
}' > tests/lle/compliance/test_fail.c
git add tests/lle/compliance/test_fail.c
git commit -m "LLE: Test Gap 1 fix"
# Expected: BLOCKED with test failure
```

---

## Gap 2: Fix Git Amend Handling

**Problem**: `git commit --amend` shows only newly staged files in `git diff --cached`, not files already in the commit. This bypasses living document checks.

**Fix Location**: Insert at line 10 (right after CHECKS_FAILED=0)

**Code to Add**:
```bash
# ============================================================================
# GAP 2 FIX: Detect git amend and check ALL files in commit (BLOCKING)
# ============================================================================

# Detect if this is an amend by checking if HEAD exists and has differences
IS_AMEND=0
if git rev-parse --verify HEAD >/dev/null 2>&1; then
    # Check if files in HEAD differ from what's staged
    if git diff HEAD --name-only | grep -q .; then
        IS_AMEND=1
        echo "Git amend detected - checking all files in commit..."
    fi
fi

# Helper function to get all changed files (handles both normal and amend)
get_all_changed_files() {
    if [ $IS_AMEND -eq 1 ]; then
        # For amends: files already in commit + newly staged
        git diff HEAD --name-only
        git diff --cached --name-only
    else
        # For normal commits: only staged files
        git diff --cached --name-only
    fi | sort -u
}
```

**Then UPDATE line 73** (where LLE files are detected):
```bash
# OLD:
LLE_FILES_CHANGED=$(git diff --cached --name-only | grep -E "include/lle/|src/lle/|tests/lle/" || true)

# NEW:
LLE_FILES_CHANGED=$(get_all_changed_files | grep -E "include/lle/|src/lle/|tests/lle/" || true)
```

**And UPDATE line 83** (where living docs are checked):
```bash
# OLD:
LIVING_DOCS_UPDATED=$(git diff --cached --name-only | grep -E "AI_ASSISTANT_HANDOFF_DOCUMENT.md|..." || true)

# NEW:
LIVING_DOCS_UPDATED=$(get_all_changed_files | grep -E "AI_ASSISTANT_HANDOFF_DOCUMENT.md|..." || true)
```

**Test Case**:
```bash
# Make a commit with living docs
echo "test" > test.txt
git add test.txt AI_ASSISTANT_HANDOFF_DOCUMENT.md
git commit -m "LLE: Initial commit"

# Try to amend with LLE file but no living doc update
echo "change" >> src/lle/event_system.c
git add src/lle/event_system.c
git commit --amend --no-edit
# Expected: BLOCKED because living docs in original commit aren't updated
```

---

## Gap 5: New Test Files Must Pass

**Problem**: New test files can be added without verifying they actually pass.

**Fix Location**: Insert after line 230 (after compliance test section, before Gap 1 fix)

**Code to Add**:
```bash
    # ============================================================================
    # GAP 5 FIX: New test files must pass before commit (BLOCKING)
    # ============================================================================
    
    # Check if any NEW unit/functional/integration tests are being added
    NEW_TEST_FILES=$(git diff --cached --name-only --diff-filter=A | grep "tests/lle/.*test_.*\.c$" | grep -v "compliance" || true)
    
    if [ -n "$NEW_TEST_FILES" ]; then
        echo "New test file(s) detected - verifying they pass..."
        echo "⚠ NOTE: This requires tests to be added to meson.build first"
        echo ""
        
        # Run the full LLE test suite (new tests should be included)
        if [ -f "build/build.ninja" ]; then
            echo "Running LLE test suite..."
            if meson test -C build --suite lle-unit --suite lle-functional --suite lle-integration --no-rebuild --print-errorlogs > /tmp/test_run.txt 2>&1; then
                echo "✓ All LLE tests pass (including new tests)"
                rm -f /tmp/test_run.txt
            else
                echo "❌ BLOCKER: LLE test suite contains failures"
                echo ""
                echo "TEST OUTPUT:"
                echo "========================================================"
                cat /tmp/test_run.txt
                echo "========================================================"
                echo ""
                echo "FORBIDDEN: Cannot commit new tests that don't pass."
                echo ""
                echo "New test files detected:"
                echo "$NEW_TEST_FILES"
                echo ""
                echo "Fix the tests or ensure they're properly integrated into"
                echo "meson.build before committing."
                echo ""
                rm -f /tmp/test_run.txt
                CHECKS_FAILED=1
            fi
        else
            echo "⚠ WARNING: Build directory not found - cannot verify tests pass"
            echo "  Run 'meson setup build' first to enable test verification"
            echo ""
            sleep 2
        fi
    fi
```

**Test Case**:
```bash
# Create a failing test
cat > tests/lle/unit/test_broken.c << 'EOF'
#include <assert.h>
int main() {
    assert(1 == 2);  // This will fail
    return 0;
}
EOF

# Add to meson.build
echo "test_broken = executable('test_broken', 'tests/lle/unit/test_broken.c', include_directories: inc, dependencies: [lle_dep])" >> meson.build
echo "test('Broken Test', test_broken, suite: 'lle-unit')" >> meson.build

# Try to commit
git add tests/lle/unit/test_broken.c meson.build
git commit -m "LLE: Test Gap 5 fix"
# Expected: BLOCKED because test fails
```

---

## Gap 6: New Headers Must Compile Standalone

**Problem**: New header files can be added without verifying they compile independently.

**Fix Location**: Insert after line 230 (with other new checks)

**Code to Add**:
```bash
    # ============================================================================
    # GAP 6 FIX: New headers must compile standalone (BLOCKING)
    # ============================================================================
    
    # Check if any NEW header files are being added
    NEW_HEADERS=$(git diff --cached --name-only --diff-filter=A | grep "include/lle/.*\.h$" || true)
    
    if [ -n "$NEW_HEADERS" ]; then
        echo "New header file(s) detected - verifying standalone compilation..."
        
        for header in $NEW_HEADERS; do
            header_name=$(basename "$header")
            echo "  Compiling $header_name standalone..."
            
            # Try to compile the header standalone
            if ! gcc -std=c11 -Wall -Werror -Wextra -fsyntax-only \
                -I./include -D_POSIX_C_SOURCE=200809L \
                "$header" 2>/tmp/header_compile_error.txt; then
                echo "❌ BLOCKER: Header $header FAILS standalone compilation"
                echo ""
                echo "COMPILATION ERRORS:"
                echo "========================================================"
                cat /tmp/header_compile_error.txt
                echo "========================================================"
                echo ""
                echo "FORBIDDEN: Headers must compile independently without errors."
                echo "Common issues:"
                echo "  - Missing #include guards"
                echo "  - Missing #include for dependencies"
                echo "  - Forward declarations needed"
                echo "  - Undeclared types"
                echo ""
                rm -f /tmp/header_compile_error.txt
                CHECKS_FAILED=1
            else
                echo "  ✓ $header_name compiles standalone"
            fi
        done
        
        rm -f /tmp/header_compile_error.txt
        
        if [ $CHECKS_FAILED -eq 0 ]; then
            echo "✓ All new headers compile standalone"
        fi
    fi
```

**Test Case**:
```bash
# Create a broken header
cat > include/lle/test_broken.h << 'EOF'
// Missing include guard
// Missing includes

void function_using_undefined_type(some_undefined_type_t *ptr);
EOF

git add include/lle/test_broken.h
git commit -m "LLE: Test Gap 6 fix"
# Expected: BLOCKED with compilation errors
```

---

## Summary of Changes

### Files to Modify
1. `.git/hooks/pre-commit` - Add all gap fixes

### Line Insertions
1. **Line 10**: Add Gap 2 fix (amend detection)
2. **Line 73**: Update to use `get_all_changed_files()`
3. **Line 83**: Update to use `get_all_changed_files()`
4. **Line 230**: Add Gap 5, Gap 6, and Gap 1 fixes (in that order)

### Total Added Lines
- Gap 2: ~20 lines
- Gap 5: ~45 lines
- Gap 6: ~40 lines
- Gap 1: ~50 lines
- **Total**: ~155 lines added

### New Hook Size
- Current: 437 lines
- After improvements: ~592 lines

---

## Implementation Approach

**Option 1: Manual Edit** (Recommended for safety)
1. Open `.git/hooks/pre-commit` in editor
2. Insert each section at specified line numbers
3. Save and test with intentional violations

**Option 2: Automated Script** (Faster but riskier)
```bash
# Create improved hook with all fixes applied
# (Would need full script here - safer to do manually)
```

**Option 3: Completely New Hook** (Cleanest but most work)
1. Write new hook from scratch with all improvements
2. Test thoroughly
3. Replace old hook

---

## Testing Plan

After implementing, test each gap fix:

1. **Gap 1**: Try committing broken compliance test → Should block
2. **Gap 2**: Try amending with LLE file, no living doc → Should block
3. **Gap 5**: Try committing failing test → Should block
4. **Gap 6**: Try committing broken header → Should block

**Success Criteria**: All 4 test cases block the commit with clear error messages.

---

## Rollback Plan

If improvements cause problems:
```bash
cd /home/mberry/Lab/c/lusush
cp .git/hooks/pre-commit.backup-20251030 .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

---

**Status**: READY TO IMPLEMENT  
**Estimated Time**: 30-45 minutes for manual implementation + testing  
**Risk**: Low - backup exists, changes are additive  
**Impact**: HIGH - Closes 4 of 7 critical gaps
