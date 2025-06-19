#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== IF STATEMENT MULTIPLE COMMANDS TEST ==="
echo

echo "Test: Multiple commands in if then body"
if true; then
    if_a=A
    if_b=B  
    if_c=C
fi
echo "if_a=$if_a if_b=$if_b if_c=$if_c"

echo
echo "Test: Multiple commands in if-else"
if false; then
    false_a=FA
    false_b=FB
else
    else_a=EA
    else_b=EB
fi
echo "false_a=$false_a false_b=$false_b else_a=$else_a else_b=$else_b"

echo
echo "=== IF STATEMENT TEST COMPLETE ==="
