#!/bin/bash

# Debug script for while loop infinite loop issue

SHELL_PATH="./builddir/lusush"

echo "=== Debug While Loop Issue ==="

echo "1. Simple variable test:"
$SHELL_PATH <<< "x=1; echo x is \$x"

echo ""
echo "2. Variable assignment with arithmetic:"
$SHELL_PATH <<< "x=1; x=\$((x+1)); echo x is now \$x"

echo ""
echo "3. Test command with variables:"
$SHELL_PATH <<< "x=1; [ \$x -le 2 ] && echo x=\$x is le 2"
$SHELL_PATH <<< "x=3; [ \$x -le 2 ] && echo x=\$x is le 2 || echo x=\$x is gt 2"

echo ""
echo "4. Manual while loop steps:"
echo "Step 1 - Initial setup:"
$SHELL_PATH <<< "x=1; echo x=\$x; [ \$x -le 2 ] && echo condition true || echo condition false"

echo "Step 2 - After increment:"
$SHELL_PATH <<< "x=1; x=\$((x+1)); echo x=\$x; [ \$x -le 2 ] && echo condition true || echo condition false"

echo "Step 3 - After second increment:"
$SHELL_PATH <<< "x=1; x=\$((x+1)); x=\$((x+1)); echo x=\$x; [ \$x -le 2 ] && echo condition true || echo condition false"

echo ""
echo "5. Testing while loop with debug:"
echo "Running: x=1; while [ \$x -le 2 ]; do echo count \$x; x=\$((x+1)); done"
echo "With 3 second timeout..."

timeout 3s $SHELL_PATH <<< "x=1; while [ \$x -le 2 ]; do echo count \$x; x=\$((x+1)); done" || echo "TIMEOUT - infinite loop detected"

echo ""
echo "6. Variable scope test in sequences:"
$SHELL_PATH <<< "x=1; echo step1 x=\$x; x=\$((x+1)); echo step2 x=\$x; x=\$((x+1)); echo step3 x=\$x"
