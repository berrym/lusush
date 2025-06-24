#!/bin/bash

echo "=== SIMPLE WAIT RETURN CODE TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "Test 1: Invalid argument"
echo "wait abc" | "$LUSUSH"
echo "Shell exit code: $?"
echo

echo "Test 2: Valid wait with no jobs"
echo "wait" | "$LUSUSH"
echo "Shell exit code: $?"
echo

echo "Test 3: Wait with background job"
echo "sleep 0.1 & wait" | "$LUSUSH"
echo "Shell exit code: $?"
echo

echo "Test 4: Testing return value directly"
cat > temp_test.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
echo "wait abc" | ./builddir/lusush
exit $?
EOF
chmod +x temp_test.sh
./temp_test.sh
echo "Direct script exit code: $?"
rm temp_test.sh
