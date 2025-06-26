#!/usr/bin/env bash

# Debug script for Test 108: compound command redirection scoping issue
# This script tests the specific failing case where redirection scope bleeds
# to adjacent commands in a sequence

echo "=== DEBUG TEST 108: Compound Command Redirection Scoping ==="
echo

# Test the exact failing command from Test 108
echo "Testing: cd /tmp; pwd; cd - >/dev/null"
echo "Expected output: /tmp (from pwd command)"
echo "Actual output:"
echo "cd /tmp; pwd; cd - >/dev/null" | ./builddir/lusush
echo "--- End of output ---"
echo

# Test individual components to isolate the issue
echo "=== COMPONENT TESTING ==="
echo

echo "1. Testing 'cd /tmp; pwd' (without redirection):"
echo "cd /tmp; pwd" | ./builddir/lusush
echo

echo "2. Testing 'pwd; cd - >/dev/null' (pwd first, then cd with redirection):"
echo "cd /tmp" | ./builddir/lusush  # Set up /tmp directory first
echo "pwd; cd - >/dev/null" | ./builddir/lusush
echo

echo "3. Testing 'cd -' alone:"
echo "cd /tmp" | ./builddir/lusush  # Set up /tmp directory first
echo "cd -" | ./builddir/lusush
echo

echo "4. Testing 'cd - >/dev/null' alone:"
echo "cd /tmp" | ./builddir/lusush  # Set up /tmp directory first
echo "cd - >/dev/null" | ./builddir/lusush
echo

echo "=== REDIRECTION SCOPE ANALYSIS ==="
echo

echo "5. Testing compound commands with redirection at different positions:"
echo "Command: echo hello; echo world >/dev/null"
echo "Expected: hello (world should be redirected)"
echo "echo hello; echo world >/dev/null" | ./builddir/lusush
echo

echo "Command: echo hello >/dev/null; echo world"
echo "Expected: world (hello should be redirected)"
echo "echo hello >/dev/null; echo world" | ./builddir/lusush
echo

echo "=== PIPE REDIRECTION TESTING ==="
echo

echo "6. Testing with different redirection forms:"
echo "Command: cd /tmp; pwd; cd - 2>/dev/null"
echo "cd /tmp; pwd; cd - 2>/dev/null" | ./builddir/lusush
echo

echo "Command: cd /tmp; pwd; cd - 1>/dev/null"
echo "cd /tmp; pwd; cd - 1>/dev/null" | ./builddir/lusush
echo

echo "=== END DEBUG TEST 108 ==="
