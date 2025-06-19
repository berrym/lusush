#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== SEMICOLON PARSING TEST ==="

echo "Test 1: Simple semicolon at top level"
semi_a=A; semi_b=B
echo "semi_a=$semi_a semi_b=$semi_b"

echo
echo "Test 2: Semicolon in if statement (single line)"
if true; then semi_if_a=IFA; semi_if_b=IFB; fi
echo "semi_if_a=$semi_if_a semi_if_b=$semi_if_b"

echo
echo "=== SEMICOLON TEST COMPLETE ==="
