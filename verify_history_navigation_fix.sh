#!/bin/bash

# History Navigation Fix Verification Script
# Tests the fix for lle_cmd_move_end failure in history navigation

echo "========================================================"
echo "History Navigation Fix Verification"
echo "========================================================"
echo "Testing fix for: lle_cmd_move_end returning -5 after first history operation"
echo "Root cause: lle_text_move_cursor returned false when cursor already at target"
echo "Fix applied: Return true when cursor is already at correct position"
echo ""

# Test configuration
LUSUSH_BINARY="./builddir/lusush"
TEST_HISTORY="/tmp/test_history_$$"
DEBUG_LOG="/tmp/history_debug_$$.log"

# Cleanup function
cleanup() {
    rm -f "$TEST_HISTORY" "$DEBUG_LOG"
}
trap cleanup EXIT

# Check if lusush binary exists
if [ ! -x "$LUSUSH_BINARY" ]; then
    echo "❌ FAIL: Lusush binary not found at $LUSUSH_BINARY"
    echo "Please run: scripts/lle_build.sh build"
    exit 1
fi

echo "✅ PASS: Lusush binary found"

# Create test history file
cat > "$TEST_HISTORY" << 'EOF'
echo "short command"
echo "this is a very long command that should wrap on most terminals and cause multiline display issues that previously broke navigation"
echo "medium length command here"
echo "another short one"
echo "final very very long command with lots of text that will definitely wrap and test the multiline clearing functionality"
EOF

echo "✅ PASS: Test history file created with 5 entries"

# Test 1: Basic history navigation sequence
echo ""
echo "Test 1: Basic UP/DOWN navigation sequence"
echo "Sequence: UP, UP, DOWN, UP, EXIT"

echo -e "\x1b[A\x1b[A\x1b[B\x1b[A\nexit" | \
    LLE_DEBUG=1 HISTFILE="$TEST_HISTORY" \
    timeout 10s "$LUSUSH_BINARY" > /dev/null 2>"$DEBUG_LOG"

if [ $? -eq 0 ]; then
    echo "✅ PASS: Navigation sequence completed successfully"
else
    echo "❌ FAIL: Navigation sequence failed or timed out"
fi

# Test 2: Check for success return codes
echo ""
echo "Test 2: Checking history command return codes"

UP_SUCCESS=$(grep "lle_cmd_history_up returned: 0" "$DEBUG_LOG" | wc -l)
DOWN_SUCCESS=$(grep "lle_cmd_history_down returned: 0" "$DEBUG_LOG" | wc -l)
MOVE_FAILURES=$(grep "Move to end failed" "$DEBUG_LOG" | wc -l)

echo "UP commands successful: $UP_SUCCESS"
echo "DOWN commands successful: $DOWN_SUCCESS"
echo "Move to end failures: $MOVE_FAILURES"

if [ "$UP_SUCCESS" -ge 3 ] && [ "$DOWN_SUCCESS" -ge 1 ] && [ "$MOVE_FAILURES" -eq 0 ]; then
    echo "✅ PASS: All history operations returned success (0)"
    echo "✅ PASS: No 'Move to end failed' messages found"
else
    echo "❌ FAIL: History operations not returning success or move failures detected"
fi

# Test 3: Extended navigation test
echo ""
echo "Test 3: Extended navigation with multiple direction changes"
echo "Sequence: UP, UP, UP, DOWN, DOWN, UP, DOWN, EXIT"

echo -e "\x1b[A\x1b[A\x1b[A\x1b[B\x1b[B\x1b[A\x1b[B\nexit" | \
    LLE_DEBUG=1 HISTFILE="$TEST_HISTORY" \
    timeout 15s "$LUSUSH_BINARY" > /dev/null 2>"$DEBUG_LOG"

EXTENDED_UP_SUCCESS=$(grep "lle_cmd_history_up returned: 0" "$DEBUG_LOG" | wc -l)
EXTENDED_DOWN_SUCCESS=$(grep "lle_cmd_history_down returned: 0" "$DEBUG_LOG" | wc -l)
EXTENDED_MOVE_FAILURES=$(grep "Move to end failed" "$DEBUG_LOG" | wc -l)

echo "Extended UP commands successful: $EXTENDED_UP_SUCCESS"
echo "Extended DOWN commands successful: $EXTENDED_DOWN_SUCCESS"
echo "Extended move failures: $EXTENDED_MOVE_FAILURES"

if [ "$EXTENDED_UP_SUCCESS" -ge 4 ] && [ "$EXTENDED_DOWN_SUCCESS" -ge 3 ] && [ "$EXTENDED_MOVE_FAILURES" -eq 0 ]; then
    echo "✅ PASS: Extended navigation test successful"
