#!/bin/bash

echo "=== SINGLE QUOTE BEHAVIOR TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "Current USER: $USER"
echo

echo "1. Testing single quotes (should NOT expand \$USER):"
echo "echo 'Value: \$USER'" | "$LUSUSH"
echo

echo "2. Testing double quotes (should expand \$USER):"
echo 'echo "Value: $USER"' | "$LUSUSH"
echo

echo "3. Testing mixed quotes:"
echo "echo 'Single: \$USER' \"Double: \$USER\"" | "$LUSUSH"
echo

echo "4. Testing variable assignment in single quotes:"
echo "VAR='test \$USER here'" | "$LUSUSH"
echo "echo \$VAR" | "$LUSUSH"
echo

echo "5. Testing command with single quotes directly:"
echo 'Value: $USER' | "$LUSUSH"
echo

echo "6. Testing echo command parsing:"
cat << 'EOF' | "$LUSUSH"
echo 'Value: $USER'
EOF
echo

echo "7. Testing complex single quote case:"
cat << 'EOF' | "$LUSUSH"
echo 'This should be literal: $USER $HOME $PWD'
EOF
echo

echo "8. Testing comparison with bash behavior:"
echo "Expected from bash:"
bash -c "echo 'Value: \$USER'"
echo "Actual from lusush:"
echo "echo 'Value: \$USER'" | "$LUSUSH"
echo

echo "Done!"
