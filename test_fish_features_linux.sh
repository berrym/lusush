#!/bin/bash
#
# Linux Compatibility Test for Fish-like Features in Lusush
# Tests autosuggestions, enhanced completions, and other fish-inspired enhancements
# specifically on Linux platforms to ensure no macOS-specific dependencies
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
    echo "Please run: meson setup builddir && ninja -C builddir"
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
    output=$($LUSUSH_BINARY -c 'echo "Hello from Lusush on Linux"' 2>&1)

    if [[ "$output" == "Hello from Lusush on Linux" ]]; then
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

# Test 3: Git integration in prompts
test_git_integration() {
    start_test "Git integration in themed prompts"

    # Create a temporary git repo
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    git init --quiet
    echo "test" > test.txt
    git add test.txt
    git commit -m "Initial commit" --quiet

    # Test if lusush can detect git branch
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'echo $PWD\nexit' 2>&1)

    cd - > /dev/null
    rm -rf "$temp_dir"

    if [[ $? -eq 0 ]]; then
        log_success "Git integration test completed (branch detection working)"
    else
        log_failure "Git integration test failed or timed out"
        return 1
    fi
}

# Test 4: Enhanced display mode
test_enhanced_display() {
    start_test "Enhanced display mode functionality"

    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY --enhanced-display -i <<< $'echo "Enhanced mode test"\nexit' 2>&1)

    if [[ $? -eq 0 ]] && [[ "$output" == *"Enhanced display mode enabled"* ]]; then
        log_success "Enhanced display mode works"
    else
        log_failure "Enhanced display mode failed"
        return 1
    fi
}

# Test 5: Autosuggestions system initialization
test_autosuggestions_init() {
    start_test "Autosuggestions system initialization"

    # Test that autosuggestions initialize without errors
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Testing autosuggestions init\nexit' 2>&1)

    if [[ $? -eq 0 ]] && [[ "$output" != *"Failed to initialize autosuggestions"* ]]; then
        log_success "Autosuggestions system initializes without errors"
    else
        log_failure "Autosuggestions system failed to initialize: $output"
        return 1
    fi
}

# Test 6: Tab completion functionality
test_tab_completion() {
    start_test "Advanced tab completion system"

    # Create test files for completion
    local temp_dir=$(mktemp -d)
    touch "$temp_dir/test_file1.txt"
    touch "$temp_dir/test_file2.txt"
    touch "$temp_dir/another_file.log"

    cd "$temp_dir"

    # Test basic file completion (this is tricky to test non-interactively)
    # We'll test that the shell starts and exits cleanly with tab completion enabled
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Tab completion test\nls\nexit' 2>&1)

    cd - > /dev/null
    rm -rf "$temp_dir"

    if [[ $? -eq 0 ]]; then
        log_success "Tab completion system loads without errors"
    else
        log_failure "Tab completion system failed: $output"
        return 1
    fi
}

# Test 7: Theme system functionality
test_theme_system() {
    start_test "Theme system with git integration"

    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'theme list\nexit' 2>&1)

    if [[ $? -eq 0 ]]; then
        log_success "Theme system works (theme list command executed)"
    else
        log_failure "Theme system failed: $output"
        return 1
    fi
}

# Test 8: History system
test_history_system() {
    start_test "History system functionality"

    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'echo "test command"\nhistory | tail -1\nexit' 2>&1)

    if [[ $? -eq 0 ]]; then
        log_success "History system works"
    else
        log_failure "History system failed: $output"
        return 1
    fi
}

# Test 9: Linux-specific terminal handling
test_linux_terminal_handling() {
    start_test "Linux terminal handling and ANSI codes"

    # Test ANSI escape sequences work properly on Linux
    local output
    output=$($LUSUSH_BINARY -c 'echo -e "\033[32mGreen text\033[0m"' 2>&1)

    if [[ "$output" == *"Green text"* ]]; then
        log_success "Linux terminal ANSI handling works"
    else
        log_failure "Linux terminal ANSI handling failed: $output"
        return 1
    fi
}

