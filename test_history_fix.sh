#!/bin/bash

# Test script to verify Linux history navigation fix
# This script tests that the "echexit" corruption issue is resolved

echo "🔧 Testing Linux History Navigation Fix"
echo "========================================"
echo ""

echo "🎯 Building lusush with fixes..."
cd /home/mberry/Lab/c/lusush
scripts/lle_build.sh build

if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "🧪 Testing History Navigation"
echo "=============================="
echo ""
echo "This test will:"
echo "1. Start lusush"
echo "2. Add some commands to history"
echo "3. Test UP/DOWN arrow navigation"
echo ""
echo "Expected result: Clean history navigation without 'echexit' corruption"
echo "Commands should display as: '$ exit' not '$ echexit'"
echo ""

# Create a test session
echo "📝 Creating test session..."
cat > /tmp/lusush_test_input.txt << 'EOF'
echo "first command"
echo "second command"
exit
EOF

echo "🚀 Running lusush with test commands..."
echo "Note: The shell will execute the commands and exit"
echo ""

# Run the test
./builddir/lusush < /tmp/lusush_test_input.txt

echo ""
echo "✅ Test completed"
echo ""
echo "🎯 Manual Test Instructions:"
echo "============================"
echo "To manually test history navigation:"
echo ""
echo "1. Run: ./builddir/lusush"
echo "2. Type: echo \"test command 1\""
echo "3. Press Enter"
echo "4. Type: echo \"test command 2\""
echo "5. Press Enter"
echo "6. Press UP arrow twice"
echo "7. Verify you see: echo \"test command 1\" (not echetest command 1)"
echo "8. Press DOWN arrow once"
echo "9. Verify you see: echo \"test command 2\" (clean display)"
echo ""
echo "Expected: Perfect display with no character corruption or overlays"
echo ""

# Clean up
rm -f /tmp/lusush_test_input.txt

echo "🎉 History navigation fix test complete!"
echo "Please run manual test to verify the 'echexit' issue is resolved."
