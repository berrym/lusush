#!/bin/bash

# Comprehensive test suite for wait built-in command
# Tests POSIX compliance and edge cases

echo "=== LUSUSH WAIT BUILT-IN TEST SUITE ==="
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

# Test function
test_wait() {
    local description="$1"
    local script="$2"
    local expected_exit="$3"

    echo -n "Testing: $description... "

    # Create temporary script
    local temp_script=$(mktemp)
    echo "$script" > "$temp_script"

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

# Test function with output capture
test_wait_output() {
    local description="$1"
    local script="$2"
    local expected_pattern="$3"

    echo -n "Testing: $description... "

    # Create temporary script
    local temp_script=$(mktemp)
    local temp_output=$(mktemp)
    echo "$script" > "$temp_script"

    # Run test
    timeout 10s "$LUSUSH" < "$temp_script" > "$temp_output" 2>&1
    local actual_exit=$?

    if grep -q "$expected_pattern" "$temp_output"; then
        echo "✅ PASS"
        ((PASS++))
    else
        echo "❌ FAIL (pattern '$expected_pattern' not found)"
        echo "   Output: $(cat "$temp_output")"
        ((FAIL++))
    fi

    rm -f "$temp_script" "$temp_output"
}

echo "=== BASIC WAIT FUNCTIONALITY ==="

# Test 1: wait with no arguments (should succeed when no background jobs)
test_wait "wait with no arguments" "wait" 0

# Test 2: wait for specific PID that doesn't exist
test_wait "wait for non-existent PID" "wait 99999" 127

# Test 3: wait for background job
test_wait "wait for background job" "sleep 0.1 & wait" 0

# Test 4: wait for multiple background jobs
test_wait "wait for multiple background jobs" "sleep 0.1 & sleep 0.1 & wait" 0

echo
echo "=== WAIT WITH PROCESS IDs ==="

# Test 5: wait for specific background job PID
test_wait "wait for specific PID" "sleep 0.1 & PID=\$!; wait \$PID" 0

# Test 6: wait for job that has already completed
test_wait "wait for completed job" "sleep 0.1 & sleep 0.2; wait" 0

# Test 7: Multiple wait calls for same job
test_wait "multiple wait calls for same job" "sleep 0.1 & PID=\$!; wait \$PID; wait \$PID" 127

echo
echo "=== ERROR HANDLING ==="

# Test 8: wait with invalid PID format
test_wait "wait with invalid PID" "wait abc" 2

# Test 9: wait with negative PID
test_wait "wait with negative PID" "wait -1" 127

# Test 10: wait with multiple PIDs (some valid, some invalid)
test_wait "wait with mixed PIDs" "sleep 0.1 & wait \$! 99999" 127

echo
echo "=== EXIT STATUS PROPAGATION ==="

# Test 11: wait should return exit status of waited process
test_wait "wait returns process exit status" "sh -c 'exit 42' & wait" 42

# Test 12: wait for process that exits with status 1
test_wait "wait for failing process" "sh -c 'exit 1' & wait" 1

# Test 13: wait for process that exits with status 0
test_wait "wait for successful process" "sh -c 'exit 0' & wait" 0

echo
echo "=== COMPLEX SCENARIOS ==="

# Test 14: wait in subshell
test_wait "wait in subshell" "(sleep 0.1 & wait)" 0

# Test 15: wait with command substitution
test_wait "wait with command substitution" "sleep 0.1 & PID=\$(echo \$!); wait \$PID" 0

# Test 16: Multiple background jobs, wait for specific one
test_wait "wait for specific job among many" "sleep 0.2 & sleep 0.1 & PID=\$!; wait \$PID" 0

echo
echo "=== POSIX COMPLIANCE TESTS ==="

# Test 17: wait should work with job control notation (%1)
# Note: This might not work if job control is not fully implemented
test_wait "wait with job notation" "sleep 0.1 & wait %1" 0

# Test 18: wait should handle SIGCHLD properly
test_wait "wait handles SIGCHLD" "sleep 0.1 & sleep 0.05; wait" 0

echo
echo "=== EDGE CASES ==="

# Test 19: wait with empty argument
test_wait "wait with empty argument" "wait ''" 2

# Test 20: wait with whitespace argument
test_wait "wait with whitespace" "wait ' '" 2

# Test 21: wait with very long PID string
test_wait "wait with long PID" "wait 123456789012345" 127

# Test 22: wait for PID 0 (should be invalid)
test_wait "wait for PID 0" "wait 0" 127

# Test 23: wait for PID 1 (init, should fail for non-child)
test_wait "wait for PID 1" "wait 1" 127

echo
echo "=== INTERACTIVE SCENARIOS ==="

# Test 24: Multiple wait commands
test_wait "multiple sequential waits" "sleep 0.1 & wait; sleep 0.1 & wait" 0

# Test 25: wait while other jobs are running
test_wait "wait while other jobs running" "sleep 0.3 & sleep 0.1 & PID=\$!; wait \$PID" 0

echo
echo "=== HELP AND USAGE ==="

# Test 26: wait --help (if implemented)
test_wait_output "wait help" "wait --help 2>&1" "usage\\|Usage\\|USAGE\\|wait"

# Test 27: wait with too many arguments
test_wait "wait with many arguments" "wait 1 2 3 4 5 6 7 8 9 10" 127

echo
echo "=== SIGNAL HANDLING ==="

# Test 28: wait for process that receives signal
test_wait "wait for signaled process" "sleep 10 & PID=\$!; kill -TERM \$PID 2>/dev/null; wait \$PID" 143

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
