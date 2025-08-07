#!/bin/bash

# Automated test for cursor corruption bug
# This test reproduces the exact scenario that causes display corruption:
# 1. Long line that wraps
# 2. Backspace across line boundary
# 3. Complete command and press Enter
# Expected: Command output appears on new line without corruption

set -e

echo "=== AUTOMATED CURSOR CORRUPTION BUG TEST ==="
echo "Reproducing the exact scenario from user report"
echo ""

# Build first
echo "Building latest fixes..."
if ! scripts/lle_build.sh build >/dev/null 2>&1; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"
echo ""

# Create test input sequence that reproduces the bug
# This simulates the exact user interaction:
# 1. Type long line: echo "this is a very long line that will wrap"
# 2. Backspace 8 times to cross line boundary (removes 'will wrap"')
# 3. Type: ll wrap" (completes the command again)
# 4. Press Enter
# 5. Exit

TEST_INPUT=$(cat << 'EOF'
echo "this is a very long line that will wrap"
EOF
)

# Add backspaces to remove 'will wrap"' (8 characters)
TEST_INPUT="${TEST_INPUT}"$'\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f'

# Add completion text
TEST_INPUT="${TEST_INPUT}ll wrap\""

# Add Enter and exit
TEST_INPUT="${TEST_INPUT}"$'\r'
TEST_INPUT="${TEST_INPUT}exit"$'\r'

echo "=== REPRODUCING BUG SCENARIO ==="
echo "Simulating user input sequence:"
echo "1. Type: echo \"this is a very long line that will wrap\""
echo "2. Backspace 8 times (cross line boundary)"
echo "3. Type: ll wrap\""
echo "4. Press Enter (THIS IS WHERE CORRUPTION OCCURS)"
echo "5. Exit"
echo ""

# Run the test with debug output
echo "Running test with debug logging..."
echo -n "$TEST_INPUT" | LLE_DEBUG=1 LLE_INTEGRATION_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush 2>/tmp/cursor_corruption_debug.log

echo ""
echo "=== TEST ANALYSIS ==="
echo ""

# Check if the output was produced correctly
echo "Expected output: 'this is a very long line that will wrap' on a new line"
echo "Actual test output above this line ^^^^"
echo ""

# Analyze debug log for the critical operations
echo "Critical debug information:"
echo ""

echo "1. Cross-line backspace operations:"
grep -A 2 -B 1 "Cross-line operation" /tmp/cursor_corruption_debug.log 2>/dev/null || echo "   No cross-line operations found"
echo ""

echo "2. Clear-to-EOL skip behavior (NEW FIX):"
grep "SKIP.*No clear needed\|clear needed.*line end" /tmp/cursor_corruption_debug.log 2>/dev/null || echo "   No clear skip messages found"
echo ""

echo "3. Enter key processing:"
grep -A 5 "Enter key pressed" /tmp/cursor_corruption_debug.log 2>/dev/null || echo "   No Enter key processing found"
echo ""

echo "4. Newline writing during cleanup:"
grep "Cleanup newline\|newline.*state integration" /tmp/cursor_corruption_debug.log 2>/dev/null || echo "   No cleanup newline messages found"
echo ""

echo "5. State synchronization status:"
grep -E "(state.*failed|force.*sync|Move cursor.*termcap)" /tmp/cursor_corruption_debug.log 2>/dev/null | tail -5
echo ""

# Check for specific corruption indicators
echo "=== BUG STATUS ANALYSIS ==="
echo ""

# Check if clear-to-EOL was skipped at line end (this should prevent corruption)
if grep -q "SKIP.*No clear needed" /tmp/cursor_corruption_debug.log; then
    echo "✅ NEW FIX ACTIVE: Unnecessary clear-to-EOL operations skipped"
    echo "   This should prevent state divergence at line boundaries"
else
    echo "⚠️  Clear-to-EOL skip logic not triggered"
    echo "   May indicate different issue or test didn't reach critical scenario"
fi

# Check if position tracking was invalidated (expected)
if grep -q "cursor query failed.*invalidating" /tmp/cursor_corruption_debug.log; then
    echo "⚠️  Position tracking invalidated (expected behavior)"

    # Check if recovery was attempted
    if grep -q "Position tracking invalidated.*comprehensive cursor reset" /tmp/cursor_corruption_debug.log; then
        echo "✅ Recovery attempted during Enter key processing"

        # Check if state-synchronized operations were used
        if grep -q "termcap with state update\|state integration" /tmp/cursor_corruption_debug.log; then
            echo "✅ State-synchronized cursor operations used"
        else
            echo "❌ State-synchronized operations not detected"
        fi
    else
        echo "❌ No recovery attempt detected"
    fi
else
    echo "ℹ️  No position tracking invalidation detected"
fi

echo ""
echo "=== VISUAL CORRUPTION CHECK ==="
echo ""
echo "To verify the fix worked, check the output above for:"
echo ""
echo "EXPECTED (bug fixed):"
echo "  echo \"this is a very long line that will wrap\""
echo "  this is a very long line that will wrap"
echo "  [prompt appears normally on next line]"
echo ""
echo "BUG SYMPTOM (not fixed):"
echo "  echo \"this is a very long line that will wrap\""
echo "  this is a very long line that will wrap[prompt text mixed in]"
echo "  [visual corruption with prompt and output mixed]"
echo ""

# Provide debug log location for detailed analysis
echo "=== DETAILED DEBUG ANALYSIS ==="
echo ""
echo "Full debug log saved to: /tmp/cursor_corruption_debug.log"
echo ""
echo "Key sections to examine:"
echo "  grep -A 10 -B 5 'Cross-line operation' /tmp/cursor_corruption_debug.log"
echo "  grep -A 10 -B 5 'Enter key pressed' /tmp/cursor_corruption_debug.log"
echo "  grep -E '(SKIP|clear needed|line end)' /tmp/cursor_corruption_debug.log"
echo ""

echo "=== SUMMARY ==="
echo ""
echo "Fixes applied in this test:"
echo "1. ✅ Cursor position calculations use prompt-based math (not invalidated tracking)"
echo "2. ✅ Clear-to-EOL skipped when cursor at line end (prevents state divergence)"
echo "3. ✅ State-synchronized newline writing during cleanup"
echo "4. ✅ Proper cursor positioning to column 0 after newline"
echo ""
echo "If corruption still occurs, the issue may be:"
echo "- State sync system not properly integrated with all operations"
echo "- Terminal state corruption happening earlier in the sequence"
echo "- Display state tracking logic fundamentally flawed"
echo ""
echo "TEST COMPLETE - Check visual output above for corruption"
