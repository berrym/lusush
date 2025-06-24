#!/bin/bash

# Comprehensive test suite for tilde expansion functionality
# Tests POSIX-compliant tilde expansion for path handling

echo "=== LUSUSH TILDE EXPANSION TEST ==="
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

echo "=== TEST 1: Basic tilde expansion ==="
echo "Testing simple ~ expansion to HOME..."

cat > tilde_test1.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing basic tilde expansion:"
echo "HOME directory: ~"
echo "HOME with path: ~/Documents"
echo "Current HOME: $HOME"
EOF

chmod +x tilde_test1.sh
echo "Running: ./tilde_test1.sh"
./tilde_test1.sh
echo ""

echo "=== TEST 2: Tilde expansion in commands ==="
echo "Testing tilde expansion in command arguments..."

cat > tilde_test2.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing tilde in commands:"
ls -la ~ | head -3
echo "Listing ~/.*rc files:"
ls ~/.bashrc ~/.profile 2>/dev/null || echo "RC files not found"
EOF

chmod +x tilde_test2.sh
echo "Running: ./tilde_test2.sh"
./tilde_test2.sh
echo ""

echo "=== TEST 3: Tilde expansion with user names ==="
echo "Testing ~user expansion..."

cat > tilde_test3.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing user tilde expansion:"
echo "Root home: ~root"
echo "Non-existent user: ~nonexistentuser12345"
echo "Current user explicit: ~$USER"
EOF

chmod +x tilde_test3.sh
echo "Running: ./tilde_test3.sh"
./tilde_test3.sh
echo ""

echo "=== TEST 4: Tilde expansion in different contexts ==="
echo "Testing tilde expansion in various contexts..."

cat > tilde_test4.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing tilde in different contexts:"

# Variable assignment
HOME_DIR=~
echo "Variable assignment: HOME_DIR=$HOME_DIR"

# Command substitution
TILDE_RESULT=$(echo ~)
echo "Command substitution: $TILDE_RESULT"

# In quotes (should not expand in single quotes)
echo "Single quotes: '~'"
echo "Double quotes: \"~\""
EOF

chmod +x tilde_test4.sh
echo "Running: ./tilde_test4.sh"
./tilde_test4.sh
echo ""

echo "=== TEST 5: Tilde expansion with paths ==="
echo "Testing tilde expansion with various path constructions..."

cat > tilde_test5.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing tilde with paths:"
echo "Home subdirectory: ~/bin"
echo "Deep path: ~/Documents/Projects/test"
echo "Hidden directory: ~/.config"
echo "Multiple tildes: ~ and ~/tmp"
EOF

chmod +x tilde_test5.sh
echo "Running: ./tilde_test5.sh"
./tilde_test5.sh
echo ""

echo "=== TEST 6: Tilde expansion in interactive mode ==="
echo "Testing tilde expansion in interactive mode..."

echo 'echo "Interactive tilde test: ~"' | ./builddir/lusush
echo 'echo "Interactive path: ~/test"' | ./builddir/lusush
echo ""

echo "=== TEST 7: Tilde expansion in command mode ==="
echo "Testing tilde expansion in -c command mode..."

./builddir/lusush -c 'echo "Command mode tilde: ~"'
./builddir/lusush -c 'echo "Command mode path: ~/Documents"'
echo ""

echo "=== TEST 8: Tilde expansion edge cases ==="
echo "Testing edge cases and error conditions..."

cat > tilde_test8.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing edge cases:"
echo "Just tilde: ~"
echo "Tilde with space: ~ /tmp"
echo "Multiple words with tilde: ls ~ /tmp"
echo "Tilde in middle: /tmp/~/test"
echo "No tilde: /home/user/test"
EOF

chmod +x tilde_test8.sh
echo "Running: ./tilde_test8.sh"
./tilde_test8.sh
echo ""

echo "=== TEST 9: Tilde expansion with redirection ==="
echo "Testing tilde expansion in redirection contexts..."

