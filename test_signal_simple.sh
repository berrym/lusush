#!/bin/bash

# Simple Signal Test for Lusush with Built-in Debug
# Tests Ctrl+C behavior using lusush's built-in debug command

echo "=== Simple Signal Test for Lusush ==="
echo "Testing Ctrl+C behavior with internal debugging"
echo ""

# Ensure lusush is built
if [ ! -f builddir/lusush ]; then
    echo "❌ lusush binary not found. Building..."
    ninja -C builddir
    exit 1
fi

echo "This test will:"
echo "1. Start lusush with internal signal debugging enabled"
echo "2. Test Ctrl+C behavior"
echo "3. Show debug output to diagnose the issue"
echo ""

echo "Creating test input that enables debugging and tests signals..."

# Create input that enables debugging and tests signal behavior
cat > /tmp/signal_test_input.txt << 'EOF'
debug signal on
echo "Signal debugging enabled"
echo "Type Ctrl+C after this message appears..."
echo "This line should be interrupted by Ctrl+C"
exit
EOF

echo "Test input created. Starting lusush..."
echo "Look for [SIGNAL_DEBUG] messages in the output"
echo ""

# Run the test with the input
script -q -c './builddir/lusush' /dev/null < /tmp/signal_test_input.txt

echo ""
echo "=== Test Completed ==="
echo ""

echo "Analysis of the output above:"
echo ""
echo "If you see [SIGNAL_DEBUG] messages:"
echo "  ✅ Signal debugging is working"
echo "  🔍 Check the signal handler behavior details"
echo ""

echo "If you DON'T see [SIGNAL_DEBUG] messages:"
echo "  ❌ Signal debugging may not be enabled"
echo "  🔍 Check debug command implementation"
echo ""

echo "Next step: Manual interactive testing required"
echo "Run: script -q -c './builddir/lusush' /dev/null"
echo "Then: debug signal on"
echo "Then: Test Ctrl+C behavior manually"

# Cleanup
rm -f /tmp/signal_test_input.txt

echo ""
echo "Test script completed: $(date)"
