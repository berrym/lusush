#!/bin/bash

# Simple debug test to verify variable expansion is working in lusush

echo "=== LUSUSH VARIABLE EXPANSION DEBUG ==="
echo ""

# Build the shell
echo "Building lusush..."
cd builddir && ninja && cd ..
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "=== TEST 1: Simple variable expansion ==="
echo "Command: echo 'x=hello; echo \$x' | ./builddir/lusush"
echo 'x=hello; echo $x' | ./builddir/lusush

echo ""
echo "=== TEST 2: Special variable expansion ==="
echo "Command: echo 'echo \$\$' | ./builddir/lusush"
echo 'echo $$' | ./builddir/lusush

echo ""
echo "=== TEST 3: Exit status variable ==="
echo "Command: echo 'false; echo \$?' | ./builddir/lusush"
echo 'false; echo $?' | ./builddir/lusush

echo ""
echo "=== TEST 4: Variable in double quotes ==="
echo "Command: echo 'x=world; echo \"hello \$x\"' | ./builddir/lusush"
echo 'x=world; echo "hello $x"' | ./builddir/lusush

echo ""
echo "=== TEST 5: Check if modern executor is being used ==="
echo "Command: echo 'echo \"Testing modern executor\"' | ./builddir/lusush"
echo 'echo "Testing modern executor"' | ./builddir/lusush

echo ""
echo "=== TEST 6: Parameter expansion syntax ==="
echo "Command: echo 'x=test; echo \${x}' | ./builddir/lusush"
echo 'x=test; echo ${x}' | ./builddir/lusush

echo ""
echo "=== TEST 7: Compare with direct variable access ==="
echo "Creating simple C test to check shell variables..."
cat > var_test.c << 'EOF'
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

// Declare external variables
extern int shell_argc;
extern char **shell_argv;
extern pid_t shell_pid;
extern int last_exit_status;

int main() {
    printf("C test - shell_argc: %d\n", shell_argc);
    if (shell_argv && shell_argc > 0) {
        printf("C test - shell_argv[0]: %s\n", shell_argv[0]);
    }
    printf("C test - shell_pid: %d\n", (int)shell_pid);
    printf("C test - last_exit_status: %d\n", last_exit_status);
    return 0;
}
EOF

echo "Compiling C test..."
# gcc -o var_test var_test.c builddir/lusush.p/src_globals.c.o 2>/dev/null
echo "(Skipping C test compilation due to linking complexity)"

echo ""
echo "=== EXPANSION DEBUG COMPLETE ==="

# Cleanup
rm -f var_test.c var_test
