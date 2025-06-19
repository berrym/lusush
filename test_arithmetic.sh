#!/bin/bash

echo "=== ARITHMETIC EXPANSION TEST ==="
echo

echo "Basic arithmetic:"
echo "  5 + 3 = $((5 + 3))"
echo "  10 - 4 = $((10 - 4))"
echo "  6 * 7 = $((6 * 7))"
echo "  15 / 3 = $((15 / 3))"
echo "  17 % 5 = $((17 % 5))"

echo
echo "Advanced arithmetic:"
echo "  2 ** 3 = $((2 ** 3))"
echo "  (5 + 3) * 2 = $(((5 + 3) * 2))"
echo "  15 / (2 + 1) = $((15 / (2 + 1)))"

echo
echo "Variable arithmetic:"
a=10
b=5
echo "  a=$a, b=$b"
echo "  a + b = $((a + b))"
echo "  a * b = $((a * b))"
echo "  a - b = $((a - b))"

echo
echo "Complex expressions:"
echo "  (10 + 5) * 2 - 8 = $(((10 + 5) * 2 - 8))"
echo "  3 * (4 + 2) / 2 = $((3 * (4 + 2) / 2))"

echo
echo "=== ARITHMETIC TEST COMPLETE ==="
