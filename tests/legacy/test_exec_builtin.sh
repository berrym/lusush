#!/bin/bash

# Comprehensive test suite for the exec built-in command
# Tests POSIX-compliant exec functionality for process replacement

echo "=== LUSUSH EXEC BUILT-IN COMMAND TEST ==="
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

echo "=== TEST 1: Basic exec command replacement ==="
echo "Testing exec replacing shell process..."

cat > exec_test1.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Script starting - PID: $$"
echo "About to exec echo command..."
exec echo "Hello from exec!"
echo "This line should never be printed"
EOF

chmod +x exec_test1.sh
echo "Running: ./exec_test1.sh"
./exec_test1.sh
echo ""

echo "=== TEST 2: exec with arguments ==="
echo "Testing exec with multiple arguments..."

cat > exec_test2.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Script with exec arguments test"
echo "Original PID: $$"
exec echo "Argument 1" "Argument 2" "Argument 3"
echo "This should not execute"
EOF

chmod +x exec_test2.sh
echo "Running: ./exec_test2.sh"
./exec_test2.sh
echo ""

echo "=== TEST 3: exec with PATH lookup ==="
echo "Testing exec with commands in PATH..."

cat > exec_test3.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing exec with PATH commands"
echo "Current directory: $(pwd)"
exec ls -la /tmp | head -5
EOF

chmod +x exec_test3.sh
echo "Running: ./exec_test3.sh"
./exec_test3.sh
echo ""

echo "=== TEST 4: exec with non-existent command ==="
echo "Testing exec error handling..."

cat > exec_test4.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing exec with non-existent command"
echo "This should cause exec to fail"
exec nonexistent_command_12345
echo "This should not execute if exec fails properly"
EOF

chmod +x exec_test4.sh
echo "Running: ./exec_test4.sh (should show error)"
./exec_test4.sh 2>&1
echo "Exit code: $?"
echo ""

echo "=== TEST 5: exec without arguments ==="
echo "Testing exec with no arguments (should do nothing)..."

cat > exec_test5.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Before exec with no arguments"
exec
echo "After exec with no arguments - this should print"
echo "Script completed normally"
EOF

chmod +x exec_test5.sh
echo "Running: ./exec_test5.sh"
./exec_test5.sh
echo ""

echo "=== TEST 6: exec with EXIT trap ==="
echo "Testing that EXIT traps execute before exec..."

cat > exec_test6.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "EXIT trap executed before exec"' EXIT

echo "Script with EXIT trap"
echo "About to exec..."
exec echo "Exec command executed"
EOF

chmod +x exec_test6.sh
echo "Running: ./exec_test6.sh"
./exec_test6.sh
echo ""

echo "=== TEST 7: exec in interactive mode ==="
echo "Testing exec in interactive mode..."
echo 'echo "Interactive test"; exec echo "Exec in interactive mode"' | ./builddir/lusush
echo ""

echo "=== TEST 8: exec in command mode ==="
echo "Testing exec in -c command mode..."
./builddir/lusush -c 'echo "Command mode test"; exec echo "Exec in command mode"'
echo ""

echo "=== TEST 9: exec with shell script ==="
echo "Testing exec with another shell script..."

cat > exec_target.sh << 'EOF'
#!/bin/bash
echo "Target script executed via exec"
echo "Arguments received: $*"
echo "PID: $$"
EOF

cat > exec_test9.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "About to exec another script"
echo "Current PID: $$"
exec ./exec_target.sh arg1 arg2 arg3
EOF

chmod +x exec_target.sh exec_test9.sh
echo "Running: ./exec_test9.sh"
./exec_test9.sh
echo ""

echo "=== TEST 10: exec with environment variables ==="
echo "Testing exec with environment variable setting..."

cat > exec_test10.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Setting environment variable before exec"
export TEST_VAR="Hello from parent"
exec env | grep TEST_VAR
EOF

chmod +x exec_test10.sh
echo "Running: ./exec_test10.sh"
./exec_test10.sh
echo ""

echo "=== TEST 11: exec preserves exit status ==="
echo "Testing that exec preserves command exit status..."

cat > exec_test11.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing exec exit status preservation"
exec false
EOF

chmod +x exec_test11.sh
echo "Running: ./exec_test11.sh (should exit with status 1)"
./exec_test11.sh
echo "Exit code: $?"
echo ""

echo "=== TEST 12: exec with complex command ==="
echo "Testing exec with pipes and complex commands..."

cat > exec_test12.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing exec with complex command"
echo "About to exec a pipeline"
exec echo "Line 1\nLine 2\nLine 3" | grep Line
EOF

chmod +x exec_test12.sh
echo "Running: ./exec_test12.sh"
./exec_test12.sh
echo ""

echo "=== COMPARISON WITH BASH ==="
echo "Comparing exec behavior with bash..."

cat > bash_exec_test.sh << 'EOF'
#!/bin/bash
echo "BASH exec test"
echo "PID before exec: $$"
exec echo "BASH exec completed"
echo "This should not print in bash"
EOF

cat > lusush_exec_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "LUSUSH exec test"
echo "PID before exec: $$"
exec echo "LUSUSH exec completed"
echo "This should not print in lusush"
EOF

chmod +x bash_exec_test.sh lusush_exec_test.sh

echo ""
echo "Bash output:"
./bash_exec_test.sh

echo ""
echo "Lusush output:"
./lusush_exec_test.sh

echo ""

echo "=== POSIX COMPLIANCE VERIFICATION ==="
echo "Verifying POSIX-required exec behavior..."

cat > posix_exec_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

# POSIX Test 1: Basic exec replacement
echo "POSIX Test 1 - Basic exec:"
exec echo "POSIX exec works"

# This should never execute
echo "POSIX Test 1 FAILED - code after exec executed"
EOF

chmod +x posix_exec_test.sh
echo "Running POSIX compliance test:"
./posix_exec_test.sh
echo ""

echo "=== ERROR CONDITION TESTS ==="
echo "Testing various error conditions..."

echo "Test: exec with permission denied"
cat > no_perm_test << 'EOF'
#!/bin/bash
echo "No permission test"
EOF
chmod -x no_perm_test

cat > exec_error_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "Testing exec with permission denied file"
exec ./no_perm_test
EOF

chmod +x exec_error_test.sh
echo "Running permission denied test:"
./exec_error_test.sh 2>&1
echo "Exit code: $?"
echo ""

# Cleanup
rm -f exec_test1.sh exec_test2.sh exec_test3.sh exec_test4.sh exec_test5.sh
rm -f exec_test6.sh exec_test7.sh exec_test8.sh exec_test9.sh exec_test10.sh
rm -f exec_test11.sh exec_test12.sh exec_target.sh
rm -f bash_exec_test.sh lusush_exec_test.sh posix_exec_test.sh
rm -f exec_error_test.sh no_perm_test

echo "=== EXEC BUILT-IN COMMAND TEST COMPLETE ==="
echo ""
echo "Expected behavior:"
echo "- exec command [args] should replace shell process with command"
echo "- exec without arguments should do nothing and continue"
echo "- EXIT traps should execute before process replacement"
echo "- exec should preserve command exit status"
echo "- exec should handle PATH lookup correctly"
echo "- exec failure should exit shell with status 127"
echo "- Process replacement should be complete (no code after exec runs)"
echo "- Environment variables should be passed to exec'd command"
echo "- exec should work with all types of commands and scripts"
