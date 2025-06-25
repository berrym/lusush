#!/bin/bash

# =============================================================================
# DEBUG NESTED PARAMETER EXPANSION TEST
# =============================================================================
#
# This script reproduces the specific nested parameter expansion bug mentioned
# in the handoff reference: ${TEST:+prefix_${TEST}_suffix}
#
# Expected behavior: prefix_value_suffix
# Current behavior:  prefix__suffix}
#
# This will help isolate and fix the parsing issue in parse_parameter_expansion
# =============================================================================

SHELL_UNDER_TEST="./builddir/lusush"

echo "=== NESTED PARAMETER EXPANSION BUG REPRODUCTION ==="
echo

# Test Case 1: Basic nested expansion that should work
echo "Test 1: Basic nested parameter expansion"
echo "Command: TEST=value; echo \"\${TEST:+prefix_\${TEST}_suffix}\""
echo "Expected: prefix_value_suffix"
echo -n "Actual:   "
echo 'TEST=value; echo "${TEST:+prefix_${TEST}_suffix}"' | $SHELL_UNDER_TEST
echo

# Test Case 2: Simpler nested case
echo "Test 2: Simple nested case"
echo "Command: A=hello; echo \"\${A:+\${A}_world}\""
echo "Expected: hello_world"
echo -n "Actual:   "
echo 'A=hello; echo "${A:+${A}_world}"' | $SHELL_UNDER_TEST
echo

# Test Case 3: Empty variable case
echo "Test 3: Empty variable (should produce empty string)"
echo "Command: unset TEST; echo \"\${TEST:+prefix_\${TEST}_suffix}\""
echo "Expected: (empty)"
echo -n "Actual:   "
echo 'unset TEST; echo "${TEST:+prefix_${TEST}_suffix}"' | $SHELL_UNDER_TEST
echo

# Test Case 4: Variable concatenation issue
echo "Test 4: Variable concatenation spacing"
echo "Command: a=1; b=2; echo \$a\$b"
echo "Expected: 12"
echo -n "Actual:   "
echo 'a=1; b=2; echo $a$b' | $SHELL_UNDER_TEST
echo

# Test Case 5: Division by zero
echo "Test 5: Division by zero handling"
echo "Command: echo \$((5 / 0))"
echo "Expected: Error or non-zero exit"
echo -n "Actual:   "
echo 'echo $((5 / 0))' | $SHELL_UNDER_TEST
echo "Exit code: $?"
echo

echo "=== DEBUG COMPLETE ==="
echo
echo "This script helps identify the exact behavior of the known issues"
echo "mentioned in the handoff reference for targeted fixes."
