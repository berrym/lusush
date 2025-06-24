#!/bin/bash

# Corrected comprehensive test suite for wait built-in command
# Tests POSIX compliance using proper newline separation

echo "=== LUSUSH WAIT BUILT-IN TEST SUITE (CORRECTED) ==="
echo

# Build lusush first
echo "Building lusush..."
cd "$(dirname "$0")"
ninja -C builddir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"
echo

LUSUSH="./builddir/lusush"
PASS=0
FAIL=0

# Test function with newline-separated commands
test_wait_newline() {
    local description="$1"
    local commands="$2"
    local expected_exit="$3"

    echo -n "Testing: $description... "

    # Create temporary script with newline-separated commands
    local temp_script=$(mktemp)
    echo -e "$commands" > "$temp_script"

    # Run test and capture output
    local temp_output=$(mktemp)
    timeout 10s "$LUSUSH" < "$temp_script" > "$temp_output" 2>&1
    local actual_exit=$?

    # Extract the exit code from output if it contains "Exit code:" pattern
    if grep -q "Exit code:" "$temp_output"; then
        local reported_exit=$(grep "Exit code:" "$temp_output" | tail -1 | sed 's/.*Exit code: //')
        if [ "$reported_exit" = "$expected_exit" ]; then
            echo "✅ PASS"
            ((PASS++))
        else
            echo "❌ FAIL (expected $expected_exit, got $reported_exit)"
            ((FAIL++))
        fi
    else
        echo "❌ FAIL (no exit code reported)"
        echo "   Output: $(cat "$temp_output")"
        ((FAIL++))
    fi

    rm -f "$temp_script" "$temp_output"
}

# Test function for direct shell exit codes
test_wait_direct() {
    local description="$1"
    local commands="$2"
    local expected_exit="$3"

    echo -n "Testing: $description... "

    # Create temporary script
    local temp_script=$(mktemp)
    echo -e "$commands" > "$temp_script"

    # Run test
    timeout 10s "$LUSUSH" < "$temp_script" > /dev/null 2>&1
    local actual_exit=$?

    rm -f "$temp_script"

    if [ "$actual_exit" -eq "$expected_exit" ]; then
        echo "✅ PASS"
        ((PASS++))
    else
        echo "❌ FAIL (expected $expected_exit, got $actual_exit)"
        ((FAIL++))
    fi
}

echo "=== BASIC WAIT FUNCTIONALITY ==="

# Test 1: wait with no arguments (should succeed when no background jobs)
test_wait_newline "wait with no arguments" "wait\necho Exit code: \$?" 0

# Test 2: wait for non-existent PID
test_wait_newline "wait for non-existent PID" "wait 99999\necho Exit code: \$?" 127

# Test 3: wait for background job
test_wait_newline "wait for background job" "sleep 0.1 &\nwait\necho Exit code: \$?" 0

# Test 4: wait for multiple background jobs
test_wait_newline "wait for multiple background jobs" "sleep 0.1 &\nsleep 0.1 &\nwait\necho Exit code: \$?" 0

echo
echo "=== ERROR HANDLING ==="

# Test 5: wait with invalid PID format
test_wait_newline "wait with invalid PID" "wait abc\necho Exit code: \$?" 1

# Test 6: wait with negative PID
test_wait_newline "wait with negative PID" "wait -1\necho Exit code: \$?" 1

# Test 7: wait with zero PID
test_wait_newline "wait with zero PID" "wait 0\necho Exit code: \$?" 1

# Test 8: wait with empty string
test_wait_newline "wait with empty string" "wait ''\necho Exit code: \$?" 1

# Test 9: wait with whitespace
test_wait_newline "wait with whitespace" "wait ' '\necho Exit code: \$?" 1

echo
echo "=== POSIX COMPLIANCE ==="

# Test 10: wait for PID 1 (should fail for non-child)
test_wait_newline "wait for PID 1" "wait 1\necho Exit code: \$?" 127

# Test 11: wait with job notation %1
test_wait_newline "wait with job notation" "sleep 0.1 &\nwait %1\necho Exit code: \$?" 0

echo
echo "=== EXIT STATUS PROPAGATION ==="

# Test 12: wait should return exit status of successful process
test_wait_newline "wait for successful process" "sh -c 'exit 0' &\nwait\necho Exit code: \$?" 0

# Test 13: wait in subshell
test_wait_newline "wait in subshell" "(sleep 0.1 &; wait)\necho Exit code: \$?" 0

echo
echo "=== SHELL EXIT CODES ==="

# Test 14: shell exit code with wait error
test_wait_direct "shell exit with wait error" "wait abc" 0

# Test 15: shell exit code with successful wait
test_wait_direct "shell exit with successful wait" "wait" 0

echo
echo "=== COMPLEX SCENARIOS ==="

# Test 16: Multiple wait commands
test_wait_newline "multiple sequential waits" "sleep 0.1 &\nwait\nsleep 0.1 &\nwait\necho Exit code: \$?" 0

# Test 17: wait while other jobs are running
test_wait_newline "wait while other jobs running" "sleep 0.3 &\nsleep 0.1 &\nwait\necho Exit code: \$?" 0

echo
echo "=== EDGE CASES ==="

# Test 18: wait with very long number
test_wait_newline "wait with long number" "wait 123456789012345\necho Exit code: \$?" 127

# Test 19: wait with multiple invalid arguments
test_wait_newline "wait with multiple invalid args" "wait abc def\necho Exit code: \$?" 1

echo
echo "=== BACKGROUND JOB TESTS ==="

# Test 20: Basic background job completion
test_wait_newline "basic background job" "echo 'Background job starting' &\nwait\necho Exit code: \$?" 0

# Test 21: Background job with sleep
test_wait_newline "background sleep job" "sleep 0.05 &\nwait\necho Exit code: \$?" 0

echo
echo "=== ARGUMENT VALIDATION ==="

# Test 22: wait with mixed valid/invalid arguments
test_wait_newline "wait with mixed arguments" "wait 1 abc\necho Exit code: \$?" 1

# Test 23: wait with multiple spaces
test_wait_newline "wait with multiple spaces" "wait    123456\necho Exit code: \$?" 127

# Test 24: wait with tab character
test_wait_newline "wait with tab character" "wait\t123456\necho Exit code: \$?" 127

echo
echo "=== RESULTS ==="
echo "Tests passed: $PASS"
echo "Tests failed: $FAIL"
echo "Total tests: $((PASS + FAIL))"

if [ $FAIL -eq 0 ]; then
    echo "🎉 All tests passed!"
    exit 0
else
    echo "⚠️  Some tests failed"
    exit 1
fi
