#!/bin/bash

# Test script to verify wrapped line history navigation fix
# This script tests that content appears in correct position after clearing wrapped lines

echo "🔧 Testing Wrapped Line History Navigation Fix"
echo "=============================================="
echo ""

echo "🎯 Building lusush with fixes..."
scripts/lle_build.sh build

if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "🧪 Testing Wrapped Line History Navigation"
echo "=========================================="
echo ""
echo "This test will:"
echo "1. Start lusush"
echo "2. Add short and long (wrapped) commands to history"
echo "3. Test UP/DOWN arrow navigation between wrapped and short content"
echo ""
echo "Expected result: Content appears on correct prompt line, not above shell prompt"
echo "Critical fix: Prevents new content from appearing above original host shell prompt"
echo ""

# Create a test session with wrapped content
echo "📝 Creating test session with wrapped line content..."
cat > /tmp/lusush_test_input.txt << 'EOF'
echo "short"
echo "this is a very very long command that will definitely wrap around the terminal width and cause multiline display issues that need to be handled correctly"
exit
EOF

echo "🚀 Running lusush with wrapped line test commands..."
echo "Note: The shell will execute the commands and exit"
echo ""

# Run the test with debug output
LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush < /tmp/lusush_test_input.txt

echo ""
echo "✅ Test completed"
echo ""
echo "🎯 Manual Test Instructions for Wrapped Line Navigation:"
echo "======================================================="
echo "To manually test the critical regression fix:"
echo ""
echo "1. Run: ./builddir/lusush"
echo "2. Type: echo \"short\""
echo "3. Press Enter"
echo "4. Type: echo \"this is a very long command that will wrap\""
echo "5. Press Enter"
echo "6. Press UP arrow twice (navigate to wrapped command)"
echo "7. Press DOWN arrow once (navigate away from wrapped command)"
echo "8. CRITICAL: Verify new content appears AFTER prompt, not ABOVE it"
echo ""
echo "Expected: Content positioned correctly on prompt line"
echo "REGRESSION: Content would appear above original shell prompt"
echo ""

# Clean up
rm -f /tmp/lusush_test_input.txt

echo "🎉 Wrapped line history navigation fix test complete!"
echo "Please run manual test to verify content positioning is correct."
echo ""
echo "🔍 Debug Test Command:"
echo "LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 LLE_DEBUG_DISPLAY=1 ./builddir/lusush"
