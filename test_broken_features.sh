#!/bin/bash

echo "=== TESTING SUPPOSEDLY BROKEN FEATURES ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "Testing features that were reported as broken in documentation:"
echo

echo "1. ARITHMETIC EXPANSION TEST"
echo "============================================"
echo "Test: echo \$((5 + 3)) should output 8"
echo "Result:"
echo 'echo $((5 + 3))' | "$LUSUSH"
echo "Expected: 8"
echo

echo "Test: echo \$((10 * 2)) should output 20"
echo "Result:"
echo 'echo $((10 * 2))' | "$LUSUSH"
echo "Expected: 20"
echo

echo "Test: echo \$((100 / 4)) should output 25"
echo "Result:"
echo 'echo $((100 / 4))' | "$LUSUSH"
echo "Expected: 25"
echo

echo "2. COMMAND SUBSTITUTION TEST"
echo "============================================"
echo "Test: echo \"Today: \$(date)\" should show current date"
echo "Result:"
echo 'echo "Today: $(date)"' | "$LUSUSH"
echo "Expected: Today: [current date]"
echo

echo "Test: echo \"User: \$(whoami)\" should show current user"
echo "Result:"
echo 'echo "User: $(whoami)"' | "$LUSUSH"
echo "Expected: User: [current username]"
echo

echo "Test: echo \"Files: \$(ls | wc -l)\" should show file count"
echo "Result:"
echo 'echo "Files: $(ls | wc -l)"' | "$LUSUSH"
echo "Expected: Files: [number]"
echo

echo "3. SINGLE QUOTE LITERAL PROTECTION TEST"
echo "============================================"
echo "Test: echo 'Value: \$USER' should output literal \$USER"
echo "Result:"
echo "echo 'Value: \$USER'" | "$LUSUSH"
echo "Expected: Value: \$USER (literal)"
echo

echo "Test: echo 'Command: \$(date)' should output literal \$(date)"
echo "Result:"
echo "echo 'Command: \$(date)'" | "$LUSUSH"
echo "Expected: Command: \$(date) (literal)"
echo

echo "Test: echo 'Math: \$((2+2))' should output literal \$((2+2))"
echo "Result:"
echo "echo 'Math: \$((2+2))'" | "$LUSUSH"
echo "Expected: Math: \$((2+2)) (literal)"
echo

echo "4. COMPLEX MIXED TESTS"
echo "============================================"
echo "Test: Mixed arithmetic and command substitution"
echo "Result:"
cat << 'EOF' | "$LUSUSH"
echo "Result: $((5 + $(echo 3)))"
EOF
echo "Expected: Result: 8"
echo

echo "Test: Nested command substitution"
echo "Result:"
cat << 'EOF' | "$LUSUSH"
echo "Nested: $(echo $(echo hello))"
EOF
echo "Expected: Nested: hello"
echo

echo "Test: Arithmetic with variables"
echo "Result:"
cat << 'EOF' | "$LUSUSH"
a=10
b=5
echo "Sum: $((a + b))"
EOF
echo "Expected: Sum: 15"
echo

echo "5. EDGE CASES AND ERROR HANDLING"
echo "============================================"
echo "Test: Invalid arithmetic"
echo "Result:"
echo 'echo $((5 + ))' | "$LUSUSH" 2>&1
echo

echo "Test: Invalid command substitution"
echo "Result:"
echo 'echo "$(nonexistentcommand 2>/dev/null)"' | "$LUSUSH" 2>&1
echo

echo "Test: Mixed quotes with expansions"
echo "Result:"
cat << 'EOF' | "$LUSUSH"
echo 'Literal: $USER' "Expanded: $USER"
EOF
echo "Expected: Literal: \$USER Expanded: [username]"
echo

echo "6. REGRESSION TEST SPECIFIC CASES"
echo "============================================"
echo "Test: Arithmetic in double quotes"
echo "Result:"
echo 'echo "Math result: $((2 * 3))"' | "$LUSUSH"
echo "Expected: Math result: 6"
echo

echo "Test: Command substitution in double quotes"
echo "Result:"
echo 'echo "Current dir: $(pwd | basename)"' | "$LUSUSH" 2>/dev/null
echo "Expected: Current dir: [directory name]"
echo

echo "Test: Single quotes preserve everything literally"
echo "Result:"
echo "echo 'Everything literal: \$USER \$(date) \$((1+1))'" | "$LUSUSH"
echo "Expected: Everything literal: \$USER \$(date) \$((1+1))"
echo

echo "=== TEST COMPLETE ==="
echo
echo "If any feature shows unexpected results above, that feature is broken."
echo "If all results match expectations, the features are working correctly."
