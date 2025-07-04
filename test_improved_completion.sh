#!/bin/bash

# Improved Tab Completion Test Script for LUSUSH
# Tests the new intelligent completion system that handles various completion scenarios properly

echo "==============================================================================="
echo "LUSUSH Improved Tab Completion Test"
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

# Create test environment
echo "Setting up test environment..."
mkdir -p /tmp/lusush_improved_completion_test
cd /tmp/lusush_improved_completion_test

# Create files for different completion scenarios
echo "Creating test files..."

# Few completions (≤6) - should show in single line
touch test_unique_file.txt
touch test_unique_script.sh
touch test_unique_data.csv

# Medium completions (7-20) - should show in compact grid
touch medium_file_1.txt medium_file_2.txt medium_file_3.txt
touch medium_file_4.txt medium_file_5.txt medium_file_6.txt
touch medium_file_7.txt medium_file_8.txt medium_file_9.txt
touch medium_file_10.txt medium_file_11.txt medium_file_12.txt

# Many completions (create many files starting with 'many_')
for i in {1..50}; do
    touch "many_completion_file_${i}.txt"
done

echo "✅ Created test files"
echo ""

# Return to lusush directory
cd - > /dev/null

echo "==============================================================================="
echo "IMPROVED COMPLETION BEHAVIOR DEMONSTRATION"
echo "==============================================================================="
echo ""

echo "🎯 NEW INTELLIGENT COMPLETION SYSTEM:"
echo "   • Few completions (≤6): Shows all in single line"
echo "   • Medium completions (7-20): Shows in compact grid format"
echo "   • Many completions (>20): Shows first 8 with message to narrow down"
echo ""

echo "🚀 IMPROVEMENTS MADE:"
echo "   • No more massive unreadable completion dumps"
echo "   • Intelligent thresholds based on usability"
echo "   • Clear guidance when there are too many options"
echo "   • Proper cycling only for manageable sets"
echo ""

echo "==============================================================================="
echo "DEMONSTRATION SCENARIOS"
echo "==============================================================================="
echo ""

echo "📁 Test Directory Setup:"
echo "   Location: /tmp/lusush_improved_completion_test"
echo "   Few completions: test_unique_* files (3 files)"
echo "   Medium completions: medium_file_* files (12 files)"
echo "   Many completions: many_completion_file_* files (50 files)"
echo ""

echo "🧪 TEST SCENARIOS TO TRY:"
echo ""

echo "1. FEW COMPLETIONS TEST:"
echo "   Command: cd /tmp/lusush_improved_completion_test"
echo "   Then type: 'cat test_unique' + TAB"
echo "   Expected: Shows all 3 files in single line format"
echo ""

echo "2. MEDIUM COMPLETIONS TEST:"
echo "   Type: 'cat medium_file' + TAB"
echo "   Expected: Shows all 12 files in compact 4-column grid"
echo ""

echo "3. MANY COMPLETIONS TEST:"
echo "   Type: 'cat many_completion' + TAB"
echo "   Expected: Shows first 8 files with message to narrow down"
echo ""

echo "4. BUILTIN COMPLETIONS TEST:"
echo "   Type: 'ex' + TAB"
echo "   Expected: Shows 'export' and 'exit' in single line"
echo ""

echo "5. CYCLING TEST (Medium):"
echo "   Type: 'cat medium_file' + TAB, then press TAB repeatedly"
echo "   Expected: Cycles through the 12 files one by one"
echo ""

echo "6. NO CYCLING TEST (Many):"
echo "   Type: 'cat many_completion' + TAB, then press TAB again"
echo "   Expected: Shows sample with message, no cycling through 50 files"
echo ""

echo "7. NARROWING DOWN TEST:"
echo "   Type: 'cat many_completion_file_1' + TAB"
echo "   Expected: Shows only files matching the more specific pattern"
echo ""

echo "==============================================================================="
echo "INTERACTIVE TESTING SESSION"
echo "==============================================================================="
echo ""

echo "🚀 Starting interactive lusush session..."
echo ""
echo "You are now in the enhanced lusush shell."
echo "Navigate to the test directory and try the scenarios above."
echo ""
echo "Quick commands to try:"
echo "  cd /tmp/lusush_improved_completion_test"
echo "  ls                                      # See all test files"
echo "  cat test_unique<TAB>                    # Test few completions"
echo "  cat medium_file<TAB>                    # Test medium completions"
echo "  cat many_completion<TAB>                # Test many completions"
echo "  cat many_completion_file_1<TAB>         # Test narrowing"
echo "  ex<TAB>                                 # Test builtin completions"
echo "  exit                                    # Return to original shell"
echo ""

echo "🎯 WHAT TO OBSERVE:"
echo "   • No more massive unreadable completion dumps"
echo "   • Sensible behavior for different numbers of completions"
echo "   • Clear guidance when there are too many options"
echo "   • Good cycling behavior for manageable sets"
echo "   • Encouragement to narrow down when appropriate"
echo ""

echo "==============================================================================="
echo ""

# Start lusush
exec ./builddir/lusush

# Cleanup (only reached if lusush exits normally)
echo ""
echo "Cleaning up test environment..."
rm -rf /tmp/lusush_improved_completion_test
echo "✅ Test environment cleaned up"
echo "✅ Improved completion system demonstration complete"
