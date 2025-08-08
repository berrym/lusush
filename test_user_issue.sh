#!/bin/bash

# Test script to simulate the exact user issue with history navigation
echo "=== User Issue Simulation Test ==="
echo ""

# Build first
echo "Building..."
scripts/lle_build.sh build > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "🔍 Simulating user's exact test scenario..."
echo ""
echo "This test will simulate:"
echo "1. Starting lusush"
echo "2. User creates history by running commands"
echo "3. User navigates with UP/DOWN arrows"
echo "4. Check if long content gets cleared properly"
echo ""

# Create test input that simulates user interaction
cat > /tmp/lusush_user_test.txt << 'EOF'
echo "short"

echo "this is a very very long command that will definitely wrap around the terminal width and cause multiline display issues that need to be handled correctly"

EOF

echo "📝 Created test input simulating user commands"
echo ""
echo "🚀 Running test with debug output..."
echo "Look for [HISTORY_DEBUG] and [DEBUG] messages..."
echo ""

# Run the test with all debug flags
echo "Command: LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 LLE_DEBUG_DISPLAY=1 LLE_INTEGRATION_DEBUG=1 ./builddir/lusush < /tmp/lusush_user_test.txt"
echo ""

LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 LLE_DEBUG_DISPLAY=1 LLE_INTEGRATION_DEBUG=1 ./builddir/lusush < /tmp/lusush_user_test.txt 2>&1 | head -200

echo ""
echo "=== Analysis ==="
echo "Look for these debug messages:"
echo "- [HISTORY_DEBUG] messages showing history navigation attempts"
echo "- [DEBUG] messages from replace_content showing clearing logic"
echo "- Check if state_integration is NULL (fallback path used)"
echo "- Verify if history entries are being found"
echo ""

# Clean up
rm -f /tmp/lusush_user_test.txt

echo "🎯 Manual Test Instructions:"
echo "To reproduce the exact user issue manually:"
echo ""
echo "1. Run: LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 LLE_DEBUG_DISPLAY=1 LLE_INTEGRATION_DEBUG=1 ./builddir/lusush"
echo "2. Type: echo \"short\""
echo "3. Press Enter"
echo "4. Type: echo \"this is a very very long command that will definitely wrap around the terminal width\""
echo "5. Press Enter"
echo "6. Press UP arrow twice (to get to long command)"
echo "7. Press DOWN arrow once (to navigate away)"
echo "8. Check if long content is still visible (USER'S ISSUE)"
echo "9. Type 'exit' to quit"
echo ""
echo "Expected: Long content should be completely cleared"
echo "User Issue: Long content remains visible on screen"
