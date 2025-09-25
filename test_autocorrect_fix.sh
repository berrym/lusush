#!/bin/bash

# Simple test to verify autocorrection fix for non-interactive mode
# This test ensures that autocorrection prompts don't appear when running in pipes/scripts

echo "Testing autocorrection fix..."

# Test 1: Command that would normally trigger autocorrection should not prompt
echo "Test 1: Non-existent command (should not hang or prompt)"
echo 'nonexistentcommand123' | timeout 5s ./builddir/lusush
if [ $? -eq 124 ]; then
    echo "❌ FAILED: Test timed out (likely hanging on prompt)"
    exit 1
else
    echo "✅ PASSED: No hanging on non-existent command"
fi

# Test 2: Misspelled command should not prompt
echo "Test 2: Misspelled command (should not hang or prompt)"
echo 'eco hello' | timeout 5s ./builddir/lusush
if [ $? -eq 124 ]; then
    echo "❌ FAILED: Test timed out (likely hanging on prompt)"
    exit 1
else
    echo "✅ PASSED: No hanging on misspelled command"
fi

# Test 3: Valid command should work normally
echo "Test 3: Valid command should work"
output=$(echo 'echo "test successful"' | timeout 5s ./builddir/lusush)
if [ "$output" = "test successful" ]; then
    echo "✅ PASSED: Valid commands work correctly"
else
    echo "❌ FAILED: Valid command didn't work. Output: '$output'"
    exit 1
fi

# Test 4: false command should return correct exit code
echo "Test 4: false command exit code"
echo 'false' | timeout 5s ./builddir/lusush
exit_code=$?
if [ $exit_code -eq 1 ]; then
    echo "✅ PASSED: false command returns exit code 1"
else
    echo "❌ FAILED: false command returned exit code $exit_code instead of 1"
    exit 1
fi

echo ""
echo "🎉 All autocorrection fix tests PASSED!"
echo "✅ Autocorrection no longer blocks non-interactive testing"
echo "✅ Commands execute properly in pipe/script mode"
