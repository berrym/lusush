#!/bin/bash

echo "=== Debugging Arithmetic Variable Resolution ==="

echo
echo "1. Test basic variable assignment and retrieval:"
printf 'n1=4\necho "n1 is: $n1"\n' | ./builddir/lusush

echo
echo "2. Test arithmetic with simple literals:"
printf 'echo "4*6 = $((4*6))"\n' | ./builddir/lusush

echo
echo "3. Test single variable in arithmetic:"
printf 'n1=4\necho "n1*2 = $(($n1*2))"\n' | ./builddir/lusush

echo
echo "4. Test two variables assigned separately:"
printf 'n1=4\nn2=6\necho "n1=$n1, n2=$n2"\necho "n1*n2 = $(($n1*$n2))"\n' | ./builddir/lusush

echo
echo "5. Test variables with debug output:"
printf 'n1=4\nn2=6\necho $(($n1*$n2))\n' | ARITHMETIC_DEBUG=1 ./builddir/lusush

echo
echo "6. Test variable assignment in arithmetic context:"
printf 'echo "result = $((n1=4, n2=6, n1*n2))"\necho "n1=$n1, n2=$n2"\n' | ./builddir/lusush

echo
echo "7. Test with environment variables:"
N1=4 N2=6 printf 'echo "env vars: $((N1*N2))"\n' | ./builddir/lusush

echo
echo "8. Test simple variable reference:"
printf 'x=5\necho "x = $x"\necho "x in arithmetic = $(($x))"\n' | ./builddir/lusush

echo
echo "9. Test variable substitution directly:"
printf 'val=7\necho "Direct: $val"\necho "In arithmetic: $((val))"\n' | ./builddir/lusush
