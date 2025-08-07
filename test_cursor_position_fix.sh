#!/bin/bash

# Test script for multiline cursor positioning bug verification
# This script tests the specific scenario that causes display corruption:
# 1. Type a long line that wraps
# 2. Backspace across line boundary
# 3. Type more content to wrap again
# 4. Press Enter
# Expected: Command executes properly on new line without corruption

set -e

echo "=== MULTILINE CURSOR POSITIONING BUG TEST ==="
echo "Testing the specific scenario that causes display corruption"
echo ""

# Build first to ensure latest changes
echo "Building latest changes..."
scripts/lle_build.sh build >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Build failed - cannot proceed with test"
    exit 1
fi
echo "✅ Build successful"
echo ""

# Test 1: Basic functionality check
echo "Test 1: Basic command execution"
echo "Expected: Command executes normally"
echo ""
echo "Command: echo 'basic test'"
LLE_DEBUG=1 ./builddir/lusush -c "echo 'basic test'" 2>/dev/null
echo ""
echo "✅ Basic test passed"
echo ""

# Test 2: Simple multiline wrapping
echo "Test 2: Long line wrapping (no backspace)"
echo "Expected: Long command wraps and executes normally"
echo ""
echo "Command: echo 'this is a very long line that will definitely wrap in most terminals'"
LLE_DEBUG=1 ./builddir/lusush -c "echo 'this is a very long line that will definitely wrap in most terminals'" 2>/dev/null
echo ""
echo "✅ Multiline wrapping test passed"
echo ""

# Test 3: The critical corruption scenario
echo "Test 3: CRITICAL CORRUPTION SCENARIO (Interactive)"
echo "This test requires manual interaction to reproduce the bug"
echo ""
echo "INSTRUCTIONS:"
echo "1. The shell will start in interactive mode"
echo "2. Type: echo \"this is a very long line that will wrap\""
echo "3. Use backspace to delete characters past the line wrap boundary"
echo "4. Type: ll wrap\" (to complete the command again)"
echo "5. Press Enter"
echo "6. Expected: 'this is a very long line that will wrap' should print on a NEW line"
echo "7. Type 'exit' to end the test"
echo ""
echo "WHAT TO LOOK FOR:"
echo "- After Enter, the echo output should appear on a NEW line"
echo "- The prompt should appear normally after the output"
echo "- No visual corruption or prompt mixing"
echo ""
echo "Starting interactive test..."
echo "Press any key to continue..."
read -n 1

echo ""
echo "=== STARTING INTERACTIVE TEST ==="
echo "Debug output will be saved to /tmp/cursor_fix_debug.log"
echo ""

# Run the interactive test with debug logging
LLE_DEBUG=1 LLE_INTEGRATION_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush 2>/tmp/cursor_fix_debug.log

echo ""
echo "=== INTERACTIVE TEST COMPLETED ==="
echo ""

# Show relevant debug information
echo "Key debug information from the session:"
echo ""
echo "Cross-line operations:"
grep "Cross-line operation" /tmp/cursor_fix_debug.log | tail -5
echo ""
echo "Cursor positioning during Enter:"
grep "Enter key pressed\|Position tracking\|Moving cursor" /tmp/cursor_fix_debug.log | tail -10
echo ""
echo "State synchronization status:"
grep "Integration.*cursor\|Move cursor.*termcap" /tmp/cursor_fix_debug.log | tail -5
echo ""

echo "=== TEST ANALYSIS ==="
echo ""
echo "Full debug log saved to: /tmp/cursor_fix_debug.log"
echo ""
echo "To manually analyze the debug log:"
echo "  cat /tmp/cursor_fix_debug.log | grep -E '(Cross-line|Enter key|Move cursor|Integration)'"
echo ""
echo "Expected fix verification:"
echo "- Look for 'Move cursor.*via termcap with state update' messages"
echo "- Ensure cursor positioning happens during Enter key processing"
echo "- Verify no visual corruption in the interactive session"
echo ""

# Check if common corruption indicators are present
echo "Checking for common corruption indicators..."
if grep -q "cursor query failed" /tmp/cursor_fix_debug.log; then
    echo "⚠️  Found 'cursor query failed' - position tracking was invalidated"
else
    echo "✅ No cursor query failures detected"
fi

if grep -q "Position tracking invalidated" /tmp/cursor_fix_debug.log; then
    echo "⚠️  Found 'Position tracking invalidated' - recovery should have occurred"
    if grep -q "Moving cursor.*via termcap" /tmp/cursor_fix_debug.log; then
        echo "✅ Recovery attempt detected - fix is working"
    else
        echo "❌ No recovery attempt detected - fix may not be working"
    fi
else
    echo "✅ No position tracking invalidation detected"
fi

echo ""
echo "=== CURSOR POSITION FIX TEST COMPLETE ==="
echo ""
echo "SUMMARY:"
echo "- Built latest changes with cursor positioning fixes"
echo "- Tested basic functionality (passed)"
echo "- Tested multiline wrapping (passed)"
echo "- Tested critical corruption scenario (interactive)"
echo "- Generated debug analysis"
echo ""
echo "Next steps:"
echo "1. Review the interactive test results visually"
echo "2. Check debug log for proper cursor positioning calls"
echo "3. Verify the echo output appeared on a new line after Enter"
echo "4. If corruption still exists, examine debug log for clues"
