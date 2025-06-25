#!/bin/bash

# Comprehensive file test operator test script for lusush shell
# Tests various file test operators to diagnose Test 85 failure

echo "=== File Test Operators Comprehensive Test ==="

# Create test files and directories for testing
mkdir -p /tmp/lusush_test
cd /tmp/lusush_test

# Create test files
echo "content" > regular_file.txt
ln -sf regular_file.txt symlink_file
mkfifo named_pipe 2>/dev/null || echo "mkfifo not available"
mkdir test_directory

echo "Test files created in /tmp/lusush_test"
echo ""

# Test 1: Regular file test (-f)
echo "Test 1: Regular file test (-f)"
echo "Testing with regular file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -f regular_file.txt ] && echo "is regular file" || echo "not regular file"'
echo -n "Bash:   "
bash -c '[ -f regular_file.txt ] && echo "is regular file" || echo "not regular file"'
echo ""

# Test 2: Regular file test with /dev/null
echo "Test 2: Regular file test with /dev/null (-f)"
echo -n "Lusush: "
../../builddir/lusush -c '[ -f /dev/null ] && echo "is regular file" || echo "not regular file"'
echo -n "Bash:   "
bash -c '[ -f /dev/null ] && echo "is regular file" || echo "not regular file"'
echo ""

# Test 3: File existence test (-e)
echo "Test 3: File existence test (-e)"
echo "Testing with regular file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -e regular_file.txt ] && echo "exists" || echo "does not exist"'
echo -n "Bash:   "
bash -c '[ -e regular_file.txt ] && echo "exists" || echo "does not exist"'
echo ""

# Test 4: Existence test with /dev/null
echo "Test 4: Existence test with /dev/null (-e)"
echo -n "Lusush: "
../../builddir/lusush -c '[ -e /dev/null ] && echo "exists" || echo "does not exist"'
echo -n "Bash:   "
bash -c '[ -e /dev/null ] && echo "exists" || echo "does not exist"'
echo ""

# Test 5: Directory test (-d)
echo "Test 5: Directory test (-d)"
echo "Testing with directory:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -d test_directory ] && echo "is directory" || echo "not directory"'
echo -n "Bash:   "
bash -c '[ -d test_directory ] && echo "is directory" || echo "not directory"'
echo ""

# Test 6: Directory test with /dev/null
echo "Test 6: Directory test with /dev/null (-d)"
echo -n "Lusush: "
../../builddir/lusush -c '[ -d /dev/null ] && echo "is directory" || echo "not directory"'
echo -n "Bash:   "
bash -c '[ -d /dev/null ] && echo "is directory" || echo "not directory"'
echo ""

# Test 7: Character device test (-c)
echo "Test 7: Character device test (-c)"
echo "Testing with /dev/null:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -c /dev/null ] && echo "is char device" || echo "not char device"'
echo -n "Bash:   "
bash -c '[ -c /dev/null ] && echo "is char device" || echo "not char device"'
echo ""

# Test 8: Symbolic link test (-L or -h)
echo "Test 8: Symbolic link test (-L)"
echo "Testing with symlink:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -L symlink_file ] && echo "is symlink" || echo "not symlink"'
echo -n "Bash:   "
bash -c '[ -L symlink_file ] && echo "is symlink" || echo "not symlink"'
echo ""

# Test 9: Readable test (-r)
echo "Test 9: Readable test (-r)"
echo "Testing with regular file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -r regular_file.txt ] && echo "is readable" || echo "not readable"'
echo -n "Bash:   "
bash -c '[ -r regular_file.txt ] && echo "is readable" || echo "not readable"'
echo ""

# Test 10: Writable test (-w)
echo "Test 10: Writable test (-w)"
echo "Testing with regular file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -w regular_file.txt ] && echo "is writable" || echo "not writable"'
echo -n "Bash:   "
bash -c '[ -w regular_file.txt ] && echo "is writable" || echo "not writable"'
echo ""

# Test 11: Executable test (-x)
echo "Test 11: Executable test (-x)"
chmod +x regular_file.txt
echo "Testing with executable file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -x regular_file.txt ] && echo "is executable" || echo "not executable"'
echo -n "Bash:   "
bash -c '[ -x regular_file.txt ] && echo "is executable" || echo "not executable"'
echo ""

# Test 12: Non-empty file test (-s)
echo "Test 12: Non-empty file test (-s)"
echo "Testing with non-empty file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -s regular_file.txt ] && echo "is non-empty" || echo "is empty"'
echo -n "Bash:   "
bash -c '[ -s regular_file.txt ] && echo "is non-empty" || echo "is empty"'
echo ""

# Test 13: Test with non-existent file
echo "Test 13: Tests with non-existent file"
echo "Testing -f with non-existent file:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -f nonexistent.txt ] && echo "exists" || echo "does not exist"'
echo -n "Bash:   "
bash -c '[ -f nonexistent.txt ] && echo "exists" || echo "does not exist"'
echo ""

# Test 14: The exact failing test case
echo "Test 14: Exact failing test case from comprehensive test"
echo "Command: if [ -f /dev/null ]; then echo exists; fi"
echo -n "Lusush: "
../../builddir/lusush -c 'if [ -f /dev/null ]; then echo exists; fi'
echo -n "Bash:   "
bash -c 'if [ -f /dev/null ]; then echo exists; fi'
echo "(Expected: no output from either, because /dev/null is not a regular file)"
echo ""

# Test 15: Alternative test that should work
echo "Test 15: Alternative test using -e instead of -f"
echo "Command: if [ -e /dev/null ]; then echo exists; fi"
echo -n "Lusush: "
../../builddir/lusush -c 'if [ -e /dev/null ]; then echo exists; fi'
echo -n "Bash:   "
bash -c 'if [ -e /dev/null ]; then echo exists; fi'
echo ""

# Test 16: Test builtin command directly
echo "Test 16: Test builtin command directly"
echo "Testing [ -f /dev/null ] exit code:"
../../builddir/lusush -c '[ -f /dev/null ]; echo "Exit code: $?"'
bash -c '[ -f /dev/null ]; echo "Bash exit code: $?"'
echo ""

# Test 17: Test with standard files
echo "Test 17: Test with standard system files"
echo "Testing [ -f /etc/passwd ]:"
echo -n "Lusush: "
../../builddir/lusush -c '[ -f /etc/passwd ] && echo "is regular file" || echo "not regular file or does not exist"'
echo -n "Bash:   "
bash -c '[ -f /etc/passwd ] && echo "is regular file" || echo "not regular file or does not exist"'
echo ""

# Cleanup
cd /tmp
rm -rf lusush_test

echo "=== File Test Operators Test Complete ==="
echo ""
echo "Summary:"
echo "- If -f operator works correctly with regular files, the implementation is good"
echo "- The Test 85 failure might be due to incorrect test expectation"
echo "- /dev/null is a character device, not a regular file, so [ -f /dev/null ] should return false"
echo "- Use [ -e /dev/null ] to test for existence of any file type"
