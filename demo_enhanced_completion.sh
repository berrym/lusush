#!/bin/bash

# Enhanced Tab Completion Demonstration for LUSUSH
# Shows the new improved completion behavior that displays all options
# instead of just cycling through them

echo "==============================================================================="
echo "LUSUSH Enhanced Tab Completion Demonstration"
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

# Create comprehensive test environment
echo "Setting up comprehensive test environment..."
DEMO_DIR="/tmp/lusush_completion_demo"
mkdir -p "$DEMO_DIR"

# Create various file types for testing
cd "$DEMO_DIR"

# System-like files
touch system.log system.conf system.cfg system.ini
touch app.log app.conf app.cfg app.ini
touch user.log user.conf user.cfg user.ini

# Code files
touch main.c main.cpp main.h main.hpp
touch script.sh script.py script.pl script.rb
touch test.c test.cpp test.h test.hpp
touch config.json config.xml config.yaml config.toml

# Documentation files
touch README.md README.txt readme.rst
touch CHANGELOG.md CHANGELOG.txt changelog.rst
touch LICENSE.md LICENSE.txt license.rst
touch INSTALL.md INSTALL.txt install.rst

# Data files
touch data.csv data.json data.xml data.yaml
touch backup.tar backup.zip backup.gz backup.bz2
touch database.db database.sqlite database.sql

# Image files
touch image.png image.jpg image.gif image.bmp
touch photo.png photo.jpg photo.gif photo.bmp
touch icon.png icon.jpg icon.gif icon.bmp

# Create directories
mkdir -p src include lib bin docs tests examples

echo "Created comprehensive test environment with multiple file types"
echo ""

# Return to lusush directory
cd - > /dev/null

echo "==============================================================================="
echo "ENHANCED COMPLETION FEATURES DEMONSTRATION"
echo "==============================================================================="
echo ""

echo "🎯 NEW ENHANCED COMPLETION BEHAVIOR:"
echo "   • Single completion: Auto-completes immediately"
echo "   • Few completions (≤10): Shows all in grid format instantly"
echo "   • Many completions (>10): Shows count, then all on second TAB"
echo ""

echo "🔧 IMPROVEMENTS MADE:"
echo "   • Eliminated confusing cycling behavior"
echo "   • Added intelligent grid display of completions"
echo "   • Proper 'show all' functionality that actually shows all"
echo "   • Better visual organization of completion results"
echo ""

echo "==============================================================================="
echo "DEMONSTRATION SCENARIOS"
echo "==============================================================================="
echo ""

echo "📁 Test Directory Contents:"
echo "   Located at: $DEMO_DIR"
echo "   Files created: $(ls -1 "$DEMO_DIR" | wc -l)"
echo "   File types: .c .cpp .h .py .sh .md .txt .json .xml .png .jpg .tar .zip"
echo ""

echo "🧪 TEST SCENARIOS TO TRY:"
echo ""
echo "1. SINGLE COMPLETION TEST:"
echo "   Type: 'cat CHANGE' + TAB"
echo "   Expected: Auto-completes to 'CHANGELOG.md'"
echo ""

echo "2. FEW COMPLETIONS TEST:"
echo "   Type: 'cat main' + TAB"
echo "   Expected: Shows all 'main.*' files in grid format immediately"
echo ""

echo "3. MANY COMPLETIONS TEST:"
echo "   Type: 'cat ' + TAB"
echo "   Expected: Shows count message, then TAB again shows all files"
echo ""

echo "4. BUILTIN COMMANDS TEST:"
echo "   Type: 'ex' + TAB"
echo "   Expected: Shows 'export' and 'exit' options"
echo ""

echo "5. DIRECTORY COMPLETION TEST:"
echo "   Type: 'cd ' + TAB"
echo "   Expected: Shows available directories"
echo ""

echo "6. VARIABLE COMPLETION TEST:"
echo "   Type: 'echo \$HO' + TAB"
echo "   Expected: Shows \$HOME variable"
echo ""

echo "7. PARTIAL MATCH TEST:"
echo "   Type: 'cat config' + TAB"
echo "   Expected: Shows all config.* files in grid"
echo ""

echo "8. EXTENSION GROUPING TEST:"
echo "   Type: 'cat image' + TAB"
echo "   Expected: Shows all image.* files"
echo ""

echo "==============================================================================="
echo "INTERACTIVE DEMONSTRATION"
echo "==============================================================================="
echo ""

echo "🚀 Starting interactive lusush session..."
echo ""
echo "You are now in the enhanced lusush shell."
echo "The test files are available in: $DEMO_DIR"
echo ""
echo "Try the test scenarios above to experience the enhanced completion!"
echo "Notice how the completion behavior is now much more intuitive:"
echo "  • No more confusing cycling"
echo "  • Clear display of all available options"
echo "  • Smart handling of different completion counts"
echo ""
echo "Commands to try:"
echo "  cd $DEMO_DIR    # Navigate to test directory"
echo "  ls              # See all test files"
echo "  cat <TAB>       # Test file completion"
echo "  echo \$<TAB>      # Test variable completion"
echo "  ex<TAB>         # Test builtin completion"
echo "  exit            # Return to original shell"
echo ""
echo "==============================================================================="
echo ""

# Start lusush in the demo directory
cd "$DEMO_DIR"
exec ../../../../../builddir/lusush

# Cleanup (only reached if lusush exits normally)
echo ""
echo "Cleaning up demonstration environment..."
rm -rf "$DEMO_DIR"
echo "✅ Demonstration environment cleaned up"
