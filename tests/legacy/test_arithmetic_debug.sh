#!/bin/bash

# Debug test for arithmetic expansion in lusush shell
# This test helps diagnose issues with the modern arithmetic implementation

echo "=== Arithmetic Expansion Debug Test ==="

# Test simple numbers
echo "Testing simple numbers:"
echo '$((5))' | ./builddir/lusush
echo '$((42))' | ./builddir/lusush

# Test basic operations
echo "Testing basic operations:"
echo '$((5 + 3))' | ./builddir/lusush
echo '$((10 - 4))' | ./builddir/lusush
echo '$((6 * 7))' | ./builddir/lusush
echo '$((20 / 4))' | ./builddir/lusush

# Test operator precedence
echo "Testing operator precedence:"
echo '$((2 + 3 * 4))' | ./builddir/lusush
echo '$((10 * 5 + 3))' | ./builddir/lusush
echo '$(((2 + 3) * 4))' | ./builddir/lusush

# Test variables
echo "Testing variables:"
echo 'x=10; echo $((x + 5))' | ./builddir/lusush
echo 'a=3; b=4; echo $((a * b))' | ./builddir/lusush

# Test comparison operations
echo "Testing comparison operations:"
echo '$((5 > 3))' | ./builddir/lusush
echo '$((2 < 1))' | ./builddir/lusush
echo '$((5 == 5))' | ./builddir/lusush

# Test inside double quotes (the original bug)
echo "Testing inside double quotes:"
echo 'echo "Result: $((5 + 3))"' | ./builddir/lusush
echo 'echo "Complex: $((10 * 5 + 3))"' | ./builddir/lusush

# Test nested expressions
echo "Testing nested expressions:"
echo '$((1 + (2 * 3)))' | ./builddir/lusush
echo '$(((1 + 2) * (3 + 4)))' | ./builddir/lusush

echo "=== Debug Test Complete ==="
