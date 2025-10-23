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

# Run Spec 14 compliance tests (Performance Monitoring)
echo "-------------------------------------------------------------------"
echo "Spec 14: Performance Monitoring Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_14_performance_compliance.c" ]; then
    echo "Compiling spec_14_performance_compliance..."

    gcc -o spec_14_compliance \
        compliance/spec_14_performance_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_14_compliance
    SPEC_14_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_14_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_14_compliance

    echo ""
else
    echo "Spec 14 compliance test not found - skipping"
    echo ""
fi

# Run Spec 15 compliance tests (Memory Management)
echo "-------------------------------------------------------------------"
echo "Spec 15: Memory Management Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_15_memory_management_compliance.c" ]; then
    echo "Compiling spec_15_memory_management_compliance..."

    gcc -o spec_15_compliance \
        compliance/spec_15_memory_management_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_15_compliance
    SPEC_15_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_15_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_15_compliance

    echo ""
else
    echo "Spec 15 compliance test not found - skipping"
    echo ""
fi

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
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
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

# Run Spec 17 compliance tests (Testing Framework)
echo "-------------------------------------------------------------------"
echo "Spec 17: Testing Framework Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_17_testing_framework_compliance.c" ]; then
    echo "Compiling spec_17_testing_framework_compliance..."

    gcc -o spec_17_compliance \
        compliance/spec_17_testing_framework_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_17_compliance
    SPEC_17_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_17_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_17_compliance

    echo ""
else
    echo "Spec 17 compliance test not found - skipping"
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
