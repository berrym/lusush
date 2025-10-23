# Mandatory Compliance Test Policy

**Version**: 1.0.0  
**Date**: 2025-10-23  
**Status**: ACTIVE - Enforced by pre-commit hooks  
**Classification**: Development Policy

---

## Policy Statement

**MANDATORY**: Every LLE specification implementation MUST have accompanying compliance tests that verify 100% specification accuracy.

This is not optional. This is not a best practice. This is a hard requirement enforced by automated tooling.

---

## The Mandate

Compliance tests are the **only automated mechanism** that can verify implementations match specifications exactly. Without them:

1. Manual review cannot catch all deviations
2. Incorrect constants slip through
3. Enum values drift from spec
4. Structure fields get simplified
5. False compliance claims cannot be verified

**User directive**: "create compliance tests for all specs currently implemented at all and make it an enforced policy to create these compliance tests any time spec implementation code is being written"

---

## What Must Be Tested

### For Every Specification Implementation

When implementing any LLE specification, you MUST create compliance tests that verify:

1. **Constants Match Spec**
   - All `#define` constants have exact values from spec
   - Array sizes match spec requirements
   - Threshold values are correct
   - Alignment values are accurate

2. **Enumerations Match Spec**
   - All enum values are defined
   - Enum values are accessible and compile
   - Starting values are correct (if specified in spec)

3. **Error Codes Match Spec**
   - All error codes exist
   - Error code values are exact
   - Error code ranges are correct

4. **Structures Match Spec** (when testable)
   - Field counts are correct
   - Field types match spec
   - Field names match spec

---

## Compliance Test File Structure

### Naming Convention

```
tests/lle/compliance/spec_<NUMBER>_<name>_compliance.c
```

Examples:
- `spec_14_performance_compliance.c`
- `spec_15_memory_management_compliance.c`
- `spec_16_error_handling_compliance.c`
- `spec_17_testing_framework_compliance.c`

### File Template

```c
/**
 * @file spec_XX_name_compliance.c
 * @brief Spec XX Name - Compliance Verification Tests
 * 
 * Verifies that the [Name] implementation is 100% compliant
 * with specification XX_name_complete.md
 * 
 * ZERO-TOLERANCE ENFORCEMENT:
 * These tests verify spec compliance. Any failure indicates a violation
 * and the code MUST be corrected before commit is allowed.
 * 
 * Tests verify:
 * - [List what is tested]
 * 
 * Spec Reference: docs/lle_specification/XX_name_complete.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Include only the header being tested */
#include "lle/header_name.h"

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Simple assertion macros */
#define ASSERT_EQ(expected, actual, message) \
    do { \
        tests_run++; \
        if ((expected) != (actual)) { \
            printf("  FAIL: %s\n", message); \
            printf("        Expected: %d, Got: %d\n", (int)(expected), (int)(actual)); \
            tests_failed++; \
            return false; \
        } else { \
            tests_passed++; \
        } \
    } while(0)

/* Test functions */
static bool test_something(void) {
    printf("  Testing something...\n");
    ASSERT_EQ(expected_value, CONSTANT_NAME, "CONSTANT_NAME must equal expected_value");
    return true;
}

/* Main function */
int main(void) {
    printf("Running Spec XX (Name) Compliance Tests...\n");
    printf("=======================================================\n\n");
    
    /* Run tests */
    test_something();
    
    /* Print results */
    printf("\n");
    printf("=======================================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("=======================================================\n\n");
    
    if (tests_failed == 0) {
        printf("RESULT: ALL COMPLIANCE TESTS PASSED\n\n");
        printf("Implementation is 100%% spec-compliant\n");
        return 0;
    } else {
        printf("RESULT: SPEC COMPLIANCE VIOLATION\n\n");
        printf("Implementation DOES NOT match specification\n");
        printf("This is a ZERO-TOLERANCE violation\n");
        printf("Commit MUST be blocked until violations are corrected\n");
        return 1;
    }
}
```

