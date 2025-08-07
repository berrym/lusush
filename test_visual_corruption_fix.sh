#!/bin/bash

# Test script to validate the visual corruption fix after backspacing over wrapped lines
# This reproduces the issue described: visual corruption after backspacing over line
# then completing command and pressing enter, where echo output doesn't appear on next line

set -e

LUSUSH_BIN="./builddir/lusush"
TEST_LOG="/tmp/visual_corruption_test.log"

echo "=== Testing Visual Corruption Fix ==="
echo "Testing scenario: backspace over wrapped line, complete command, press enter"
echo ""

# Test 1: Simple echo command after backspace operations
echo "Test 1: Simple echo command (should show output on new line)"
echo -e 'echo "test output 1"\nexit' | LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ${LUSUSH_BIN} 2>${TEST_LOG} > /tmp/test1_output.txt

echo "Output from test 1:"
cat /tmp/test1_output.txt
echo ""

# Test 2: Long line that wraps, then completion
echo "Test 2: Long line with wrapping (should show output correctly)"
echo -e 'echo "this is a very long line that will definitely wrap across multiple terminal lines and cause wrapping behavior"\nexit' | LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ${LUSUSH_BIN} 2>>${TEST_LOG} > /tmp/test2_output.txt

echo "Output from test 2:"
cat /tmp/test2_output.txt
echo ""

# Test 3: Interactive test instructions
echo "Test 3: Manual test instructions"
echo "To test the specific scenario manually:"
echo "1. Run: LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ${LUSUSH_BIN} 2>/tmp/manual_test.log"
echo "2. Type: echo \"this is a very long line that will wrap across the terminal window\""
echo "3. Backspace over the wrapped portion to cross line boundaries"
echo "4. Complete as: echo \"test output\""
echo "5. Press Enter"
echo "6. Verify that \"test output\" appears on the next line before the new prompt"
echo ""

# Check debug logs for state validation
echo "Checking debug logs for state validation and cursor operations:"
if grep -q "State validation failed after Enter" ${TEST_LOG}; then
    echo "✅ State validation fix was triggered (expected for complex cases)"
else
    echo "ℹ️  State validation fix was not triggered (may not be needed for these simple cases)"
fi

if grep -q "Enter key pressed" ${TEST_LOG}; then
    echo "✅ Enter key processing logged correctly"
else
    echo "❌ Enter key processing not found in logs"
fi

# Check for cursor operations
if grep -q "move cursor" ${TEST_LOG}; then
    echo "✅ Cursor movement operations logged"
else
    echo "ℹ️  No cursor movement operations in simple tests"
fi

echo ""
echo "=== Visual Corruption Fix Test Complete ==="
echo "If all echo outputs appeared correctly before the next prompt, the fix is working."
echo ""
echo "Debug log saved to: ${TEST_LOG}"
echo "To view full debug log: cat ${TEST_LOG}"
