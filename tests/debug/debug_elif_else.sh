#!/bin/bash

echo "=== Elif-Else Debug Script ==="
echo

echo "Test 1: Simple if-else (should work)"
echo 'x=3; if [ $x -eq 1 ]; then echo one; else echo other; fi' | ./builddir/lusush
echo

echo "Test 2: If-elif (should work)"
echo 'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; fi' | ./builddir/lusush
echo

echo "Test 3: If-elif-else (currently broken - both elif and else execute)"
echo 'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi' | ./builddir/lusush
echo

echo "Test 4: If-elif-else with different condition (else should execute)"
echo 'x=3; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi' | ./builddir/lusush
echo

echo "Test 5: Multiple elif clauses"
echo 'x=3; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; elif [ $x -eq 3 ]; then echo three; else echo other; fi' | ./builddir/lusush
echo

echo "Test 6: Expected behavior with bash"
echo "Expected for test 3 (bash):"
bash -c 'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi'
echo

echo "Test 7: Expected behavior for test 4 (bash):"
bash -c 'x=3; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi'
echo

echo "Test 8: Debug with verbose output"
echo 'NEW_PARSER_DEBUG=1 ./builddir/lusush -c "x=2; if [ \$x -eq 1 ]; then echo one; elif [ \$x -eq 2 ]; then echo two; else echo other; fi"'
NEW_PARSER_DEBUG=1 ./builddir/lusush -c 'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi'
echo
