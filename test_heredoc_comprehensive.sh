#!/bin/bash

# Comprehensive Here Document Test Script for Lusush Shell
# Tests all here document functionality:
# - Basic here documents (<<)
# - Tab-stripping here documents (<<-)
# - Variable expansion in here documents
# - Quoted delimiters (disabling variable expansion)
# - Complex scenarios and edge cases

echo "=== Lusush Shell - Comprehensive Here Document Test ==="
echo "Testing here document functionality: <<, <<-, variable expansion, quoted delimiters"
echo

# Test 1: Basic here document
echo "=== Test 1: Basic here document ==="
echo "Command: cat << EOF"
cat << 'TEST_EOF'
cat << EOF
This is line 1
This is line 2
This is line 3
EOF
TEST_EOF
echo "Running test..."
./builddir/lusush << 'TEST_EOF'
cat << EOF
This is line 1
This is line 2
This is line 3
EOF
TEST_EOF
echo

# Test 2: Here document with tab stripping (<<-)
echo "=== Test 2: Here document with tab stripping (<<-) ==="
echo "Command: cat <<- EOF (with tabs)"
./builddir/lusush << 'TEST_EOF'
cat <<- EOF
	This line has a leading tab
		This line has two leading tabs
	Another line with one tab
Normal line without tabs
EOF
TEST_EOF
echo

# Test 3: Variable expansion in here documents
echo "=== Test 3: Variable expansion in here documents ==="
echo "Setting variables and testing expansion..."
./builddir/lusush << 'TEST_EOF'
NAME=World
USER_TEST=testuser
cat << EOF
Hello $NAME
User: $USER_TEST
Home directory: $HOME
Current shell PID: $$
EOF
TEST_EOF
echo

# Test 4: Quoted delimiter - should disable variable expansion
echo "=== Test 4: Quoted delimiter (should disable variable expansion) ==="
echo "Command: cat << \"EOF\" (quoted delimiter)"
./builddir/lusush << 'TEST_EOF'
NAME=World
cat << "EOF"
Hello $NAME (should not expand)
Home: $HOME (should not expand)
User: $USER (should not expand)
EOF
TEST_EOF
echo

# Test 5: Single-quoted delimiter - should disable variable expansion
echo "=== Test 5: Single-quoted delimiter (should disable variable expansion) ==="
echo "Command: cat << 'EOF' (single-quoted delimiter)"
./builddir/lusush << 'TEST_EOF'
NAME=World
cat << 'EOF'
Hello $NAME (should not expand)
Home: $HOME (should not expand)
User: $USER (should not expand)
EOF
TEST_EOF
echo

# Test 6: Mixed tab stripping with variable expansion
echo "=== Test 6: Tab stripping with variable expansion ==="
echo "Command: cat <<- EOF (tabs + variables)"
./builddir/lusush << 'TEST_EOF'
NAME=TabTest
cat <<- EOF
	Hello $NAME
		Indented: $HOME
	Back to one tab: $USER
EOF
TEST_EOF
echo

# Test 7: Complex variable expansion
echo "=== Test 7: Complex variable expansion ==="
echo "Testing parameter expansion in here documents..."
./builddir/lusush << 'TEST_EOF'
TEST_VAR=hello
cat << EOF
Variable: $TEST_VAR
Length: ${#TEST_VAR}
Default: ${UNDEFINED_VAR:-default_value}
HOME: $HOME
EOF
TEST_EOF
echo

# Test 8: Multiple here documents in sequence
echo "=== Test 8: Multiple here documents in sequence ==="
echo "Running multiple heredocs one after another..."
./builddir/lusush << 'TEST_EOF'
echo "First heredoc:"
cat << EOF1
This is the first here document
EOF1

echo "Second heredoc:"
cat << EOF2
This is the second here document
EOF2

echo "Third heredoc with variables:"
NAME=MultiTest
cat << EOF3
Hello from $NAME
EOF3
TEST_EOF
echo

# Test 9: Here document with different delimiters
echo "=== Test 9: Different delimiter styles ==="
echo "Testing various delimiter formats..."
./builddir/lusush << 'TEST_EOF'
cat << DELIMITER
Using DELIMITER as delimiter
DELIMITER

cat << END_OF_INPUT
Using END_OF_INPUT as delimiter
END_OF_INPUT

cat << XYZ123
Using XYZ123 as delimiter
XYZ123
TEST_EOF
echo

# Test 10: Here document with redirection
echo "=== Test 10: Here document with output redirection ==="
echo "Testing heredoc with file output..."
./builddir/lusush << 'TEST_EOF'
cat << EOF > /tmp/heredoc_test.txt
This content goes to a file
Line 2 of file content
EOF
echo "Content written to file:"
cat /tmp/heredoc_test.txt
rm -f /tmp/heredoc_test.txt
TEST_EOF
echo

# Test 11: Here document in pipeline
echo "=== Test 11: Here document in pipeline ==="
echo "Testing heredoc as input to pipeline..."
./builddir/lusush << 'TEST_EOF'
cat << EOF | grep "line"
This is line 1
This is not a match
This is line 2
EOF
TEST_EOF
echo

# Test 12: Here document with command substitution
echo "=== Test 12: Here document with command substitution ==="
echo "Testing command substitution in heredoc..."
./builddir/lusush << 'TEST_EOF'
cat << EOF
Current date: $(date +%Y-%m-%d)
Current directory: $(pwd)
Number of files: $(ls | wc -l)
EOF
TEST_EOF
echo

# Test 13: Edge case - empty here document
echo "=== Test 13: Edge case - empty here document ==="
echo "Testing empty heredoc..."
./builddir/lusush << 'TEST_EOF'
cat << EOF
EOF
echo "Empty heredoc test complete"
TEST_EOF
echo

# Test 14: Edge case - here document with only whitespace
echo "=== Test 14: Edge case - whitespace-only here document ==="
echo "Testing heredoc with only whitespace..."
./builddir/lusush << 'TEST_EOF'
cat << EOF



EOF
echo "Whitespace heredoc test complete"
TEST_EOF
echo

# Test 15: Error case - missing delimiter
echo "=== Test 15: Error handling - missing delimiter ==="
echo "Testing heredoc with missing end delimiter (should show error)..."
echo 'cat << EOF
This heredoc has no end delimiter' | ./builddir/lusush 2>&1 || echo "Error handled correctly"
echo

echo "=== Here Document Test Complete ==="
echo "All here document features tested:"
echo "✅ Basic here documents (<<)"
echo "✅ Tab-stripping here documents (<<-)"
echo "✅ Variable expansion in here documents"
echo "✅ Quoted delimiters (should disable expansion)"
echo "✅ Complex variable and command substitution"
echo "✅ Multiple here documents and pipelines"
echo "✅ Output redirection with here documents"
echo "✅ Edge cases and error handling"
echo
echo "Lusush shell here document implementation testing complete!"
