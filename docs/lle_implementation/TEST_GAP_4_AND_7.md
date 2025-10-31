# Gap 4 and Gap 7 Automation Test Plan

**Date**: 2025-10-30  
**Purpose**: Verify Gap 4 (living document date sync) and Gap 7 (API source documentation) enforcement works correctly

## Test 1: Gap 4 - Stale Date Detection (BLOCKING)

### Setup
1. Modify a test LLE file (e.g., add a comment to `tests/lle/integration/manual_input_test.c`)
2. Do NOT update `AI_ASSISTANT_HANDOFF_DOCUMENT.md` date
3. Attempt to commit

### Expected Result
```
❌ LIVING DOCUMENT VIOLATION: AI_ASSISTANT_HANDOFF_DOCUMENT.md date is not current

REQUIRED: The **Date** field must be updated to today's date when committing LLE changes.
   Found: 2025-10-29
   Today: 2025-10-30

This ensures living documents stay synchronized with actual work dates.
```

**Commit should be BLOCKED**

### Test Commands
```bash
# Add a harmless comment to trigger LLE file change
echo "/* Test comment */" >> tests/lle/integration/manual_input_test.c

# Try to commit without updating handoff date
git add tests/lle/integration/manual_input_test.c
git commit -m "LLE Test: Gap 4 enforcement test"

# Should FAIL with date violation
```

### Fix
```bash
# Update handoff document date to today
# Then commit should succeed
```

---

## Test 2: Gap 4 - Current Date Acceptance (PASSING)

### Setup
1. Modify a test LLE file
2. Update `AI_ASSISTANT_HANDOFF_DOCUMENT.md` date to today (2025-10-30)
3. Attempt to commit

### Expected Result
```
✓ Handoff document date is current
```

**Commit should SUCCEED**

### Test Commands
```bash
# Update handoff date first
# Then add change and commit
git add tests/lle/integration/manual_input_test.c AI_ASSISTANT_HANDOFF_DOCUMENT.md
git commit -m "LLE Test: Gap 4 with current date"

# Should PASS
```

---

## Test 3: Gap 7 - Missing API Source Documentation (WARNING)

### Setup
1. Create a new compliance test WITHOUT the API source comment
2. Attempt to commit

### Expected Result
```
⚠️ WARNING: Compliance test should document API source

File: tests/lle/compliance/spec_99_test_compliance.c

BEST PRACTICE: Compliance tests should document which header file
they verified against and when, to prevent API assumptions.

Add comment near the top of the file:
  /* API verified from include/lle/xxx.h on YYYY-MM-DD */

This ensures the test author actually read the header file
rather than making assumptions about the API.

See docs/lle_implementation/SPEC_04_COMPLIANCE_TEST_LESSONS.md for details.
```

**Commit should succeed after 3 second delay (WARNING, not BLOCKER)**

### Test Commands
```bash
# Create minimal compliance test without API source comment
cat > tests/lle/compliance/spec_99_test_compliance.c << 'EOF'
/* Test compliance test for Gap 7 enforcement */

#include <stdio.h>

int main(void) {
    printf("Gap 7 test\n");
    return 0;
}
EOF

# Try to commit
git add tests/lle/compliance/spec_99_test_compliance.c
git commit -m "LLE Test: Gap 7 enforcement test"

# Should show WARNING and pause for 3 seconds, but allow commit
```

---

## Test 4: Gap 7 - With API Source Documentation (PASSING)

### Setup
1. Create a new compliance test WITH the API source comment
2. Attempt to commit

### Expected Result
```
✓ tests/lle/compliance/spec_99_test_compliance.c documents API source
```

**Commit should SUCCEED immediately (no warning)**

### Test Commands
```bash
# Create compliance test WITH proper API source comment
cat > tests/lle/compliance/spec_99_test_compliance.c << 'EOF'
/* Test compliance test for Gap 7 enforcement */
/* API verified from include/lle/buffer.h on 2025-10-30 */

#include <stdio.h>

int main(void) {
    printf("Gap 7 test with API source\n");
    return 0;
}
EOF

# Try to commit
git add tests/lle/compliance/spec_99_test_compliance.c
git commit -m "LLE Test: Gap 7 with API source documentation"

# Should PASS immediately with no warning
```

---

## Test 5: Combined Test - Both Gaps (BLOCKING on Gap 4)

### Setup
1. Create new compliance test WITHOUT API source comment
2. Modify LLE file
3. Do NOT update handoff date
4. Attempt to commit

### Expected Result
- Gap 4 should BLOCK the commit (date violation)
- Gap 7 warning should not appear (commit blocked before it gets there)

### Test Commands
```bash
# Create test file without API source
cat > tests/lle/compliance/spec_99_test_compliance.c << 'EOF'
#include <stdio.h>
int main(void) { return 0; }
EOF

# Modify LLE file
echo "/* Combined test */" >> tests/lle/integration/manual_input_test.c

# Try to commit without updating date
git add tests/lle/compliance/spec_99_test_compliance.c tests/lle/integration/manual_input_test.c
git commit -m "LLE Test: Combined Gap 4 and 7"

# Should FAIL on Gap 4 before reaching Gap 7
```

---

## Cleanup After Tests

```bash
# Remove test compliance file
git rm tests/lle/compliance/spec_99_test_compliance.c

# Restore manual_input_test.c
git checkout tests/lle/integration/manual_input_test.c

# Clean up any test commits
git reset --soft HEAD~N  # where N is number of test commits
```

---

## Success Criteria

✅ Gap 4 BLOCKS commits when handoff date is stale  
✅ Gap 4 ALLOWS commits when handoff date is current  
✅ Gap 7 shows WARNING for compliance tests missing API source  
✅ Gap 7 is SILENT for compliance tests with proper API source  
✅ Both gaps work correctly when triggered simultaneously

---

## Notes

- Gap 4 is BLOCKING (CHECKS_FAILED=1) - prevents commit
- Gap 7 is WARNING only (sleep 3) - allows commit after delay
- Gap 4 runs before Gap 7 in the hook execution order
- Tests should be run on a test branch to avoid polluting git history
