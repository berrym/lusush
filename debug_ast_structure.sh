#!/bin/bash

# Debug script to analyze AST sibling structure for if statements
# This helps understand why execute_command_chain is traversing too many siblings

echo "=== AST SIBLING STRUCTURE DEBUG ANALYSIS ==="
echo "Shell: ./builddir/lusush"
echo

echo "The issue is that execute_command_chain traverses ALL siblings from a given node."
echo "In if-elif-else statements, this causes unwanted execution of subsequent clauses."
echo
echo "Let's trace through what should happen vs what actually happens:"
echo

# Test case: x=1; if [ $x -eq 1 ]; then echo one; else echo other; fi
echo "=== TEST CASE: Simple if-else ==="
echo "Command: x=1; if [ \$x -eq 1 ]; then echo one; else echo other; fi"
echo

echo "Expected AST sibling structure:"
echo "if_node:"
echo "  ├── condition: [ \$x -eq 1 ]"
echo "  ├── then_body: echo one"
echo "  └── else_body: echo other"
echo

echo "Current execution flow:"
echo "1. execute_if() called"
echo "2. condition ([ \$x -eq 1 ]) evaluates to 0 (success)"
echo "3. execute_command_chain() called on then_body"
echo "4. execute_command_chain() traverses ALL siblings starting from then_body"
echo "5. This includes then_body AND else_body"
echo "6. Result: both 'one' and 'other' are printed"
echo

echo "Actual output:"
echo 'x=1; if [ $x -eq 1 ]; then echo one; else echo other; fi' | ./builddir/lusush
echo

echo "=== ROOT CAUSE ANALYSIS ==="
echo "The problem is in execute_command_chain() in src/executor.c:"
echo "  while (current) {"
echo "      last_result = execute_node(executor, current);"
echo "      current = current->next_sibling;  // <-- This is the problem!"
echo "  }"
echo

echo "SOLUTIONS:"
echo "1. Change execute_if() to use execute_node() instead of execute_command_chain()"
echo "2. Or modify execute_command_chain() to have a single-node mode"
echo "3. Or restructure the AST so if-elif-else bodies don't have sibling relationships"
echo

echo "=== TESTING SOLUTION APPROACH ==="
echo "The fix should be in execute_if() to call execute_node() for individual bodies"
echo "instead of execute_command_chain() which traverses siblings."
echo

echo "Current problematic calls in execute_if():"
echo "  return execute_command_chain(executor, current);  // then body"
echo "  return execute_command_chain(executor, current->next_sibling);  // elif body"
echo "  return execute_command_chain(executor, current);  // else body"
echo

echo "These should be changed to:"
echo "  return execute_node(executor, current);  // then body"
echo "  return execute_node(executor, current->next_sibling);  // elif body"
echo "  return execute_node(executor, current);  // else body"
echo

echo "This will execute only the specific body node without traversing siblings."
