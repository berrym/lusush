#!/bin/bash

# Minimal test for nested parameter expansion debugging
# Focus on the exact failing case to understand the issue

echo "=== MINIMAL NESTED PARAMETER EXPANSION DEBUG ==="
echo "Shell: ./builddir/lusush"
echo

# The exact failing test case
echo "Test: VAR=test; echo \${VAR:+prefix_\${VAR}_suffix}"
echo "Expected: prefix_test_suffix"
echo "Actual:"
echo 'VAR=test; echo ${VAR:+prefix_${VAR}_suffix}' | ./builddir/lusush
echo

# Test components step by step
echo "=== COMPONENT BREAKDOWN ==="

echo "1. Variable assignment works:"
echo 'VAR=test; echo "VAR is: $VAR"' | ./builddir/lusush

echo "2. Simple alternative works:"
echo 'VAR=test; echo ${VAR:+alternative}' | ./builddir/lusush

echo "3. Nested expansion outside alternative works:"
echo 'VAR=test; echo prefix_${VAR}_suffix' | ./builddir/lusush

echo "4. The combination fails:"
echo 'VAR=test; echo ${VAR:+prefix_${VAR}_suffix}' | ./builddir/lusush

echo "=== HYPOTHESIS ==="
echo "The issue is in expand_variables_in_string when processing the alternative value"
echo "It should expand 'prefix_\${VAR}_suffix' but the nested \${VAR} fails"
echo ""
echo "Likely causes:"
echo "1. Incorrect brace matching in nested context"
echo "2. Recursion limit or infinite loop prevention"
echo "3. String parsing issue with the alternative value"
