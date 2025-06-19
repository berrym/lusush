#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== FINAL COMPREHENSIVE MULTI-COMMAND TEST ==="
echo

echo "Testing IF statements:"
echo "1. Newline separators:"
if true; then
    if_nl_a=A
    if_nl_b=B
    if_nl_c=C
fi
echo "   if_nl_a=$if_nl_a if_nl_b=$if_nl_b if_nl_c=$if_nl_c"

echo "2. Semicolon separators:"
if true; then
    if_sc_a=A; if_sc_b=B; if_sc_c=C
fi
echo "   if_sc_a=$if_sc_a if_sc_b=$if_sc_b if_sc_c=$if_sc_c"

echo "3. IF-ELSE with mixed separators:"
if false; then
    false_var=SHOULD_NOT_SET
else
    else_nl=NL; else_sc=SC
    else_mixed=MIXED
fi
echo "   false_var=$false_var else_nl=$else_nl else_sc=$else_sc else_mixed=$else_mixed"

echo
echo "Testing FOR loops:"
echo "1. Newline separators:"
for item in single; do
    for_nl_a=FA
    for_nl_b=FB
    for_nl_c=FC
done
echo "   for_nl_a=$for_nl_a for_nl_b=$for_nl_b for_nl_c=$for_nl_c"

echo "2. Semicolon separators:"
for item in single; do
    for_sc_a=FA; for_sc_b=FB; for_sc_c=FC
done
echo "   for_sc_a=$for_sc_a for_sc_b=$for_sc_b for_sc_c=$for_sc_c"

echo
echo "Testing WHILE loops:"
echo "1. Newline separators:"
iteration=0
while [ "$iteration" -lt 1 ]; do
    while_nl_a=WA
    while_nl_b=WB
    iteration=$((iteration + 1))
done
echo "   while_nl_a=$while_nl_a while_nl_b=$while_nl_b"

echo "2. Semicolon separators:"
iteration2=0
while [ "$iteration2" -lt 1 ]; do
    while_sc_a=WA; while_sc_b=WB; iteration2=$((iteration2 + 1))
done
echo "   while_sc_a=$while_sc_a while_sc_b=$while_sc_b"

echo
echo "=== ALL TESTS COMPLETE ==="
