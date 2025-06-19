#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== ADVANCED CONTROL STRUCTURE TESTS ==="
echo

echo "Test 1: Nested IF statements"
if true; then
    outer_var=OUTER
    if true; then
        inner_var=INNER
        nested_var=NESTED
    fi
    after_nested=AFTER
fi
echo "Results: outer_var=$outer_var inner_var=$inner_var nested_var=$nested_var after_nested=$after_nested"
echo

echo "Test 2: FOR loop with nested IF"
for num in 1 2; do
    loop_var=LOOP_$num
    if [ "$num" = "2" ]; then
        special_var=SPECIAL_TWO
    fi
done
echo "Results: loop_var=$loop_var special_var=$special_var"
echo

echo "Test 3: Multiple variable assignments and commands"
if true; then
    var1=A; var2=B; var3=C
    echo "Inside if: var1=$var1 var2=$var2 var3=$var3"
    final_var=FINAL
fi
echo "Results: final_var=$final_var"

echo
echo "=== ADVANCED TESTS COMPLETE ==="
