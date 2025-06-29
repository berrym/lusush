#!/bin/bash

echo "==============================================================================="
echo "LUSUSH CTRL-C BEHAVIOR TEST"
echo "==============================================================================="
echo
echo "This script tests that Ctrl-C (SIGINT) properly terminates processes"
echo "without exiting the shell itself."
echo
echo "Testing various scenarios:"
echo

# Test 1: Long-running command that can be interrupted
echo "Test 1: Long-running sleep command"
echo "Instructions:"
echo "1. The shell will run 'sleep 10'"
echo "2. Press Ctrl-C after 2-3 seconds"
echo "3. The sleep should be terminated but shell should remain"
echo "4. You should see the prompt again"
echo
echo "Starting test in 3 seconds..."
sleep 3

echo "sleep 10" | timeout 5 ./builddir/lusush -i
echo "✓ Sleep command test completed"
echo

# Test 2: Interactive command
echo "Test 2: Interactive shell behavior"
echo "Instructions:"
echo "1. Type a few commands like 'pwd', 'echo hello'"
echo "2. Try pressing Ctrl-C at the prompt (should just give new prompt)"
echo "3. Type 'exit' to quit when done"
echo
echo "Starting interactive test..."
echo "Type 'exit' when you're done testing Ctrl-C behavior"
echo

./builddir/lusush

echo
echo "==============================================================================="
echo "CTRL-C TEST COMPLETED"
echo "==============================================================================="
echo
echo "Expected behavior:"
echo "✓ Ctrl-C should interrupt running commands (like sleep)"
echo "✓ Ctrl-C should NOT exit the shell"
echo "✓ Shell should return to prompt after Ctrl-C"
echo "✓ Only 'exit' command should terminate the shell"
echo
