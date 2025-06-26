#!/usr/bin/env bash

# Debug script for redirection scope bleeding issue
# This script tests how redirection affects command sequences

echo "=== REDIRECTION SCOPE DEBUGGING ==="
echo

echo "Problem: Redirection at end of command sequence affects earlier commands"
echo "Expected: Only the last command should be redirected"
echo

# Test 1: Simple case - this should work correctly
echo "Test 1: Simple redirection (should work)"
echo "Command: echo hello >/dev/null"
echo "Expected: (no output)"
echo "Actual:"
echo 'echo hello >/dev/null' | ./builddir/lusush
echo "--- End Test 1 ---"
echo

# Test 2: Command sequence with redirection at end
echo "Test 2: Command sequence with final redirection (PROBLEM CASE)"
echo "Command: echo first; echo second; echo third >/dev/null"
echo "Expected: first\\nsecond (third should be redirected)"
echo "Actual:"
echo 'echo first; echo second; echo third >/dev/null' | ./builddir/lusush
echo "--- End Test 2 ---"
echo

# Test 3: The specific failing test case
echo "Test 3: The failing Test 108 case"
echo "Command: cd /tmp; pwd; cd - >/dev/null"
echo "Expected: /tmp (pwd output, cd - should be silent)"
echo "Actual:"
echo 'cd /tmp; pwd; cd - >/dev/null' | ./builddir/lusush
echo "--- End Test 3 ---"
echo

# Test 4: Verify individual commands work
echo "Test 4: Individual commands (verification)"
echo "Command: cd /tmp; pwd"
echo "Expected: /tmp"
echo "Actual:"
echo 'cd /tmp; pwd' | ./builddir/lusush
echo "--- End Test 4 ---"
echo

# Test 5: Test redirection placement variations
echo "Test 5: Redirection placement variations"
echo "Command: echo first >/dev/null; echo second"
echo "Expected: second (first should be redirected)"
echo "Actual:"
echo 'echo first >/dev/null; echo second' | ./builddir/lusush
echo "--- End Test 5 ---"
echo

# Test 6: Multiple redirections
echo "Test 6: Multiple redirections in sequence"
echo "Command: echo one; echo two >/dev/null; echo three"
echo "Expected: one\\nthree (two should be redirected)"
echo "Actual:"
echo 'echo one; echo two >/dev/null; echo three' | ./builddir/lusush
echo "--- End Test 6 ---"
echo

# Test 7: Background vs redirection
echo "Test 7: Background execution test"
echo "Command: echo background &"
echo "Expected: background (with job info)"
echo "Actual:"
echo 'echo background &' | ./builddir/lusush
sleep 1  # Give background job time to complete
echo "--- End Test 7 ---"
echo

echo "=== ANALYSIS ==="
echo "If Test 2, 3, and 6 show missing output, then redirection scope"
echo "is bleeding from the last command to earlier commands in the sequence."
echo "This indicates the parser or executor is applying redirections"
echo "at the wrong scope level (entire sequence vs individual command)."
echo