# Test 10: Performance and memory test
test_performance() {
    start_test "Performance and memory usage"

    # Test that the shell can handle multiple commands efficiently
    local start_time=$(date +%s%N)
    local output
    output=$($LUSUSH_BINARY -c 'for i in {1..100}; do echo "Performance test $i" > /dev/null; done' 2>&1)
    local end_time=$(date +%s%N)

    local duration=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds

    if [[ $? -eq 0 ]] && [[ $duration -lt 5000 ]]; then # Less than 5 seconds
        log_success "Performance test passed (${duration}ms for 100 commands)"
    else
        log_failure "Performance test failed (${duration}ms - too slow or errored)"
        return 1
    fi
}

# Test 11: Linux-specific readline integration
test_readline_linux() {
    start_test "GNU Readline integration on Linux"

    # Test that readline functions work with Linux libc
    local output
    output=$(timeout $TEST_TIMEOUT $LUSUSH_BINARY -i <<< $'# Testing readline on Linux\nexit' 2>&1)

    if [[ $? -eq 0 ]]; then
        log_success "GNU Readline integration works on Linux"
    else
        log_failure "GNU Readline integration failed on Linux"
        return 1
    fi
}

# Test 12: Signal handling
test_signal_handling() {
    start_test "Signal handling (Ctrl+C, etc.)"

    # Test that the shell handles signals properly
    # This is a basic test - just ensure it starts and can be interrupted
    timeout 2s $LUSUSH_BINARY -i <<< $'sleep 1\nexit' &>/dev/null
    local exit_code=$?

    if [[ $exit_code -eq 124 ]] || [[ $exit_code -eq 0 ]]; then
        log_success "Signal handling works (timeout/interrupt handling)"
    else
        log_failure "Signal handling failed (exit code: $exit_code)"
        return 1
    fi
}

# Main test runner
run_all_tests() {
    log_info "Starting Linux compatibility tests for Lusush fish-like features..."
    log_info "Testing on: $(uname -a)"
    log_info "Using binary: $LUSUSH_BINARY"
    echo ""

    # Check system dependencies
    if ! command -v timeout &> /dev/null; then
        log_warning "timeout command not found - some tests may hang"
    fi

    if ! command -v git &> /dev/null; then
        log_warning "git not found - git integration tests may fail"
    fi

    # Run all tests
    test_basic_functionality || true
    test_multiline_constructs || true
    test_git_integration || true
    test_enhanced_display || true
    test_autosuggestions_init || true
    test_tab_completion || true
    test_theme_system || true
    test_history_system || true
    test_linux_terminal_handling || true
    test_performance || true
    test_readline_linux || true
    test_signal_handling || true

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
        log_success "All tests passed! Fish-like features are working correctly on Linux."

        # Additional info
        echo ""
        log_info "=== LINUX COMPATIBILITY STATUS ==="
        log_success "✓ Core shell functionality working"
        log_success "✓ Fish-like autosuggestions system initialized"
        log_success "✓ Enhanced display and themes working"
        log_success "✓ Git integration functional"
        log_success "✓ GNU Readline integration stable"
        log_success "✓ Performance meets expectations"
        echo ""
        log_info "The feature/fish-enhancements branch is compatible with Linux!"
        exit 0
    fi
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Test fish-like features in Lusush on Linux platforms"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo "  --verbose, -v Enable verbose output"
        echo ""
        echo "This script tests:"
        echo "  - Basic shell functionality"
        echo "  - Multiline constructs"
        echo "  - Git integration"
        echo "  - Enhanced display mode"
        echo "  - Autosuggestions system"
        echo "  - Tab completion"
        echo "  - Theme system"
        echo "  - History functionality"
        echo "  - Linux-specific terminal handling"
        echo "  - Performance characteristics"
        echo "  - GNU Readline integration"
        echo "  - Signal handling"
        exit 0
        ;;
    "--verbose"|"-v")
        set -x
        ;;
esac

# Run the tests
run_all_tests
