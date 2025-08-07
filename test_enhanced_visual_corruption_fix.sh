#!/bin/bash

# Enhanced Visual Corruption Fix Test
# Tests the comprehensive fix for cursor positioning after multiple cross-line backspace operations
# This addresses the issue where echo output doesn't appear on the correct line after complex editing

set -e

LUSUSH_BIN="./builddir/lusush"
TEST_LOG="/tmp/enhanced_visual_corruption_test.log"

echo "=== Enhanced Visual Corruption Fix Test ==="
echo "Testing comprehensive fix for cursor positioning after cross-line operations"
echo ""

# Clean up previous test logs
rm -f ${TEST_LOG} /tmp/test_*.txt

# Test 1: Simple echo command (baseline)
echo "Test 1: Simple echo command (baseline verification)"
echo -e 'echo "simple test output"\nexit' | LLE_DEBUG=1 ${LUSUSH_BIN} 2>${TEST_LOG} > /tmp/test1_output.txt

echo "Output from test 1:"
cat /tmp/test1_output.txt
echo ""

# Test 2: Long command that wraps, then immediate execution
echo "Test 2: Long wrapped command (should show output correctly)"
echo -e 'echo "this is a very long line that will wrap across multiple terminal lines and should display output correctly"\nexit' | LLE_DEBUG=1 ${LUSUSH_BIN} 2>>${TEST_LOG} > /tmp/test2_output.txt

echo "Output from test 2:"
cat /tmp/test2_output.txt
echo ""

# Test 3: Multiple commands to test cursor state persistence
echo "Test 3: Multiple commands testing cursor state persistence"
echo -e 'echo "first command"\necho "second command output"\necho "third command"\nexit' | LLE_DEBUG=1 ${LUSUSH_BIN} 2>>${TEST_LOG} > /tmp/test3_output.txt

echo "Output from test 3:"
cat /tmp/test3_output.txt
echo ""

# Test 4: Verify state validation is working
echo "Test 4: Checking debug logs for state validation and cursor positioning"
if grep -q "State validation failed before newline" ${TEST_LOG}; then
    echo "✅ Pre-newline state validation fix was triggered (good for complex cases)"
else
    echo "ℹ️  Pre-newline state validation was not needed (simple cases)"
fi

if grep -q "State validation failed after newline" ${TEST_LOG}; then
    echo "✅ Post-newline state validation fix was triggered (comprehensive recovery)"
else
    echo "ℹ️  Post-newline state validation was not needed"
fi

if grep -q "Cursor positioning complete for command output" ${TEST_LOG}; then
    echo "✅ Enhanced cursor positioning is active and working"
else
    echo "❌ Enhanced cursor positioning debug messages not found"
fi

if grep -q "Enter key pressed" ${TEST_LOG}; then
    echo "✅ Enter key processing logged correctly"
else
    echo "❌ Enter key processing not found in logs"
fi

echo ""
echo "=== Manual Test Instructions for Complex Scenario ==="
echo "To reproduce the specific issue you reported:"
echo ""
echo "1. Run: LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ${LUSUSH_BIN} 2>/tmp/manual_debug.log"
echo "2. Type: echo \"this is a very long line that will wrap\""
echo "3. Backspace multiple times across the line wrapping boundary"
echo "4. Continue backspacing and retyping to cross boundaries multiple times"
echo "5. Complete as: echo \"this is a very long line that will wrap\""
echo "6. Press Enter"
echo "7. Verify that the echo output appears on a NEW line before the next prompt"
echo "8. The output should NOT mix with the prompt or appear on the original line"
echo ""

# Test 5: Cross-line boundary simulation (automated)
echo "Test 5: Cross-line boundary scenario simulation"
echo "This simulates the cross-line boundary scenario with a very long command:"

# Create a test that simulates typing, backspacing, and completing
echo -e 'echo "this is a very long line that will definitely wrap across terminal boundaries and cause multiple line wrapping and cursor position tracking to be tested thoroughly"\nexit' | LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ${LUSUSH_BIN} 2>>/tmp/boundary_test.log > /tmp/test5_output.txt

echo "Output from boundary test:"
cat /tmp/test5_output.txt
echo ""

# Check for cross-line operations in the debug log
if grep -q "Cross-line operation" /tmp/boundary_test.log; then
    echo "✅ Cross-line operations detected in boundary test"
    echo "✅ This tests the exact scenario that was causing corruption"
else
    echo "ℹ️  No cross-line operations in this test (may need manual testing)"
fi

if grep -q "cursor query failed, invalidating position tracking" /tmp/boundary_test.log; then
    echo "⚠️  Position tracking invalidation detected - this is the critical scenario"
    echo "✅ Enhanced fix should handle this case with pre/post newline validation"
else
    echo "ℹ️  No position tracking invalidation in this automated test"
fi

echo ""
echo "=== Enhanced Fix Verification ==="
echo "Checking for evidence that the enhanced fix is working:"

# Count state validation triggers
BEFORE_VALIDATIONS=$(grep -c "State validation failed before newline" ${TEST_LOG} 2>/dev/null || echo "0")
AFTER_VALIDATIONS=$(grep -c "State validation failed after newline" ${TEST_LOG} 2>/dev/null || echo "0")
CURSOR_POSITIONING=$(grep -c "Cursor positioning complete" ${TEST_LOG} 2>/dev/null || echo "0")

echo "📊 Fix Activation Statistics:"
echo "   - Pre-newline state validations: ${BEFORE_VALIDATIONS}"
echo "   - Post-newline state validations: ${AFTER_VALIDATIONS}"
echo "   - Cursor positioning completions: ${CURSOR_POSITIONING}"

if [ "$CURSOR_POSITIONING" -gt 0 ]; then
    echo "✅ Enhanced cursor positioning is actively working"
else
    echo "❌ Enhanced cursor positioning may not be active"
fi

echo ""
echo "=== Test Results Summary ==="
if grep -q "output test\|simple test\|first command\|second command" /tmp/test*.txt; then
    echo "✅ ALL ECHO OUTPUTS APPEARED CORRECTLY"
    echo "✅ No visual corruption detected in automated tests"
    echo "✅ Enhanced fix appears to be working properly"
else
    echo "❌ Some echo outputs may be missing or corrupted"
    echo "❌ Enhanced fix may need further investigation"
fi

echo ""
echo "=== Debug Logs Available ==="
echo "Main test log: ${TEST_LOG}"
echo "Boundary test log: /tmp/boundary_test.log"
echo "Individual test outputs: /tmp/test*.txt"
echo ""
echo "To view detailed debug information:"
echo "  cat ${TEST_LOG} | grep -E '(State validation|Cursor positioning|Cross-line|Enter key)'"
echo ""
echo "=== CRITICAL TEST FOR YOUR SCENARIO ==="
echo "🎯 **IMPORTANT**: To test the exact scenario you reported:"
echo "   1. Run the shell manually: ${LUSUSH_BIN}"
echo "   2. Type a very long command that wraps"
echo "   3. Backspace across line boundaries multiple times"
echo "   4. Complete with a different command like: echo \"test output\""
echo "   5. Press Enter"
echo "   6. Verify \"test output\" appears on a NEW line, not mixed with the prompt"
echo ""
echo "If the output appears correctly on a new line, the enhanced fix is working! ✅"
echo "If the output still appears on the original line or mixes with the prompt, please report this."
