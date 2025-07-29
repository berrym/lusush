#!/bin/bash

# Ctrl+R Keybinding Test Script for Lusush Line Editor
# Tests that Ctrl+A, Ctrl+E, Ctrl+U, and Ctrl+G work properly inside Ctrl+R reverse search mode

echo "=== Ctrl+R Keybinding Test for Lusush Line Editor ==="
echo
echo "This script tests that essential keybindings work correctly inside Ctrl+R mode:"
echo "  Ctrl+R: Enter reverse search mode"
echo "  Ctrl+A: Move to beginning of found text (in search mode)"
echo "  Ctrl+E: Move to end of found text (in search mode)"
echo "  Ctrl+U: Clear found text but stay in search mode"
echo "  Ctrl+G: Exit search mode and restore original line"
echo

# Create test directory and history
echo "Setting up test environment..."
mkdir -p ctrl_r_test_dir
cd ctrl_r_test_dir

# Create a test history file with various commands
cat > .lusush_history << 'EOF'
echo hello world
ls -la /tmp
cat /etc/passwd | grep root
find . -name "*.txt" -type f
grep -r "function" /usr/include
echo this is a very long command that will definitely wrap around the terminal
ps aux | grep ssh
history | tail -20
git status --porcelain
make clean && make all
EOF

echo "Created test history with 10 commands:"
cat -n .lusush_history
echo

echo "=== Test Instructions ==="
echo
echo "SETUP:"
echo "1. The shell will start with debug output enabled"
echo "2. History is preloaded with test commands"
echo "3. Follow the test steps below"
echo

echo "TEST 1: Basic Ctrl+R Functionality"
echo "  1. Press Ctrl+R"
echo "  2. Expected: Shows '(reverse-i-search)\`': ' prompt"
echo "  3. Type: 'echo'"
echo "  4. Expected: Shows matching command from history"
echo

echo "TEST 2: Ctrl+A in Reverse Search Mode"
echo "  1. From TEST 1, press Ctrl+A"
echo "  2. Expected: Cursor moves to beginning of found command text"
echo "  3. Verify: Cursor is at start of command (after search prompt)"
echo "  4. Result: Should work smoothly without breaking search mode"
echo

echo "TEST 3: Ctrl+E in Reverse Search Mode"
echo "  1. From TEST 2, press Ctrl+E"
echo "  2. Expected: Cursor moves to end of found command text"
echo "  3. Verify: Cursor is at end of the found command"
echo "  4. Result: Should work smoothly without breaking search mode"
echo

echo "TEST 4: Ctrl+U in Reverse Search Mode"
echo "  1. From TEST 3, press Ctrl+U"
echo "  2. Expected: Found text is cleared, but search mode continues"
echo "  3. Verify: Shows '(reverse-i-search)\`echo': ' with empty command area"
echo "  4. Result: Should stay in search mode with empty text"
echo

echo "TEST 5: Ctrl+G in Reverse Search Mode"
echo "  1. From any search state, press Ctrl+G"
echo "  2. Expected: Exits search mode completely"
echo "  3. Verify: Returns to normal prompt with original line restored"
echo "  4. Result: Should cleanly exit search and restore previous state"
echo

echo "TEST 6: Complex Search and Navigate"
echo "  1. Press Ctrl+R"
echo "  2. Type: 'grep'"
echo "  3. Press Ctrl+R again to find next match"
echo "  4. Use Ctrl+A and Ctrl+E to navigate within found text"
echo "  5. Press Enter to accept the command"
echo "  6. Expected: Command is executed normally"
echo

echo "=== Expected Behavior Summary ==="
echo "✅ Ctrl+R: Enters search mode with proper prompt"
echo "✅ Ctrl+A in search: Moves cursor to beginning of found text"
echo "✅ Ctrl+E in search: Moves cursor to end of found text"
echo "✅ Ctrl+U in search: Clears found text but stays in search mode"
echo "✅ Ctrl+G in search: Exits search and restores original line"
echo "✅ Enter in search: Accepts found command and executes it"
echo "✅ Escape in search: Exits search and restores original line"
echo

echo "=== Common Issues to Watch For ==="
echo "❌ Cursor doesn't move visually with Ctrl+A/Ctrl+E"
echo "❌ Ctrl+U exits search mode instead of clearing text"
echo "❌ Ctrl+G doesn't restore original line properly"
echo "❌ Search prompt gets corrupted or duplicated"
echo "❌ Cursor positioning is wrong after operations"
echo "❌ Display artifacts or garbled text"
echo

echo "=== Debug Information ==="
echo "Environment setup:"
echo "  LUSUSH_HISTORY=.lusush_history"
echo "  LLE_DEBUG=1 (enabled for detailed output)"
echo "  Terminal: $(tput cols)x$(tput lines)"
echo

# Export environment variables
export LUSUSH_HISTORY=.lusush_history
export LLE_DEBUG=1

echo "=== Starting Lusush with Ctrl+R Test Environment ==="
echo "History loaded, debug enabled, follow test instructions above"
echo "Type 'exit' when finished testing"
echo

# Start lusush for manual testing
../builddir/lusush

# Cleanup
echo
echo "=== Cleaning up ==="
cd ..
rm -rf ctrl_r_test_dir
echo "Ctrl+R keybinding test completed!"

echo
echo "=== Post-Test Analysis ==="
echo "After testing, verify these behaviors worked correctly:"
echo "1. Ctrl+A moved cursor to beginning of found text in search mode"
echo "2. Ctrl+E moved cursor to end of found text in search mode"
echo "3. Ctrl+U cleared found text but kept search mode active"
echo "4. Ctrl+G properly exited search mode and restored original line"
echo "5. Search display remained clean without artifacts"
echo "6. Cursor positioning was accurate throughout all operations"
echo
echo "If any of these failed, the fix needs refinement!"
