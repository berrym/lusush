#!/bin/bash

# Focused debug test for wait built-in
echo "=== DEBUGGING WAIT BUILT-IN ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "1. Testing basic wait with no arguments:"
echo "wait" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "2. Testing wait with invalid PID:"
echo "wait abc" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "3. Testing wait with non-existent PID:"
echo "wait 99999" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "4. Testing wait with background job and exit status:"
echo "sh -c 'exit 42' & wait" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "5. Testing wait for specific PID:"
echo "sleep 0.1 & echo PID: \$!; wait \$!" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "6. Testing multiple waits for same job:"
echo "sleep 0.1 & PID=\$!; wait \$PID; echo First wait done; wait \$PID; echo Second wait done" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "7. Testing wait with negative PID:"
echo "wait -1" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "8. Testing wait with PID that's not a child:"
echo "wait 1" | "$LUSUSH"
echo "Exit code: $?"
echo

echo "9. Manual test - checking actual PID handling:"
cat << 'EOF' | "$LUSUSH"
echo "Starting background job..."
sleep 0.2 &
PID=$!
echo "Background job PID: $PID"
echo "Waiting for PID $PID..."
wait $PID
EXIT_CODE=$?
echo "Wait completed with exit code: $EXIT_CODE"
EOF
echo "Script exit code: $?"
echo

echo "10. Testing argument parsing:"
cat << 'EOF' | "$LUSUSH"
echo "Testing empty string:"
wait ""
echo "Exit code: $?"

echo "Testing whitespace:"
wait " "
echo "Exit code: $?"

echo "Testing zero:"
wait 0
echo "Exit code: $?"
EOF
