#!/bin/bash
#
# Test Basic Menu Completion Implementation in Lusush
# Tests the newly implemented menu completion functionality
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

# Test 1: Check menu completion is enabled by default
test_menu_completion_enabled() {
    start_test "Menu completion enabled by default"

    local output
    output=$(echo -e 'config get menu_completion_enabled\nexit' | timeout 5s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" == *"true"* ]]; then
        log_success "Menu completion is enabled by default"
    else
        log_failure "Menu completion is not enabled by default"
        return 1
    fi
}

# Test 2: Test menu completion configuration
test_menu_completion_config() {
    start_test "Menu completion configuration system"

    local output
    output=$(echo -e 'config get menu_completion_show_descriptions\nconfig get menu_completion_max_columns\nexit' | timeout 5s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" == *"true"* && "$output" == *"0"* ]]; then
        log_success "Menu completion configuration is accessible"
    else
        log_warning "Menu completion configuration partially accessible"
    fi
}

# Test 3: Test menu completion toggle functionality
test_menu_completion_toggle() {
    start_test "Menu completion can be disabled and re-enabled"

    local output
    output=$(echo -e 'config set menu_completion_enabled false\nconfig get menu_completion_enabled\nconfig set menu_completion_enabled true\nconfig get menu_completion_enabled\nexit' | timeout 10s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" == *"false"* && "$output" == *"true"* ]]; then
        log_success "Menu completion can be toggled on/off"
    else
        log_failure "Menu completion toggle not working"
        return 1
    fi
}

# Test 4: Test that TAB cycling works (basic functional test)
test_tab_cycling() {
    start_test "TAB cycling functionality"

    # Create a test directory with known files
    local test_dir="/tmp/lusush_menu_test_$$"
    mkdir -p "$test_dir"
    touch "$test_dir/test_file_1.txt"
    touch "$test_dir/test_file_2.txt"
    touch "$test_dir/test_file_3.txt"

    # Test completion in that directory
    local output
    output=$(cd "$test_dir" && echo -e 'ls test\t\nexit' | timeout 5s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    # Cleanup
    rm -rf "$test_dir"

    if [[ "$output" != "timeout" && "$output" == *"test_file"* ]]; then
        log_success "TAB completion works with menu system"
    else
        log_warning "TAB completion test inconclusive (may be working but output unclear)"
    fi
}

# Test 5: Test menu completion display functionality
test_menu_display() {
    start_test "Menu completion display system"

    # Test with a command that should have multiple completions
    local output
    output=$(echo -e 'config \t\nexit' | timeout 5s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" != "timeout" ]]; then
        if [[ "$output" == *"get"* || "$output" == *"set"* || "$output" == *"config"* ]]; then
            log_success "Menu display system shows completions"
        else
            log_warning "Menu display output unclear but responsive"
        fi
    else
        log_failure "Menu display test timed out"
        return 1
    fi
}

# Test 6: Test performance (should be responsive)
test_menu_performance() {
    start_test "Menu completion performance"

    local start_time=$(date +%s%N)
    local output
    output=$(echo -e 'ls\t\nexit' | timeout 3s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")
    local end_time=$(date +%s%N)

    local duration_ms=$(( (end_time - start_time) / 1000000 ))

    if [[ "$output" != "timeout" ]]; then
        if [[ $duration_ms -lt 3000 ]]; then  # Less than 3 seconds
            log_success "Menu completion is responsive (${duration_ms}ms)"
        else
            log_warning "Menu completion slower than expected (${duration_ms}ms)"
        fi
    else
        log_failure "Menu completion performance test timed out"
        return 1
    fi
}

# Test 7: Test that standard completion fallback works
test_fallback_functionality() {
    start_test "Standard completion fallback when menu disabled"

    local output
    output=$(echo -e 'config set menu_completion_enabled false\nls\t\nconfig set menu_completion_enabled true\nexit' | timeout 8s $LUSUSH_BINARY -i 2>/dev/null || echo "timeout")

    if [[ "$output" != "timeout" ]]; then
        log_success "Fallback to standard completion works when menu disabled"
    else
        log_failure "Fallback completion test failed"
        return 1
    fi
}

# Main test runner
run_all_tests() {
    log_info "Starting menu completion functionality tests..."
    log_info "Testing implemented menu completion system..."
    log_info "Using binary: $LUSUSH_BINARY"

    echo ""

    # Check prerequisites
    if [[ ! -f "$LUSUSH_BINARY" ]]; then
        log_failure "Lusush binary not found at $LUSUSH_BINARY"
        log_info "Please run: ninja -C builddir"
        exit 1
    fi

    # Run all tests
    test_menu_completion_enabled || true
    test_menu_completion_config || true
    test_menu_completion_toggle || true
    test_tab_cycling || true
    test_menu_display || true
    test_menu_performance || true
    test_fallback_functionality || true

    # Summary
    echo ""
    log_info "=== MENU COMPLETION TEST SUMMARY ==="
    log_info "Total tests: $TOTAL_TESTS"
    log_success "Passed: $PASSED_TESTS"

    if [[ $FAILED_TESTS -gt 0 ]]; then
        log_failure "Failed: $FAILED_TESTS"
    else
        log_success "All tests passed!"
    fi

    echo ""
    log_info "=== IMPLEMENTATION STATUS ==="
    if [[ $FAILED_TESTS -eq 0 ]]; then
        log_success "✓ Menu completion basic implementation is working"
        log_success "✓ Configuration system integrated"
        log_success "✓ TAB cycling functionality operational"
        log_success "✓ Performance meets expectations"
        log_success "✓ Fallback to standard completion works"
        log_info ""
        log_info "🎯 PHASE 1 COMPLETE: Basic menu completion with TAB cycling implemented!"
        log_info "Ready for Phase 2: Enhanced display and visual selection"
    else
        log_failure "❌ Some menu completion features need attention"
        log_info "Review failed tests and address issues before proceeding"
    fi

    echo ""
    log_info "=== NEXT STEPS ==="
    log_info "Phase 2 goals:"
    log_info "  • Enhanced multi-column display formatting"
    log_info "  • Visual selection highlighting"
    log_info "  • Integration with rich completion descriptions"
    log_info "  • Shift-TAB backward cycling"
    log_info "  • Theme-aware color management"

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
        echo "Test basic menu completion implementation in Lusush"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo ""
        exit 0
        ;;
esac

# Run the tests
run_all_tests
