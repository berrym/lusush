#!/usr/bin/env bash

# Debug script to isolate the file descriptor issue
# This script tests why redirection in compound commands affects captured output

echo "=== FILE DESCRIPTOR ISSUE DEBUGGING ==="
echo

# Test the exact scenario that's failing
echo "The failing scenario:"
echo "Command: cd /tmp; pwd; cd - >/dev/null"
echo "When run with output capture (like in tests), output is empty"
echo "When run directly to terminal, output shows /tmp"
echo

# Test with different capture methods
echo "=== Testing different capture methods ==="
echo

echo "1. Direct execution (should work):"
echo 'cd /tmp; pwd; cd - >/dev/null' | ./builddir/lusush
echo

echo "2. Captured with command substitution (test environment):"
result=$(echo 'cd /tmp; pwd; cd - >/dev/null' | ./builddir/lusush)
echo "Result: [$result]"
echo "Length: ${#result}"
echo

echo "3. Captured with file redirection:"
echo 'cd /tmp; pwd; cd - >/dev/null' | ./builddir/lusush > temp_capture.txt
echo "File contents: [$(cat temp_capture.txt)]"
echo "File size: $(wc -c < temp_capture.txt) bytes"
echo

echo "4. Testing with stderr capture too:"
result_both=$(echo 'cd /tmp; pwd; cd - >/dev/null' | ./builddir/lusush 2>&1)
echo "Result with stderr: [$result_both]"
echo

echo "=== Component isolation ==="
echo

echo "5. Testing without the problematic redirection:"
result_no_redir=$(echo 'cd /tmp; pwd; cd -' | ./builddir/lusush)
echo "Result without >/dev/null: [$result_no_redir]"
echo

echo "6. Testing simpler redirection case:"
result_simple=$(echo 'echo hello; echo world >/dev/null' | ./builddir/lusush)
echo "Result simple case: [$result_simple]"
echo

echo "7. Testing redirection position:"
result_pos=$(echo 'echo first >/dev/null; echo second' | ./builddir/lusush)
echo "Result different position: [$result_pos]"
echo

echo "=== File descriptor state analysis ==="
echo

echo "8. Check if stdout is being permanently redirected:"
echo 'cd /tmp; pwd; cd - >/dev/null; echo after' | ./builddir/lusush
echo

echo "9. Multiple commands after redirection:"
echo 'echo before; echo middle >/dev/null; echo after1; echo after2' | ./builddir/lusush
echo

echo "=== Hypothesis testing ==="
echo

echo "If the issue is file descriptor management:"
echo "- Test 2 and 3 should show empty results (PROBLEM)"
echo "- Test 5 should show proper output (WORKING)"
echo "- Test 8 should show 'after' (if stdout is properly restored)"
echo "- Test 9 should show before, after1, after2 (if scoping works)"
echo

echo "If output is empty in tests 2-3 but present in test 1,"
echo "then the issue is that internal redirection (>/dev/null) is"
echo "interfering with the shell's ability to write to its own stdout"
echo "when that stdout is being captured externally."
echo

# Cleanup
rm -f temp_capture.txt
