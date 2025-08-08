#!/bin/bash

# Interactive Tab Completion Validation Script for Lusush Line Editor
#
# This script provides step-by-step instructions for validating tab completion
# functionality in a real TTY environment where tab completion can work properly.

echo "========================================================================"
echo "LUSUSH LINE EDITOR - INTERACTIVE TAB COMPLETION VALIDATION"
echo "========================================================================"

# Get absolute path to lusush binary
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_PATH="$SCRIPT_DIR/builddir/lusush"

if [[ ! -x "$SHELL_PATH" ]]; then
    echo "❌ Error: Shell not found at $SHELL_PATH"
    echo "Please build the shell first: scripts/lle_build.sh build"
    exit 1
fi

echo "✅ Shell found: $SHELL_PATH"
echo ""

# Create test environment
echo "=== SETTING UP TEST ENVIRONMENT ==="
TEST_DIR="/tmp/lle_tab_validation_$$"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create test files for completion
echo "test content" > test_file.txt
echo "another file" > another_test.txt
echo "executable script" > test_script.sh
chmod +x test_script.sh
echo "more data" > example.dat
mkdir test_directory

echo "✅ Created test files in: $TEST_DIR"
ls -la
echo ""

echo "=== TAB COMPLETION VALIDATION INSTRUCTIONS ==="
echo ""
echo "The shell will start in INTERACTIVE mode where tab completion can work."
echo "Please test the following scenarios manually:"
echo ""
echo "📋 TEST CHECKLIST:"
echo ""
echo "1. ✅ Command Completion:"
echo "   Type: ec<TAB>"
echo "   Expected: Should complete to 'echo' or show command options"
echo "   Result: [ ] PASS [ ] FAIL"
echo ""
echo "2. ✅ File Completion:"
echo "   Type: cat te<TAB>"
echo "   Expected: Should complete to 'test_' prefix or show test files"
echo "   Result: [ ] PASS [ ] FAIL"
echo ""
echo "3. ✅ Directory Completion:"
echo "   Type: cd te<TAB>"
echo "   Expected: Should complete to 'test_directory/' or show directories"
echo "   Result: [ ] PASS [ ] FAIL"
echo ""
echo "4. ✅ Multiple Completion Cycling:"
echo "   Type: test<TAB><TAB><TAB>"
echo "   Expected: Should cycle through test_file.txt, test_script.sh, test_directory"
echo "   Result: [ ] PASS [ ] FAIL"
echo ""
echo "5. ✅ No Completion Available:"
echo "   Type: xyz<TAB>"
echo "   Expected: No completion (no effect or beep)"
echo "   Result: [ ] PASS [ ] FAIL"
echo ""

echo "🔧 DEBUG ANALYSIS:"
echo ""
echo "If completion doesn't work, check debug output for:"
echo "- [ENHANCED_TAB_COMPLETION] messages"
echo "- LLE_KEY_TAB event processing"
echo "- lle_enhanced_tab_completion_handle() calls"
echo "- Completion provider registration"
echo ""

echo "🎯 VALIDATION PROTOCOL:"
echo ""
echo "1. Test each scenario above"
echo "2. Note which ones work vs fail"
echo "3. Check debug output (saved to /tmp/lle_debug_$$.log)"
echo "4. Report findings to determine next development priority"
echo ""

echo "========================================================================"
echo "STARTING INTERACTIVE SHELL WITH DEBUG OUTPUT"
echo "========================================================================"
echo ""
echo "💡 Tips:"
echo "- Press TAB to trigger completion"
echo "- Use UP/DOWN arrows to test history navigation (should work perfectly)"
echo "- Type 'exit' to end the session"
echo "- Debug output will be saved for analysis"
echo ""

# Start shell with debug output
echo "Starting lusush with debug output enabled..."
echo "Debug log will be saved to: /tmp/lle_debug_$$.log"
echo ""

LLE_DEBUG=1 LLE_DEBUG_COMPLETION=1 "$SHELL_PATH" 2>/tmp/lle_debug_$$.log

echo ""
echo "========================================================================"
echo "VALIDATION SESSION COMPLETE"
echo "========================================================================"

echo ""
echo "📊 DEBUG OUTPUT ANALYSIS:"
echo ""

if [[ -f "/tmp/lle_debug_$$.log" ]]; then
    echo "✅ Debug log generated. Key findings:"
    echo ""

    # Check for completion initialization
    if grep -q "Enhanced tab completion initialized" "/tmp/lle_debug_$$.log"; then
        echo "✅ Tab completion system initialized"
    else
        echo "❌ Tab completion system not initialized"
    fi

    # Check for tab key processing
    TAB_EVENTS=$(grep -c "LLE_KEY_TAB\|TAB.*case" "/tmp/lle_debug_$$.log" 2>/dev/null || echo "0")
    echo "📊 TAB key events detected: $TAB_EVENTS"

    # Check for completion handling
    COMPLETION_CALLS=$(grep -c "lle_enhanced_tab_completion_handle\|ENHANCED_TAB_COMPLETION" "/tmp/lle_debug_$$.log" 2>/dev/null || echo "0")
    echo "📊 Completion handler calls: $COMPLETION_CALLS"

    # Check for completion provider activity
    PROVIDER_ACTIVITY=$(grep -c "completion.*provider\|provider.*completion" "/tmp/lle_debug_$$.log" 2>/dev/null || echo "0")
    echo "📊 Completion provider activity: $PROVIDER_ACTIVITY"

    echo ""
    echo "🔍 Last 10 lines of debug output:"
    tail -10 "/tmp/lle_debug_$$.log"

    echo ""
    echo "💾 Full debug log available at: /tmp/lle_debug_$$.log"

else
    echo "❌ No debug log generated"
fi

echo ""
echo "📋 NEXT STEPS BASED ON RESULTS:"
echo ""
echo "If tab completion worked perfectly:"
echo "  → Tab completion is COMPLETE ✅"
echo "  → Move to next priority (likely fixing remaining test failures)"
echo "  → Focus on LLE-019 Ctrl+R reverse search"
echo ""
echo "If tab completion partially worked:"
echo "  → Identify specific issues (command vs file completion)"
echo "  → Debug completion provider registration"
echo "  → Check word extraction logic"
echo ""
echo "If tab completion didn't work at all:"
echo "  → Check LLE_KEY_TAB event processing"
echo "  → Verify lle_enhanced_tab_completion_handle() is being called"
echo "  → Check completion context initialization"
echo ""

echo "📚 FOR DETAILED ANALYSIS:"
echo "View debug log: cat /tmp/lle_debug_$$.log"
echo "Search for specific events: grep -i 'tab\\|completion' /tmp/lle_debug_$$.log"
echo ""

# Cleanup test environment
echo "🧹 Cleaning up test environment..."
cd "$SCRIPT_DIR"
rm -rf "$TEST_DIR"

echo "========================================================================"
echo "TAB COMPLETION VALIDATION COMPLETE"
echo "========================================================================"
