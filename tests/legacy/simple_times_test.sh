#!/bin/bash

echo "=== SIMPLE TIMES TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "1. Testing basic times command:"
echo "times" | "$LUSUSH"
echo

echo "2. Testing times with some CPU work:"
echo "for i in 1 2 3 4 5; do echo \$i; done; times" | "$LUSUSH"
echo

echo "3. Testing times with background process:"
echo "sleep 0.1 & wait; times" | "$LUSUSH"
echo

echo "4. Testing times exit code:"
echo -e "times\necho Exit code: \$?" | "$LUSUSH"
echo

echo "5. Testing times with arguments (should ignore):"
echo "times arg1 arg2" | "$LUSUSH"
echo

echo "6. Testing times format consistency:"
echo "times" | "$LUSUSH" | grep -c "m.*s"
echo

echo "Done!"
