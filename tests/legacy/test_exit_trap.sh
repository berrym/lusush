#!/bin/bash

# Specific test for EXIT trap integration
# Tests that EXIT traps execute properly when scripts terminate

echo "=== EXIT TRAP INTEGRATION TEST ==="
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

echo "=== TEST 1: EXIT trap on normal script completion ==="
cat > exit_trap_normal.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "EXIT trap executed - script completed normally"' EXIT

echo "Script starting..."
echo "Doing some work..."
sleep 0.5
echo "Script ending normally..."
# EXIT trap should execute here
EOF

chmod +x exit_trap_normal.sh
echo "Running: ./exit_trap_normal.sh"
./exit_trap_normal.sh
echo ""

echo "=== TEST 2: EXIT trap with explicit exit command ==="
cat > exit_trap_explicit.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "EXIT trap executed - explicit exit called"' EXIT

echo "Script starting..."
echo "About to call exit explicitly..."
exit 0
echo "This should not be printed"
EOF

chmod +x exit_trap_explicit.sh
echo "Running: ./exit_trap_explicit.sh"
./exit_trap_explicit.sh
echo ""

echo "=== TEST 3: EXIT trap with exit code ==="
cat > exit_trap_code.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "EXIT trap executed - exit code will be preserved"' EXIT

echo "Script starting..."
echo "Exiting with code 42..."
exit 42
EOF

chmod +x exit_trap_code.sh
echo "Running: ./exit_trap_code.sh"
./exit_trap_code.sh
echo "Exit code was: $?"
echo ""

echo "=== TEST 4: Multiple EXIT traps (last one wins) ==="
cat > exit_trap_multiple.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "First EXIT trap"' EXIT
trap 'echo "Second EXIT trap (should override first)"' EXIT

echo "Script with multiple EXIT traps..."
EOF

chmod +x exit_trap_multiple.sh
echo "Running: ./exit_trap_multiple.sh"
./exit_trap_multiple.sh
echo ""

echo "=== TEST 5: EXIT trap removal ==="
cat > exit_trap_removal.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "This EXIT trap should be removed"' EXIT
echo "EXIT trap set..."

trap - EXIT
echo "EXIT trap removed with 'trap - EXIT'"
echo "Script ending - no EXIT trap should execute"
EOF

chmod +x exit_trap_removal.sh
echo "Running: ./exit_trap_removal.sh"
./exit_trap_removal.sh
echo ""

echo "=== TEST 6: EXIT trap with cleanup operations ==="
cat > exit_trap_cleanup.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "Cleanup: removing temporary files"; rm -f /tmp/lusush_test_$$; echo "Cleanup complete"' EXIT

echo "Creating temporary file..."
touch /tmp/lusush_test_$$
echo "Temporary file created: /tmp/lusush_test_$$"
ls -l /tmp/lusush_test_$$ 2>/dev/null && echo "File exists before exit"
echo "Script ending - cleanup should happen automatically"
EOF

chmod +x exit_trap_cleanup.sh
echo "Running: ./exit_trap_cleanup.sh"
./exit_trap_cleanup.sh
echo ""

echo "=== TEST 7: EXIT trap in command mode ==="
echo "Testing EXIT trap with -c option..."
./builddir/lusush -c 'trap "echo \"EXIT trap in command mode\"" EXIT; echo "Command executed"; exit 0'
echo ""

echo "=== TEST 8: EXIT trap in interactive mode ==="
echo "Testing EXIT trap in interactive mode..."
echo 'trap "echo \"EXIT trap in interactive mode\"" EXIT; exit' | ./builddir/lusush
echo ""

echo "=== TEST 9: EXIT trap with variable access ==="
cat > exit_trap_variables.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

script_name="exit_trap_variables.sh"
start_time=$(date)

trap 'echo "EXIT trap: Script $script_name completed"; echo "EXIT trap: Started at $start_time"' EXIT

echo "Script starting with variable access..."
echo "Script will demonstrate variable access in EXIT trap"
EOF

chmod +x exit_trap_variables.sh
echo "Running: ./exit_trap_variables.sh"
./exit_trap_variables.sh
echo ""

echo "=== TEST 10: Nested script with EXIT trap ==="
cat > nested_parent.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "Parent script EXIT trap"' EXIT

echo "Parent script starting..."
echo "Calling nested script..."
./nested_child.sh
echo "Parent script ending..."
EOF

cat > nested_child.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

trap 'echo "Child script EXIT trap"' EXIT

echo "Child script running..."
echo "Child script completing..."
EOF

chmod +x nested_parent.sh nested_child.sh
echo "Running: ./nested_parent.sh"
./nested_parent.sh
echo ""

# Compare with bash behavior
echo "=== COMPARISON WITH BASH ==="
cat > bash_exit_trap.sh << 'EOF'
#!/bin/bash
trap 'echo "BASH EXIT trap executed"' EXIT
echo "BASH script running..."
exit 0
EOF

cat > lusush_exit_trap.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
trap 'echo "LUSUSH EXIT trap executed"' EXIT
echo "LUSUSH script running..."
exit 0
EOF

chmod +x bash_exit_trap.sh lusush_exit_trap.sh

echo "Bash behavior:"
./bash_exit_trap.sh
echo ""

echo "Lusush behavior:"
./lusush_exit_trap.sh
echo ""

# Cleanup
rm -f exit_trap_normal.sh exit_trap_explicit.sh exit_trap_code.sh
rm -f exit_trap_multiple.sh exit_trap_removal.sh exit_trap_cleanup.sh
rm -f exit_trap_variables.sh nested_parent.sh nested_child.sh
rm -f bash_exit_trap.sh lusush_exit_trap.sh

echo "=== EXIT TRAP INTEGRATION TEST COMPLETE ==="
echo ""
echo "Expected behavior:"
echo "- EXIT traps should execute when scripts complete normally"
echo "- EXIT traps should execute when 'exit' command is called"
echo "- EXIT code should be preserved after EXIT trap execution"
echo "- Multiple EXIT traps should override previous ones"
echo "- 'trap - EXIT' should remove EXIT traps"
echo "- EXIT traps should have access to script variables"
echo "- Each script should have its own EXIT trap context"
