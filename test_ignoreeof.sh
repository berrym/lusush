#!/bin/bash

# Test script for ignoreeof functionality
echo "=== Testing ignoreeof option ==="

# Test 1: Default behavior (ignoreeof off)
echo "Test 1: Default behavior (ignoreeof should be OFF)"
echo "set -o | grep ignoreeof; exit" | ./builddir/lusush -i

echo ""
echo "Test 2: Enable ignoreeof option"
echo "set -o ignoreeof; set -o | grep ignoreeof; exit" | ./builddir/lusush -i

echo ""
echo "Test 3: Disable ignoreeof option"
echo "set -o ignoreeof; set +o ignoreeof; set -o | grep ignoreeof; exit" | ./builddir/lusush -i

echo ""
echo "Test 4: Manual interactive test (requires user input)"
echo "Run: ./builddir/lusush -i"
echo "Then type: set -o ignoreeof"
echo "Then press Ctrl+D multiple times - should show 'Use \"exit\" to leave the shell.'"
echo "Then type: exit"
echo ""
echo "=== All pipe-based tests completed ==="
echo "Note: ignoreeof only prevents EOF exit in true interactive sessions"
echo "When input comes from pipes/files, EOF naturally terminates the shell"
