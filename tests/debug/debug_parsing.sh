#!/bin/bash

echo "=== Debugging Parameter Expansion Parsing ==="

echo
echo "1. Testing single parameter expansion:"
printf '${n1=4}\necho "n1=$n1"\n' | ./builddir/lusush

echo
echo "2. Testing multiple parameter expansions on one line:"
printf '${n1=4} ${n2=6}\necho "n1=$n1, n2=$n2"\n' | ./builddir/lusush

echo
echo "3. Testing multiple parameter expansions on separate lines:"
printf '${n1=4}\n${n2=6}\necho "n1=$n1, n2=$n2"\n' | ./builddir/lusush

echo
echo "4. Testing with debug output:"
printf '${n1=4} ${n2=6}\necho "vars set"\n' | NEW_PARSER_DEBUG=1 ./builddir/lusush

echo
echo "5. Testing arithmetic with variables:"
printf 'n1=4\nn2=6\necho "n1=$n1, n2=$n2"\necho "arithmetic: $(($n1*$n2))"\n' | ./builddir/lusush

echo
echo "6. Testing arithmetic with literals:"
printf 'echo "literal arithmetic: $((4*6))"\n' | ./builddir/lusush

echo
echo "7. Testing assignment followed by arithmetic:"
printf 'n1=4; n2=6; echo $(($n1*$n2))\n' | ./builddir/lusush
