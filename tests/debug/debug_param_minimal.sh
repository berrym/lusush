#!/bin/bash

echo "=== Minimal Parameter Expansion Debug ==="

echo
echo "Testing parsing of multiple parameter expansions..."

echo
echo "1. Single expansion (baseline):"
printf '${a=1}\necho "a=$a"\n' | ./builddir/lusush

echo
echo "2. Two expansions on same line:"
printf '${a=1} ${b=2}\necho "a=$a, b=$b"\n' | ./builddir/lusush

echo
echo "3. Two expansions on separate lines:"
printf '${a=1}\n${b=2}\necho "a=$a, b=$b"\n' | ./builddir/lusush

echo
echo "4. With command prefix:"
printf 'true ${a=1} ${b=2}\necho "a=$a, b=$b"\n' | ./builddir/lusush

echo
echo "5. With echo command:"
printf 'echo ${a=1} ${b=2}\necho "a=$a, b=$b"\n' | ./builddir/lusush

echo
echo "6. Testing parser AST structure with debug:"
printf '${a=1} ${b=2}\necho "done"\n' | PARAM_EXPANSION_DEBUG=1 ./builddir/lusush

echo
echo "7. Compare with bash behavior:"
printf '${a=1} ${b=2}\necho "a=$a, b=$b"\n' | bash 2>/dev/null || echo "bash failed"

echo
echo "8. Alternative syntax test:"
printf ': ${a=1} ${b=2}\necho "a=$a, b=$b"\n' | bash
