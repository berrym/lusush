#!/bin/bash

# =============================================================================
# SIMPLE NESTED PARAMETER EXPANSION DEBUG
# =============================================================================
#
# This script creates the simplest possible test case to understand
# what's happening with nested parameter expansion parsing.
#
# We'll test step by step to isolate the exact issue.
# =============================================================================

SHELL_UNDER_TEST="./builddir/lusush"

echo "=== STEP-BY-STEP NESTED PARAMETER EXPANSION DEBUG ==="
echo

# Test 1: Basic variable expansion (should work)
echo "Test 1: Basic variable expansion"
echo "Command: TEST=hello; echo \${TEST}"
echo "Expected: hello"
echo -n "Actual:   "
echo 'TEST=hello; echo ${TEST}' | $SHELL_UNDER_TEST
echo

# Test 2: Basic alternative expansion (should work)
echo "Test 2: Basic alternative expansion"
echo "Command: TEST=hello; echo \"\${TEST:+world}\""
echo "Expected: world"
echo -n "Actual:   "
echo 'TEST=hello; echo "${TEST:+world}"' | $SHELL_UNDER_TEST
echo

# Test 3: Alternative with literal text
echo "Test 3: Alternative with literal text"
echo "Command: TEST=hello; echo \"\${TEST:+prefix_suffix}\""
echo "Expected: prefix_suffix"
echo -n "Actual:   "
echo 'TEST=hello; echo "${TEST:+prefix_suffix}"' | $SHELL_UNDER_TEST
echo

# Test 4: Simple concatenation outside expansion
echo "Test 4: Variable concatenation outside expansion"
echo "Command: A=hello; echo \"prefix_\${A}_suffix\""
echo "Expected: prefix_hello_suffix"
echo -n "Actual:   "
echo 'A=hello; echo "prefix_${A}_suffix"' | $SHELL_UNDER_TEST
echo

# Test 5: The problematic nested case - minimal
echo "Test 5: Minimal nested case"
echo "Command: A=hello; echo \"\${A:+\${A}}\""
echo "Expected: hello"
echo -n "Actual:   "
echo 'A=hello; echo "${A:+${A}}"' | $SHELL_UNDER_TEST
echo

# Test 6: The full problematic case
echo "Test 6: Full nested case"
echo "Command: TEST=value; echo \"\${TEST:+prefix_\${TEST}_suffix}\""
echo "Expected: prefix_value_suffix"
echo -n "Actual:   "
echo 'TEST=value; echo "${TEST:+prefix_${TEST}_suffix}"' | $SHELL_UNDER_TEST
echo

echo "=== DEBUG ANALYSIS ==="
echo "This will help us understand exactly where the parsing breaks down."
echo "Each test builds complexity step by step."
