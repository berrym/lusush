#!/bin/bash

# Debug script to understand how arguments are being processed

echo "=== LUSUSH ARGUMENT PROCESSING DEBUG ==="
echo ""

# Build the shell
echo "Building lusush..."
cd builddir && ninja && cd ..

echo ""
echo "=== DEBUG 1: Direct shell execution with -c ==="
echo "Command: ./builddir/lusush -c 'echo \"Args: \$# - \$0 - \$1 - \$2\"'"
./builddir/lusush -c 'echo "Args: $# - $0 - $1 - $2"'

echo ""
echo "=== DEBUG 2: Check if variables are expanded at all ==="
echo "Command: echo 'echo \"PID: \$\$\"' | ./builddir/lusush"
echo 'echo "PID: $$"' | ./builddir/lusush

echo ""
echo "=== DEBUG 3: Check script invocation directly ==="
cat > debug_script.sh << 'EOF'
echo "Script: $0"
echo "Args: $#"
echo "Arg1: $1"
echo "Arg2: $2"
echo "PID: $$"
echo "Exit: $?"
EOF

echo "Command: ./builddir/lusush debug_script.sh arg1 arg2"
./builddir/lusush debug_script.sh arg1 arg2

echo ""
echo "=== DEBUG 4: Check what argc/argv look like ==="
echo "Creating C debug program..."
cat > debug_args.c << 'EOF'
#include <stdio.h>
int main(int argc, char **argv) {
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    return 0;
}
EOF

gcc -o debug_args debug_args.c
echo "Command: ./debug_args hello world"
./debug_args hello world

echo ""
echo "=== DEBUG 5: Test shebang directly ==="
cat > shebang_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "In shebang script"
echo "Script: $0"
echo "Args: $# arguments"
echo "Arg1: $1"
echo "PID: $$"
EOF

chmod +x shebang_test.sh
echo "Command: ./shebang_test.sh hello world"
./shebang_test.sh hello world

echo ""
echo "=== DEBUG 6: Compare with bash ==="
cat > bash_test.sh << 'EOF'
#!/bin/bash
echo "BASH VERSION:"
echo "Script: $0"
echo "Args: $# arguments"
echo "Arg1: $1"
echo "Arg2: $2"
echo "PID: $$"
EOF

chmod +x bash_test.sh
echo "Command: ./bash_test.sh hello world"
./bash_test.sh hello world

# Cleanup
rm -f debug_script.sh debug_args.c debug_args shebang_test.sh bash_test.sh

echo ""
echo "=== ARGUMENT PROCESSING DEBUG COMPLETE ==="
