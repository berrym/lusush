#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== INCREMENTAL COMPLEXITY TEST ==="
echo

echo "Test 1: Single command in if"
if true; then
    simple_var=SIMPLE
fi
echo "simple_var=$simple_var"

echo
echo "Test 2: Two commands in if"
if true; then
    first_var=FIRST
    second_var=SECOND
fi
echo "first_var=$first_var second_var=$second_var"

echo
echo "Test 3: Three commands in if"
if true; then
    a=A
    b=B
    c=C
fi
echo "a=$a b=$b c=$c"

echo
echo "=== INCREMENTAL COMPLEXITY COMPLETE ==="
