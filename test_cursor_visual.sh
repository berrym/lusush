#!/bin/bash

# Visual Cursor Movement Test Script
# Tests if Ctrl+A and Ctrl+E actually move the cursor visually

echo "🧪 Visual Cursor Movement Test"
echo "=============================="
echo ""

# Build the project
echo "Building lusush..."
if ! scripts/lle_build.sh build > /dev/null 2>&1; then
    echo "❌ Build failed!"
    exit 1
fi
echo "✅ Build successful"
echo ""

echo "📋 MANUAL TEST INSTRUCTIONS:"
echo "1. This will start the lusush shell"
echo "2. Type some text: 'hello world'"
echo "3. Press Ctrl+A - cursor should jump to beginning"
echo "4. Press Ctrl+E - cursor should jump to end"
echo "5. Press Ctrl+U - line should clear"
echo "6. Type 'exit' to quit"
echo ""

echo "🎯 Expected behavior:"
echo "  - Ctrl+A: Cursor visually moves to start (before 'h')"
echo "  - Ctrl+E: Cursor visually moves to end (after 'd')"
echo "  - Ctrl+U: Line clears immediately"
echo ""

echo "🚨 If cursor doesn't move visually, the bug still exists!"
echo ""

read -p "Press Enter to start the test..."

echo ""
echo "Starting lusush (type 'exit' to quit)..."
echo "========================================="

# Run lusush interactively
./builddir/lusush

echo ""
echo "Test completed. Did the cursor move visually? (y/n)"
read -r response

if [[ "$response" =~ ^[Yy]$ ]]; then
    echo "✅ SUCCESS: Visual cursor movement is working!"
    exit 0
else
    echo "❌ FAILED: Visual cursor movement still not working"
    echo ""
    echo "Debug suggestions:"
    echo "1. Run with debug: LLE_DEBUG=1 ./builddir/lusush"
    echo "2. Check if lle_terminal_move_cursor() is being called"
    echo "3. Verify cursor position calculations are correct"
    exit 1
fi
