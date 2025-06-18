#!/bin/bash

echo "=== Testing For Loop and Variable Expansion ==="

# Build the shell first
cd /home/mberry/Lab/c/lusush
if ! meson compile -C builddir; then
    echo "Build failed"
    exit 1
fi

echo -e "\n=== Test 1: Basic variable setting and expansion ==="
echo 'x=hello; echo "x is: $x"' | ./builddir/lusush

echo -e "\n=== Test 2: Variable in for loop body ==="
echo 'x=world; for i in a b c; do echo "$i: $x"; done' | ./builddir/lusush

echo -e "\n=== Test 3: Loop variable expansion ==="
echo 'for i in one two three; do echo "Item: $i"; done' | ./builddir/lusush

echo -e "\n=== Test 4: Multiple variable expansions ==="
echo 'x=hello; y=world; for i in 1 2; do echo "$i: $x $y"; done' | ./builddir/lusush

echo -e "\n=== Test 5: Environment vs shell variables ==="
echo 'export ENV_VAR=env_value; SHELL_VAR=shell_value; echo "ENV: $ENV_VAR, SHELL: $SHELL_VAR"' | ./builddir/lusush

echo -e "\n=== Test 6: Variable assignment in different contexts ==="
cat << 'EOF' | ./builddir/lusush
VAR1=simple
export VAR2=exported
echo "VAR1=$VAR1 VAR2=$VAR2"
for item in a b; do
    echo "In loop: VAR1=$VAR1 VAR2=$VAR2 item=$item"
done
EOF

echo -e "\n=== Test 7: Complex for loop parsing ==="
cat << 'EOF' | ./builddir/lusush
echo "Testing complex for loop"
for x in alpha beta gamma
do
    echo "Processing: $x"
    for y in 1 2
    do
        echo "  Inner: $x-$y"
    done
done
echo "Done with complex loop"
EOF

echo -e "\n=== Test 8: For loop with quoted values ==="
echo 'for item in "hello world" "foo bar"; do echo "Item: [$item]"; done' | ./builddir/lusush

echo -e "\n=== All tests completed ==="
