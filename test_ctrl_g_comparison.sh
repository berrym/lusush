#!/bin/bash

# Test to compare Ctrl+G (abort line) vs Down Arrow (clear line) behavior
# This test verifies that down arrow clearing works the same as Ctrl+G

set -e

LUSUSH_BINARY="./builddir/lusush"
HISTORY_FILE="$HOME/.lusush_history"

echo "=== CTRL+G vs DOWN ARROW COMPARISON TEST ==="
echo ""

# Check if lusush binary exists
if [ ! -f "$LUSUSH_BINARY" ]; then
    echo "ERROR: Lusush binary not found at $LUSUSH_BINARY"
    echo "Please run: meson setup builddir --wipe && ninja -C builddir"
    exit 1
fi

# Setup simple test history
echo "Setting up test history..."
rm -f "$HISTORY_FILE"
cat > "$HISTORY_FILE" << 'EOF'
echo "test command"
exit
EOF

echo "Test history:"
cat "$HISTORY_FILE" | nl
echo ""

echo "=== TEST 1: Ctrl+G Behavior (Known Working) ==="
echo "Testing: UP arrow → Ctrl+G → ENTER"
echo ""

# Test Ctrl+G: \033[A (up arrow) + \007 (Ctrl+G) + \n (enter)
ctrl_g_output=$(printf '\033[A\007\necho "ctrl+g test successful"\nexit\n' | timeout 5s "$LUSUSH_BINARY" -i 2>&1)

echo "Ctrl+G test output:"
echo "$ctrl_g_output"
echo ""

if echo "$ctrl_g_output" | grep -q "ctrl+g test successful"; then
    echo "✅ Ctrl+G WORKS: Line was cleared, new command executed successfully"
    ctrl_g_works=true
else
    echo "❌ Ctrl+G FAILED: Line was not cleared properly"
    ctrl_g_works=false
fi

if echo "$ctrl_g_output" | grep -q "test command"; then
    echo "❌ Ctrl+G FAILED: Previous command was executed instead of cleared"
    ctrl_g_works=false
fi

echo ""
echo "=== TEST 2: Down Arrow Behavior (Under Test) ==="
echo "Testing: UP arrow → DOWN arrow → ENTER"
echo ""

# Test Down Arrow: \033[A (up arrow) + \033[B (down arrow) + \n (enter)
down_arrow_output=$(printf '\033[A\033[B\necho "down arrow test successful"\nexit\n' | timeout 5s "$LUSUSH_BINARY" -i 2>&1)

echo "Down arrow test output:"
echo "$down_arrow_output"
echo ""

if echo "$down_arrow_output" | grep -q "down arrow test successful"; then
    echo "✅ DOWN ARROW WORKS: Line was cleared, new command executed successfully"
    down_arrow_works=true
else
    echo "❌ DOWN ARROW FAILED: Line was not cleared properly"
    down_arrow_works=false
fi

if echo "$down_arrow_output" | grep -q "test command"; then
    echo "❌ DOWN ARROW FAILED: Previous command was executed instead of cleared"
    down_arrow_works=false
fi

echo ""
echo "=== COMPARISON ANALYSIS ==="
echo ""

if [ "$ctrl_g_works" = true ] && [ "$down_arrow_works" = true ]; then
    echo "🎉 PERFECT: Both Ctrl+G and Down Arrow work correctly!"
    echo "   - Ctrl+G clears line properly ✅"
    echo "   - Down Arrow clears line properly ✅"
    echo "   - Both allow new commands to execute ✅"
    echo ""
    echo "SUCCESS: Down arrow now uses the same methodology as Ctrl+G"

elif [ "$ctrl_g_works" = true ] && [ "$down_arrow_works" = false ]; then
    echo "⚠️  ISSUE: Ctrl+G works but Down Arrow doesn't"
    echo "   - Ctrl+G clears line properly ✅"
    echo "   - Down Arrow fails to clear line ❌"
    echo ""
    echo "RECOMMENDATION: Down arrow implementation needs to match Ctrl+G exactly"

elif [ "$ctrl_g_works" = false ] && [ "$down_arrow_works" = true ]; then
    echo "🤔 UNEXPECTED: Down Arrow works but Ctrl+G doesn't"
    echo "   - Ctrl+G fails to clear line ❌"
    echo "   - Down Arrow clears line properly ✅"
    echo ""
    echo "NOTE: This is unexpected - Ctrl+G should be the reference implementation"

else
    echo "❌ CRITICAL: Neither Ctrl+G nor Down Arrow work properly"
    echo "   - Ctrl+G fails to clear line ❌"
    echo "   - Down Arrow fails to clear line ❌"
    echo ""
    echo "CRITICAL: Basic line clearing functionality is broken"
fi

echo ""
echo "=== VISUAL INSPECTION TEST ==="
echo ""
echo "For manual verification, run: $LUSUSH_BINARY -i"
echo ""
echo "Test Ctrl+G:"
echo "1. Press UP arrow (shows 'echo \"test command\"')"
echo "2. Press Ctrl+G (should clear line visually)"
echo "3. Type 'hello' and press ENTER (should work normally)"
echo ""
echo "Test Down Arrow:"
echo "1. Press UP arrow (shows 'echo \"test command\"')"
echo "2. Press DOWN arrow (should clear line visually - SAME as Ctrl+G)"
echo "3. Type 'world' and press ENTER (should work normally)"
echo ""
echo "Both should behave identically!"

# Exit with appropriate code
if [ "$ctrl_g_works" = true ] && [ "$down_arrow_works" = true ]; then
    echo ""
    echo "🎉 ALL TESTS PASSED - Down arrow behavior matches Ctrl+G!"
    exit 0
else
    echo ""
    echo "❌ TESTS FAILED - Down arrow behavior differs from Ctrl+G"
    exit 1
fi
