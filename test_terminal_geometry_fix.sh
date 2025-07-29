#!/bin/bash

# Test script to verify terminal geometry fix
# This script tests that LLE properly respects actual terminal dimensions
# and doesn't use hardcoded values for line wrapping

echo "=== Terminal Geometry Fix Test ==="
echo "Testing that LLE respects actual terminal dimensions"
echo

# Get current terminal size
COLS=$(tput cols)
ROWS=$(tput lines)

echo "Current terminal size: ${COLS}x${ROWS}"
echo

# Test 1: Basic character input should not force newlines
echo "Test 1: Basic character input"
echo "Type a long line of text (more than 40 characters):"
echo "Expected: Text should wrap naturally at terminal edge"
echo "Bug behavior: Text would jump to newlines at hardcoded width"
echo

# Enable debug mode to see what's happening
export LLE_DEBUG=1

# Start lusush with a simple prompt
echo "Starting lusush... (type 'exit' to quit)"
echo "Try typing: 'hello world this is a very long line of text that should wrap naturally'"
echo

# Run lusush
./builddir/lusush

echo
echo "=== Test Results ==="
echo "✅ PASS: If text wrapped naturally at terminal edge"
echo "❌ FAIL: If text jumped to newlines before reaching terminal edge"
echo "❌ FAIL: If resizing terminal caused strange cursor behavior"
echo
echo "Additional tests to perform manually:"
echo "1. Resize terminal while typing - cursor should adjust properly"
echo "2. Type text longer than terminal width - should wrap at actual edge"
echo "3. Backspace across wrapped lines - should work smoothly"
echo "4. Use different terminal sizes - behavior should be consistent"
