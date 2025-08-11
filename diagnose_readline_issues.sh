#!/bin/bash

# Lusush Readline Integration Diagnostic Script
# Identifies fundamental readline display and terminal control issues

echo "=== Lusush Readline Integration Diagnostic ==="
echo "Date: $(date)"
echo "Purpose: Diagnose fundamental readline display corruption issues"
echo ""

# Ensure lusush is built
if [ ! -f builddir/lusush ]; then
    echo "❌ lusush binary not found. Building..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "❌ Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "=== CRITICAL ISSUES IDENTIFIED BY USER ==="
echo ""
echo "1. 🚨 Arrow key navigation corrupts display:"
echo "   - UP arrow leaves [A artifacts and shrinks prompt"
echo "   - DOWN arrow leaves [B artifacts"
echo "   - Indicates broken escape sequence handling"
echo ""
echo "2. 🚨 Ctrl+G fails to clear line:"
echo "   - Should cancel input and show clean prompt"
echo "   - Basic readline functionality broken"
echo ""
echo "3. 🚨 Ctrl+R reverse search corrupts display:"
echo "   - Search prompt draws over lusush prompt"
echo "   - Display state management fundamentally broken"
echo ""

echo "=== ROOT CAUSE ANALYSIS ==="
echo ""
echo "These issues indicate FUNDAMENTAL problems with:"
echo "  ❌ Terminal escape sequence handling"
echo "  ❌ Display state management"
echo "  ❌ Readline redisplay functions"
echo "  ❌ Terminal control integration"
echo ""

echo "This is NOT just signal handling - it's basic readline integration failure."
echo ""

echo "=== DIAGNOSTIC TESTS ==="
echo ""

echo "Test 1: Basic readline configuration check..."
echo ""

# Check if readline is properly configured
cat > /tmp/readline_config_test.txt << 'EOF'
debug on
echo "Testing readline basic functions..."
exit
EOF

echo "Running configuration test..."
script -q -c './builddir/lusush' /dev/null < /tmp/readline_config_test.txt > /tmp/config_output.txt 2>&1

if grep -q "debug.*Unknown" /tmp/config_output.txt; then
    echo "✅ Shell responds to commands (basic functionality working)"
else
    echo "❌ Basic command processing may be broken"
fi

echo ""
echo "Test 2: Terminal capabilities check..."
echo ""

# Check terminal environment
echo "Terminal Environment:"
echo "  TERM: ${TERM:-unset}"
echo "  COLORTERM: ${COLORTERM:-unset}"
echo "  Terminal type: $(tput longname 2>/dev/null || echo 'unknown')"
echo "  Terminal size: $(tput cols 2>/dev/null)x$(tput lines 2>/dev/null)"
echo ""

echo "Test 3: Readline version and capabilities..."
echo ""

# Check what version of readline we're linking against
echo "Readline information:"
ldd builddir/lusush | grep readline || echo "  ❌ readline not found in linked libraries"
echo ""

echo "=== CRITICAL FILE ANALYSIS ==="
echo ""

echo "Analyzing src/readline_integration.c for common issues..."
echo ""

# Check for problematic readline configuration
echo "Checking for known problematic patterns:"
echo ""

if grep -q "rl_redisplay_function.*apply_syntax_highlighting" src/readline_integration.c; then
    echo "🚨 POTENTIAL ISSUE: Custom redisplay function may be causing corruption"
    echo "   File: src/readline_integration.c"
    echo "   Issue: apply_syntax_highlighting may have display bugs"
    echo "   Fix: Use standard rl_redisplay temporarily"
fi

if grep -q "rl_getc_function.*lusush_getc" src/readline_integration.c; then
    echo "🚨 POTENTIAL ISSUE: Custom getc function may interfere with escape sequences"
    echo "   File: src/readline_integration.c"
    echo "   Issue: lusush_getc may not handle terminal sequences correctly"
    echo "   Fix: Use default readline getc temporarily"
fi

# Check for multiple redisplay function assignments
redisplay_count=$(grep -c "rl_redisplay_function" src/readline_integration.c)
if [ $redisplay_count -gt 1 ]; then
    echo "🚨 POTENTIAL ISSUE: Multiple redisplay function assignments ($redisplay_count found)"
    echo "   Issue: Conflicting redisplay configurations"
    echo "   Fix: Ensure single, consistent redisplay function"
