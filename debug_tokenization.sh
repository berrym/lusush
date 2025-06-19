#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== TOKENIZATION ANALYSIS ==="
echo

echo "Let's see how this gets tokenized:"
echo "for i in 1; do"
echo "var=VALUE"  
echo "echo \"\$var\""
echo "done"
echo
echo "Result when executed:"

for i in 1; do
var=VALUE
echo "$var"
done

echo "After for: var=$var"
