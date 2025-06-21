#!/bin/bash

echo "=== FOR LOOP DEBUG TEST ==="

echo "Test 1: Simple variable assignment (should work)"
echo 'x=hello; echo "x is: $x"' | ./builddir/lusush

echo -e "\nTest 2: Manual loop variable setting"
echo 'i=1; echo "i is: $i"' | ./builddir/lusush

echo -e "\nTest 3: FOR loop with debug"
echo 'for i in 1; do echo "i is: $i"; done' | NEW_PARSER_DEBUG=1 ./builddir/lusush

echo -e "\nTest 4: Check if FOR loop variable is being set"
echo 'for i in 1; do echo "about to check i"; echo "$i"; echo "checked i"; done' | NEW_PARSER_DEBUG=1 ./builddir/lusush
