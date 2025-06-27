#!/bin/bash

echo "=== SIMPLE GETOPTS TEST ==="

cd "$(dirname "$0")"
LUSUSH="./builddir/lusush"

echo "1. Testing basic getopts usage:"
cat << 'EOF' | "$LUSUSH"
while getopts "ab:c" opt; do
    case $opt in
        a) echo "Option a found" ;;
        b) echo "Option b found with argument: $OPTARG" ;;
        c) echo "Option c found" ;;
        \?) echo "Invalid option: $OPTARG" ;;
    esac
done
echo "OPTIND is now: $OPTIND"
EOF
echo

echo "2. Testing getopts with arguments (-a -b value -c):"
cat << 'EOF' | "$LUSUSH"
# Simulate arguments by setting positional parameters
set -- -a -b hello -c
while getopts "ab:c" opt; do
    case $opt in
        a) echo "Found -a" ;;
        b) echo "Found -b with arg: $OPTARG" ;;
        c) echo "Found -c" ;;
        \?) echo "Invalid option" ;;
    esac
done
echo "Final OPTIND: $OPTIND"
EOF
echo

echo "3. Testing getopts error handling (invalid option):"
cat << 'EOF' | "$LUSUSH"
set -- -a -x -c
while getopts "ab:c" opt; do
    case $opt in
        a) echo "Found -a" ;;
        b) echo "Found -b with arg: $OPTARG" ;;
        c) echo "Found -c" ;;
        \?) echo "Invalid option: $OPTARG" ;;
    esac
done
EOF
echo

echo "4. Testing getopts silent mode (leading colon):"
cat << 'EOF' | "$LUSUSH"
set -- -a -x -c
while getopts ":ab:c" opt; do
    case $opt in
        a) echo "Found -a" ;;
        b) echo "Found -b with arg: $OPTARG" ;;
        c) echo "Found -c" ;;
        \?) echo "Unknown option: $OPTARG" ;;
        :) echo "Option $OPTARG requires an argument" ;;
    esac
done
EOF
echo

echo "5. Testing getopts with missing argument:"
cat << 'EOF' | "$LUSUSH"
set -- -a -b -c
while getopts "ab:c" opt; do
    case $opt in
        a) echo "Found -a" ;;
        b) echo "Found -b with arg: $OPTARG" ;;
        c) echo "Found -c" ;;
        \?) echo "Invalid option: $OPTARG" ;;
    esac
done
EOF
echo

echo "6. Testing getopts exit codes:"
cat << 'EOF' | "$LUSUSH"
set -- -a -b hello
while getopts "ab:c" opt; do
    echo "getopts returned: $?"
    case $opt in
        a) echo "Found -a" ;;
        b) echo "Found -b with arg: $OPTARG" ;;
        c) echo "Found -c" ;;
    esac
done
echo "Final getopts call returned: $?"
EOF
echo

echo "7. Testing getopts usage error:"
echo -e "getopts\necho Exit code: \$?" | "$LUSUSH"
echo

echo "Done!"
