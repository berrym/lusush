#!/bin/bash

echo "=== MINIMAL WAIT DEBUG ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "1. Testing wait return value directly in C code..."
echo "Checking if wait function actually returns error codes..."

echo
echo "2. Simple invalid wait test:"
cat << 'EOF' | "$LUSUSH"
wait abc
echo "After wait abc, exit code variable is: $?"
EOF

echo
echo "3. Testing with debug output:"
cat << 'EOF' | "$LUSUSH"
echo "Before wait command"
wait abc
WAIT_EXIT=$?
echo "Wait returned: $WAIT_EXIT"
echo "Current \$? is: $?"
EOF

echo
echo "4. Testing if any command sets exit status correctly:"
cat << 'EOF' | "$LUSUSH"
/bin/false
echo "After /bin/false: $?"
EOF

echo
echo "5. Testing if builtin cd sets exit status:"
cat << 'EOF' | "$LUSUSH"
cd /nonexistent 2>/dev/null
echo "After cd /nonexistent: $?"
EOF

echo
echo "6. Testing variable assignment with command substitution:"
cat << 'EOF' | "$LUSUSH"
echo "Testing command substitution exit status..."
VAR=$(wait abc 2>&1)
echo "Command output: $VAR"
echo "Exit status after command substitution: $?"
EOF

echo
echo "7. Manual exit code test:"
cat << 'EOF' | "$LUSUSH"
echo "Testing manual exit..."
exit 42
EOF
echo "Shell actually exited with: $?"
