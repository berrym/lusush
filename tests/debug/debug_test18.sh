#!/bin/bash

# Debug script for Test 18 - Command substitution with $()

SHELL_PATH="./builddir/lusush"

echo "=== Debug Test 18: Command substitution with \$() ==="

# Test case from test_posix_regression.sh
TEST_INPUT="echo \"Today: \$(date '+%Y')\""
EXPECTED_PATTERN="Today: 20"

echo "Input: $TEST_INPUT"
echo "Expected pattern: $EXPECTED_PATTERN"
echo ""

# Execute the command
echo "Executing command..."
output=$($SHELL_PATH <<< "$TEST_INPUT" 2>&1)
exit_code=$?

echo "Exit code: $exit_code"
echo "Raw output: '$output'"
echo "Output length: ${#output}"
echo ""

# Check pattern matching
if echo "$output" | grep -q "$EXPECTED_PATTERN"; then
    echo "✓ Pattern match: SUCCESS"
else
    echo "✗ Pattern match: FAILED"
fi

# Show hex dump of output
echo ""
echo "Hex dump of output:"
echo "$output" | hexdump -C

# Test variations
echo ""
echo "=== Testing variations ==="

echo "1. Direct date command:"
date '+%Y'

echo ""
echo "2. Command substitution without quotes:"
$SHELL_PATH <<< "echo Today: \$(date '+%Y')"

echo ""
echo "3. Simpler command substitution:"
$SHELL_PATH <<< "echo \"Files: \$(echo test)\""

echo ""
echo "4. Test pattern matching manually:"
test_output="Today: 2025"
if echo "$test_output" | grep -q "$EXPECTED_PATTERN"; then
    echo "Manual pattern test: SUCCESS"
else
    echo "Manual pattern test: FAILED"
fi
