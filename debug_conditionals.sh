#!/bin/bash

echo "=== Testing Conditional Components ==="

echo "Test 1: Simple echo command"
echo 'echo "hello world"' | timeout 3s ./builddir/lusush

echo ""
echo "Test 2: Test builtin only"
echo 'test "a" = "a"; echo "Exit code: $?"' | timeout 3s ./builddir/lusush

echo ""
echo "Test 3: Bracket test only"
echo '[ "a" = "a" ]; echo "Exit code: $?"' | timeout 3s ./builddir/lusush

echo ""
echo "Test 4: Simple if without test"
echo 'if true; then echo "if works"; fi' | timeout 3s ./builddir/lusush

echo ""
echo "Test 5: Simple if with test builtin"
echo 'if test "a" = "a"; then echo "test builtin works"; fi' | timeout 3s ./builddir/lusush

echo ""
echo "Test 6: Simple if with bracket test"
echo 'if [ "a" = "a" ]; then echo "bracket test works"; fi' | timeout 3s ./builddir/lusush

echo ""
echo "Test 7: Logical AND operator"
echo 'true && echo "AND works"' | timeout 3s ./builddir/lusush

echo ""
echo "Test 8: Pipeline test"
echo 'echo "hello" | cat' | timeout 3s ./builddir/lusush