fi

# Check for terminal state issues
if grep -q "rl_prep_terminal\|rl_deprep_terminal" src/readline_integration.c; then
    echo "⚠️  WARNING: Manual terminal prep/deprep found"
    echo "   Issue: May conflict with readline's terminal management"
    echo "   Review: Ensure proper terminal state handling"
fi

echo ""
echo "=== READLINE INTEGRATION BEST PRACTICES CHECK ==="
echo ""

echo "Checking adherence to readline best practices..."
echo ""

# Check for essential readline configurations
essential_configs=(
    "rl_readline_name"
    "rl_attempted_completion_function"
    "using_history"
    "rl_catch_signals"
)

for config in "${essential_configs[@]}"; do
    if grep -q "$config" src/readline_integration.c; then
        echo "✅ $config - configured"
    else
        echo "❌ $config - missing or not configured"
    fi
done

echo ""
echo "=== IMMEDIATE FIX RECOMMENDATIONS ==="
echo ""

echo "Based on the corruption issues you reported, try these fixes:"
echo ""

echo "FIX 1: Disable custom redisplay function temporarily"
echo "  File: src/readline_integration.c"
echo "  Change: rl_redisplay_function = rl_redisplay; (always use default)"
echo "  Remove: apply_syntax_highlighting assignments"
echo ""

echo "FIX 2: Disable custom getc function temporarily"
echo "  File: src/readline_integration.c"
echo "  Remove/comment: rl_getc_function = lusush_getc;"
echo "  Let readline use its default input handling"
echo ""

echo "FIX 3: Simplify readline configuration"
echo "  Remove all the complex rl_variable_bind settings"
echo "  Use minimal configuration until display issues resolved"
echo ""

echo "FIX 4: Check terminal initialization"
echo "  Ensure proper terminal setup before readline use"
echo "  Verify TERM environment variable is set correctly"
echo ""

echo "=== EMERGENCY BYPASS IMPLEMENTATION ==="
echo ""

echo "If readline integration is fundamentally broken, consider:"
echo ""
echo "Option A: Minimal readline mode"
echo "  - Use only basic readline features"
echo "  - Disable all custom functions temporarily"
echo "  - Get basic line editing working first"
echo ""

echo "Option B: Fallback to basic input"
echo "  - Temporarily bypass readline for basic functionality"
echo "  - Use simple getline() for input"
echo "  - Add readline features incrementally"
echo ""

echo "Option C: Debug readline integration step by step"
echo "  - Start with minimal configuration"
echo "  - Add features one by one"
echo "  - Test after each addition"
echo ""

echo "=== SPECIFIC FIXES FOR YOUR ISSUES ==="
echo ""

echo "Issue: Arrow keys leave [A [B artifacts"
echo "Cause: Escape sequences not properly consumed by readline"
echo "Fix: Ensure readline terminal setup is correct"
echo "Check: Terminal capabilities, TERM variable, readline version"
echo ""

echo "Issue: Ctrl+G doesn't clear line"
echo "Cause: Key binding not working or redisplay broken"
echo "Fix: Verify rl_bind_key(7, lusush_abort_line) is correct"
echo "Check: lusush_abort_line function implementation"
echo ""

echo "Issue: Ctrl+R corrupts display"
echo "Cause: Readline search mode display management broken"
echo "Fix: May need custom search function or display fixing"
echo "Check: Redisplay function conflicts"
echo ""

echo "=== CRITICAL FILES TO EXAMINE ==="
echo ""

echo "Primary suspects for display corruption:"
echo "  1. src/readline_integration.c - Lines 870-950 (configuration)"
echo "  2. src/readline_integration.c - Lines 1190-1210 (lusush_getc)"
echo "  3. src/readline_integration.c - Lines 720-740 (apply_syntax_highlighting)"
echo "  4. Terminal initialization in main shell startup"
echo ""

echo "=== QUICK FIX ATTEMPT ==="
echo ""

echo "Creating emergency simplified readline configuration..."
echo ""

