#!/bin/bash

# Critical Keybinding Test Script for Lusush Line Editor
# Tests the 5 most important keybindings: Ctrl+A, Ctrl+E, Ctrl+U, Ctrl+G, Ctrl+R

echo "=== Critical Keybinding Test for Lusush Line Editor ==="
echo
echo "This script will test the 5 most important keybindings:"
echo "  Ctrl+A - Move to beginning of line"
echo "  Ctrl+E - Move to end of line"
echo "  Ctrl+U - Clear entire line"
echo "  Ctrl+G - Cancel/abort current line"
echo "  Ctrl+R - Reverse search through history"
echo

# Create test directory and history
echo "Setting up test environment..."
mkdir -p keybinding_test_dir
cd keybinding_test_dir

# Add some commands to history for Ctrl+R testing
echo "echo 'hello world'" > .lusush_history
echo "ls -la" >> .lusush_history
echo "cat /etc/passwd" >> .lusush_history
echo "grep test file.txt" >> .lusush_history
echo "find . -name '*.txt'" >> .lusush_history

echo "Created test history with 5 commands"
echo

echo "=== Manual Testing Instructions ==="
echo
echo "TEST 1: Ctrl+A (Move to Beginning)"
echo "  1. Type: 'echo this is a test'"
echo "  2. Press Ctrl+A"
echo "  3. Expected: Cursor moves to beginning of line"
echo "  4. Verify: Cursor is before 'echo'"
echo

echo "TEST 2: Ctrl+E (Move to End)"
echo "  1. From previous test, press Ctrl+E"
echo "  2. Expected: Cursor moves to end of line"
echo "  3. Verify: Cursor is after 'test'"
echo

echo "TEST 3: Ctrl+U (Clear Line)"
echo "  1. With text in line, press Ctrl+U"
echo "  2. Expected: Entire line is cleared"
echo "  3. Verify: Line is empty, cursor at beginning"
echo

echo "TEST 4: Ctrl+G (Cancel Line)"
echo "  1. Type some text: 'this will be cancelled'"
echo "  2. Press Ctrl+G"
echo "  3. Expected: Line is cleared (same as Ctrl+U in many shells)"
echo "  4. Verify: Line is empty"
echo

echo "TEST 5: Ctrl+R (Reverse Search)"
echo "  1. Press Ctrl+R"
echo "  2. Expected: Shows '(reverse-i-search)\`': ' prompt"
echo "  3. Type: 'echo'"
echo "  4. Expected: Shows matching history entry"
echo "  5. Press Ctrl+R again to cycle through matches"
echo "  6. Press Enter to select, or Escape to cancel"
echo

echo "=== Starting Lusush with Debug Output ==="
echo "LUSUSH_HISTORY=.lusush_history will be used for history"
echo "Debug output enabled to show keybinding processing"
echo

# Export environment for testing
export LUSUSH_HISTORY=.lusush_history
export LLE_DEBUG=1

# Function to test specific keybinding with expect if available
test_keybinding() {
    local test_name="$1"
    local test_input="$2"
    local expected="$3"

    echo "=== $test_name ==="
    echo "Input: $test_input"
    echo "Expected: $expected"
    echo
}

# If expect is available, run automated tests
if command -v expect >/dev/null 2>&1; then
    echo "Expect available - running automated tests..."

    expect << 'EOF'
set timeout 10
spawn ../builddir/lusush
expect "$ "

# Test Ctrl+A
send "echo test line"
sleep 0.5
send "\001"
expect "$ "
send "\r"
expect "$ "

# Test Ctrl+E
send "echo another test"
sleep 0.5
send "\001"
sleep 0.5
send "\005"
expect "$ "
send "\r"
expect "$ "

# Test Ctrl+U
send "this will be cleared"
sleep 0.5
send "\025"
expect "$ "

# Test Ctrl+G
send "this will be cancelled"
sleep 0.5
send "\007"
expect "$ "

# Test Ctrl+R
send "\022"
expect "(reverse-i-search)"
send "echo"
sleep 0.5
send "\r"
expect "$ "

send "exit\r"
expect eof
EOF

    echo "Automated tests completed!"
else
    echo "Expect not available - starting manual testing session..."
    echo "Follow the test instructions above"
    echo "Type 'exit' to quit when testing is complete"
    echo

    # Start lusush with history
    ../builddir/lusush
fi

# Cleanup
echo
echo "=== Cleaning up ==="
cd ..
rm -rf keybinding_test_dir
echo "Keybinding test completed!"

echo
echo "=== Expected Results Summary ==="
echo "✅ Ctrl+A: Should move cursor to beginning immediately"
echo "✅ Ctrl+E: Should move cursor to end immediately"
echo "✅ Ctrl+U: Should clear entire line immediately"
echo "✅ Ctrl+G: Should clear/cancel line immediately"
echo "🔧 Ctrl+R: Should enter reverse search mode and allow navigation"
echo
echo "If any keybinding doesn't work as expected, it needs fixing!"
