#!/bin/bash

# Automated Tab Completion Test for Lusush Line Editor
# Tests if tab completion is actually working in practice

echo "========================================================================"
echo "LUSUSH LINE EDITOR - TAB COMPLETION FUNCTIONALITY TEST"
echo "========================================================================"

# Get absolute path to lusush binary
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_PATH="$SCRIPT_DIR/builddir/lusush"

if [[ ! -x "$SHELL_PATH" ]]; then
    echo "Error: Shell not found at $SHELL_PATH"
    echo "Please build the shell first: scripts/lle_build.sh build"
    exit 1
fi

echo "Testing shell: $SHELL_PATH"
echo ""

# Create test files for completion testing
mkdir -p /tmp/lle_tab_test
cd /tmp/lle_tab_test
echo "test content" > test_file.txt
echo "another file" > another_test.txt
echo "executable script" > executable_script.sh
chmod +x executable_script.sh

echo "=== TAB COMPLETION AUTOMATED TESTS ==="

# Test 1: Command completion
echo "Test 1: Command completion for 'ec' + TAB (should complete to 'echo')"
echo -e "ec\t\nexit\n" | timeout 5s "$SHELL_PATH" 2>/tmp/tab_debug.log
TAB_RESULT=$?
echo "Command completion test result: $TAB_RESULT"

# Test 2: File completion
echo ""
echo "Test 2: File completion for 'cat te' + TAB (should complete to test files)"
echo -e "cat te\t\nexit\n" | timeout 5s "$SHELL_PATH" 2>>/tmp/tab_debug.log
FILE_RESULT=$?
echo "File completion test result: $FILE_RESULT"

# Test 3: Enhanced tab completion debug output
echo ""
echo "Test 3: Checking tab completion debug output..."
if [[ -f /tmp/tab_debug.log ]]; then
    echo "Debug log contents:"
    cat /tmp/tab_debug.log | grep -i "tab\|completion" | head -10

    # Check for completion initialization
    if grep -q "Enhanced tab completion initialized" /tmp/tab_debug.log; then
        echo "✅ Tab completion system initialized successfully"
    else
        echo "❌ Tab completion system not initializing"
    fi

    # Check for completion handling
    if grep -q "TAB_COMPLETION" /tmp/tab_debug.log; then
        echo "✅ Tab completion events being processed"
    else
        echo "❌ Tab completion events not being processed"
    fi
else
    echo "❌ No debug log generated"
fi

# Test 4: Interactive completion framework test
echo ""
echo "Test 4: Testing completion framework directly..."
echo "Testing completion list creation and basic operations..."

# Run specific completion tests
cd "$SCRIPT_DIR"
meson test -C builddir test_lle_029_completion_framework test_lle_030_basic_file_completion test_lle_031_completion_display --verbose 2>/tmp/completion_test.log
COMPLETION_TESTS=$?

if [[ $COMPLETION_TESTS -eq 0 ]]; then
    echo "✅ All completion framework tests passing"
else
    echo "❌ Completion framework tests failing"
    echo "Last few lines of completion test output:"
    tail -10 /tmp/completion_test.log
fi

# Test 5: Enhanced tab completion functionality check
echo ""
echo "Test 5: Checking enhanced tab completion implementation..."

# Check if enhanced tab completion is being called
if grep -q "lle_enhanced_tab_completion_handle" src/line_editor/line_editor.c; then
    echo "✅ Enhanced tab completion integrated in main input loop"
else
    echo "❌ Enhanced tab completion not integrated"
fi

# Check if completion providers are working
if [[ -f src/line_editor/enhanced_tab_completion.c ]]; then
    echo "✅ Enhanced tab completion implementation exists"
else
    echo "❌ Enhanced tab completion implementation missing"
fi

echo ""
echo "========================================================================"
echo "TAB COMPLETION TEST SUMMARY"
echo "========================================================================"

# Summary of findings
TOTAL_ISSUES=0

echo "Completion Framework Status:"
if [[ $COMPLETION_TESTS -eq 0 ]]; then
    echo "  ✅ Unit tests: PASSING"
else
    echo "  ❌ Unit tests: FAILING"
    ((TOTAL_ISSUES++))
fi

echo ""
echo "Integration Status:"
if grep -q "lle_enhanced_tab_completion_handle" src/line_editor/line_editor.c; then
    echo "  ✅ Main loop integration: COMPLETE"
else
    echo "  ❌ Main loop integration: MISSING"
    ((TOTAL_ISSUES++))
fi

echo ""
echo "Implementation Status:"
if [[ -f src/line_editor/enhanced_tab_completion.c ]] && [[ -f src/line_editor/completion.c ]]; then
    echo "  ✅ Backend implementation: COMPLETE"
else
    echo "  ❌ Backend implementation: INCOMPLETE"
    ((TOTAL_ISSUES++))
fi

echo ""
echo "Debug Output Status:"
if [[ -f /tmp/tab_debug.log ]] && grep -q "completion" /tmp/tab_debug.log; then
    echo "  ✅ Debug instrumentation: WORKING"
else
    echo "  ❌ Debug instrumentation: MISSING"
    ((TOTAL_ISSUES++))
fi

echo ""
if [[ $TOTAL_ISSUES -eq 0 ]]; then
    echo "🎉 TAB COMPLETION STATUS: LIKELY WORKING"
    echo "   All framework components present and passing tests"
    echo "   Integration detected in main input loop"
    echo "   Next step: Manual interactive testing recommended"
else
    echo "🔧 TAB COMPLETION STATUS: NEEDS ATTENTION"
    echo "   Found $TOTAL_ISSUES potential issues"
    echo "   Recommend debugging before proceeding"
fi

echo ""
echo "Next Steps:"
echo "1. Run interactive test: ./test_tab_completion.sh"
echo "2. Enable debug mode: LLE_DEBUG=1 ./builddir/lusush"
echo "3. Test completion manually: type 'ec<TAB>' and 'cat <TAB>'"
echo "4. Check debug output for completion events"

# Cleanup
rm -rf /tmp/lle_tab_test
rm -f /tmp/tab_debug.log /tmp/completion_test.log

echo "========================================================================"
