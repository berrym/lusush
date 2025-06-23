#!/bin/bash

echo "=== Testing Function Parameter Passing ==="

echo "Test 1: Simple parameter test"
echo 'test_param() { echo "param1=[$1]"; }; test_param hello' | ./builddir/lusush

echo ""
echo "Test 2: Parameter in conditional"
echo 'test_cond() { echo "param1=[$1]"; if [ "$1" = "test" ]; then echo "match"; else echo "no match"; fi; }; test_cond test' | ./builddir/lusush

echo ""
echo "Test 3: Basic conditional outside function"
echo 'if [ "test" = "test" ]; then echo "basic conditional works"; fi' | ./builddir/lusush

echo ""
echo "Test 4: Test command standalone"
echo '[ "test" = "test" ] && echo "test command works"' | ./builddir/lusush

echo ""
echo "Test 5: Parameter expansion test"
echo 'show_param() { echo "First param: [$1]"; echo "Second param: [$2]"; }; show_param alpha beta' | ./builddir/lusush
