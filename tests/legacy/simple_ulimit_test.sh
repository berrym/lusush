#!/bin/bash

echo "=== SIMPLE ULIMIT TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "1. Testing ulimit help:"
echo "ulimit -h" | "$LUSUSH"
echo

echo "2. Testing ulimit -a (show all limits):"
echo "ulimit -a" | "$LUSUSH"
echo

echo "3. Testing specific limits:"
echo "ulimit -n" | "$LUSUSH"  # file descriptors
echo "ulimit -f" | "$LUSUSH"  # file size
echo "ulimit -t" | "$LUSUSH"  # CPU time
echo

echo "4. Testing ulimit setting (soft limit):"
echo -e "ulimit -n 1024\nulimit -n" | "$LUSUSH"
echo

echo "5. Testing error handling:"
echo -e "ulimit -n abc\necho Exit code: \$?" | "$LUSUSH"
echo

echo "6. Testing unlimited value:"
echo -e "ulimit -t unlimited\nulimit -t" | "$LUSUSH"
echo

echo "7. Testing invalid option:"
echo -e "ulimit -z\necho Exit code: \$?" | "$LUSUSH"
echo

echo "Done!"
