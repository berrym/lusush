#!/bin/bash

# Debug script to analyze elif-else AST structure
# This helps understand how the parser builds the AST for if-elif-else statements

echo "=== ELIF-ELSE AST DEBUG ANALYSIS ==="
echo "Shell: ./builddir/lusush"
echo

# Test 1: Simple if-else (working baseline)
echo "Test 1: Simple if-else"
echo 'x=1; if [ $x -eq 1 ]; then echo one; else echo other; fi' | ./builddir/lusush
echo "Expected: one"
echo

# Test 2: Simple if-elif (no else)
echo "Test 2: Simple if-elif (no else)"
echo 'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; fi' | ./builddir/lusush
echo "Expected: two"
echo

# Test 3: The problematic if-elif-else
echo "Test 3: Problematic if-elif-else"
echo 'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi' | ./builddir/lusush
echo "Expected: two"
echo "Current result shows both 'two' and 'other' - this is the bug"
echo

# Test 4: Multiple elif with else
echo "Test 4: Multiple elif with else"
echo 'x=3; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; elif [ $x -eq 3 ]; then echo three; else echo other; fi' | ./builddir/lusush
echo "Expected: three"
echo

# Test 5: All conditions false, should hit else
echo "Test 5: All conditions false, should hit else only"
echo 'x=99; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi' | ./builddir/lusush
echo "Expected: other"
echo

# Test 6: Multiple elif, all false, should hit else
echo "Test 6: Multiple elif, all false, should hit else only"
echo 'x=99; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; elif [ $x -eq 3 ]; then echo three; else echo other; fi' | ./builddir/lusush
echo "Expected: other"
echo

echo "=== ANALYSIS ==="
echo "The issue appears to be in the execute_if function in src/executor.c"
echo "The logic for handling elif-else chains may be incorrectly structured"
echo "When an elif condition succeeds, it should execute the elif body and NOT continue to else"
echo
echo "Key insight: The AST structure and execution logic needs careful alignment"
echo "If elif condition succeeds -> execute elif body -> STOP (don't check else)"
echo "Only if ALL conditions fail should the else clause be executed"
