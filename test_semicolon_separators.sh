#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== SEMICOLON SEPARATOR TEST ==="
echo

echo "1. Testing IF with semicolon separators:"
if true; then
    semi_a=A; semi_b=B; semi_c=C
fi
echo "Results: semi_a=$semi_a semi_b=$semi_b semi_c=$semi_c"

echo
echo "2. Testing FOR with semicolon separators:"
for item in single; do
    for_semi_a=FA; for_semi_b=FB
done
echo "Results: for_semi_a=$for_semi_a for_semi_b=$for_semi_b"

echo
echo "=== SEMICOLON TEST COMPLETE ==="
