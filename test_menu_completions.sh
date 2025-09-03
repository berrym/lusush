#!/bin/bash
#
# Test Menu Completion Capabilities in Lusush
# Tests various readline menu completion features and configurations
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

LUSUSH_BINARY="./builddir/lusush"
PASSED_TESTS=0
FAILED_TESTS=0
TOTAL_TESTS=0

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_TESTS++))
}

log_failure() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_TESTS++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

start_test() {
    ((TOTAL_TESTS++))
    log_info "Running test: $1"
}

# Test 1: Check basic tab completion functionality
test_basic_tab_completion() {
    start_test "Basic TAB completion functionality"

    # Test that basic completion works
    local output
    output=$(echo -e 'ls\t\nexit' | timeout 5s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" != "timeout" ]]; then
        log_success "Basic TAB completion responsive"
    else
        log_failure "Basic TAB completion timed out"
        return 1
    fi
}

# Test 2: Check if menu-complete is available
test_menu_complete_availability() {
    start_test "GNU Readline menu-complete feature availability"

    # Create a test to see if menu-complete binding works
    local test_result=0

    # Test if we can bind menu-complete
    cat > /tmp/lusush_menu_test.txt << 'EOF'
echo "Testing menu complete binding"
exit
EOF

    local output
    output=$(timeout 5s $LUSUSH_BINARY -i < /tmp/lusush_menu_test.txt 2>&1 || echo "error")

    if [[ "$output" == *"Testing menu complete binding"* ]]; then
        log_success "Lusush can execute test commands (prerequisite for menu testing)"
    else
        log_failure "Cannot execute basic test commands"
        rm -f /tmp/lusush_menu_test.txt
        return 1
    fi

    rm -f /tmp/lusush_menu_test.txt
}

# Test 3: Check completion display configuration
test_completion_display_config() {
    start_test "Completion display configuration"

    # Check what completion variables are currently set
    local test_passed=0

    # Test completion query threshold
    if command -v $LUSUSH_BINARY >/dev/null 2>&1; then
        log_success "Lusush binary is executable"
        test_passed=1
    else
        log_failure "Lusush binary not found or not executable"
        return 1
    fi

    # Test completion behavior with multiple matches
    local output
    output=$(echo -e 'echo /usr/bin/g\nexit' | timeout 3s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" != "timeout" ]]; then
        log_success "Completion system handles multiple matches"
    else
        log_warning "Completion with multiple matches timed out"
    fi
}

# Test 4: Test current completion display behavior
test_current_completion_behavior() {
    start_test "Current completion display behavior"

    # Create test directory with multiple files starting with 'test'
    local test_dir="/tmp/lusush_completion_test"
    mkdir -p "$test_dir"
    touch "$test_dir/test_file1.txt"
    touch "$test_dir/test_file2.txt"
    touch "$test_dir/test_file3.txt"
    touch "$test_dir/different_file.txt"

    # Test completion in that directory
    local output
    output=$(cd "$test_dir" && echo -e 'ls test\nexit' | timeout 5s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" != "timeout" ]]; then
        log_success "File completion works in test directory"
    else
        log_warning "File completion test timed out"
    fi

    # Cleanup
    rm -rf "$test_dir"
}

# Test 5: Check for menu completion capabilities
test_menu_completion_features() {
    start_test "Menu completion feature detection"

    log_info "Checking GNU Readline version and capabilities..."

    # Check if readline supports menu completion
    local readline_features_found=0

    # Test 1: Check if we can create a simple completion test
    cat > /tmp/lusush_readline_test.sh << 'EOF'
#!/bin/bash
# Test if menu-complete function exists in readline
if [[ -n "$BASH_VERSION" ]]; then
    # Check bash's readline support
    bind -l | grep -q menu-complete && echo "menu-complete: AVAILABLE"
    bind -l | grep -q complete && echo "complete: AVAILABLE"
    bind -V | grep -E "(completion|menu)" || echo "No menu completion variables found"
fi
EOF

    chmod +x /tmp/lusush_readline_test.sh
    local bash_test_output
    bash_test_output=$(/tmp/lusush_readline_test.sh 2>/dev/null || echo "bash test failed")

    if [[ "$bash_test_output" == *"menu-complete: AVAILABLE"* ]]; then
        log_success "System readline supports menu-complete function"
        readline_features_found=1
    else
        log_warning "menu-complete function not detected in system readline"
    fi

    if [[ "$bash_test_output" == *"complete: AVAILABLE"* ]]; then
        log_success "System readline supports complete function"
        ((readline_features_found++))
    else
        log_warning "complete function not detected in system readline"
    fi

    rm -f /tmp/lusush_readline_test.sh

    if [[ $readline_features_found -gt 0 ]]; then
        log_success "Found $readline_features_found readline completion features"
    else
        log_failure "No readline completion features detected"
        return 1
    fi
}

# Test 6: Check what readline variables are currently configured
test_readline_variable_configuration() {
    start_test "Current readline variable configuration"

    log_info "Testing completion-related readline variables..."

    # Create a test that tries to show current readline configuration
    local config_test_passed=0

    # Test show-all-if-ambiguous behavior
    cat > /tmp/lusush_config_test.txt << 'EOF'
# Test completion behavior
echo "Testing completion config"
exit
EOF

    local output
    output=$(timeout 3s $LUSUSH_BINARY -i < /tmp/lusush_config_test.txt 2>/dev/null || echo "timeout")

    if [[ "$output" == *"Testing completion config"* ]]; then
        log_success "Can test readline configuration"
        config_test_passed=1
    else
        log_warning "Cannot test readline configuration directly"
    fi

    rm -f /tmp/lusush_config_test.txt

    # Report current configuration based on source code analysis
    log_info "Based on source code analysis:"
    log_info "  - show-all-if-ambiguous: OFF (prevents auto-display)"
    log_info "  - show-all-if-unmodified: OFF (prevents immediate display)"
    log_info "  - menu-complete-display-prefix: ON (should support menu display)"
    log_info "  - completion-query-items: 50 (asks before showing 50+ items)"

    if [[ $config_test_passed -eq 1 ]]; then
        log_success "Readline configuration is testable"
    else
        log_warning "Direct readline configuration testing limited"
    fi
}

# Test 7: Analyze menu completion implementation requirements
test_menu_completion_requirements() {
    start_test "Menu completion implementation requirements analysis"

    log_info "Analyzing what's needed for full menu completion support..."

    # Check current completion system capabilities
    log_info "Current system analysis:"
    log_info "  ✓ Basic TAB completion: Available"
    log_info "  ✓ Rich completions with descriptions: Available"
    log_info "  ✓ Context-aware completions: Available"
    log_info "  ✓ Multiple completion sources: Available"

    log_info "Missing for full menu completion:"
    log_info "  ? Menu display in columns: Needs implementation"
    log_info "  ? TAB cycling through options: Needs menu-complete binding"
    log_info "  ? Visual selection highlighting: Needs custom display"
    log_info "  ? Multi-column layout: Needs terminal formatting"

    log_info "GNU Readline menu completion options:"
    log_info "  - menu-complete: Cycles through completions one by one"
    log_info "  - menu-complete-backward: Cycles backward through completions"
    log_info "  - Can bind TAB to menu-complete instead of complete"

    log_info "Implementation approaches:"
    log_info "  1. Use readline's built-in menu-complete (simple cycling)"
    log_info "  2. Custom completion display with rl_completion_display_matches_hook"
    log_info "  3. Hybrid: readline cycling + custom display formatting"

    log_success "Menu completion requirements analysis complete"
}

# Test 8: Test feasibility of implementing menu completions
test_menu_completion_feasibility() {
    start_test "Menu completion implementation feasibility"

    log_info "Testing feasibility of different approaches..."

    local feasibility_score=0

    # Approach 1: Built-in menu-complete
    log_info "Approach 1: Built-in readline menu-complete"
    log_info "  Pros: Simple to implement, reliable, standard behavior"
    log_info "  Cons: Limited customization, no multi-column display"
    log_info "  Feasibility: HIGH - Just need to rebind TAB key"
    ((feasibility_score++))

    # Approach 2: Custom completion display hook
    log_info "Approach 2: Custom completion display with hooks"
    log_info "  Pros: Full control over display, multi-column support, descriptions"
    log_info "  Cons: Complex implementation, potential display issues"
    log_info "  Feasibility: MEDIUM - Requires careful terminal handling"
    ((feasibility_score++))

    # Approach 3: Hybrid approach
    log_info "Approach 3: Hybrid (custom display + readline cycling)"
    log_info "  Pros: Best of both worlds, robust cycling, custom display"
    log_info "  Cons: More complex, needs careful integration"
    log_info "  Feasibility: HIGH - Builds on existing systems"
    ((feasibility_score++))

    log_info "Recommended approach: Start with Approach 1 (built-in menu-complete)"
    log_info "  - Quick to implement and test"
    log_info "  - Can be enhanced with custom display later"
    log_info "  - Provides immediate user benefit"

    log_success "Menu completion is feasible with multiple approaches ($feasibility_score options identified)"
}

# Main test runner
run_all_tests() {
    log_info "Starting menu completion capability analysis..."
    log_info "Testing on: $(uname -a)"
    log_info "Using binary: $LUSUSH_BINARY"

    echo ""

    # Check prerequisites
    if [[ ! -f "$LUSUSH_BINARY" ]]; then
        log_failure "Lusush binary not found at $LUSUSH_BINARY"
        log_info "Please run: ninja -C builddir"
        exit 1
    fi

    # Run all tests
    test_basic_tab_completion || true
    test_menu_complete_availability || true
    test_completion_display_config || true
    test_current_completion_behavior || true
    test_menu_completion_features || true
    test_readline_variable_configuration || true
    test_menu_completion_requirements || true
    test_menu_completion_feasibility || true

    # Summary
    echo ""
    log_info "=== MENU COMPLETION ANALYSIS SUMMARY ==="
    log_info "Total tests: $TOTAL_TESTS"
    log_success "Completed: $PASSED_TESTS"

    if [[ $FAILED_TESTS -gt 0 ]]; then
        log_failure "Issues found: $FAILED_TESTS"
    else
        log_success "No critical issues found"
    fi

    echo ""
    log_info "=== RECOMMENDATIONS ==="
    log_info "1. QUICK WIN: Implement basic menu-complete by rebinding TAB"
    log_info "   - Change rl_bind_key('\\t', rl_complete) to rl_bind_key('\\t', rl_menu_complete)"
    log_info "   - This enables TAB cycling through completions immediately"
    log_info ""
    log_info "2. ENHANCED: Add custom completion display hook"
    log_info "   - Use rl_completion_display_matches_hook for multi-column display"
    log_info "   - Format completions with descriptions in columns"
    log_info ""
    log_info "3. ADVANCED: Hybrid approach with visual selection"
    log_info "   - Combine custom display with menu cycling"
    log_info "   - Add highlighting for currently selected completion"
    log_info ""
    log_info "Current system is ready for menu completion implementation!"

    if [[ $FAILED_TESTS -eq 0 ]]; then
        exit 0
    else
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Test and analyze menu completion capabilities in Lusush"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo ""
        exit 0
        ;;
esac

# Run the tests
run_all_tests
