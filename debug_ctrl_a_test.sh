#!/bin/bash

# Debug script to test Ctrl+A cursor movement with detailed logging
# This script will help identify why visual cursor movement isn't working

echo "🔍 Debug Test: Ctrl+A Cursor Movement"
echo "===================================="
echo ""

# Build first
echo "Building lusush..."
if ! scripts/lle_build.sh build > /dev/null 2>&1; then
    echo "❌ Build failed!"
    exit 1
fi
echo "✅ Build successful"
echo ""

# Create a test input sequence that simulates:
# 1. Type "hello world"
# 2. Press Ctrl+A (ASCII 1)
# 3. Type "START:" to verify cursor is at beginning
# 4. Exit

echo "📝 Creating test input sequence..."
cat > /tmp/test_input.txt << 'EOF'
hello world
START:
exit
EOF

echo "🧪 Test sequence:"
echo "  1. Type 'hello world'"
echo "  2. Press Ctrl+A (should move cursor to beginning)"
echo "  3. Type 'START:' (should appear at beginning if cursor moved)"
echo "  4. Exit"
echo ""

echo "Expected result if working:"
echo "  START:hello world"
echo ""

echo "Expected result if broken:"
echo "  hello worldSTART:"
echo ""

# Test with debug output
echo "🚨 Running test with debug output..."
echo "Debug output will be saved to /tmp/debug_ctrl_a.log"
echo ""

# Use printf with \x01 to send Ctrl+A
printf "hello world\x01START:\nexit\n" | LLE_DEBUG=1 ./builddir/lusush > /tmp/test_output.txt 2> /tmp/debug_ctrl_a.log

echo "📊 Test Results:"
echo "==============="

# Show the actual output
echo "Actual output:"
cat /tmp/test_output.txt
echo ""

# Check if START: appears at the beginning (success) or end (failure)
if grep -q "START:hello world" /tmp/test_output.txt; then
    echo "✅ SUCCESS: Ctrl+A worked! Cursor moved to beginning."
elif grep -q "hello worldSTART:" /tmp/test_output.txt; then
    echo "❌ FAILED: Ctrl+A didn't work. Cursor stayed at end."
else
    echo "🤔 UNCLEAR: Unexpected output format."
fi

echo ""
echo "🔍 Debug Analysis:"
echo "=================="

# Check if our debug messages appear
if grep -q "LLE_MOVE_HOME" /tmp/debug_ctrl_a.log; then
    echo "✅ Ctrl+A was detected and processed"

    if grep -q "Display update cursor result: SUCCESS" /tmp/debug_ctrl_a.log; then
        echo "✅ Cursor update reported success"
    else
        echo "❌ Cursor update failed or didn't complete"
    fi

    if grep -q "LLE_CURSOR_UPDATE.*Cursor positioned successfully" /tmp/debug_ctrl_a.log; then
        echo "✅ Terminal cursor positioning succeeded"
    else
        echo "❌ Terminal cursor positioning failed"
    fi

    if grep -q "LLE_DISPLAY_INCREMENTAL.*Positioning cursor after text update" /tmp/debug_ctrl_a.log; then
        echo "✅ Incremental update is positioning cursor"
    else
        echo "❌ Incremental update is NOT positioning cursor"
    fi

else
    echo "❌ Ctrl+A was not detected or processed"
fi

echo ""
echo "📋 Debug Log Summary:"
echo "===================="
echo "Key debug messages from /tmp/debug_ctrl_a.log:"
echo ""

# Show relevant debug lines
grep -E "(LLE_MOVE_HOME|LLE_CURSOR_UPDATE|LLE_DISPLAY_INCREMENTAL|LLE_PROMPT_POSITION)" /tmp/debug_ctrl_a.log | head -20

echo ""
echo "📁 Full debug log available at: /tmp/debug_ctrl_a.log"
echo "📁 Test output available at: /tmp/test_output.txt"
echo ""

if grep -q "START:hello world" /tmp/test_output.txt; then
    echo "🎉 DIAGNOSIS: Ctrl+A is working correctly!"
    echo "The visual cursor movement issue may be resolved."
else
    echo "🔧 DIAGNOSIS: Ctrl+A is still not working."
    echo "Further investigation needed:"
    echo "  1. Check cursor position calculations"
    echo "  2. Verify terminal cursor movement functions"
    echo "  3. Look for timing issues or conflicts"
fi
