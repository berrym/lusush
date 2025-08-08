#!/bin/bash

# Test script to build actual history then test navigation with backspace clearing
echo "=== History Navigation Test with Actual History ==="
echo ""

# Build first
echo "Building lusush..."
scripts/lle_build.sh build > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "🧪 Testing History Navigation with Real History"
echo "=============================================="
echo ""

# Create a test history file with known commands
TEST_HISTORY_FILE="/tmp/lle_test_history"
cat > "$TEST_HISTORY_FILE" << 'EOF'
echo "short command"
echo "this is a very very long command that will definitely wrap around the terminal width and cause multiline display issues that need to be handled correctly by the backspace boundary logic system"
ls -la
pwd
date
EOF

echo "📝 Created test history with commands:"
cat "$TEST_HISTORY_FILE" | sed 's/^/  - /'
echo ""

# Create a test session that simulates building history then navigating
echo "🚀 Creating test session that builds history..."

# Create input that builds history first, then tests navigation
cat > /tmp/lusush_history_test_input.txt << 'EOF'
echo "command one"

echo "this is a super long command that wraps around multiple lines and should test the backspace boundary crossing logic properly when clearing during history navigation"

echo "command three"

exit
EOF

echo "📋 Test will:"
echo "1. Execute three commands to build history"
echo "2. Commands include short, long wrapped, and medium length"
echo "3. Exit to complete session"
echo ""

echo "🔍 Running test with debug to see backspace clearing..."
echo "Command: LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush < /tmp/lusush_history_test_input.txt"
echo ""

# Set history file and run test
export HISTFILE="$TEST_HISTORY_FILE"
LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush < /tmp/lusush_history_test_input.txt 2>&1 | head -100

echo ""
echo "=== Manual Test Instructions ==="
echo "To manually test with actual history:"
echo ""
echo "1. Run: HISTFILE=$TEST_HISTORY_FILE LLE_DEBUG=1 ./builddir/lusush"
echo "2. Type some commands to build history:"
echo "   echo \"short\""
echo "   echo \"very long command that wraps around terminal\""
echo "   echo \"medium command\""
echo "3. Press UP arrow to navigate through history"
echo "4. Press DOWN arrow to navigate back"
echo "5. Check if long content is properly cleared using backspace logic"
echo "6. Type 'exit' to quit"
echo ""

echo "🎯 Expected Behavior:"
echo "- History navigation should work through all commands"
echo "- Long wrapped content should be cleared using proven backspace logic"
echo "- Each navigation should show complete clearing and proper content display"
echo "- No wrapped content artifacts should remain"
echo ""

echo "📊 Key Debug Messages to Look For:"
echo "- [LLE_INTEGRATION] Backspace-based content replacement: SUCCESS"
echo "- [MATH_DEBUG] BACKSPACE: showing boundary crossing"
echo "- [LLE_INPUT_LOOP] lle_cmd_history_up/down returned: 0"
echo "- No display corruption or wrapped text artifacts"
echo ""

# Clean up
rm -f /tmp/lusush_history_test_input.txt
rm -f "$TEST_HISTORY_FILE"

echo "✅ Test complete. Run manual test to verify backspace clearing behavior."
