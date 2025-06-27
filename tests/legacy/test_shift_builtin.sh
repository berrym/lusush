#!/bin/bash

# Comprehensive test suite for the shift built-in command
# Tests POSIX-compliant shift functionality

echo "=== LUSUSH SHIFT BUILT-IN COMMAND TEST ==="
echo ""

# Build the shell
echo "Building lusush..."
cd builddir && ninja && cd ..
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo ""

# Create test scripts for shift command
cat > shift_test1.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Initial state:"
echo "Arg count: $#"
echo "Args: $1 $2 $3 $4 $5"
echo ""

echo "After shift (default 1):"
shift
echo "Arg count: $#"
echo "Args: $1 $2 $3 $4 $5"
echo ""

echo "After shift 2:"
shift 2
echo "Arg count: $#"
echo "Args: $1 $2 $3 $4 $5"
EOF

cat > shift_test2.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing shift with exact parameter count:"
echo "Initial: $# args: $*"
shift $#
echo "After shifting all: $# args: $*"
EOF

cat > shift_test3.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing shift beyond available parameters:"
echo "Initial: $# args: $*"
shift 10
echo "After shifting 10 (more than available): $# args: $*"
EOF

cat > shift_test4.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing shift with zero:"
echo "Initial: $# args: $*"
shift 0
echo "After shift 0: $# args: $*"
EOF

chmod +x shift_test1.sh shift_test2.sh shift_test3.sh shift_test4.sh

echo "=== TEST 1: Basic shift functionality ==="
echo "Running: ./shift_test1.sh one two three four five"
./shift_test1.sh one two three four five
echo ""

echo "=== TEST 2: Shift exact parameter count ==="
echo "Running: ./shift_test2.sh a b c"
./shift_test2.sh a b c
echo ""

echo "=== TEST 3: Shift beyond available parameters ==="
echo "Running: ./shift_test3.sh x y"
./shift_test3.sh x y
echo ""

echo "=== TEST 4: Shift zero parameters ==="
echo "Running: ./shift_test4.sh alpha beta"
./shift_test4.sh alpha beta
echo ""

echo "=== TEST 5: Error handling - invalid arguments ==="
echo "Testing invalid shift arguments..."

cat > shift_error_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing negative shift:"
shift -1
echo "Exit code: $?"
echo ""

echo "Testing non-numeric shift:"
shift abc
echo "Exit code: $?"
echo ""

echo "Testing valid shift after error:"
echo "Args before: $*"
shift 1
echo "Args after: $*"
EOF

chmod +x shift_error_test.sh
echo "Running: ./shift_error_test.sh test1 test2 test3"
./shift_error_test.sh test1 test2 test3
echo ""

echo "=== TEST 6: Interactive mode shift ==="
echo "Testing shift in interactive mode..."
echo 'echo "Before: $*"; shift; echo "After: $*"' | ./builddir/lusush
echo ""

echo "=== TEST 7: Command mode shift ==="
echo "Testing shift in -c command mode..."
./builddir/lusush -c 'echo "This should show no args: $*"; shift; echo "After shift: $*"'
echo ""

echo "=== TEST 8: Shift in functions ==="
echo "Testing shift within function context..."

cat > shift_function_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

test_function() {
    echo "Function args before shift: $*"
    shift
    echo "Function args after shift: $*"
}

echo "Script args: $*"
test_function func_arg1 func_arg2 func_arg3
echo "Script args after function: $*"
EOF

chmod +x shift_function_test.sh
echo "Running: ./shift_function_test.sh script_arg1 script_arg2"
./shift_function_test.sh script_arg1 script_arg2
echo ""

echo "=== TEST 9: Multiple shifts in sequence ==="
echo "Testing multiple shift commands..."

cat > shift_multiple_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Start: $# args: $*"
shift
echo "After 1st shift: $# args: $*"
shift
echo "After 2nd shift: $# args: $*"
shift
echo "After 3rd shift: $# args: $*"
shift
echo "After 4th shift: $# args: $*"
EOF

chmod +x shift_multiple_test.sh
echo "Running: ./shift_multiple_test.sh a b c d e f"
./shift_multiple_test.sh a b c d e f
echo ""

echo "=== COMPARISON WITH BASH ==="
echo "Comparing shift behavior with bash..."

cat > bash_shift_test.sh << 'EOF'
#!/bin/bash
echo "BASH shift test:"
echo "Initial: $# args: $*"
shift 2
echo "After shift 2: $# args: $*"
shift 5
echo "After shift 5 (beyond available): $# args: $*"
EOF

cat > lusush_shift_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "LUSUSH shift test:"
echo "Initial: $# args: $*"
shift 2
echo "After shift 2: $# args: $*"
shift 5
echo "After shift 5 (beyond available): $# args: $*"
EOF

chmod +x bash_shift_test.sh lusush_shift_test.sh

echo ""
echo "Bash output:"
./bash_shift_test.sh one two three four
echo ""
echo "Lusush output:"
./lusush_shift_test.sh one two three four
echo ""

echo "=== POSIX COMPLIANCE VERIFICATION ==="
echo "Verifying POSIX-required shift behavior..."

# Test POSIX requirements:
# 1. shift [n] - where n is optional and defaults to 1
# 2. shift beyond available parameters should not be an error
# 3. shift with invalid arguments should return non-zero exit status
# 4. Positional parameters should be renumbered correctly

cat > posix_shift_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

# POSIX Test 1: Default shift (no argument)
echo "POSIX Test 1 - Default shift:"
set -- a b c d
echo "Before: $*"
shift
echo "After: $*"
echo ""

# POSIX Test 2: Shift with explicit count
echo "POSIX Test 2 - Explicit count:"
set -- x y z
echo "Before: $*"
shift 2
echo "After: $*"
echo ""

# POSIX Test 3: Shift beyond available (should not error)
echo "POSIX Test 3 - Beyond available:"
set -- m n
echo "Before: $*"
shift 5
echo "After: $*"
echo "Exit status: $?"
EOF

chmod +x posix_shift_test.sh
echo "Running POSIX compliance tests:"
./posix_shift_test.sh
echo ""

# Cleanup
rm -f shift_test1.sh shift_test2.sh shift_test3.sh shift_test4.sh
rm -f shift_error_test.sh shift_function_test.sh shift_multiple_test.sh
rm -f bash_shift_test.sh lusush_shift_test.sh posix_shift_test.sh

echo "=== SHIFT BUILT-IN COMMAND TEST COMPLETE ==="
echo ""
echo "Expected behavior:"
echo "- shift [n] should shift positional parameters left by n positions"
echo "- Default shift count is 1 when no argument provided"
echo "- Shifting beyond available parameters should shift all remaining"
echo "- Invalid arguments should return non-zero exit status"
echo "- Parameter count (\$#) should be updated correctly"
echo "- Remaining parameters should be renumbered starting from \$1"
