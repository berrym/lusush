#!/bin/bash

# Test script for command sequence execution
# Tests the fix for command sequences like "a=test; for i in 1; do ...; done"

cd "$(dirname "$0")"

echo "=== COMMAND SEQUENCE EXECUTION TESTS ==="

echo
echo "Test 1: Assignment followed by echo"
echo 'a=test; echo "Value: $a"' | ./builddir/lusush

echo
echo "Test 2: Multiple assignments and commands"
echo 'x=hello; y=world; echo "$x $y"; z=done; echo "Final: $z"' | ./builddir/lusush

echo
echo "Test 3: Assignment followed by for loop"
echo 'counter=0; for i in 1 2 3; do echo "Item $i (counter=$counter)"; done' | ./builddir/lusush

echo
echo "Test 4: Complex sequence with multiple command types"
echo 'name=John; age=25; echo "Name: $name"; echo "Age: $age"; greeting="Hello $name"; echo "$greeting"' | ./builddir/lusush

echo
echo "Test 5: Original problematic case"
echo 'a=test; for i in 1; do echo "Loop iteration: i=$i, a=$a"; done' | ./builddir/lusush

echo
echo "Test 6: Assignment with arithmetic and commands"
echo 'x=10; y=20; sum=$((x + y)); echo "Sum of $x and $y is $sum"' | ./builddir/lusush

echo
echo "=== Command sequence tests complete ==="