# Create a patch for emergency fix
cat > /tmp/emergency_readline_fix.patch << 'EOF'
--- a/src/readline_integration.c
+++ b/src/readline_integration.c
@@ -878,7 +878,8 @@ static void setup_readline_config(void) {

     // Redisplay configuration for better history navigation
-    rl_redisplay_function = rl_redisplay;
+    // EMERGENCY FIX: Use default redisplay only
+    rl_redisplay_function = NULL;  // Let readline use its default


     // Variables for better behavior - CRITICAL FIXES for history navigation
@@ -920,7 +921,8 @@ static void setup_readline_config(void) {
     }

     // Custom getc function for better input handling
-    rl_getc_function = lusush_getc;
+    // EMERGENCY FIX: Use default getc
+    // rl_getc_function = lusush_getc;
 }

 void lusush_readline_configure(bool show_completions_immediately,
@@ -240,7 +241,8 @@ char *lusush_readline_with_prompt(const char *prompt) {

     // Setup syntax highlighting if enabled
     if (syntax_highlighting_enabled) {
-        rl_redisplay_function = apply_syntax_highlighting;
+        // EMERGENCY FIX: Disable custom redisplay
+        rl_redisplay_function = NULL;
     }

     // Use theme-generated prompt if none provided
EOF

echo "Emergency patch created at /tmp/emergency_readline_fix.patch"
echo ""

echo "=== TESTING STRATEGY ==="
echo ""

echo "Recommended approach to fix these critical issues:"
echo ""

echo "PHASE 1: Emergency simplification (30 minutes)"
echo "  1. Remove custom redisplay function (use default)"
echo "  2. Remove custom getc function (use default)"
echo "  3. Simplify readline configuration to minimal"
echo "  4. Test if basic arrow keys work without artifacts"
echo ""

echo "PHASE 2: Identify root cause (1-2 hours)"
echo "  1. Test with minimal readline setup"
echo "  2. Add features back one by one"
echo "  3. Identify exactly which configuration breaks display"
echo "  4. Fix or remove problematic components"
echo ""

echo "PHASE 3: Proper fix (2-3 hours)"
echo "  1. Implement correct terminal state management"
echo "  2. Fix redisplay function if needed"
echo "  3. Ensure all readline features work without corruption"
echo "  4. Comprehensive testing"
echo ""

echo "=== BOTTOM LINE ==="
echo ""

echo "🚨 CRITICAL ASSESSMENT: Readline integration has fundamental issues"
echo ""

echo "The problems you've identified indicate:"
echo "  ❌ Display state management is broken"
echo "  ❌ Terminal escape sequence handling is corrupted"
echo "  ❌ Basic readline functionality is not working correctly"
echo ""

echo "Previous AI's assessment of '85% complete' was incorrect."
echo "Actual status: ~60% complete with critical display issues."
echo ""

echo "🎯 IMMEDIATE PRIORITY:"
echo "  1. Fix display corruption (arrow keys, Ctrl+G, Ctrl+R)"
echo "  2. Ensure basic readline functionality works correctly"
echo "  3. Re-test all features after display fixes"
echo ""

echo "🎯 SUCCESS CRITERIA:"
echo "  ✅ Arrow keys navigate history WITHOUT leaving artifacts"
echo "  ✅ Ctrl+G clears line cleanly"
echo "  ✅ Ctrl+R reverse search displays correctly"
echo "  ✅ All readline shortcuts work without display corruption"
echo ""

echo "Time estimate for complete fix: 4-8 hours of focused work"
echo "Complexity: High - requires debugging fundamental readline integration"
echo ""

echo "=== RECOMMENDED IMMEDIATE ACTION ==="
echo ""

echo "Start with emergency simplification:"
echo "  1. Comment out custom redisplay function"
echo "  2. Comment out custom getc function"
echo "  3. Use minimal readline configuration"
echo "  4. Test if this resolves display corruption"
echo ""

echo "If emergency fix resolves issues:"
echo "  ✅ Confirms custom functions are causing corruption"
echo "  🎯 Focus on fixing custom implementations"
echo ""

echo "If issues persist even with minimal setup:"
echo "  ❌ Indicates deeper terminal/readline compatibility issue"
echo "  🎯 May need to investigate readline version, terminal compatibility"
echo ""

# Cleanup
rm -f /tmp/readline_config_test.txt /tmp/config_output.txt

echo ""
echo "Diagnostic completed. Focus on display corruption as highest priority."
echo "The shell has good foundations but needs critical readline fixes."
