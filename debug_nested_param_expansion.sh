#!/bin/bash

# Debug script for nested parameter expansion issues
# Testing ${VAR:+prefix_${VAR}_suffix} and ${A:+${A}_${B}} patterns

echo "=== NESTED PARAMETER EXPANSION DEBUG ==="
echo "Shell: ./builddir/lusush"
echo

# Test 1: Simple parameter expansion (baseline)
echo "Test 1: Simple parameter expansion"
echo 'VAR=test; echo $VAR' | ./builddir/lusush
echo "Expected: test"
echo

# Test 2: Simple braced expansion (baseline)
echo "Test 2: Simple braced expansion"
echo 'VAR=test; echo ${VAR}' | ./builddir/lusush
echo "Expected: test"
echo

# Test 3: Simple alternative expansion (baseline)
echo "Test 3: Simple alternative expansion"
echo 'VAR=test; echo ${VAR:+simple}' | ./builddir/lusush
echo "Expected: simple"
echo

# Test 4: Alternative with literal text (baseline)
echo "Test 4: Alternative with literal text"
echo 'VAR=test; echo ${VAR:+prefix_literal_suffix}' | ./builddir/lusush
echo "Expected: prefix_literal_suffix"
echo

# Test 5: The failing nested expansion case
echo "Test 5: FAILING - Nested expansion in alternative"
echo 'VAR=test; echo ${VAR:+prefix_${VAR}_suffix}' | ./builddir/lusush
echo "Expected: prefix_test_suffix"
echo "Actual result above - if empty, this is the bug"
echo

# Test 6: The other failing nested expansion case
echo "Test 6: FAILING - Nested braced expansions"
echo 'A=hello; B=world; echo ${A:+${A}_${B}}' | ./builddir/lusush
echo "Expected: hello_world"
echo "Actual result above - if empty, this is the bug"
echo

# Test 7: Debugging - step by step
echo "Test 7: Step by step debugging"
echo "7a: Variable values"
echo 'VAR=test; echo "VAR=$VAR"' | ./builddir/lusush
echo

echo "7b: Alternative without nesting"
echo 'VAR=test; echo ${VAR:+found}' | ./builddir/lusush
echo

echo "7c: Variable expansion outside braces"
echo 'VAR=test; echo prefix_${VAR}_suffix' | ./builddir/lusush
echo

echo "7d: The problematic combination"
echo 'VAR=test; echo ${VAR:+prefix_${VAR}_suffix}' | ./builddir/lusush
echo

# Test 8: Different nesting patterns
echo "Test 8: Different nesting patterns"
echo "8a: Single nested variable"
echo 'A=hello; echo ${A:+${A}}' | ./builddir/lusush
echo "Expected: hello"
echo

echo "8b: Two separate nested variables"
echo 'A=hello; B=world; echo ${A:+${A}} ${B:+${B}}' | ./builddir/lusush
echo "Expected: hello world"
echo

echo "8c: Two nested in same alternative"
echo 'A=hello; B=world; echo ${A:+${A}_${B}}' | ./builddir/lusush
echo "Expected: hello_world"
echo

echo "=== ANALYSIS ==="
echo "If tests 5 and 6 produce empty output, the issue is in parameter expansion"
echo "recursion. The expand_variables_in_string function may not properly handle"
echo "nested ${} expressions within alternative values."
echo
echo "The problem likely occurs in parse_parameter_expansion when it calls"
echo "expand_variables_in_string on the alternative value containing nested ${}"
echo
echo "Root cause hypothesis:"
echo "1. ${VAR:+prefix_\${VAR}_suffix} is parsed correctly"
echo "2. VAR is found to be non-empty, so alternative should be used"
echo "3. expand_variables_in_string is called on 'prefix_\${VAR}_suffix'"
echo "4. But the nested \${VAR} expansion fails or returns empty"
echo
echo "Next step: Add debug prints to expand_variables_in_string and"
echo "parse_parameter_expansion to trace the recursion"
