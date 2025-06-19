#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== COMPREHENSIVE MULTI-COMMAND WITH SEMICOLONS TEST ==="
echo

echo "Test 1: Multiple commands in if statement with semicolons"
if true; then
    semi_if_a=A; semi_if_b=B; semi_if_c=C
fi
echo "semi_if_a=$semi_if_a semi_if_b=$semi_if_b semi_if_c=$semi_if_c"

echo
echo "Test 2: Multiple commands in for loop with semicolons"
for item in single; do
    semi_for_a=FA; semi_for_b=FB; semi_for_c=FC
done
echo "semi_for_a=$semi_for_a semi_for_b=$semi_for_b semi_for_c=$semi_for_c"

echo
echo "Test 3: Mixed newlines and semicolons in if-else"
if false; then
    false_a=SHOULD_NOT_SET
    false_b=SHOULD_NOT_SET
else
    else_a=ELSE_A; else_b=ELSE_B
    else_c=ELSE_C
fi
echo "false_a=$false_a false_b=$false_b"
echo "else_a=$else_a else_b=$else_b else_c=$else_c"

echo
echo "=== SEMICOLON TEST COMPLETE ==="
