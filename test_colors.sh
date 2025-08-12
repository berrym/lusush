#!/bin/bash

# Minimal test script to verify syntax highlighting in Lusush
# This will test if colors are actually being displayed

echo "=== Testing Lusush Syntax Highlighting ==="
echo

# Build first
cd "$(dirname "$0")"
ninja -C builddir > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful. Testing syntax highlighting..."
echo

# Test 1: Simple command with expect to capture output
echo "Test 1: Basic command highlighting"
echo "Expected: 'echo' should appear in GREEN"

# Use script to capture terminal output with colors
echo 'echo "hello"' | script -q -c './builddir/lusush' /dev/null 2>/dev/null | head -n 5

echo
echo "Test 2: Manual interactive test"
echo "Run: ./builddir/lusush"
echo "Then type: echo \"hello world\""
echo "You should see:"
echo "  - 'echo' in bright green"
echo "  - \"hello world\" in bright yellow"
echo "  - No literal escape codes like ^A^[[1;32m^B"
echo

echo "Test 3: Check if syntax highlighting is enabled"
./builddir/lusush -c 'echo "Syntax highlighting test completed"'

echo
echo "If you see colors above (not literal escape codes), syntax highlighting is working!"
echo "If you see ^A^[[1;32m^B codes, there's still an issue."
echo
echo "To test interactively, run: ./builddir/lusush"
