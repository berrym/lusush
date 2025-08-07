#!/bin/bash

# Simple backspace test for LLE cursor positioning fix
# This script creates a controlled test environment to verify the fix

set -e

echo "=========================================="
echo "Simple LLE Backspace Test"
echo "=========================================="

# Build first
echo "Building LLE..."
if ! scripts/lle_build.sh build; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build successful!"
echo ""

# Create a test input sequence
echo "Creating test input sequence..."
cat > /tmp/backspace_test_input.txt << 'EOF'
hello world
EOF

echo "Test sequence created: 'hello world' + Enter"
echo ""

# Run the test with debug output
echo "Running LLE with test input and debug logging..."
echo "The shell should:"
echo "1. Display the prompt"
echo "2. Accept the input 'hello world'"
echo "3. Execute the command"
echo "4. Return to prompt"
echo ""

echo "Debug output will be saved to /tmp/simple_backspace_debug.log"
echo ""

# Run the test
LLE_DEBUG=1 ./builddir/lusush < /tmp/backspace_test_input.txt 2>/tmp/simple_backspace_debug.log

echo "Test completed!"
echo ""

# Analyze the debug output
echo "=========================================="
echo "Debug Output Analysis"
echo "=========================================="

if [ -f /tmp/simple_backspace_debug.log ]; then
    echo "Debug log found. Analyzing..."

    # Check for visual debug messages
    if grep -q "VISUAL_DEBUG" /tmp/simple_backspace_debug.log; then
        echo "✅ Visual debug output detected"

        # Show key debug messages
        echo ""
        echo "Key debug messages:"
        grep "VISUAL_DEBUG.*Prompt width\|VISUAL_DEBUG.*Terminal width" /tmp/simple_backspace_debug.log | head -5

        # Check for cursor positioning messages
        if grep -q "VISUAL_DEBUG.*Moving cursor left" /tmp/simple_backspace_debug.log; then
            echo "✅ Cursor left movement detected"
        else
            echo "ℹ️  No cursor left movement (expected if no backspace occurred)"
        fi

    else
        echo "ℹ️  No visual debug output (expected for simple command execution)"
    fi

    # Check for any errors
    if grep -q "ERROR" /tmp/simple_backspace_debug.log; then
        echo "❌ Errors detected:"
        grep "ERROR" /tmp/simple_backspace_debug.log | head -3
    else
        echo "✅ No errors detected"
    fi

    # Show terminal information
    echo ""
    echo "Terminal information:"
    grep "Terminal.*zed\|Size:.*x\|Width:" /tmp/simple_backspace_debug.log | head -3

else
    echo "❌ Debug log not found"
fi

echo ""
echo "=========================================="
echo "Manual Test Instructions"
echo "=========================================="

echo "For manual testing of backspace functionality:"
echo ""
echo "1. Run: LLE_DEBUG=1 ./builddir/lusush"
echo "2. Type: hello world this is a long command that wraps"
echo "3. Press backspace several times"
echo "4. Observe: Characters should disappear on same line (no staircase effect)"
echo "5. Press Ctrl+C to exit"
echo ""
echo "Expected behavior:"
echo "✅ Characters disappear smoothly on the same line"
echo "✅ No visual 'staircase' or new line creation"
echo "✅ Prompt remains stable and visible"
echo "✅ Terminal cursor position matches visual content"
echo ""
echo "If you see staircase effects or line corruption, the fix needs refinement."

echo ""
echo "Debug log location: /tmp/simple_backspace_debug.log"
echo "Test input file: /tmp/backspace_test_input.txt"
echo ""
echo "To view full debug log: cat /tmp/simple_backspace_debug.log"
echo "To search debug log: grep 'VISUAL_DEBUG' /tmp/simple_backspace_debug.log"
echo ""
echo "Test script completed!"
