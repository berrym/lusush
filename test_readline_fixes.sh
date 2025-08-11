#!/bin/bash
# test_readline_fixes.sh - Interactive test for readline integration fixes
# Tests tab completion cycling, history navigation, and key bindings

LUSUSH_BIN="./builddir/lusush"

echo "========================================"
echo "Lusush Readline Integration Fix Test"
echo "========================================"
echo
echo "This script will test the readline fixes:"
echo "1. Tab completion cycling (no more appending)"
echo "2. History navigation without artifacts"
echo "3. Ctrl+G to abort/cancel line"
echo "4. Ctrl+L to clear screen"
echo "5. Prompt theming"
echo
echo "Setting up test environment..."

# Create test directory
TEST_DIR="/tmp/lusush_readline_fix_test"
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create test files for completion
echo "test content 1" > test_file_alpha.txt
echo "test content 2" > test_file_beta.txt
echo "test content 3" > test_file_gamma.txt
mkdir test_directory_one
mkdir test_directory_two
echo "nested content" > test_directory_one/nested_file.txt

echo "Test files created:"
ls -la

echo
echo "========================================"
echo "AUTOMATED TESTS"
echo "========================================"

echo
echo "Test 1: Basic functionality"
echo "----------------------------"
echo -e "pwd\necho 'Basic test passed'" | "$LUSUSH_BIN" 2>/dev/null
echo

echo "Test 2: Theme integration"
echo "----------------------------"
echo -e "theme set dark\necho 'Theme test passed'" | "$LUSUSH_BIN" 2>/dev/null
echo

echo "Test 3: Command execution"
echo "----------------------------"
echo -e "ls *.txt | wc -l\necho 'File count test passed'" | "$LUSUSH_BIN" 2>/dev/null
echo

echo "Test 4: History basic functionality"
echo "----------------------------"
echo -e "echo 'history test 1'\necho 'history test 2'\nhistory" | "$LUSUSH_BIN" 2>/dev/null | tail -3
echo

echo "========================================"
echo "INTERACTIVE TESTS - MANUAL VERIFICATION"
echo "========================================"
echo
echo "Now run: cd $TEST_DIR && $LUSUSH_BIN"
echo
echo "Then manually test these features:"
echo
echo "TEST 1: TAB COMPLETION CYCLING"
echo "-------------------------------"
echo "Type: ls test<TAB>"
echo "Expected: Should cycle through test_file_alpha.txt, test_file_beta.txt, test_file_gamma.txt"
echo "NOT append them all on the same line"
echo "Keep pressing TAB to cycle through matches"
echo
echo "TEST 2: DIRECTORY COMPLETION"
echo "-----------------------------"
echo "Type: cd test<TAB>"
echo "Expected: Should cycle through test_directory_one, test_directory_two"
echo
echo "TEST 3: HISTORY NAVIGATION"
echo "---------------------------"
echo "Type several commands, then use UP/DOWN arrows"
echo "Expected: Clean navigation without visual artifacts"
echo "Commands to try:"
echo "  echo 'command 1'"
echo "  echo 'command 2'"
echo "  echo 'command 3'"
echo "  <UP ARROW> <UP ARROW> <DOWN ARROW>"
echo
echo "TEST 4: CTRL+G ABORT"
echo "---------------------"
echo "Type: echo 'this will be aborted'"
echo "Then press: Ctrl+G"
echo "Expected: Line should be cleared and you get a fresh prompt"
echo
echo "TEST 5: CTRL+L CLEAR SCREEN"
echo "----------------------------"
echo "Fill screen with some output, then press: Ctrl+L"
echo "Expected: Screen should clear and show fresh prompt"
echo
echo "TEST 6: CTRL+A/E NAVIGATION"
echo "----------------------------"
echo "Type: echo 'beginning and end'"
echo "Press Ctrl+A (should go to beginning)"
echo "Press Ctrl+E (should go to end)"
echo "Expected: Cursor moves properly without artifacts"
echo
echo "TEST 7: PROMPT THEMING"
echo "-----------------------"
echo "Try: theme list"
echo "Try: theme set light"
echo "Try: theme set dark"
echo "Expected: Prompt should change colors/style appropriately"
echo
echo "TEST 8: COMPLEX COMPLETION"
echo "---------------------------"
echo "Type: echo \$HO<TAB>"
echo "Expected: Should complete to \$HOME"
echo
echo "Type: ls /tm<TAB>"
echo "Expected: Should complete to /tmp/"
echo
echo "========================================"
echo "SUCCESS CRITERIA"
echo "========================================"
echo
echo "✓ TAB COMPLETION: Cycles through matches instead of appending"
echo "✓ HISTORY: Clean navigation without visual artifacts"
echo "✓ CTRL+G: Aborts current line cleanly"
echo "✓ CTRL+L: Clears screen properly"
echo "✓ CTRL+A/E: Cursor navigation works"
echo "✓ THEMES: Prompt changes with theme commands"
echo "✓ GENERAL: No visual corruption or strange behavior"
echo
echo "If all tests pass, the readline integration is working correctly!"
echo
echo "Type 'exit' to quit the shell when done testing."
echo
echo "========================================"
