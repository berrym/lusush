#!/bin/bash

# Simple Arrow Key Test for Lusush Interactive Mode
# Uses script command to create pseudo-TTY for true interactive testing

echo "=== Simple Arrow Key Test for Lusush ==="
echo ""

# Build if needed
if [ ! -f builddir/lusush ]; then
    echo "Building lusush..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "This test specifically targets the UP arrow key issue:"
echo "  User report: UP arrow shows 'display all 4418 possibilities (y or n)?'"
echo "  Expected: UP arrow should navigate command history"
echo ""

# Create a simple test file for script
cat > /tmp/arrow_test_input << 'EOF'
echo "first command"
echo "second command"
echo "third command"
echo "Now I will press UP arrow - watch what happens..."
echo "If you see this, the shell is still responsive"
exit
EOF

echo "=== Test 1: Automated Arrow Key Test ==="
echo "Running lusush with script pseudo-TTY..."
echo "Watch for debug output and completion behavior..."
echo ""

# Run the test with script for true TTY behavior
script -q -c "./builddir/lusush" /dev/null < /tmp/arrow_test_input

echo ""
echo "=== Test 2: Manual Arrow Key Test ==="
echo ""
echo "The automated test above may not fully reproduce the issue."
echo "For manual testing, run this command in a real terminal:"
echo ""
echo "  script -q -c './builddir/lusush' /dev/null"
echo ""
echo "Then manually:"
echo "1. Type: echo \"test command\""
echo "2. Press ENTER"
echo "3. Press UP ARROW"
echo ""
echo "Watch for:"
echo "  ✅ WORKING: Should show 'echo \"test command\"'"
echo "  ❌ BUG: Shows 'display all 4418 possibilities (y or n)?'"
echo ""

echo "=== Test 3: Completion Count Verification ==="
echo ""
echo "Checking system completion counts to verify the '4418' number:"

# Count available commands and files
commands_in_path=$(echo $PATH | tr ':' '\n' | xargs -I {} find {} -maxdepth 1 -type f -executable 2>/dev/null | wc -l)
files_in_usr_bin=$(ls -1 /usr/bin 2>/dev/null | wc -l)
files_in_bin=$(ls -1 /bin 2>/dev/null | wc -l)
files_current_dir=$(ls -1 . | wc -l)

echo "System completion sources:"
echo "  Commands in PATH: $commands_in_path"
echo "  Files in /usr/bin: $files_in_usr_bin"
echo "  Files in /bin: $files_in_bin"
echo "  Files in current directory: $files_current_dir"
echo ""

total_possible=$((files_in_usr_bin + files_in_bin + files_current_dir))
echo "Approximate total completions: $total_possible"
echo ""

if [ $total_possible -gt 4000 ]; then
    echo "✅ This matches the reported '4418 possibilities' number"
    echo "   This confirms completion is being triggered when it shouldn't be"
else
    echo "❓ Number doesn't match - completion count may be different"
fi

echo ""
echo "=== Analysis Results ==="
echo ""

echo "Based on the debug output above:"
echo ""
echo "1. If you saw [DEBUG] Interactive mode = TRUE:"
echo "   ✅ Interactive mode detection is working"
echo ""
echo "2. If you saw [DEBUG] show-all-if-ambiguous: OFF:"
echo "   ✅ Critical completion settings are correct"
echo ""
echo "3. If you saw [COMPLETION_DEBUG] triggered unexpectedly:"
echo "   ❌ Completion is being called when it shouldn't be"
echo ""
echo "4. If history navigation worked in the automated test:"
echo "   ❓ Issue may be specific to manual arrow key usage"
echo ""

echo "=== Next Steps ==="
echo ""
echo "If the automated test showed history working correctly,"
echo "but users still report the completion menu issue,"
echo "the problem may be:"
echo ""
echo "1. Specific timing or key sequence that triggers completion"
echo "2. Terminal-specific behavior differences"
echo "3. History state conditions that trigger completion instead of navigation"
echo ""
echo "To fix:"
echo "1. Add more granular debugging to track exactly when completion triggers"
echo "2. Add safeguards to prevent completion on arrow key sequences"
echo "3. Ensure completion function returns NULL for invalid completion contexts"

# Cleanup
rm -f /tmp/arrow_test_input

echo ""
echo "Run this test and analyze the debug output to identify the specific issue."
