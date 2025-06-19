#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "Debug: Testing simple assignment"
test_var=TEST
echo "test_var=$test_var"

echo "Debug: Testing if with single command"
if true; then
    single_if=IF_SINGLE
fi
echo "single_if=$single_if"

echo "Debug: Testing if with two commands"
if true; then
    multi_a=MA
    multi_b=MB
fi
echo "multi_a=$multi_a multi_b=$multi_b" 
echo "Expected: multi_a=MA multi_b=MB"
