#!/bin/bash

# Test script to check history navigation behavior in LLE
# This script tests the current state of Up/Down arrow history navigation

echo "=== LLE History Navigation Test ==="
echo "Testing current behavior of Up/Down arrow keys"
echo ""

# Build the project first
echo "Building LLE..."
if ! scripts/lle_build.sh build > /dev/null 2>&1; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"
echo ""

# Create a test history file with known commands
TEST_HISTORY_FILE="/tmp/lle_test_history"
cat > "$TEST_HISTORY_FILE" << 'EOF'
ls -la
pwd
echo "hello world"
cat /etc/passwd
ps aux | grep bash
date
whoami
EOF

echo "📝 Created test history with commands:"
cat "$TEST_HISTORY_FILE" | sed 's/^/  - /'
echo ""

# Test with debug mode to see what's happening
echo "🔍 Testing history navigation with debug output..."
echo "Commands to test:"
echo "  1. Start lusush"
echo "  2. Press Up arrow multiple times"
echo "  3. Press Down arrow to navigate back"
echo "  4. Type 'exit' to quit"
echo ""

# Create a simple expect-like test using timeout
echo "Starting interactive test (will timeout after 10 seconds)..."
echo "Use Up/Down arrows to test history navigation"
echo ""

# Set up environment for testing
export LLE_DEBUG=1
export LLE_DEBUG_INPUT=1
export HISTFILE="$TEST_HISTORY_FILE"

# Launch lusush with a timeout
timeout 10s ./builddir/lusush 2>&1 | head -50 &
LUSUSH_PID=$!

# Give it a moment to start
sleep 1

# Check if lusush is still running
if kill -0 $LUSUSH_PID 2>/dev/null; then
    echo "✅ Lusush started successfully"
    echo "📋 Debug output should show key processing..."
    echo ""
    echo "Manual test instructions:"
    echo "1. Run: LLE_DEBUG=1 ./builddir/lusush"
    echo "2. Load history with: history -r $TEST_HISTORY_FILE"
    echo "3. Press Up arrow - should show 'whoami'"
    echo "4. Press Up again - should show 'date'"
    echo "5. Press Down - should go back to 'whoami'"
    echo "6. Type 'exit' to quit"

    # Wait for timeout
    wait $LUSUSH_PID 2>/dev/null
else
    echo "❌ Lusush failed to start"
    exit 1
fi

echo ""
echo "=== Analysis Required ==="
echo "1. Check if Up/Down arrows are detected as LLE_KEY_ARROW_UP/DOWN"
echo "2. Verify history navigation calls lle_history_navigate()"
echo "3. Confirm display updates after history selection"
echo "4. Look for any error messages in debug output"
echo ""

# Cleanup
rm -f "$TEST_HISTORY_FILE"

echo "Test complete. Check debug output above for navigation behavior."
