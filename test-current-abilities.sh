#!/home/mberry/Lab/c/lusush/builddir/lusush

echo "=== CURRENT LUSUSH CAPABILITIES TEST ==="
echo ""

echo "1. BASIC COMMAND EXECUTION:"
echo "Hello from lusush!"
echo ""

echo "2. VARIABLE ASSIGNMENT AND EXPANSION:"
name="lusush"
version="0.1.0"
echo "Shell: $name version $version"
echo ""

echo "3. COMMAND SUBSTITUTION:"
echo "Current directory: $(pwd)"
echo "Files in directory: $(ls | wc -l)"
echo ""

echo "4. ARITHMETIC EXPANSION:"
a=10
b=5
echo "$a + $b = $((a + b))"
echo "$a * $b = $((a * b))"
echo ""

echo "5. PARAMETER EXPANSION:"
unset testvar
echo "Unset variable with default: ${testvar:-default_value}"
testvar="set"
echo "Set variable: ${testvar}"
echo "Length of variable: ${#testvar}"
echo ""

echo "6. PIPELINES:"
echo "hello world" | grep "world"
echo ""

echo "7. LOGICAL OPERATORS:"
true && echo "AND operator works"
false || echo "OR operator works"
echo ""

echo "8. GLOBBING:"
echo "Markdown files: *.md"
echo ""

echo "9. CONTROL STRUCTURES:"
echo "For loop test:"
for i in 1 2 3; do
    echo "  Iteration $i"
done

echo "If statement test:"
if test -f README.md; then
    echo "  README.md exists"
fi
echo ""

echo "10. REDIRECTION:"
echo "Testing redirection" > /tmp/lusush_test.txt
echo "File contents: $(cat /tmp/lusush_test.txt)"
rm -f /tmp/lusush_test.txt
echo ""

echo "=== CAPABILITIES TEST COMPLETE ==="