---

## Integration Requirements

### 1. Add to run_compliance_tests.sh

Every new compliance test MUST be added to `tests/lle/run_compliance_tests.sh`:

```bash
# Run Spec XX compliance tests (Name)
echo "-------------------------------------------------------------------"
echo "Spec XX: Name Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_XX_name_compliance.c" ]; then
    echo "Compiling spec_XX_name_compliance..."
    
    gcc -o spec_XX_compliance \
        compliance/spec_XX_name_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g
    
    echo "Compilation successful"
    echo ""
    
    # Run the test
    ./spec_XX_compliance
    SPEC_XX_RESULT=$?
    
    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_XX_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))
    
    # Cleanup
    rm -f spec_XX_compliance
    
    echo ""
else
    echo "Spec XX compliance test not found - skipping"
    echo ""
fi
```

### 2. Update Living Documents

When compliance tests are created, update:
- `AI_ASSISTANT_HANDOFF_DOCUMENT.md` - List test coverage status
- `SPEC_IMPLEMENTATION_ORDER.md` - Note which specs have compliance tests

---

## Enforcement Mechanism

### Pre-commit Hook

The pre-commit hook enforces this policy:

1. **Runs all compliance tests** - Any failure blocks commit
2. **Warns on new implementations** - Reminds developer to create tests
3. **Blocks violations** - Exit code 1 prevents commit if tests fail

### What Happens When You Commit

```bash
# Developer commits LLE code
git commit -m "LLE Implement Spec 03 Phase 1"

# Pre-commit hook runs:
Running pre-commit checks...
LLE source files detected in commit...
Running LLE specification compliance tests...

# If compliance test fails:
❌ ZERO TOLERANCE VIOLATION: Spec compliance tests FAILED

COMPLIANCE TEST OUTPUT:
========================================================
  FAIL: LLE_ERROR_INVALID_PARAMETER must equal 1000
        Expected: 1000, Got: 995
========================================================

FORBIDDEN: All LLE implementations must be 100% spec-compliant
Fix the violations above before committing.

# Commit is BLOCKED
```

---

## Current Test Coverage

### Specs with 100% Compliance Test Coverage

| Spec | Name | Test File | Assertions | Status |
|------|------|-----------|------------|--------|
| 14 | Performance Monitoring | spec_14_performance_compliance.c | 22 | ✓ PASSING |
| 15 | Memory Management | spec_15_memory_management_compliance.c | 29 | ✓ PASSING |
| 16 | Error Handling | spec_16_error_handling_compliance.c | 69 | ✓ PASSING |
| 17 | Testing Framework | spec_17_testing_framework_compliance.c | 11 | ✓ PASSING |

**Total**: 4 specs, 131 assertions, 100% passing

### Specs Requiring Compliance Tests

When implementing these specs, compliance tests MUST be created:

- Spec 01: Core Architecture
- Spec 02: Event System  
- Spec 03: Buffer Management
- Spec 04: Terminal Abstraction
- Spec 05: Display Integration
- Spec 06: Input Handling
- Spec 07: Text Rendering
- Spec 08: Display Integration
- Spec 09: Signal Handling
- Spec 10: Configuration Management
- Spec 11: History System
- Spec 12: Autosuggestions
- Spec 13: Tab Completion
- Spec 18: Internationalization
- Spec 19: Security
- Spec 20: Plugin System
- Spec 21: Syntax Highlighting
- Spec 22: User Interface System
- (Additional specs as implemented)

---

## Benefits of Mandatory Compliance Testing

### 1. Prevents Violations Before They Occur

- **Old approach**: Implement code, discover violations later, execute nuclear option
- **New approach**: Violations caught immediately, cannot be committed

### 2. Builds Trust in Compliance Claims

- **Old approach**: Commit says "100% compliant" - no verification possible
- **New approach**: 131 passing assertions prove compliance

### 3. Catches Accidental Regressions

