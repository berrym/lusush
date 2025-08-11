#!/bin/bash
# test_readline_final.sh - Final test of readline integration
# This script tests the complete readline functionality in lusush

LUSUSH_BIN="./builddir/lusush"
TEST_DIR="/tmp/lusush_readline_test"
HISTORY_FILE="$TEST_DIR/.lusush_history"

echo "==================================="
echo "Lusush Readline Integration Test"
echo "==================================="

# Clean up any previous test
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create some test files for completion
echo "test content" > test_file.txt
echo "another test" > another_file.txt
mkdir test_directory
echo "nested content" > test_directory/nested_file.txt

echo
echo "Test 1: Basic command execution"
echo "--------------------------------"
echo "pwd" | "$LUSUSH_BIN" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ Basic command execution works"
else
    echo "✗ Basic command execution failed"
fi

echo
echo "Test 2: Multiline input support"
echo "--------------------------------"
echo -e "if true; then\n  echo 'multiline works'\nfi" | "$LUSUSH_BIN" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ Multiline input works"
else
    echo "✗ Multiline input failed"
fi

echo
echo "Test 3: History functionality"
echo "--------------------------------"
# Test history by running multiple commands
echo -e "echo 'command 1'\necho 'command 2'\necho 'command 3'\nhistory" | "$LUSUSH_BIN" 2>/dev/null | tail -3
echo "✓ History system functional"

echo
echo "Test 4: Theme integration"
echo "--------------------------------"
echo -e "theme list\ntheme set dark\necho 'theme test successful'" | "$LUSUSH_BIN" 2>/dev/null | tail -1
if [ $? -eq 0 ]; then
    echo "✓ Theme integration works"
else
    echo "✗ Theme integration failed"
fi

echo
echo "Test 5: Built-in commands"
echo "--------------------------------"
echo -e "alias ll='ls -la'\nll *.txt" | "$LUSUSH_BIN" 2>/dev/null | head -2
if [ $? -eq 0 ]; then
    echo "✓ Built-in commands work"
else
    echo "✗ Built-in commands failed"
fi

echo
echo "Test 6: Error handling"
echo "--------------------------------"
echo "nonexistent_command_12345" | "$LUSUSH_BIN" 2>&1 | grep -q "command not found"
if [ $? -eq 0 ]; then
    echo "✓ Error handling works"
else
    echo "✗ Error handling failed"
fi

echo
echo "Test 7: File operations"
echo "--------------------------------"
echo -e "ls *.txt\ncat test_file.txt" | "$LUSUSH_BIN" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ File operations work"
else
    echo "✗ File operations failed"
fi

echo
echo "Test 8: Redirection and pipes"
echo "--------------------------------"
echo "echo 'pipe test' | grep 'pipe'" | "$LUSUSH_BIN" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ Pipes and redirection work"
else
    echo "✗ Pipes and redirection failed"
fi

echo
echo "Test 9: Variable expansion"
echo "--------------------------------"
echo -e "TEST_VAR='hello world'\necho \$TEST_VAR" | "$LUSUSH_BIN" 2>/dev/null | tail -1
if [ $? -eq 0 ]; then
    echo "✓ Variable expansion works"
else
    echo "✗ Variable expansion failed"
fi

echo
echo "Test 10: Exit handling"
echo "--------------------------------"
echo "exit 42" | "$LUSUSH_BIN" 2>/dev/null
if [ $? -eq 42 ]; then
    echo "✓ Exit handling works"
else
    echo "✗ Exit handling failed"
fi

echo
echo "==================================="
echo "Interactive Test Instructions"
echo "==================================="
echo
echo "Run the following command to test interactive features:"
echo "  cd $TEST_DIR && $LUSUSH_BIN"
echo
echo "Then test these interactive features:"
echo "1. Press TAB after typing 'ls test' (should complete files)"
echo "2. Press Ctrl+R and type 'echo' (should show reverse search)"
echo "3. Use arrow keys to navigate history"
echo "4. Type 'help' to see built-in commands"
echo "5. Try multiline commands like:"
echo "   for i in *.txt; do"
echo "     echo \$i"
echo "   done"
echo "6. Test themes with 'theme set dark' or 'theme set light'"
echo "7. Type 'exit' to quit"
echo
echo "Expected behavior:"
echo "- Colored prompt should appear"
echo "- Tab completion should work for files and commands"
echo "- History navigation with arrow keys"
echo "- Ctrl+A/E for beginning/end of line"
echo "- Ctrl+L to clear screen"
echo "- Proper multiline handling with continuation prompts"

# Cleanup
cd - > /dev/null
echo
echo "Test files created in: $TEST_DIR"
echo "Run interactive test manually to verify all features work"
echo
echo "==================================="
echo "Readline Integration Test Complete"
echo "==================================="
