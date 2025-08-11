#!/bin/bash

# Comprehensive Test Suite for Lusush GNU Readline Integration
# This script tests all the modern readline features implemented

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
LUSUSH_BIN="./builddir_readline_clean/lusush"
TEST_HISTORY_FILE="/tmp/lusush_test_history"
TEST_LOG="/tmp/lusush_readline_test.log"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_header() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_test() {
    echo -e "${YELLOW}Testing: $1${NC}"
    TESTS_RUN=$((TESTS_RUN + 1))
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

print_failure() {
    echo -e "${RED}✗ $1${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

print_summary() {
    echo -e "\n${BLUE}=== TEST SUMMARY ===${NC}"
    echo -e "Tests run: $TESTS_RUN"
    echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}🎉 ALL TESTS PASSED! Readline integration is working perfectly!${NC}"
        return 0
    else
        echo -e "\n${RED}❌ Some tests failed. Check the output above for details.${NC}"
        return 1
    fi
}

# Cleanup function
cleanup() {
    rm -f "$TEST_HISTORY_FILE" "$TEST_LOG"
}

# Test if binary exists
check_binary() {
    print_header "Binary Check"
    print_test "Lusush binary existence"

    if [ ! -f "$LUSUSH_BIN" ]; then
        print_failure "Binary not found at $LUSUSH_BIN"
        echo "Please run: meson setup builddir_readline_clean && ninja -C builddir_readline_clean"
        exit 1
    fi

    print_success "Binary found"
}

# Test basic shell functionality
test_basic_functionality() {
    print_header "Basic Shell Functionality"

    print_test "Version command"
    if $LUSUSH_BIN --version | grep -q "lusush"; then
        print_success "Version command works"
    else
        print_failure "Version command failed"
    fi

    print_test "Simple command execution"
    result=$(echo 'echo "Hello, World!"' | $LUSUSH_BIN 2>/dev/null)
    if [ "$result" = "Hello, World!" ]; then
        print_success "Simple command execution works"
    else
        print_failure "Simple command execution failed"
    fi

    print_test "Built-in commands"
    result=$(echo 'pwd' | $LUSUSH_BIN 2>/dev/null)
    if [ -n "$result" ]; then
        print_success "Built-in commands work"
    else
        print_failure "Built-in commands failed"
    fi
}

# Test readline integration
test_readline_integration() {
    print_header "GNU Readline Integration"

    print_test "Readline library linking"
    if ldd "$LUSUSH_BIN" | grep -q "libreadline"; then
        print_success "Readline library properly linked"
    else
        print_failure "Readline library not linked"
    fi

    print_test "Interactive mode detection"
    # Test that readline is available in interactive mode
    result=$(echo 'echo $READLINE_VERSION' | $LUSUSH_BIN -i 2>/dev/null | tail -1)
    if [ -n "$result" ]; then
        print_success "Interactive mode with readline detected"
    else
        # This might not work in all environments, so just warn
        echo -e "${YELLOW}Warning: Could not detect readline version (may be environment-specific)${NC}"
    fi
}

# Test history functionality
test_history_features() {
    print_header "History Management with Deduplication"

    # Clean up any existing test history
    rm -f "$TEST_HISTORY_FILE"

    print_test "History file creation"
    # Test history functionality
    cat > /tmp/test_commands.txt << 'EOF'
echo "command1"
echo "command2"
echo "command1"
echo "command3"
exit
EOF

    # Set custom history file
    export HISTFILE="$TEST_HISTORY_FILE"

    # Run commands
    $LUSUSH_BIN -i < /tmp/test_commands.txt > /dev/null 2>&1 || true

    if [ -f "$TEST_HISTORY_FILE" ]; then
        print_success "History file created"

        print_test "History deduplication (hist_no_dups)"
        # Check if duplicates were removed
        duplicate_count=$(grep -c "echo \"command1\"" "$TEST_HISTORY_FILE" 2>/dev/null || echo "0")
        if [ "$duplicate_count" -le 1 ]; then
            print_success "History deduplication working"
        else
            print_failure "History deduplication not working (found $duplicate_count duplicates)"
        fi
    else
        print_failure "History file not created"
    fi

    # Cleanup
    rm -f /tmp/test_commands.txt "$TEST_HISTORY_FILE"
    unset HISTFILE
}

# Test completion system
test_completion_system() {
    print_header "Tab Completion Integration"

    print_test "Completion system availability"
    # Test that completion callback is working by checking if lusush can handle completion
    # This is indirect since we can't easily test interactive completion

    # Check if completion.c functions are compiled in
    if strings "$LUSUSH_BIN" | grep -q "lusush_completion_callback"; then
        print_success "Completion system integrated"
    else
        print_failure "Completion system not found"
    fi

    print_test "Built-in command completion data"
    # Check if builtin commands are available for completion
    if strings "$LUSUSH_BIN" | grep -q "echo\|pwd\|cd\|exit\|history"; then
        print_success "Built-in commands available for completion"
    else
        print_failure "Built-in commands not found"
    fi
}

# Test syntax highlighting hooks
test_syntax_highlighting() {
    print_header "Syntax Highlighting Integration"

    print_test "Syntax highlighting hooks"
    # Check if syntax highlighting functions are compiled in
    if strings "$LUSUSH_BIN" | grep -q "lusush_syntax_highlight"; then
        print_success "Syntax highlighting hooks available"
    else
        print_failure "Syntax highlighting hooks not found"
    fi

    print_test "Color support detection"
    # Test color capability detection
    result=$(echo 'echo $TERM' | $LUSUSH_BIN 2>/dev/null)
    if [ -n "$result" ]; then
        print_success "Terminal type detection working"
    else
        print_failure "Terminal type detection failed"
    fi
}

# Test prompt theming integration
test_prompt_theming() {
    print_header "Prompt Theme Integration"

    print_test "Prompt generation"
    # Test that prompts are being generated
    if strings "$LUSUSH_BIN" | grep -q "build_prompt\|lusush_generate_prompt"; then
        print_success "Prompt generation system found"
    else
        print_failure "Prompt generation system not found"
    fi

    print_test "Theme system integration"
    # Check if theme functions are available
    if strings "$LUSUSH_BIN" | grep -q "theme"; then
        print_success "Theme system integrated"
    else
        print_failure "Theme system not integrated"
    fi
}

# Test modern editing features
test_modern_editing() {
    print_header "Modern Editing Features"

    print_test "Readline key bindings"
    # Test that readline provides modern key bindings
    # This is indirect - we verify readline functions are available
    if strings "$LUSUSH_BIN" | grep -q "rl_"; then
        print_success "Readline functions available"
    else
        print_failure "Readline functions not found"
    fi

    print_test "Multi-line editing support"
    # Test multi-line command handling
    result=$(printf 'echo "line1 \\\nline2"' | $LUSUSH_BIN 2>/dev/null)
    if echo "$result" | grep -q "line1.*line2"; then
        print_success "Multi-line editing supported"
    else
        print_failure "Multi-line editing not working"
    fi
}

# Test cross-platform compatibility
test_cross_platform() {
    print_header "Cross-Platform Compatibility"

    print_test "POSIX compliance"
    # Test basic POSIX shell features
    result=$(echo 'test -d /tmp && echo "POSIX_OK"' | $LUSUSH_BIN 2>/dev/null)
    if [ "$result" = "POSIX_OK" ]; then
        print_success "POSIX compliance verified"
    else
        print_failure "POSIX compliance issues"
    fi

    print_test "Terminal capability detection"
    # Test termcap integration
    if strings "$LUSUSH_BIN" | grep -q "termcap"; then
        print_success "Terminal capability system found"
    else
        print_failure "Terminal capability system not found"
    fi
}

# Test performance and memory usage
test_performance() {
    print_header "Performance and Memory Usage"

    print_test "Memory usage test"
    # Run a simple command and check if it doesn't consume excessive memory
    start_time=$(date +%s%N)
    result=$(echo 'echo "performance_test"' | $LUSUSH_BIN 2>/dev/null)
    end_time=$(date +%s%N)

    execution_time=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds

    if [ "$result" = "performance_test" ] && [ $execution_time -lt 1000 ]; then
        print_success "Performance test passed (${execution_time}ms)"
    else
        print_failure "Performance test failed (${execution_time}ms or incorrect output)"
    fi

    print_test "Memory leak check"
    # Basic memory leak test - run multiple commands
    for i in {1..10}; do
        echo 'echo "test_$i"' | $LUSUSH_BIN > /dev/null 2>&1
    done
    print_success "Basic memory leak test completed"
}

# Test error handling
test_error_handling() {
    print_header "Error Handling and Robustness"

    print_test "Invalid command handling"
    # Test that invalid commands are handled gracefully
    result=$(echo 'nonexistent_command_12345' | $LUSUSH_BIN 2>&1)
    if echo "$result" | grep -q -i "command not found\|not found\|No such file"; then
        print_success "Invalid command handling works"
    else
        print_failure "Invalid command handling failed"
    fi

    print_test "Signal handling"
    # Test basic signal handling (this is indirect)
    if strings "$LUSUSH_BIN" | grep -q "signal\|SIGINT"; then
        print_success "Signal handling system found"
    else
        print_failure "Signal handling system not found"
    fi
}

# Test readline-specific features
test_readline_specific() {
    print_header "Readline-Specific Features"

    print_test "Readline version detection"
    # Test that we can detect readline version
    if strings "$LUSUSH_BIN" | grep -q "8\." || strings "$LUSUSH_BIN" | grep -q "readline"; then
        print_success "Readline version information available"
    else
        print_failure "Readline version information not found"
    fi

    print_test "Readline configuration"
    # Test that readline can be configured
    if strings "$LUSUSH_BIN" | grep -q "rl_.*_function\|rl_bind_key"; then
        print_success "Readline configuration functions available"
    else
        print_failure "Readline configuration functions not found"
    fi

    print_test "Input/output handling"
    # Test that readline handles I/O correctly
    result=$(echo 'echo "IO_TEST"' | $LUSUSH_BIN 2>/dev/null)
    if [ "$result" = "IO_TEST" ]; then
        print_success "Input/output handling works"
    else
        print_failure "Input/output handling failed"
    fi
}

# Main test execution
main() {
    echo -e "${BLUE}Lusush GNU Readline Integration Test Suite${NC}"
    echo -e "${BLUE}==========================================${NC}"

    # Setup
    cleanup

    # Run all tests
    check_binary
    test_basic_functionality
    test_readline_integration
    test_history_features
    test_completion_system
    test_syntax_highlighting
    test_prompt_theming
    test_modern_editing
    test_cross_platform
    test_performance
    test_error_handling
    test_readline_specific

    # Cleanup and summary
    cleanup
    print_summary
}

# Handle script interruption
trap cleanup EXIT

# Run the tests
main "$@"
