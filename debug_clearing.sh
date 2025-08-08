#!/bin/bash

# Simple debug test for history clearing issue
echo "=== Debug History Clearing Test ==="
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
echo "🔍 Testing history clearing with debug output..."
echo ""
echo "Instructions:"
echo "1. Start lusush (will show debug output)"
echo "2. Type: echo \"short\""
echo "3. Press Enter"
echo "4. Type a long command that wraps"
echo "5. Press Enter"
echo "6. Press UP arrow twice to get to long command"
echo "7. Press DOWN arrow once to navigate away"
echo "8. Look for [DEBUG] messages to see what's happening"
echo "9. Type 'exit' to quit"
echo ""

# Create a history file with some test commands
cat > /tmp/test_history << 'EOF'
echo "short command"
echo "this is a very very long command that will definitely wrap around the terminal width and cause multiline display issues that need to be handled correctly by the clearing system"
ls -la
pwd
EOF

echo "📝 Test history created in /tmp/test_history"
echo ""
echo "🚀 Starting lusush with debug output..."
echo "Look for [DEBUG] messages to see clearing logic execution..."
echo ""

# Run with all debug flags
LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 LLE_DEBUG_DISPLAY=1 LLE_INTEGRATION_DEBUG=1 ./builddir/lusush
