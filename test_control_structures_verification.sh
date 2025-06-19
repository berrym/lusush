#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== CONTROL STRUCTURES MULTI-COMMAND VERIFICATION ==="
echo

echo "Test 1: IF with multiple commands (newlines)"
if true; then
    test1_a=ALPHA
    test1_b=BETA
    test1_c=GAMMA
fi
echo "Results: test1_a=$test1_a test1_b=$test1_b test1_c=$test1_c"
echo "Expected: test1_a=ALPHA test1_b=BETA test1_c=GAMMA"
echo

echo "Test 2: IF with multiple commands (semicolons)"  
if true; then
    test2_a=ONE; test2_b=TWO; test2_c=THREE
fi
echo "Results: test2_a=$test2_a test2_b=$test2_b test2_c=$test2_c"
echo "Expected: test2_a=ONE test2_b=TWO test2_c=THREE"
echo

echo "Test 3: IF-ELSE with multiple commands"
if false; then
    false_var=SHOULD_NOT_SET
else
    test3_a=ELSE_A
    test3_b=ELSE_B; test3_c=ELSE_C
fi
echo "Results: false_var=$false_var test3_a=$test3_a test3_b=$test3_b test3_c=$test3_c"
echo "Expected: false_var= test3_a=ELSE_A test3_b=ELSE_B test3_c=ELSE_C"
echo

echo "Test 4: FOR loop with multiple commands"
for item in SINGLE; do
    test4_a=FOR_A
    test4_b=FOR_B; test4_c=FOR_C
done
echo "Results: test4_a=$test4_a test4_b=$test4_b test4_c=$test4_c"
echo "Expected: test4_a=FOR_A test4_b=FOR_B test4_c=FOR_C"
echo

echo "Test 5: WHILE loop with multiple commands (safe)"
counter=0
while [ "$counter" -lt 1 ]; do
    test5_a=WHILE_A; test5_b=WHILE_B
    counter=$((counter + 1))
done
echo "Results: test5_a=$test5_a test5_b=$test5_b"
echo "Expected: test5_a=WHILE_A test5_b=WHILE_B"

echo
echo "=== CONTROL STRUCTURES VERIFICATION COMPLETE ==="
