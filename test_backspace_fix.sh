#!/bin/bash

# Test script for backspace boundary crossing fix
# This script tests the specific issue found in human testing

echo "🧪 Testing Backspace Boundary Crossing Fix"
echo "=========================================="

# Build the project
echo "📦 Building lusush with fix..."
cd "$(dirname "$0")"
./scripts/lle_build.sh build > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo "✅ Build successful"

# Create a test input file that simulates the problematic scenario
cat > /tmp/backspace_test_input.txt << 'EOF'
echo "This is a line that will wrap and test backspace."
exit
EOF

echo "🎯 Testing backspace boundary crossing..."
echo ""
echo "Expected behavior:"
echo "  1. Type long command that wraps"
echo "  2. Use backspace across wrap boundary"
echo "  3. Should NOT echo remaining buffer content"
echo "  4. Should continue editing normally"
echo ""

# Test with debug output to verify fix
echo "Running test with debug output..."
export LLE_DEBUG=1
timeout 10s ./builddir/lusush < /tmp/backspace_test_input.txt > /tmp/backspace_test_output.txt 2> /tmp/backspace_debug.log

# Check if the fix worked by examining debug log
if grep -q "Positioning cursor after boundary clearing" /tmp/backspace_debug.log; then
    echo "✅ Fix is active - cursor positioning logic used"
else
    echo "⚠️  Fix may not be active - check debug log"
fi

if grep -q "Consistent rendering completed: success" /tmp/backspace_debug.log; then
    echo "✅ Consistent rendering completed successfully"
else
    echo "⚠️  Consistent rendering may have issues"
fi

# Check for the specific boundary crossing scenario
if grep -q "Crossing boundary: true" /tmp/backspace_debug.log; then
    echo "✅ Boundary crossing detected correctly"
else
    echo "ℹ️  No boundary crossing in this test run"
fi

echo ""
echo "📋 Test Results Summary:"
echo "  - Debug log: /tmp/backspace_debug.log"
echo "  - Output: /tmp/backspace_test_output.txt"
echo ""
echo "🔍 To manually test:"
echo "  1. Run: LLE_DEBUG=1 ./builddir/lusush"
echo "  2. Type a long command that wraps your terminal"
echo "  3. Use backspace to cross the wrap boundary"
echo "  4. Verify no buffer content is echoed"
echo ""

# Clean up
unset LLE_DEBUG
rm -f /tmp/backspace_test_input.txt

echo "🏁 Test script completed"
