#!/bin/bash
# Run LLE Specification Compliance Tests
#
# ZERO-TOLERANCE ENFORCEMENT:
# These tests verify that all implemented LLE code is 100% spec-compliant.
# Any test failure is a ZERO-TOLERANCE violation and must block commits.

set -e

echo "==================================================================="
echo "LLE Specification Compliance Test Suite"
echo "==================================================================="
echo ""
echo "ZERO-TOLERANCE POLICY ACTIVE"
echo "All implemented code must be 100% specification-compliant"
echo ""

# Track overall test results
TOTAL_FAILURES=0
TESTS_RUN=0

# Run Spec 16 compliance tests (Error Handling)
echo "-------------------------------------------------------------------"
echo "Spec 16: Error Handling Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_16_error_handling_compliance.c" ]; then
    echo "Compiling spec_16_error_handling_compliance..."

    gcc -o spec_16_compliance \
        compliance/spec_16_error_handling_compliance.c \
        -I../../include \
        -std=c11 \
        -Wall -Wextra \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_16_compliance
    SPEC_16_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_16_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_16_compliance

    echo ""
else
    echo "Spec 16 compliance test not found - skipping"
    echo ""
fi

# Summary
echo "==================================================================="
echo "Compliance Test Summary"
echo "==================================================================="
echo ""
echo "Spec suites executed: $TESTS_RUN"

if [ $TOTAL_FAILURES -eq 0 ]; then
    echo ""
    echo "RESULT: ALL COMPLIANCE TESTS PASSED"
    echo ""
    echo "All implemented LLE code is 100% specification-compliant"
    echo "Zero-tolerance policy: SATISFIED"
    exit 0
else
    echo ""
    echo "RESULT: COMPLIANCE VIOLATIONS DETECTED"
    echo ""
    echo "Total violations: $TOTAL_FAILURES"
    echo ""
    echo "ZERO-TOLERANCE POLICY VIOLATION"
    echo "Implementation does NOT match specifications"
    echo "Commit MUST be blocked until violations are corrected"
    echo ""
    exit 1
fi
