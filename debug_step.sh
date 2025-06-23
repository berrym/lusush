#!/bin/bash

echo "=== Step-by-step debugging of test command ==="

echo "Step 1: Testing if shell works at all"
echo 'echo "hello"' | ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 2: Testing simple command execution"
echo 'pwd' | ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 3: Testing builtin echo"
echo 'echo "builtin test"' | ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 4: Testing 'test' command with no args"
echo 'test' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 5: Testing 'test' with simple string"
echo 'test "hello"' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 6: Testing '[' command with no args"
echo '[' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 7: Testing complete bracket test"
echo '[ "a" = "a" ]' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 8: Testing test command directly"
echo 'test "a" = "a"' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 9: Testing function definition only"
echo 'f() { echo "in f"; }' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Step 10: Testing function call"
echo 'f() { echo "in f"; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"
