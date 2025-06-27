#!/bin/bash

# Test script for POSIX-compliant shell type detection
# This script verifies that the shell correctly identifies different shell types

echo "=== LUSUSH POSIX SHELL TYPE DETECTION TEST ==="
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

# Create test scripts
cat > interactive_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "This is a script execution"
echo "Script name: $0"
echo "Argument 1: $1"
echo "Shell PID: $$"
EOF

cat > login_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "Login shell test"
echo "Script: $0"
echo "PID: $$"
EOF

chmod +x interactive_test.sh login_test.sh

echo "=== TEST 1: Interactive shell (terminal) ==="
echo "This should use interactive mode with linenoise"
echo "Running: echo 'echo \"Interactive: \$\$\"' | ./builddir/lusush"
echo 'echo "Interactive: $$"' | ./builddir/lusush
echo ""

echo "=== TEST 2: Non-interactive shell (script file) ==="
echo "This should use non-interactive mode"
echo "Running: ./builddir/lusush interactive_test.sh hello world"
./builddir/lusush interactive_test.sh hello world
echo ""

echo "=== TEST 3: Non-interactive shell (-c command) ==="
echo "This should use non-interactive mode"
echo "Running: ./builddir/lusush -c 'echo \"Command mode: \$\$\"'"
./builddir/lusush -c 'echo "Command mode: $$"'
echo ""

echo "=== TEST 4: Forced interactive (-i flag) ==="
echo "This should force interactive mode even with piped input"
echo "Running: echo 'echo \"Forced interactive: \$\$\"' | ./builddir/lusush -i"
echo 'echo "Forced interactive: $$"' | ./builddir/lusush -i
echo ""

echo "=== TEST 5: Login shell (-l flag) ==="
echo "This should detect login shell behavior"
echo "Running: ./builddir/lusush -l -c 'echo \"Login shell test: \$\$\"'"
./builddir/lusush -l -c 'echo "Login shell test: $$"'
echo ""

echo "=== TEST 6: Login shell (argv[0] starts with -) ==="
echo "This should detect login shell behavior"
echo "Creating symlink to test login shell detection..."
ln -sf builddir/lusush -lusush 2>/dev/null || echo "Symlink creation failed (expected in some environments)"
if [ -e "-lusush" ]; then
    echo "Running: ./-lusush -c 'echo \"Login via argv[0]: \$\$\"'"
    ./-lusush -c 'echo "Login via argv[0]: $$"'
    rm -f -lusush
else
    echo "Skipping argv[0] login test (symlink not supported)"
fi
echo ""

echo "=== TEST 7: Stdin mode (-s flag) ==="
echo "This should use non-interactive mode"
echo "Running: echo 'echo \"Stdin mode: \$\$\"' | ./builddir/lusush -s"
echo 'echo "Stdin mode: $$"' | ./builddir/lusush -s
echo ""

echo "=== TEST 8: Variable expansion in different modes ==="
echo "Testing variable expansion consistency across shell types"
echo ""

echo "8a. Interactive mode:"
echo 'x=test; echo "Value: $x"' | ./builddir/lusush

echo ""
echo "8b. Script mode:"
echo '#!/usr/bin/env ./builddir/lusush
x=test
echo "Value: $x"' > var_test.sh
chmod +x var_test.sh
./builddir/lusush var_test.sh

echo ""
echo "8c. Command mode:"
./builddir/lusush -c 'x=test; echo "Value: $x"'

echo ""
echo "=== TEST 9: Positional parameters ==="
echo "Testing positional parameters in script execution"
echo ""

echo "Running: ./builddir/lusush interactive_test.sh arg1 arg2 arg3"
./builddir/lusush interactive_test.sh arg1 arg2 arg3

echo ""
echo "=== TEST 10: Special variables consistency ==="
echo "Testing special variables across different shell types"
echo ""

echo "10a. Exit status (\$?):"
echo 'false; echo "Exit status: $?"' | ./builddir/lusush

echo ""
echo "10b. Process ID (\$\$):"
echo 'echo "Process ID: $$"' | ./builddir/lusush

echo ""
echo "10c. Argument count (\$#) in script:"
echo 'echo "Argument count: $#"
echo "All args: $*"' > args_test.sh
chmod +x args_test.sh
./builddir/lusush args_test.sh one two three

echo ""
echo "=== TEST 11: Shebang execution ==="
echo "Testing shebang script execution"
echo ""

echo "Running: ./interactive_test.sh direct_arg1 direct_arg2"
./interactive_test.sh direct_arg1 direct_arg2

echo ""
echo "=== COMPARISON WITH STANDARD SHELL ==="
echo "Comparing behavior with bash for reference"
echo ""

cat > bash_comparison.sh << 'EOF'
#!/bin/bash
echo "BASH comparison:"
echo "Script: $0"
echo "Args: $# arguments"
echo "Arg1: $1"
echo "Arg2: $2"
echo "PID: $$"
echo "Exit status: $?"
EOF

chmod +x bash_comparison.sh
echo "Bash output:"
./bash_comparison.sh comp_arg1 comp_arg2

echo ""
echo "Lusush output:"
cat > lusush_comparison.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "LUSUSH comparison:"
echo "Script: $0"
echo "Args: $# arguments"
echo "Arg1: $1"
echo "Arg2: $2"
echo "PID: $$"
echo "Exit status: $?"
EOF

chmod +x lusush_comparison.sh
./lusush_comparison.sh comp_arg1 comp_arg2

# Cleanup
rm -f interactive_test.sh login_test.sh var_test.sh args_test.sh
rm -f bash_comparison.sh lusush_comparison.sh -lusush

echo ""
echo "=== POSIX SHELL TYPE DETECTION TEST COMPLETE ==="
echo ""
echo "Expected behavior:"
echo "- Interactive mode should handle prompts and history"
echo "- Script mode should process files with proper argument handling"
echo "- Command mode (-c) should execute single commands"
echo "- Login shell detection should work with -l flag or argv[0] starting with -"
echo "- Variable expansion should work consistently across all modes"
echo "- Positional parameters should work correctly in scripts"
