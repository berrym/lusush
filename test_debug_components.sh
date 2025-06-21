#!/bin/bash

echo "Debug: Testing individual components..."

echo "=== Test 1: Simple assignment ==="
echo 'counter=1' | ./builddir/lusush

echo ""
echo "=== Test 2: Simple WHILE ==="
echo 'while [ 1 -eq 1 ]; do echo "test"; break; done' | ./builddir/lusush

echo ""
echo "=== Test 3: Separated assignment and while ==="
echo 'counter=1' | ./builddir/lusush
echo 'while [ $counter -le 2 ]; do echo "Counter: $counter"; counter=2; done' | ./builddir/lusush

echo ""
echo "=== Test 4: FOR loop with explicit items ==="
echo 'for i in one two three; do echo "Item: $i"; done' | ./builddir/lusush
