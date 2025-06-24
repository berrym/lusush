#!/bin/bash

# Comprehensive test suite for the trap built-in command
# Tests POSIX-compliant trap functionality for signal handling

echo "=== LUSUSH TRAP BUILT-IN COMMAND TEST ==="
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

echo "=== TEST 1: Basic trap functionality ==="
echo "Testing trap command listing and signal names..."

echo "1a. List available signals:"
echo 'trap -l' | ./builddir/lusush
echo ""

echo "1b. List current traps (should be empty):"
echo 'trap' | ./builddir/lusush
echo ""

echo "=== TEST 2: Setting and listing traps ==="
echo "Testing trap setting and listing..."

cat > trap_test1.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Setting traps..."
trap 'echo "INT signal received"' INT
trap 'echo "TERM signal received"' TERM
trap 'echo "EXIT trap executed"' EXIT

echo "Current traps:"
trap

echo "Script running normally..."
sleep 1
echo "Script completed"
EOF

chmod +x trap_test1.sh
echo "Running: ./trap_test1.sh"
./trap_test1.sh
echo ""

echo "=== TEST 3: Signal handling with trap ==="
echo "Testing actual signal delivery and trap execution..."

cat > trap_signal_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "Cleanup on exit"; exit 0' EXIT
trap 'echo "Caught SIGINT - continuing..."; count=$((count + 1))' INT

count=0
echo "Running loop - send SIGINT to test trap (Ctrl+C)"
echo "Process PID: $$"

for i in 1 2 3 4 5; do
    echo "Iteration $i, interrupt count: $count"
    sleep 1
done

echo "Loop completed normally"
EOF

chmod +x trap_signal_test.sh
echo "Running: ./trap_signal_test.sh (will complete automatically)"
timeout 6 ./trap_signal_test.sh
echo ""

echo "=== TEST 4: Trap removal and reset ==="
echo "Testing trap removal with - and empty string..."

cat > trap_remove_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Setting initial traps..."
trap 'echo "INT handler 1"' INT
trap 'echo "TERM handler 1"' TERM

echo "Current traps:"
trap

echo "Removing INT trap with -:"
trap - INT

echo "Setting TERM to ignore with empty string:"
trap '' TERM

echo "Current traps after removal:"
trap

echo "Test completed"
EOF

chmod +x trap_remove_test.sh
echo "Running: ./trap_remove_test.sh"
./trap_remove_test.sh
echo ""

echo "=== TEST 5: EXIT trap testing ==="
echo "Testing EXIT trap execution on script termination..."

cat > trap_exit_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "EXIT trap: Cleaning up temporary files"; echo "EXIT trap: Script terminated"' EXIT

echo "Script starting..."
echo "Creating temporary data..."

# Simulate some work
for i in 1 2 3; do
    echo "Working... $i"
    sleep 0.5
done

echo "Script work completed"
# EXIT trap should execute here
EOF

chmod +x trap_exit_test.sh
echo "Running: ./trap_exit_test.sh"
./trap_exit_test.sh
echo ""

echo "=== TEST 6: Error handling and validation ==="
echo "Testing trap error conditions and argument validation..."

cat > trap_error_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing invalid signal names:"
trap 'echo "test"' INVALID_SIGNAL
echo "Exit code: $?"

echo "Testing missing arguments:"
trap
echo "Exit code: $?"

echo "Testing incomplete command:"
trap 'echo incomplete'
echo "Exit code: $?"

echo "Testing numeric signals:"
trap 'echo "Signal 2 (INT)"' 2
trap

echo "Error testing completed"
EOF

chmod +x trap_error_test.sh
echo "Running: ./trap_error_test.sh"
./trap_error_test.sh
echo ""

echo "=== TEST 7: Multiple signal traps ==="
echo "Testing multiple signals with same command..."

cat > trap_multiple_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Setting same command for multiple signals:"
trap 'echo "Termination signal received - cleaning up"' INT TERM QUIT

echo "Current traps:"
trap

echo "Testing with different commands:"
trap 'echo "User signal 1"' USR1
trap 'echo "User signal 2"' USR2

echo "All traps:"
trap

echo "Multiple trap test completed"
EOF

chmod +x trap_multiple_test.sh
echo "Running: ./trap_multiple_test.sh"
./trap_multiple_test.sh
echo ""

echo "=== TEST 8: Interactive trap testing ==="
echo "Testing trap in interactive mode..."

echo 'trap "echo \"Interactive INT trap\"" INT; echo "Trap set in interactive mode"; trap' | ./builddir/lusush
echo ""

echo "=== TEST 9: Command mode trap testing ==="
echo "Testing trap in -c command mode..."

./builddir/lusush -c 'trap "echo \"Command mode trap\"" EXIT; echo "Trap set"; trap; echo "Exiting"'
echo ""

echo "=== COMPARISON WITH BASH ==="
echo "Comparing trap behavior with bash..."

cat > bash_trap_test.sh << 'EOF'
#!/bin/bash
echo "BASH trap test:"
trap 'echo "BASH EXIT trap"' EXIT
trap 'echo "BASH INT trap"' INT
echo "Current BASH traps:"
trap
echo "BASH test completed"
EOF

cat > lusush_trap_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "LUSUSH trap test:"
trap 'echo "LUSUSH EXIT trap"' EXIT
trap 'echo "LUSUSH INT trap"' INT
echo "Current LUSUSH traps:"
trap
echo "LUSUSH test completed"
EOF

chmod +x bash_trap_test.sh lusush_trap_test.sh

echo ""
echo "Bash output:"
./bash_trap_test.sh
echo ""
echo "Lusush output:"
./lusush_trap_test.sh
echo ""

echo "=== POSIX COMPLIANCE VERIFICATION ==="
echo "Verifying POSIX-required trap behavior..."

cat > posix_trap_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

# POSIX Test 1: Basic trap setting and listing
echo "POSIX Test 1 - Basic functionality:"
trap 'echo "POSIX EXIT"' EXIT
trap 'echo "POSIX INT"' INT
trap

# POSIX Test 2: Signal name variations
echo "POSIX Test 2 - Signal variations:"
trap 'echo "Signal by number"' 2
trap 'echo "Signal by name"' TERM
trap

# POSIX Test 3: Trap removal
echo "POSIX Test 3 - Trap removal:"
trap - INT
trap '' TERM
trap

echo "POSIX compliance tests completed"
EOF

chmod +x posix_trap_test.sh
echo "Running POSIX compliance tests:"
./posix_trap_test.sh
echo ""

# Cleanup
rm -f trap_test1.sh trap_signal_test.sh trap_remove_test.sh trap_exit_test.sh
rm -f trap_error_test.sh trap_multiple_test.sh bash_trap_test.sh lusush_trap_test.sh
rm -f posix_trap_test.sh

echo "=== TRAP BUILT-IN COMMAND TEST COMPLETE ==="
echo ""
echo "Expected behavior:"
echo "- trap -l should list available signals"
echo "- trap without arguments should list current traps"
echo "- trap 'command' SIGNAL should set signal handler"
echo "- trap - SIGNAL should reset signal to default"
echo "- trap '' SIGNAL should ignore signal"
echo "- EXIT trap should execute when script exits"
echo "- Signal traps should execute when signals are received"
echo "- Multiple signals can use the same trap command"
echo "- Invalid signals should produce error messages"
