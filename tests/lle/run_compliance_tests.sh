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

# Run Spec 08 compliance tests (Display Integration)
echo "-------------------------------------------------------------------"
echo "Spec 08: Display Integration Compliance (Layer 0)"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_08_display_integration_compliance.c" ]; then
    echo "Compiling spec_08_display_integration_compliance..."

    gcc -o spec_08_compliance \
        compliance/spec_08_display_integration_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_08_compliance
    SPEC_08_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_08_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_08_compliance

    echo ""
else
    echo "Spec 08 compliance test not found - skipping"
    echo ""
fi

# Run Spec 02 compliance tests (Terminal Abstraction)
echo "-------------------------------------------------------------------"
echo "Spec 02: Terminal Abstraction Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_02_terminal_abstraction_compliance.c" ]; then
    echo "Compiling spec_02_terminal_abstraction_compliance..."

    gcc -o spec_02_compliance \
        compliance/spec_02_terminal_abstraction_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_02_compliance
    SPEC_02_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_02_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_02_compliance

    echo ""
else
    echo "Spec 02 compliance test not found - skipping"
    echo ""
fi

# Run Spec 03 UTF-8 and Unicode compliance tests
echo "-------------------------------------------------------------------"
echo "Spec 03: UTF-8 and Unicode TR#29 Foundation Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_03_utf8_unicode_compliance.c" ]; then
    echo "Compiling spec_03_utf8_unicode_compliance..."

    gcc -o spec_03_utf8_unicode_compliance \
        compliance/spec_03_utf8_unicode_compliance.c \
        ../../src/lle/utf8_support.c \
        ../../src/lle/unicode_grapheme.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_03_utf8_unicode_compliance
    SPEC_03_UTF8_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_03_UTF8_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_03_utf8_unicode_compliance

    echo ""
else
    echo "Spec 03 UTF-8/Unicode compliance test not found - skipping"
    echo ""
fi

# Run Spec 05 compliance tests (libhashtable Integration)
echo "-------------------------------------------------------------------"
echo "Spec 05: libhashtable Integration Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_05_libhashtable_integration_compliance.c" ]; then
    echo "Compiling spec_05_libhashtable_integration_compliance..."

    gcc -o spec_05_compliance \
        compliance/spec_05_libhashtable_integration_compliance.c \
        -I../../include \
        -I../../include/libhashtable \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_05_compliance
    SPEC_05_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_05_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_05_compliance

    echo ""
else
    echo "Spec 05 compliance test not found - skipping"
    echo ""
fi

# Run Spec 04 compliance tests (Event System)
echo "-------------------------------------------------------------------"
echo "Spec 04: Event System Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_04_event_system_compliance.c" ]; then
    echo "Compiling spec_04_event_system_compliance..."

    gcc -o spec_04_compliance \
        compliance/spec_04_event_system_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_04_compliance
    SPEC_04_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_04_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_04_compliance

    echo ""
else
    echo "Spec 04 compliance test not found - skipping"
    echo ""
fi

# Run Spec 03 compliance tests (Buffer Management)
echo "-------------------------------------------------------------------"
echo "Spec 03: Buffer Management Compliance"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_03_buffer_management_compliance.c" ]; then
    echo "Compiling spec_03_buffer_management_compliance..."

    gcc -o spec_03_compliance \
        compliance/spec_03_buffer_management_compliance.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_03_compliance
    SPEC_03_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_03_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_03_compliance

    echo ""
else
    echo "Spec 03 compliance test not found - skipping"
    echo ""
fi

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

# Run Spec 09 compliance tests (History System)
echo "-------------------------------------------------------------------"
echo "Spec 09: History System Compliance (Phase 1)"
echo "-------------------------------------------------------------------"
echo ""

if [ -f "compliance/spec_09_history_compliance.c" ]; then
    echo "Compiling spec_09_history_compliance..."

    gcc -o spec_09_compliance \
        compliance/spec_09_history_compliance.c \
        ../../src/lle/history_core.c \
        ../../src/lle/memory_management.c \
        ../../src/lle/error_handling.c \
        ../../src/lusush_memory_pool.c \
        -I../../include \
        -std=c11 \
        -D_POSIX_C_SOURCE=200809L \
        -Wall -Wextra \
        -pthread \
        -g

    echo "Compilation successful"
    echo ""

    # Run the test
    ./spec_09_compliance
    SPEC_09_RESULT=$?

    TOTAL_FAILURES=$((TOTAL_FAILURES + SPEC_09_RESULT))
    TESTS_RUN=$((TESTS_RUN + 1))

    # Cleanup
    rm -f spec_09_compliance

    echo ""
else
    echo "Spec 09 compliance test not found - skipping"
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