- Developer changes constant value
- Compliance test fails
- Developer realizes mistake before commit
- Prevents broken builds

### 4. Documentation That Cannot Drift

- Compliance tests are executable documentation
- They prove what the implementation actually does
- They cannot become outdated (they either pass or fail)

### 5. Enforces Zero-Tolerance Policy Automatically

- No human judgment required
- No "this is close enough" decisions
- Binary pass/fail: spec-compliant or not

---

## When to Create Compliance Tests

### During Spec Implementation

**REQUIRED**: Create compliance tests **as you write** spec implementation code.

Workflow:
1. Read specification section
2. Implement types/constants from that section
3. Immediately write compliance tests for what you just implemented
4. Run tests, verify they pass
5. Continue to next section

### Before First Commit

**BLOCKING**: Compliance tests MUST exist before first commit of spec implementation.

If you implement Spec 03 Phase 1, you MUST create `spec_03_buffer_management_compliance.c` in the same commit.

### When Updating Existing Implementation

**VALIDATION**: Run compliance tests to verify changes don't break spec compliance.

The tests act as regression tests, catching accidental deviations.

---

## Examples from Current Implementation

### Example 1: Error Code Compliance

Spec 16 defines error codes with exact values. Compliance test verifies:

```c
// Spec says: LLE_ERROR_INVALID_PARAMETER = 1000
ASSERT_EQ(1000, LLE_ERROR_INVALID_PARAMETER,
          "LLE_ERROR_INVALID_PARAMETER must equal 1000");

// Spec says: Input validation errors are 1000-1099
ASSERT_IN_RANGE(LLE_ERROR_NULL_POINTER, 1000, 1099,
                "LLE_ERROR_NULL_POINTER must be in range 1000-1099");
```

**Catches**: Developer accidentally sets `LLE_ERROR_INVALID_PARAMETER = 995`

### Example 2: Performance Target Compliance

Spec 14 defines performance targets. Compliance test verifies:

```c
// Spec says: Target response time is 500µs (500,000ns)
ASSERT_EQ(500000ULL, LLE_PERF_TARGET_RESPONSE_TIME_NS,
          "LLE_PERF_TARGET_RESPONSE_TIME_NS must be 500000ns");
```

**Catches**: Developer uses milliseconds instead of nanoseconds

### Example 3: Memory Pool Compliance

Spec 15 defines pool counts. Compliance test verifies:

```c
// Spec says: 8 primary memory pools
ASSERT_EQ(8, LLE_PRIMARY_POOL_COUNT,
          "LLE_PRIMARY_POOL_COUNT must be 8");
```

**Catches**: Developer creates 6 pools instead of 8

---

## Consequences of Non-Compliance

### If Tests Don't Exist

- Pre-commit hook shows warning (2-second delay)
- Commit still proceeds (for now)
- Manual review required to verify compliance
- Risk of violations slipping through

**Future**: May be upgraded to blocking error

### If Tests Fail

- Pre-commit hook **BLOCKS** commit immediately
- Developer sees exact failure details
- Developer MUST fix violations before commit succeeds
- No exceptions, no bypasses (except `--no-verify` for emergencies)

### If Tests Are Bypassed

Using `git commit --no-verify` to bypass failed tests:

- **Acceptable**: When tests incorrectly flag documentation/comments
- **Forbidden**: When tests correctly identify real violations
- **Consequence**: Next developer will discover violations, may trigger nuclear option

---

## Summary

**Policy**: Compliance tests are **mandatory** for all LLE spec implementations.

**Enforcement**: Pre-commit hooks run tests automatically, block violations.

**Coverage**: Currently 4 specs tested with 131 assertions, all passing.

**Requirement**: New spec implementations MUST include compliance tests in same commit.

**Benefit**: Automated verification of 100% spec compliance, prevents violations before they occur.

This policy ensures the zero-tolerance mandate is enforced automatically and consistently across all LLE development.