cat > tilde_test9.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Testing tilde with redirection:"
echo "Creating test file in home..."
echo "Test content" > ~/tilde_test_file.txt
echo "Reading from home file:"
cat ~/tilde_test_file.txt
echo "Cleaning up..."
rm -f ~/tilde_test_file.txt
EOF

chmod +x tilde_test9.sh
echo "Running: ./tilde_test9.sh"
./tilde_test9.sh
echo ""

echo "=== TEST 10: Tilde with environment variables ==="
echo "Testing tilde expansion combined with variables..."

cat > tilde_test10.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

export SUBDIR="Documents"
echo "Testing tilde with variables:"
echo "Home: ~"
echo "Variable: $SUBDIR"
echo "Combined: ~/$SUBDIR"
echo "User with var: ~$USER"
EOF

chmod +x tilde_test10.sh
echo "Running: ./tilde_test10.sh"
./tilde_test10.sh
echo ""

echo "=== COMPARISON WITH BASH ==="
echo "Comparing tilde expansion behavior with bash..."

cat > bash_tilde_test.sh << 'EOF'
#!/bin/bash
echo "BASH tilde expansion:"
echo "Simple tilde: ~"
echo "With path: ~/Documents"
echo "User tilde: ~root"
echo "In quotes: '~' and \"~\""
EOF

cat > lusush_tilde_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
echo "LUSUSH tilde expansion:"
echo "Simple tilde: ~"
echo "With path: ~/Documents"
echo "User tilde: ~root"
echo "In quotes: '~' and \"~\""
EOF

chmod +x bash_tilde_test.sh lusush_tilde_test.sh

echo ""
echo "Bash output:"
./bash_tilde_test.sh

echo ""
echo "Lusush output:"
./lusush_tilde_test.sh

echo ""

echo "=== POSIX COMPLIANCE VERIFICATION ==="
echo "Verifying POSIX-required tilde expansion behavior..."

cat > posix_tilde_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

# POSIX Test 1: Basic tilde expansion
echo "POSIX Test 1 - Basic expansion:"
echo "Home: ~"

# POSIX Test 2: Tilde with path
echo "POSIX Test 2 - With path:"
echo "Path: ~/bin"

# POSIX Test 3: User tilde
echo "POSIX Test 3 - User expansion:"
echo "Root: ~root"

# POSIX Test 4: Quote handling
echo "POSIX Test 4 - Quote handling:"
echo "No expansion: '~'"
echo "Should expand: \"~\""

echo "POSIX compliance tests completed"
EOF

chmod +x posix_tilde_test.sh
echo "Running POSIX compliance tests:"
./posix_tilde_test.sh
echo ""

echo "=== SPECIAL CASES ==="
echo "Testing special cases and boundary conditions..."

echo "Test: HOME variable unset"
cat > unset_home_test.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush
# Test behavior when HOME is unset
unset HOME
echo "Tilde with unset HOME: ~"
echo "Should use passwd entry"
EOF

chmod +x unset_home_test.sh
echo "Running unset HOME test:"
./unset_home_test.sh
echo ""

# Cleanup
rm -f tilde_test1.sh tilde_test2.sh tilde_test3.sh tilde_test4.sh tilde_test5.sh
rm -f tilde_test6.sh tilde_test7.sh tilde_test8.sh tilde_test9.sh tilde_test10.sh
rm -f bash_tilde_test.sh lusush_tilde_test.sh posix_tilde_test.sh
rm -f unset_home_test.sh

echo "=== TILDE EXPANSION TEST COMPLETE ==="
echo ""
echo "Expected behavior:"
echo "- ~ should expand to \$HOME directory"
echo "- ~user should expand to user's home directory"
echo "- ~/path should expand to \$HOME/path"
echo "- ~user/path should expand to user's home/path"
echo "- Tilde should not expand inside single quotes"
echo "- Tilde should expand inside double quotes"
echo "- Invalid user names should return original text"
echo "- When HOME is unset, should use passwd database"
echo "- Tilde expansion should work in all shell contexts"
