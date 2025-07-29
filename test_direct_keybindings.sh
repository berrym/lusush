#!/bin/bash

# Direct Terminal Operations Test for Lusush Line Editor
# Tests that critical keybindings work reliably using direct terminal operations

echo "=== Direct Terminal Operations Test ==="
echo
echo "This script tests the PRODUCTION-READY direct terminal operations approach:"
echo "  Ctrl+A: Move to beginning (direct terminal positioning)"
echo "  Ctrl+E: Move to end (direct terminal positioning)"
echo "  Ctrl+U: Clear line (direct terminal clearing)"
echo "  Ctrl+G: Cancel line (direct terminal clearing)"
echo "  Ctrl+R: DISABLED (prevents display corruption)"
echo
echo "Expected: ALL operations work immediately with visual feedback"
echo "Expected: NO display corruption or positioning errors"
echo "Expected: Cross-platform reliability (macOS/Linux)"
echo

# Create test environment
mkdir -p direct_ops_test
cd direct_ops_test

# Create simple history for context
cat > .lusush_history << 'EOF'
echo hello world
ls -la /tmp
cat /etc/passwd
find . -name "*.txt"
ps aux | grep ssh
EOF

echo "Created test history:"
cat -n .lusush_history
echo

echo "=== CRITICAL TEST INSTRUCTIONS ==="
echo
echo "🎯 PRIMARY TESTS (MUST WORK):"
echo
echo "TEST 1: Ctrl+A (Move to Beginning)"
echo "  1. Type: 'echo this is a long test command'"
echo "  2. Press Ctrl+A"
echo "  3. VERIFY: Cursor immediately moves to beginning (after prompt)"
echo "  4. RESULT: Should work instantly with visual feedback"
echo

echo "TEST 2: Ctrl+E (Move to End)"
echo "  1. From TEST 1, press Ctrl+E"
echo "  2. VERIFY: Cursor immediately moves to end of text"
echo "  3. RESULT: Should work instantly with visual feedback"
echo

echo "TEST 3: Ctrl+U (Clear Line)"
echo "  1. Type some text: 'this will be cleared'"
echo "  2. Press Ctrl+U"
echo "  3. VERIFY: Line is immediately cleared, cursor at prompt"
echo "  4. RESULT: Should clear instantly without artifacts"
echo

echo "TEST 4: Ctrl+G (Cancel Line)"
echo "  1. Type some text: 'this will be cancelled'"
echo "  2. Press Ctrl+G"
echo "  3. VERIFY: Line is immediately cleared, cursor at prompt"
echo "  4. RESULT: Should clear instantly without artifacts"
echo

echo "TEST 5: Ctrl+R (Disabled Protection)"
echo "  1. Press Ctrl+R"
echo "  2. VERIFY: Nothing happens OR shows message 'use up/down arrows'"
echo "  3. RESULT: Should NOT enter broken reverse search mode"
echo

echo "TEST 6: History Navigation (Alternative to Ctrl+R)"
echo "  1. Press Up Arrow"
echo "  2. VERIFY: Shows previous command from history"
echo "  3. Press Down Arrow"
echo "  4. VERIFY: Navigates through history normally"
echo "  5. RESULT: Should work as reliable Ctrl+R alternative"
echo

echo "=== CROSS-PLATFORM RELIABILITY CHECKS ==="
echo
echo "✅ EXPECTED BEHAVIORS:"
echo "  - Ctrl+A: Instant visual cursor movement to beginning"
echo "  - Ctrl+E: Instant visual cursor movement to end"
echo "  - Ctrl+U: Instant line clearing with no artifacts"
echo "  - Ctrl+G: Instant line clearing with no artifacts"
echo "  - All operations work in ANY terminal (iTerm2, Terminal, Linux)"
echo "  - NO display corruption or positioning errors"
echo "  - NO 'wildly all over the screen' behavior"
echo

echo "❌ FAILURE INDICATORS:"
echo "  - Cursor doesn't move visually"
echo "  - Display artifacts or corruption"
echo "  - Operations don't work consistently"
echo "  - Different behavior on different platforms"
echo "  - Any 'display chaos' or positioning errors"
echo

echo "=== PRODUCTION READINESS CRITERIA ==="
echo
echo "FOR SHELL TO BE PRODUCTION-READY, ALL MUST PASS:"
echo "  ✅ Ctrl+A works instantly and reliably"
echo "  ✅ Ctrl+E works instantly and reliably"
echo "  ✅ Ctrl+U works instantly and reliably"
echo "  ✅ Ctrl+G works instantly and reliably"
echo "  ✅ NO display corruption under any circumstances"
echo "  ✅ Identical behavior on macOS and Linux"
echo "  ✅ Works in all common terminals (iTerm2, Terminal, xterm, etc.)"
echo

echo "=== Starting Production Readiness Test ==="
echo "Terminal: $(tput cols)x$(tput lines)"
echo "Platform: $(uname -s)"
echo "Debug: ON"
echo

# Set environment
export LUSUSH_HISTORY=.lusush_history
export LLE_DEBUG=1

echo "🚀 LUSUSH STARTING - TEST ALL KEYBINDINGS ABOVE"
echo "Type 'exit' when finished testing"
echo

# Start lusush for testing
../builddir/lusush

# Cleanup
echo
echo "=== Test Results Analysis ==="
cd ..
rm -rf direct_ops_test

echo
echo "🎯 POST-TEST CHECKLIST:"
echo "Did Ctrl+A work instantly with visual feedback? [ Y / N ]"
echo "Did Ctrl+E work instantly with visual feedback? [ Y / N ]"
echo "Did Ctrl+U clear line instantly without artifacts? [ Y / N ]"
echo "Did Ctrl+G clear line instantly without artifacts? [ Y / N ]"
echo "Was there ANY display corruption or chaos? [ Y / N ]"
echo "Did behavior differ between test runs? [ Y / N ]"
echo
echo "✅ If ALL answers are Y/N/N/N/N/N: PRODUCTION READY"
echo "❌ If ANY other pattern: NEEDS MORE WORK"
echo
echo "Direct terminal operations test completed!"
