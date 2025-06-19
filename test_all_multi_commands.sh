#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== COMPREHENSIVE MULTI-COMMAND TEST ==="
echo

echo "1. Testing IF statement with multiple commands:"
if true; then
    if_var1=VALUE1
    if_var2=VALUE2
    if_var3=VALUE3
fi
echo "Results: if_var1=$if_var1 if_var2=$if_var2 if_var3=$if_var3"

echo
echo "2. Testing IF-ELSE statement with multiple commands:"
if false; then
    false_var=SHOULD_NOT_SET
else
    else_var1=ELSE1
    else_var2=ELSE2
fi
echo "Results: false_var=$false_var else_var1=$else_var1 else_var2=$else_var2"

echo
echo "3. Testing FOR loop with multiple commands:"
for item in single; do
    for_var1=FOR1
    for_var2=FOR2
    for_var3=FOR3
done
echo "Results: for_var1=$for_var1 for_var2=$for_var2 for_var3=$for_var3"

echo
echo "4. Testing WHILE loop with multiple commands:"
counter=1
while test $counter -eq 1; do
    while_var1=WHILE1
    while_var2=WHILE2
    counter=2
done
echo "Results: while_var1=$while_var1 while_var2=$while_var2"

echo
echo "=== ALL TESTS COMPLETE ==="
