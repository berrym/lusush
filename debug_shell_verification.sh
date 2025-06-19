#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== SHELL VERIFICATION TEST ==="
echo

echo "Testing lusush-specific behavior:"
echo "1. Parameter expansion that shows lusush artifacts:"
unset testvar
echo "Unset with default: \${testvar:-default_value}"
echo "Expected in lusush: \$\${testvar:-default_value}"

echo
echo "2. Pipeline warning test:"
echo "hello" | grep "hello"

echo
echo "3. For loop assignment test:"
for i in 1; do
    loop_var=assigned_in_loop
    echo "Inside loop: loop_var=$loop_var"
done
echo "Outside loop: loop_var=$loop_var"

echo
echo "=== VERIFICATION COMPLETE ==="
