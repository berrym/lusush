#!/bin/bash

# Test script to verify cursor positioning fix for Linux terminals
# This script tests that cursor positioning during syntax highlighting doesn't create newlines

echo "=== Cursor Positioning Fix Test ==="
echo "Testing that syntax highlighting full rewrites don't create newlines"
echo

# Get current terminal size
COLS=$(tput cols)
ROWS=$(tput lines)

echo "Current terminal size: ${COLS}x${ROWS}"
echo "Platform: $(uname -s)"
echo

# Enable debug mode to see what's happening
export LLE_DEBUG=1

echo "Starting lusush with debug output enabled..."
echo
echo "Test Cases to Try:"
echo "1. Type: 'echo test' - watch for cursor positioning messages"
echo "2. Look for: 'Linux: Skipping cursor repositioning to avoid timing issues'"
echo "3. Type: 'ls -la' - test multiple syntax-sensitive characters"
echo "4. Type: 'echo \"hello world\"' - test quote handling"
echo
echo "Expected Behavior After Fix:"
echo "✅ PASS: Text appears on same line as prompt"
echo "✅ PASS: Debug shows 'Skipping cursor repositioning' on Linux"
echo "✅ PASS: No unwanted newlines during typing"
echo "✅ PASS: Syntax highlighting still works"
echo
echo "Failure Indicators:"
echo "❌ FAIL: Text jumps to newlines"
echo "❌ FAIL: Debug shows 'Failed to move cursor to text start'"
echo "❌ FAIL: Strange cursor positioning behavior"
echo

echo "Key Debug Messages to Watch For:"
echo "- 'Linux true incremental: appending' for normal characters"
echo "- 'Linux syntax-sensitive char' for spaces/quotes"
echo "- 'Linux: Skipping cursor repositioning' during full rewrite"
echo "- 'Using Linux-safe character clearing' instead of escape sequences"
echo

# Run lusush
echo "Press Ctrl+C to exit lusush when done testing"
echo "Running lusush..."
./builddir/lusush

echo
echo "=== Test Results Analysis ==="
echo "Check the debug output above for:"
echo
echo "✅ SUCCESS INDICATORS:"
echo "  - Text stayed on same line as prompt"
echo "  - Saw 'Skipping cursor repositioning' messages"
echo "  - Most characters used 'true incremental'"
echo "  - Syntax highlighting colors appeared correctly"
echo
echo "❌ FAILURE INDICATORS:"
echo "  - Text jumped to newlines during typing"
echo "  - Saw 'Failed to move cursor to text start' errors"
echo "  - Cursor positioning seemed wrong"
echo
echo "The fix is working if:"
echo "1. Typing 'echo test' keeps text on same line"
echo "2. Space after 'echo' triggers syntax highlighting but no newlines"
echo "3. Debug shows Linux-specific escape sequence avoidance"
echo "4. All functionality works normally without positioning issues"
