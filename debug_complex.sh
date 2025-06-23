#!/bin/bash

echo "=== Testing Complex Command Combinations ==="

echo "Test 1: Command sequence with semicolon"
echo 'echo "first"; echo "second"' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 2: Test command followed by echo"
echo 'test "a" = "a"; echo "done"' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 3: Bracket test followed by echo"
echo '[ "a" = "a" ]; echo "done"' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 4: Function with simple command"
echo 'f() { echo "hello"; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 5: Function with test command"
echo 'f() { test "a" = "a"; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 6: Function with bracket test"
echo 'f() { [ "a" = "a" ]; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 7: Function with test and echo"
echo 'f() { test "a" = "a"; echo "after test"; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 8: Function with bracket test and echo"
echo 'f() { [ "a" = "a" ]; echo "after bracket"; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 9: Simple if statement"
echo 'if true; then echo "if works"; fi' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 10: If with test command"
echo 'if test "a" = "a"; then echo "test in if works"; fi' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 11: If with bracket test"
echo 'if [ "a" = "a" ]; then echo "bracket in if works"; fi' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 12: Function with if statement"
echo 'f() { if true; then echo "nested if works"; fi; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 13: Function with if and test"
echo 'f() { if test "a" = "a"; then echo "nested test works"; fi; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 14: Function with if and bracket"
echo 'f() { if [ "a" = "a" ]; then echo "nested bracket works"; fi; }; f' | timeout 3s ./builddir/lusush
echo "Result: $?"

echo ""
echo "Test 15: The exact failing test case"
echo 'cond() { if [ "$1" = "test" ]; then echo "match"; fi; }; cond test' | timeout 3s ./builddir/lusush
echo "Result: $?"
