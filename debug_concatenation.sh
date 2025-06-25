#!/bin/bash

# =============================================================================
# DEBUG VARIABLE CONCATENATION TOKENIZATION
# =============================================================================
#
# This script tests how the shell tokenizes and processes concatenated variables
# to understand why $a$b produces "1 2" instead of "12"
#
# We'll test various concatenation patterns to isolate the issue
# =============================================================================

SHELL_UNDER_TEST="./builddir/lusush"

echo "=== VARIABLE CONCATENATION DEBUG ==="
echo

# Test 1: Simple variables with space
echo "Test 1: Variables with explicit space"
echo "Command: a=1; b=2; echo \$a \$b"
echo "Expected: 1 2"
echo -n "Actual:   "
echo 'a=1; b=2; echo $a $b' | $SHELL_UNDER_TEST
echo

# Test 2: Concatenated variables (the problem case)
echo "Test 2: Concatenated variables"
echo "Command: a=1; b=2; echo \$a\$b"
echo "Expected: 12"
echo -n "Actual:   "
echo 'a=1; b=2; echo $a$b' | $SHELL_UNDER_TEST
echo

# Test 3: Variables with literal text
echo "Test 3: Variables with literal text between"
echo "Command: a=1; b=2; echo \$a-\$b"
echo "Expected: 1-2"
echo -n "Actual:   "
echo 'a=1; b=2; echo $a-$b' | $SHELL_UNDER_TEST
echo

# Test 4: Variables in double quotes
echo "Test 4: Variables in double quotes"
echo "Command: a=1; b=2; echo \"\$a\$b\""
echo "Expected: 12"
echo -n "Actual:   "
echo 'a=1; b=2; echo "$a$b"' | $SHELL_UNDER_TEST
echo

# Test 5: Mixed concatenation
echo "Test 5: Mixed concatenation with text"
echo "Command: a=hello; b=world; echo prefix\$a\$b"
echo "Expected: prefixhelloworld"
echo -n "Actual:   "
echo 'a=hello; b=world; echo prefix$a$b' | $SHELL_UNDER_TEST
echo

# Test 6: Three variables
echo "Test 6: Three concatenated variables"
echo "Command: a=1; b=2; c=3; echo \$a\$b\$c"
echo "Expected: 123"
echo -n "Actual:   "
echo 'a=1; b=2; c=3; echo $a$b$c' | $SHELL_UNDER_TEST
echo

# Test 7: Variables with curly braces
echo "Test 7: Variables with curly braces"
echo "Command: a=1; b=2; echo \${a}\${b}"
echo "Expected: 12"
echo -n "Actual:   "
echo 'a=1; b=2; echo ${a}${b}' | $SHELL_UNDER_TEST
echo

echo "=== ANALYSIS ==="
echo "This test helps determine if the issue is:"
echo "1. Tokenization: How consecutive tokens are recognized"
echo "2. Parsing: How tokens are combined into arguments"
echo "3. Execution: How arguments are processed"
echo
echo "If Test 4 (double quotes) works correctly, the issue is in"
echo "tokenization/parsing of unquoted consecutive variables."
