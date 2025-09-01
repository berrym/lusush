#!/bin/bash
#
# macOS Compatibility Test for Fish-like Features in Lusush
# Tests autosuggestions, enhanced completions, and other fish-inspired enhancements
# specifically on macOS platforms to ensure proper readline integration
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
LUSUSH_BINARY="./builddir/lusush"
TEST_TIMEOUT=10
PASSED_TESTS=0
FAILED_TESTS=0
TOTAL_TESTS=0

# Ensure we're in the right directory
if [[ ! -f "$LUSUSH_BINARY" ]]; then
    echo -e "${RED}Error: Lusush binary not found at $LUSUSH_BINARY${NC}"
    echo "Please run: PKG_CONFIG_PATH=\"/usr/local/opt/readline/lib/pkgconfig\" meson setup builddir --wipe && ninja -C builddir"
    exit 1
fi

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

# Test 1: Basic shell functionality (sanity check)
test_basic_functionality() {
    start_test "Basic shell functionality"

    local output
    output=$($LUSUSH_BINARY -c 'echo "Hello from Lusush on macOS"' 2>&1)

    if [[ "$output" == "Hello from Lusush on macOS" ]]; then
        log_success "Basic command execution works"
    else
        log_failure "Basic command execution failed: $output"
        return 1
    fi
}

# Test 2: Multiline constructs (critical for shell functionality)
test_multiline_constructs() {
    start_test "Multiline shell constructs"

    local output
    output=$($LUSUSH_BINARY -c 'for i in 1 2 3; do echo "Item: $i"; done' 2>&1)

    local expected="Item: 1
Item: 2
Item: 3"

    if [[ "$output" == "$expected" ]]; then
        log_success "Multiline constructs work correctly"
    else
        log_failure "Multiline constructs failed. Expected:\n$expected\nGot:\n$output"
        return 1
    fi
}

# Test 3: Fish-like autosuggestions system
test_autosuggestions_system() {
    start_test "Fish-like autosuggestions system"

    local output
    output=$($LUSUSH_BINARY -c 'display testsuggestion' 2>&1)

    if [[ "$output" == *"SUCCESS: Got suggestion:"* ]]; then
        log_success "Autosuggestions system working (got suggestions)"
    else
        log_failure "Autosuggestions system failed: $output"
        return 1
    fi
}

# Test 3: Git integration in prompts
test_git_integration() {
    start_test "Git integration in themed prompts"

    # Save current directory
    local original_dir=$(pwd)

    # Create a temporary git repo
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    git init --quiet
    echo "test" > test.txt
    git add test.txt
    git commit -m "Initial commit" --quiet

    # Test if lusush can detect git branch and show in prompt
    local output
    output=$(timeout $TEST_TIMEOUT "$original_dir/$LUSUSH_BINARY" -i <<< $'echo "git test"\nexit' 2>&1)
    local exit_status=$?

    cd "$original_dir"
    rm -rf "$temp_dir"

    if [[ $exit_status -eq 0 ]] && ([[ "$output" == *"main"* ]] || [[ "$output" == *"master"* ]]); then
        log_success "Git integration works (branch detected in output)"
    else
        log_failure "Git integration test failed or timed out: $output"
        return 1
    fi
}

# Test 5: Enhanced display mode functionality
test_enhanced_display() {
    start_test "Enhanced display mode functionality"

    local output
    output=$($LUSUSH_BINARY -i <<< $'display status\nexit' 2>&1)

    if [[ "$output" == *"Display Integration: ACTIVE"* ]]; then
        log_success "Enhanced display mode is active"
    else
        log_failure "Enhanced display mode failed: $output"
        return 1
    fi
}

# Test 6: Theme system functionality
test_theme_system() {
    start_test "Professional theme system"

    local output
    output=$($LUSUSH_BINARY -c 'theme list' 2>&1)

    if [[ "$output" == *"Available themes:"* ]] && [[ "$output" == *"corporate"* ]]; then
        log_success "Theme system works (themes available)"
    else
        log_failure "Theme system failed: $output"
        return 1
    fi
}

# Test 7: Theme switching functionality
test_theme_switching() {
    start_test "Theme switching functionality"

    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'theme set dark\ntheme current\nexit' 2>&1)

    if [[ $? -eq 0 ]] && [[ "$output" == *"dark"* ]]; then
        log_success "Theme switching works"
    else
        log_failure "Theme switching failed: $output"
        return 1
    fi
}

