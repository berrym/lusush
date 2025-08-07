#!/bin/bash

# Test script for LLE backspace functionality
# This script tests the visual positioning fix for multiline backspace

set -e

echo "=========================================="
echo "LLE Backspace Functionality Test"
echo "=========================================="

# Build the project first
echo "Building LLE..."
scripts/lle_build.sh build

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build successful!"
echo ""

# Test 1: Basic backspace test
echo "Test 1: Basic backspace test"
echo "Instructions:"
echo "1. Type: hello world"
echo "2. Press backspace 5 times"
echo "3. You should see 'hello w' with no visual staircase effect"
echo "4. Press Ctrl+C to exit"
echo ""

read -p "Press Enter to start Test 1..."
echo "Starting LLE with debug output..."

LLE_DEBUG=1 ./builddir/lusush 2>/tmp/backspace_test1.log

echo ""
echo "Test 1 completed. Debug output saved to /tmp/backspace_test1.log"
echo ""

# Check debug output for success indicators
echo "Checking debug output for success indicators..."
if grep -q "SUCCESS: Positioned cursor at content start" /tmp/backspace_test1.log; then
    echo "✅ Cursor positioning: SUCCESS"
else
    echo "❌ Cursor positioning: FAILED"
fi

if grep -q "SUCCESS: Cleared from cursor to end of line" /tmp/backspace_test1.log; then
    echo "✅ Line clearing: SUCCESS"
else
    echo "❌ Line clearing: FAILED"
fi

if grep -q "SUCCESS: Wrote.*characters" /tmp/backspace_test1.log; then
    echo "✅ Content writing: SUCCESS"
else
    echo "❌ Content writing: FAILED"
fi

echo ""

# Test 2: Long command test (multiline wrapping)
echo "Test 2: Long command test (multiline wrapping)"
echo "Instructions:"
echo "1. Type a long command that wraps to next line:"
echo "   echo this is a very long command that should wrap to the next line and test multiline backspace"
echo "2. Press backspace multiple times"
echo "3. Characters should disappear on the same line without staircase effect"
echo "4. Press Ctrl+C to exit"
echo ""

read -p "Press Enter to start Test 2..."
echo "Starting LLE with debug output..."

LLE_DEBUG=1 ./builddir/lusush 2>/tmp/backspace_test2.log

echo ""
echo "Test 2 completed. Debug output saved to /tmp/backspace_test2.log"
echo ""

# Check debug output for multiline test
echo "Checking debug output for multiline test..."
if grep -q "VISUAL_DEBUG.*BACKSPACE" /tmp/backspace_test2.log; then
    echo "✅ Backspace operations detected"

    # Count backspace operations
    backspace_count=$(grep -c "VISUAL_DEBUG.*BACKSPACE" /tmp/backspace_test2.log)
    echo "   Number of backspace operations: $backspace_count"

    # Check for positioning success
    positioning_success=$(grep -c "SUCCESS: Positioned cursor at content start" /tmp/backspace_test2.log)
    echo "   Successful positioning operations: $positioning_success"

    if [ "$positioning_success" -gt 0 ]; then
        echo "✅ Cursor positioning working in multiline context"
    else
        echo "❌ Cursor positioning failed in multiline context"
    fi
else
    echo "❌ No backspace operations detected"
fi

echo ""

# Test 3: Edge case - backspace all characters
echo "Test 3: Edge case test - backspace all characters"
echo "Instructions:"
echo "1. Type: test"
echo "2. Press backspace 4 times to delete all characters"
echo "3. Buffer should be empty, cursor should be at prompt end"
echo "4. Press Ctrl+C to exit"
echo ""

read -p "Press Enter to start Test 3..."
echo "Starting LLE with debug output..."

LLE_DEBUG=1 ./builddir/lusush 2>/tmp/backspace_test3.log

echo ""
echo "Test 3 completed. Debug output saved to /tmp/backspace_test3.log"
echo ""

# Check for empty buffer handling
echo "Checking empty buffer handling..."
if grep -q "Empty buffer, no content to write" /tmp/backspace_test3.log; then
    echo "✅ Empty buffer handling: SUCCESS"
else
    echo "❌ Empty buffer handling: Check manually"
fi

echo ""
echo "=========================================="
echo "TEST SUMMARY"
echo "=========================================="

echo "Test files generated:"
echo "  - /tmp/backspace_test1.log (basic test)"
echo "  - /tmp/backspace_test2.log (multiline test)"
echo "  - /tmp/backspace_test3.log (edge case test)"
echo ""

echo "Manual verification required:"
echo "1. No visual 'staircase' effect during backspace"
echo "2. Characters disappear on same line where typed"
echo "3. Prompt remains intact and visible"
echo "4. No prompt duplication or corruption"
echo ""

echo "Visual success criteria:"
echo "✅ Characters disappear on SAME line"
echo "✅ No new line creation during backspace"
echo "✅ Prompt preservation throughout operation"
echo "✅ Smooth, responsive backspace behavior"
echo ""

echo "If you observed the above criteria, the fix is SUCCESSFUL!"
echo "If you observed staircase effects or line corruption, further debugging needed."
echo ""

echo "To analyze debug logs:"
echo "  grep 'VISUAL_DEBUG' /tmp/backspace_test*.log"
echo "  grep 'SUCCESS\\|ERROR' /tmp/backspace_test*.log"
echo ""

echo "Test completed!"
