#!/bin/bash

echo "=== SIMPLE UMASK TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "1. Testing basic umask display:"
echo "umask" | "$LUSUSH"
echo

echo "2. Testing umask set and display:"
echo -e "umask 022\numask" | "$LUSUSH"
echo

echo "3. Testing error handling with exit codes:"
echo -e "umask abc\necho Exit code: \$?" | "$LUSUSH"
echo

echo "4. Testing valid octal values:"
echo -e "umask 077\numask\numask 644\numask\numask 000\numask" | "$LUSUSH"
echo

echo "5. Testing edge cases:"
echo -e "umask 0777\numask" | "$LUSUSH"
echo

echo "6. Testing too many arguments:"
echo -e "umask 022 044\necho Exit code: \$?" | "$LUSUSH"
echo

echo "7. Testing empty argument:"
echo -e "umask ''\necho Exit code: \$?" | "$LUSUSH"
echo

echo "8. Testing invalid octal:"
echo -e "umask 999\necho Exit code: \$?" | "$LUSUSH"
echo

echo "Done!"