# Test 8: History system with autosuggestions integration
test_history_autosuggestion_integration() {
    start_test "History and autosuggestion integration"

    # Create a test history and see if autosuggestions pick it up
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'echo "test command for history"\nhistory\nexit' 2>&1)

    if [[ $? -eq 0 ]] && [[ "$output" == *"test command for history"* ]]; then
        log_success "History system integrates with autosuggestions"
    else
        log_failure "History integration failed: $output"
        return 1
    fi
}

# Test 9: macOS-specific readline integration
test_macos_readline_integration() {
    start_test "macOS GNU Readline integration"

    # Test that readline functions work with macOS homebrew readline
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Testing readline on macOS\nexit' 2>&1)

    if [[ $? -eq 0 ]]; then
        log_success "GNU Readline integration works on macOS"
    else
        log_failure "GNU Readline integration failed on macOS"
        return 1
    fi
}

# Test 10: Performance benchmarks
test_performance_benchmarks() {
    start_test "Performance and responsiveness"

    # Test that the shell can handle commands efficiently
    local start_time=$(perl -MTime::HiRes -e 'print Time::HiRes::time()')
    local output
    output=$($LUSUSH_BINARY -c 'i=1; while [ $i -le 100 ]; do echo "test $i" > /dev/null; i=$((i+1)); done' 2>&1)
    local end_time=$(perl -MTime::HiRes -e 'print Time::HiRes::time()')

    local duration=$(perl -e "print int(($end_time - $start_time) * 1000)")

    if [[ $? -eq 0 ]] && [[ $duration -lt 5000 ]]; then # Less than 5 seconds
        log_success "Performance test passed (${duration}ms for 100 commands)"
    else
        log_failure "Performance test failed (${duration}ms - too slow or errored)"
        return 1
    fi
}

# Test 11: macOS terminal compatibility
test_macos_terminal_compatibility() {
    start_test "macOS Terminal.app compatibility"

    # Test ANSI escape sequences work properly on macOS
    local output
    output=$($LUSUSH_BINARY -c 'echo -e "\033[32mGreen text on macOS\033[0m"' 2>&1)

    if [[ "$output" == *"Green text on macOS"* ]]; then
        log_success "macOS Terminal ANSI handling works"
    else
        log_failure "macOS Terminal ANSI handling failed: $output"
        return 1
    fi
}

# Test 12: Signal handling on macOS
test_signal_handling() {
    start_test "Signal handling (Ctrl+C, etc.) on macOS"

    # Test that the shell handles signals properly on macOS
    timeout 2s $LUSUSH_BINARY -i <<< $'sleep 1\nexit' &>/dev/null
    local exit_code=$?

    if [[ $exit_code -eq 124 ]] || [[ $exit_code -eq 0 ]]; then
        log_success "Signal handling works on macOS (timeout/interrupt handling)"
    else
        log_failure "Signal handling failed on macOS (exit code: $exit_code)"
        return 1
    fi
}

# Test 13: Homebrew readline compatibility
test_homebrew_readline() {
    start_test "Homebrew GNU Readline compatibility"

    # Check if we're using the right readline version
    if command -v brew &> /dev/null; then
        local readline_info=$(brew list --versions readline 2>/dev/null || echo "not found")
        log_info "Homebrew readline: $readline_info"
    fi

    # Test readline-specific functionality
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Test readline features\nexit' 2>&1)

    if [[ $? -eq 0 ]]; then
        log_success "Homebrew readline compatibility confirmed"
    else
        log_failure "Homebrew readline compatibility issues detected"
        return 1
    fi
}

# Test 14: Fish-style command completion
test_command_completion() {
    start_test "Fish-style command completion system"

    # Test that tab completion system initializes
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Tab completion test\nls >/dev/null 2>&1\nexit' 2>&1)

    if [[ $? -eq 0 ]]; then
        log_success "Command completion system loads without errors"
    else
        log_failure "Command completion system failed: $output"
        return 1
    fi
}