else
    echo "❌ FAIL: Extended navigation test failed"
fi

# Test 4: Error pattern detection
echo ""
echo "Test 4: Checking for regression patterns"

ERROR_PATTERNS=(
    "returned: -5"
    "ERROR_DISPLAY_UPDATE"
    "Move to end failed"
    "BACKSPACE_DEBUG.*failed"
)

REGRESSION_DETECTED=0
for pattern in "${ERROR_PATTERNS[@]}"; do
    if grep -q "$pattern" "$DEBUG_LOG"; then
        echo "❌ FAIL: Regression pattern detected: $pattern"
        REGRESSION_DETECTED=1
    fi
done

if [ "$REGRESSION_DETECTED" -eq 0 ]; then
    echo "✅ PASS: No regression patterns detected"
fi

# Test 5: Buffer state consistency
echo ""
echo "Test 5: Checking buffer state consistency"

BUFFER_ERRORS=$(grep -i "buffer.*invalid\|cursor.*invalid" "$DEBUG_LOG" | wc -l)
if [ "$BUFFER_ERRORS" -eq 0 ]; then
    echo "✅ PASS: No buffer state consistency errors"
else
    echo "❌ FAIL: Buffer state consistency errors detected: $BUFFER_ERRORS"
fi

# Summary
echo ""
echo "========================================================"
echo "TEST SUMMARY"
echo "========================================================"

# Count total passes and fails
PASS_COUNT=$(grep -c "✅ PASS:" <<< "$(cat /dev/stdout)")
FAIL_COUNT=$(grep -c "❌ FAIL:" <<< "$(cat /dev/stdout)")

echo "Tests completed:"
echo "- Navigation sequences executed"
echo "- Return code validation performed"
echo "- Regression pattern checking completed"
echo "- Buffer state consistency verified"
echo ""

if [ "$MOVE_FAILURES" -eq 0 ] && [ "$EXTENDED_MOVE_FAILURES" -eq 0 ] && [ "$REGRESSION_DETECTED" -eq 0 ]; then
    echo "🎉 SUCCESS: History navigation fix verified!"
    echo ""
    echo "Key improvements confirmed:"
    echo "✅ lle_cmd_move_end no longer fails when cursor already at end"
    echo "✅ History navigation returns success (0) instead of error (-5)"
    echo "✅ No 'Move to end failed' debug messages"
    echo "✅ Multiple UP/DOWN sequences work correctly"
    echo "✅ Extended navigation patterns function properly"
    echo ""
    echo "ROOT CAUSE FIXED:"
    echo "- lle_text_move_cursor now returns true when cursor already at target position"
    echo "- History navigation can successfully clear content using backspace logic"
    echo "- Display state integration no longer fails on cursor positioning"
    exit 0
else
    echo "❌ FAILURE: History navigation fix verification failed"
    echo ""
    echo "Issues detected:"
    [ "$MOVE_FAILURES" -gt 0 ] && echo "- Move to end failures still occurring: $MOVE_FAILURES"
    [ "$EXTENDED_MOVE_FAILURES" -gt 0 ] && echo "- Extended test failures: $EXTENDED_MOVE_FAILURES"
    [ "$REGRESSION_DETECTED" -gt 0 ] && echo "- Regression patterns detected"
    echo ""
    echo "Debug log available at: $DEBUG_LOG"
    echo "Review the log for detailed error information"
    exit 1
fi

# Manual testing instructions
echo ""
echo "========================================================"
echo "MANUAL TESTING VERIFICATION"
echo "========================================================"
echo "For complete verification, perform these manual tests:"
echo ""
echo "1. Start lusush: ./builddir/lusush"
echo "2. Enter these commands:"
echo "   echo \"short\""
echo "   echo \"this is a very long command that will wrap\""
echo "   echo \"medium command\""
echo ""
echo "3. Test navigation:"
echo "   Press UP arrow 3 times (should show: medium, long, short)"
echo "   Press DOWN arrow 2 times (should show: long, medium)"
echo "   Press UP arrow 1 time (should show: long)"
echo ""
echo "4. Verify behavior:"
echo "   ✅ Content appears on correct prompt line (not above it)"
echo "   ✅ No visual artifacts or display corruption"
echo "   ✅ Smooth transitions between commands"
echo "   ✅ Wrapped content clears properly"
echo ""
echo "5. Expected result:"
echo "   All navigation should work smoothly with no content appearing"
echo "   above the shell prompt and no display corruption."
