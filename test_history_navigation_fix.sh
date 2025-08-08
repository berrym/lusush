#!/bin/bash

# Test script to verify history navigation fix
# This script tests that lle_cmd_move_end no longer fails after first history operation

echo "Testing history navigation fix..."

# Create test history file
HISTORY_FILE="/tmp/test_history_$$"
cat > "$HISTORY_FILE" << EOF
echo "short command"
echo "this is a very long command that should wrap on most terminals and cause multiline display issues"
echo "another short one"
echo "medium length command here"
EOF

# Test with debug output
echo "Running history navigation test with debug output..."
echo -e "\\x1b[A\\x1b[A\\x1b[B\\x1b[A\\nexit" | \
    LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 HISTFILE="$HISTORY_FILE" \
    timeout 10s ./builddir/lusush 2>/tmp/debug_output.log

# Check debug output for the fix
echo "Checking debug output..."

if grep -q "Buffer cursor movement completed successfully" /tmp/debug_output.log; then
    echo "✅ PASS: Cursor movement debug messages found"
else
    echo "❌ FAIL: Expected cursor movement debug messages not found"
fi

if grep -q "Move to end failed" /tmp/debug_output.log; then
    echo "❌ FAIL: Still seeing 'Move to end failed' messages"
else
    echo "✅ PASS: No 'Move to end failed' messages found"
fi

# Check for successful history navigation
if grep -q "returned: 0" /tmp/debug_output.log; then
    echo "✅ PASS: History navigation returning success (0)"
else
    echo "❌ FAIL: History navigation not returning success"
fi

# Clean up
rm -f "$HISTORY_FILE" /tmp/debug_output.log

echo "Test completed. Manual verification recommended:"
echo "1. Start lusush: ./builddir/lusush"
echo "2. Enter some commands including a long one that wraps"
echo "3. Use UP/DOWN arrows to navigate history"
echo "4. Verify no content appears above the shell prompt"