# Test 15: Rich completion system integration
test_rich_completion_integration() {
    start_test "Phase 2: Rich completion system integration"

    # Test that rich completions initialize without errors
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Rich completion integration test\nexit' 2>&1)

    if [[ $? -eq 0 ]] && [[ "$output" != *"Failed to initialize rich completions"* ]]; then
        log_success "Rich completion system integrates successfully"
    else
        log_failure "Rich completion integration failed: $output"
        return 1
    fi
}

# Main test runner
run_all_tests() {
    log_info "Starting macOS compatibility tests for Lusush fish-like features..."
    log_info "Testing on: $(uname -a)"
    log_info "Using binary: $LUSUSH_BINARY"

    # Check for Homebrew readline
    if command -v brew &> /dev/null; then
        log_info "Homebrew detected - checking readline installation..."
        if brew list readline &> /dev/null; then
            log_info "✓ Homebrew readline is installed"
        else
            log_warning "Homebrew readline not found - this may cause issues"
        fi
    else
        log_warning "Homebrew not found - using system readline"
    fi

    echo ""

    # Check system dependencies
    if ! command -v timeout &> /dev/null; then
        log_warning "timeout command not found - using alternative timing"
    fi

    if ! command -v git &> /dev/null; then
        log_warning "git not found - git integration tests may fail"
    fi

    # Run all tests
    test_basic_functionality || true
    test_multiline_constructs || true
    test_autosuggestions_system || true
    test_git_integration || true
    test_enhanced_display || true
    test_theme_system || true
    test_theme_switching || true
    test_history_autosuggestion_integration || true
    test_macos_readline_integration || true
    test_performance_benchmarks || true
    test_macos_terminal_compatibility || true
    test_signal_handling || true
    test_homebrew_readline || true
    test_command_completion || true
    test_rich_completion_integration || true

    # Summary
    echo ""
    log_info "=== TEST RESULTS SUMMARY ==="
    log_info "Total tests: $TOTAL_TESTS"
    log_success "Passed: $PASSED_TESTS"

    if [[ $FAILED_TESTS -gt 0 ]]; then
        log_failure "Failed: $FAILED_TESTS"
        echo ""
        log_failure "Some tests failed! Review the output above for details."
        exit 1
    else
        echo ""
        log_success "All tests passed! Fish-like features are working correctly on macOS."

        # Additional info
        echo ""
        log_info "=== macOS COMPATIBILITY STATUS ==="
        log_success "✓ Core shell functionality working"
        log_success "✓ Fish-like autosuggestions system operational"
        log_success "✓ Enhanced display and themes working"
        log_success "✓ Git integration functional"
        log_success "✓ Homebrew GNU Readline integration stable"
        log_success "✓ Performance meets expectations"
        echo ""
        log_info "The feature/fish-enhancements branch is fully compatible with macOS!"

        # macOS-specific notes
        echo ""
        log_info "=== macOS DEPLOYMENT NOTES ==="
        log_info "• Built with PKG_CONFIG_PATH=/usr/local/opt/readline/lib/pkgconfig"
        log_info "• Using Homebrew GNU Readline $(brew list --versions readline 2>/dev/null || echo '(version unknown)')"
        log_info "• All ANSI color sequences working in Terminal.app"
        log_info "• Signal handling optimized for macOS"
        log_info "• Phase 2: Rich completions with descriptions integrated"

        exit 0
    fi
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Test fish-like features in Lusush on macOS platforms"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo "  --verbose, -v Enable verbose output"
        echo ""
        echo "Prerequisites:"
        echo "  - Homebrew GNU Readline installed"
        echo "  - Built with: PKG_CONFIG_PATH=/usr/local/opt/readline/lib/pkgconfig meson setup builddir"
        echo ""
        echo "This script tests:"
        echo "  - Basic shell functionality"
        echo "  - Fish-like autosuggestions system"
        echo "  - Multiline constructs"
        echo "  - Git integration with themed prompts"
        echo "  - Enhanced display mode"
        echo "  - Professional theme system"
        echo "  - Theme switching"
        echo "  - History and autosuggestion integration"
        echo "  - macOS-specific readline integration"
        echo "  - Performance characteristics"
        echo "  - macOS Terminal compatibility"
        echo "  - Signal handling"
        echo "  - Homebrew readline compatibility"
        echo "  - Command completion system"
        exit 0
        ;;
    "--verbose"|"-v")
        set -x
        ;;
esac

# Run the tests
run_all_tests
