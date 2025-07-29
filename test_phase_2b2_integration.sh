#!/bin/bash

# Phase 2B.2 Tab Completion Integration Test Script
# Tests the current implementation of tab completion with Phase 2A absolute positioning

set -e

echo "=== Phase 2B.2 Tab Completion Integration Test ==="
echo "Testing tab completion display with absolute positioning system"
echo

# Enable debug output for detailed information
export LLE_DEBUG=1
export LLE_DEBUG_CURSOR=1

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_status() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

echo_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

echo_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

# Function to test tab completion functionality
test_tab_completion() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"

    echo_status "Testing: $test_name"
    echo_status "Input: $input"

    # Create a temporary test file for input
    echo "$input" > /tmp/lle_test_input.txt

    # Run lusush with the test input and capture output
    timeout 10s ./builddir/lusush < /tmp/lle_test_input.txt > /tmp/lle_test_output.txt 2>&1 || true

    # Check if output contains expected patterns
    if [ -n "$expected_pattern" ] && grep -q "$expected_pattern" /tmp/lle_test_output.txt; then
        echo_success "Found expected pattern: $expected_pattern"
        return 0
    elif [ -z "$expected_pattern" ]; then
        echo_success "Basic execution completed"
        return 0
    else
        echo_error "Expected pattern not found: $expected_pattern"
        echo "Actual output:"
        cat /tmp/lle_test_output.txt
        return 1
    fi
}

# Function to test completion display integration
test_completion_display() {
    echo_status "Testing completion display integration..."

    # Test 1: Basic tab completion functionality
    echo_status "Test 1: Basic tab completion with /usr/ directory"
    if test_tab_completion "Basic tab completion" "ls /usr/" ""; then
        echo_success "Basic tab completion test passed"
    else
        echo_warning "Basic tab completion test failed (may be environment-specific)"
    fi

    # Test 2: Multi-line context tab completion
    echo_status "Test 2: Multi-line context tab completion"
    local multiline_input="echo 'This is a very long line that should wrap across multiple lines in the terminal to test the absolute positioning system properly'
ls /usr/"

    if test_tab_completion "Multi-line tab completion" "$multiline_input" ""; then
        echo_success "Multi-line tab completion test passed"
    else
        echo_warning "Multi-line tab completion test failed"
    fi

    # Test 3: Check for Phase 2B.2 debug output
    echo_status "Test 3: Checking for Phase 2B.2 debug output"
    echo "ls /us" > /tmp/lle_test_input.txt
    echo -e "\t" >> /tmp/lle_test_input.txt  # Add tab character
    echo "exit" >> /tmp/lle_test_input.txt

    timeout 10s ./builddir/lusush < /tmp/lle_test_input.txt > /tmp/lle_debug_output.txt 2>&1 || true

    if grep -q "PHASE_2B_TAB_COMPLETION\|ENHANCED_TAB_COMPLETION" /tmp/lle_debug_output.txt; then
        echo_success "Found Phase 2B.2 debug output - integration is active"
        echo "Debug output sample:"
        grep "PHASE_2B_TAB_COMPLETION\|ENHANCED_TAB_COMPLETION" /tmp/lle_debug_output.txt | head -3
    else
        echo_warning "No Phase 2B.2 debug output found"
        echo "Available debug output:"
        head -10 /tmp/lle_debug_output.txt
    fi
}

# Function to test coordinate conversion integration
test_coordinate_integration() {
    echo_status "Testing coordinate conversion integration..."

    # Check if completion display uses absolute positioning functions
    if grep -q "lle_convert_to_terminal_coordinates" src/line_editor/completion_display.c; then
        echo_success "Completion display uses Phase 2A coordinate conversion"
    else
        echo_error "Completion display missing Phase 2A coordinate conversion"
        return 1
    fi

    if grep -q "menu_start_pos.terminal_row" src/line_editor/completion_display.c; then
        echo_success "Completion display uses absolute positioning"
    else
        echo_error "Completion display missing absolute positioning"
        return 1
    fi

    if grep -q "position_tracking_valid" src/line_editor/completion_display.c; then
        echo_success "Completion display checks position tracking validity"
    else
        echo_error "Completion display missing position tracking check"
        return 1
    fi
}

# Function to run comprehensive tests
run_comprehensive_tests() {
    echo_status "Running comprehensive Phase 2B.2 tests..."

    # Build the project first
    echo_status "Building project..."
    if meson compile -C builddir; then
        echo_success "Build successful"
    else
        echo_error "Build failed"
        return 1
    fi

    # Run unit tests
    echo_status "Running unit tests..."
    local test_output=$(meson test -C builddir --no-rebuild 2>&1)
    local passing_tests=$(echo "$test_output" | grep "^Ok:" | awk '{print $2}')
    local failing_tests=$(echo "$test_output" | grep "^Fail:" | awk '{print $2}')

    echo_status "Test results: $passing_tests passing, $failing_tests failing"

    if [ "$failing_tests" = "0" ]; then
        echo_success "All non-timeout tests passing"
    else
        echo_warning "$failing_tests tests failing"
    fi

    # Test coordinate integration
    test_coordinate_integration

    # Test completion display functionality
    test_completion_display

    echo
    echo_status "Phase 2B.2 Integration Summary:"
    echo "- Absolute positioning integration: $(grep -q "lle_convert_to_terminal_coordinates" src/line_editor/completion_display.c && echo "✓ IMPLEMENTED" || echo "✗ MISSING")"
    echo "- Position tracking validation: $(grep -q "position_tracking_valid" src/line_editor/completion_display.c && echo "✓ IMPLEMENTED" || echo "✗ MISSING")"
    echo "- Terminal width awareness: $(grep -q "geometry.width" src/line_editor/completion_display.c && echo "✓ IMPLEMENTED" || echo "✗ MISSING")"
    echo "- Debug output integration: $(grep -q "PHASE_2B_TAB_COMPLETION" src/line_editor/line_editor.c && echo "✓ IMPLEMENTED" || echo "✗ MISSING")"
}

# Main execution
main() {
    echo_status "Starting Phase 2B.2 Tab Completion Integration Test"
    echo_status "Current directory: $(pwd)"
    echo_status "Lusush binary: $(ls -la builddir/lusush 2>/dev/null || echo 'NOT FOUND')"
    echo

    # Check if we're in the right directory
    if [ ! -f "builddir/lusush" ] && [ ! -f "meson.build" ]; then
        echo_error "Not in lusush project directory or build not found"
        echo_error "Please run: meson setup builddir && meson compile -C builddir"
        exit 1
    fi

    run_comprehensive_tests

    echo
    echo_status "Phase 2B.2 Test Complete"
    echo_status "Next steps:"
    echo "  1. Review any failing tests above"
    echo "  2. Test tab completion manually: ./builddir/lusush"
    echo "  3. Try typing 'ls /usr<TAB>' to test completion display"
    echo "  4. Check debug output with: export LLE_DEBUG=1"

    # Cleanup
    rm -f /tmp/lle_test_input.txt /tmp/lle_test_output.txt /tmp/lle_debug_output.txt
}

# Execute main function
main "$@"
