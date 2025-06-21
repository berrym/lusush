#!/bin/bash

echo "Testing fixed WHILE loop implementation..."

echo "Test 1: Simple WHILE loop with counter"
echo 'counter=1; while [ $counter -le 3 ]; do echo "Counter: $counter"; counter=$((counter + 1)); done' | ./builddir/lusush

echo ""
echo "Test 2: FOR loop with shell variables"
echo 'for i in apple banana cherry; do echo "Fruit: $i"; done' | ./builddir/lusush

echo ""
echo "Test 3: Variable expansion in loops"
echo 'name=lusush; for item in hello $name world; do echo "Item: $item"; done' | ./builddir/lusush

echo ""
echo "Test 4: Nested variable expansion"
echo 'x=5; y=10; while [ $x -lt $y ]; do echo "x=$x, y=$y"; x=$((x + 1)); done' | ./builddir/lusush
