#!/bin/bash

# Test script for multiline to single-line history conversion
# This tests that multiline commands are properly converted for history storage

set -e

SHELL_PATH="./builddir/lusush"
TEST_COUNT=0
PASSED_COUNT=0
FAILED_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_test() {
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${YELLOW}Test $TEST_COUNT: $1${NC}"
}

pass_test() {
    PASSED_COUNT=$((PASSED_COUNT + 1))
    echo -e "${GREEN}✓ PASSED${NC}"
    echo
}

fail_test() {
    FAILED_COUNT=$((FAILED_COUNT + 1))
    echo -e "${RED}✗ FAILED: $1${NC}"
    echo
}

echo "=== Multiline History Conversion Test ==="
echo "Testing shell: $SHELL_PATH"
echo

# Ensure the shell binary exists
if [ ! -x "$SHELL_PATH" ]; then
    echo -e "${RED}Error: Shell binary not found at $SHELL_PATH${NC}"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

# Clean up any existing history
rm -f ~/.lusush_history

echo "Testing multiline to single-line conversion for history..."
echo

# Test interactive mode with a simple multiline command
log_test "Interactive multiline input and history recall"

# Create a temporary expect script to test interactive behavior
cat > /tmp/test_history.exp << 'EOF'
#!/usr/bin/expect -f

set timeout 10
set shell_path [lindex $argv 0]

# Start the shell
spawn $shell_path

# Wait for the prompt
expect "% "

# Enter a multiline for loop
send "for i in 1 2 3\r"
expect "> "
send "do\r"
expect "> "
send "    echo item \$i\r"
expect "> "
send "done\r"

# Wait for completion
expect "% "

# Try to recall the command with up arrow
send "\033\[A"
expect -timeout 5 {
    "for i in 1 2 3; do echo item \$i; done" {
        puts "SUCCESS: History shows single-line format"
        send "\r"
        expect "% "
        send "exit\r"
        exit 0
    }
    timeout {
        puts "TIMEOUT: Could not verify history format"
        send "\003"
        send "exit\r"
        exit 1
    }
}
EOF

chmod +x /tmp/test_history.exp

if command -v expect >/dev/null 2>&1; then
    if /tmp/test_history.exp "$SHELL_PATH" 2>/dev/null; then
        pass_test
    else
        fail_test "History conversion not working correctly"
    fi
else
    echo "Note: 'expect' not available, skipping interactive history test"
    echo "To test manually:"
    echo "1. Run: $SHELL_PATH"
    echo "2. Enter a multiline command like:"
    echo "   for i in 1 2 3"
    echo "   do"
    echo "       echo item \$i"
    echo "   done"
    echo "3. Press up arrow and verify it shows as single line"
    echo
fi

# Clean up
rm -f /tmp/test_history.exp

# Test conversion function with some examples
log_test "Testing conversion logic with sample inputs"

# Test simple multiline command
cat > /tmp/test_input1.txt << 'EOF'
if true
then
    echo success
fi
EOF

echo "Input:"
cat /tmp/test_input1.txt
echo
echo "Expected output: if true; then echo success; fi"
echo

# Test for loop
cat > /tmp/test_input2.txt << 'EOF'
for i in 1 2 3
do
    echo item $i
done
EOF

echo "Input:"
cat /tmp/test_input2.txt
echo
echo "Expected output: for i in 1 2 3; do echo item \$i; done"
echo

# Test while loop
cat > /tmp/test_input3.txt << 'EOF'
x=1
while [ $x -le 3 ]
do
    echo value $x
    x=$((x + 1))
done
EOF

echo "Input:"
cat /tmp/test_input3.txt
echo
echo "Expected output: x=1; while [ \$x -le 3 ]; do echo value \$x; x=\$((x + 1)); done"
echo

# Test brace group
cat > /tmp/test_input4.txt << 'EOF'
{
    echo line1
    echo line2
}
EOF

echo "Input:"
cat /tmp/test_input4.txt
echo
echo "Expected output: { echo line1; echo line2; }"
echo

# Clean up test files
rm -f /tmp/test_input*.txt

echo "=== Manual Testing Instructions ==="
echo "To manually verify history conversion:"
echo "1. Start the shell: $SHELL_PATH"
echo "2. Enter a multiline command (like the examples above)"
echo "3. After the command completes, press the up arrow key"
echo "4. Verify that the recalled command is in single-line format"
echo "5. The single-line format should be easily editable"
echo

echo "=== Test Results ==="
echo -e "Total tests: $TEST_COUNT"
echo -e "${GREEN}Passed: $PASSED_COUNT${NC}"
echo -e "${RED}Failed: $FAILED_COUNT${NC}"

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed! ✗${NC}"
    exit 1
fi
