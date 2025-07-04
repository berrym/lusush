#!/bin/bash

# Enhanced Tab Completion Test Script for LUSUSH
# Tests the new enhanced completion functionality that shows all completions
# instead of just cycling through them

echo "==============================================================================="
echo "LUSUSH Enhanced Tab Completion Test"
echo "==============================================================================="
echo ""

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ ERROR: lusush binary not found at ./builddir/lusush"
    echo "Please run 'ninja -C builddir' first"
    exit 1
fi

echo "✅ Found lusush binary"
echo ""

# Create test files for completion
echo "Setting up test environment..."
mkdir -p /tmp/lusush_completion_test
cd /tmp/lusush_completion_test

# Create various test files
touch test_file_1.txt test_file_2.txt test_file_3.txt
touch readme.md README.txt
touch script.sh script.py script.pl
touch config.conf config.ini config.json
touch document.doc document.pdf document.txt
touch image.png image.jpg image.gif
touch data.csv data.xml data.json
touch backup.tar backup.zip backup.gz

echo "Created test files for completion testing"
echo ""

# Test instructions
echo "==============================================================================="
echo "MANUAL TEST INSTRUCTIONS"
echo "==============================================================================="
echo ""
echo "The enhanced completion system has been implemented with the following behavior:"
echo ""
echo "1. ONE COMPLETION: Automatically completes (no cycling)"
echo "2. FEW COMPLETIONS (≤10): Shows all completions immediately in a grid format"
echo "3. MANY COMPLETIONS (>10): Shows count, then shows all on second TAB"
echo ""
echo "TEST SCENARIOS:"
echo "---------------"
echo ""
echo "Test 1: Single completion"
echo "  Type: 'cat read' then press TAB"
echo "  Expected: Should auto-complete to 'readme.md' (assuming unique match)"
echo ""
echo "Test 2: Few completions"
echo "  Type: 'cat test' then press TAB"
echo "  Expected: Should show all 3 test files in a grid immediately"
echo ""
echo "Test 3: Many completions"
echo "  Type: 'cat ' then press TAB"
echo "  Expected: Should show count and first completion"
echo "  Press TAB again: Should show all files in a grid"
echo ""
echo "Test 4: Built-in commands"
echo "  Type: 'ex' then press TAB"
echo "  Expected: Should show 'export' and 'exit' completions"
echo ""
echo "Test 5: Directory navigation"
echo "  Type: 'cd /' then press TAB"
echo "  Expected: Should show system directories"
echo ""
echo "==============================================================================="
echo "STARTING INTERACTIVE TEST SESSION"
echo "==============================================================================="
echo ""
echo "You are now in the lusush shell in the test directory."
echo "Try the test scenarios above to verify the enhanced completion works."
echo "Type 'exit' to return to the original shell."
echo ""
echo "Current directory: $(pwd)"
echo "Test files available:"
ls -la
echo ""
echo "Starting lusush..."
echo ""

# Change back to lusush directory and start the shell
cd - > /dev/null
exec ./builddir/lusush

# Cleanup (will only run if lusush exits normally)
echo ""
echo "Cleaning up test environment..."
rm -rf /tmp/lusush_completion_test
echo "✅ Test environment cleaned up"
