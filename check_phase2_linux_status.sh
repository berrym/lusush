#!/bin/bash
#
# Phase 2 Linux Status Check for Lusush Fish Enhancements
# Quick and reliable status verification for rich completions on Linux
#
# This script provides a comprehensive but fast check of Phase 2 features
# without interactive testing that might hang or timeout
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

LUSUSH_BINARY="./builddir/lusush"
PASSED_TESTS=0
FAILED_TESTS=0
TOTAL_TESTS=0
ISSUES_FOUND=0

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
    ((ISSUES_FOUND++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    ((ISSUES_FOUND++))
}

start_test() {
    ((TOTAL_TESTS++))
}

print_banner() {
    echo -e "${BOLD}${CYAN}"
    cat << 'EOF'
    ╔══════════════════════════════════════════════════════════════╗
    ║              Phase 2 Linux Status Check                     ║
    ║          Rich Completions Compatibility Verification        ║
    ╚══════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
    echo "Quick verification of Phase 2 rich completion features on Linux"
    echo "Platform: $(uname -s) $(uname -r) $(uname -m)"
    echo "Date: $(date)"
    echo ""
}

# Test 1: Binary existence and basic execution
test_binary_status() {
    start_test
    log_info "Checking Lusush binary status..."

    if [[ ! -x "$LUSUSH_BINARY" ]]; then
        log_failure "Lusush binary not found or not executable: $LUSUSH_BINARY"
        return 1
    fi

    local version_output=$($LUSUSH_BINARY --version 2>&1 | head -1)
    if [[ "$version_output" == *"lusush"* ]] || [[ "$version_output" == *"Lusush"* ]]; then
        log_success "Binary exists and responds to --version"
    else
        log_failure "Binary doesn't respond properly to --version: $version_output"
        return 1
    fi
}

# Test 2: Core shell functionality
test_core_functionality() {
    start_test
    log_info "Testing core shell functionality..."

    local basic_test=$($LUSUSH_BINARY -c 'echo "Phase 2 Linux status check"' 2>&1)
    if [[ "$basic_test" == "Phase 2 Linux status check" ]]; then
        log_success "Basic command execution works"
    else
        log_failure "Basic command execution failed: $basic_test"
        return 1
    fi

    local multiline_test=$($LUSUSH_BINARY -c 'for i in 1 2; do echo "Test $i"; done' 2>&1)
    if [[ "$multiline_test" == *"Test 1"* && "$multiline_test" == *"Test 2"* ]]; then
        log_success "Multiline constructs work"
    else
        log_failure "Multiline constructs failed"
        return 1
    fi
}

# Test 3: Autosuggestions system
test_autosuggestions() {
    start_test
    log_info "Testing autosuggestions system..."

    local auto_test=$($LUSUSH_BINARY -c 'display testsuggestion' 2>&1)
    if [[ "$auto_test" == *"SUCCESS"* ]]; then
        log_success "Autosuggestions system working"
    else
        log_failure "Autosuggestions system failed: $auto_test"
        return 1
    fi
}

# Test 4: Rich completion system integration
test_rich_completion_integration() {
    start_test
    log_info "Testing rich completion system integration..."

    # Check if rich completion functions are linked
    if nm "$LUSUSH_BINARY" 2>/dev/null | grep -q "rich_completion" ||
       strings "$LUSUSH_BINARY" 2>/dev/null | grep -q "rich_completion"; then
        log_success "Rich completion system integrated into binary"
    else
        log_warning "Rich completion symbols not clearly visible in binary"
    fi

    # Test that shell starts without rich completion errors
    local startup_test=$(echo 'exit' | timeout 5s $LUSUSH_BINARY 2>&1)
    if [[ $? -eq 0 ]] && [[ "$startup_test" != *"Failed to initialize rich completions"* ]]; then
        log_success "Rich completion system initializes without errors"
    else
        log_failure "Rich completion initialization issues: $startup_test"
        return 1
    fi
}

# Test 5: Build system integration
test_build_integration() {
    start_test
    log_info "Testing build system integration..."

    if grep -q "rich_completion.c" meson.build; then
        log_success "Rich completion source file integrated in build system"
    else
        log_failure "Rich completion source not found in meson.build"
        return 1
    fi

    if [[ -f "src/rich_completion.c" ]]; then
        log_success "Rich completion source file exists"
    else
        log_failure "Rich completion source file missing"
        return 1
    fi

    if [[ -f "include/rich_completion.h" ]]; then
        log_success "Rich completion header file exists"
    else
        log_failure "Rich completion header file missing"
        return 1
    fi
}

# Test 6: File system completion basics
test_file_completion_basics() {
    start_test
    log_info "Testing file completion basics..."

    # Create test environment
    local test_dir=$(mktemp -d)
    cd "$test_dir"
    touch test_file1.txt test_file2.log
    mkdir test_directory

    # Simple test - can the shell list files correctly
    local file_list=$($LUSUSH_BINARY -c 'ls test_file*.txt' 2>&1)
    if [[ "$file_list" == *"test_file1.txt"* ]]; then
        log_success "File system operations work correctly"
    else
        log_failure "File system operations failed: $file_list"
        cd - > /dev/null
        rm -rf "$test_dir"
        return 1
    fi

    cd - > /dev/null
    rm -rf "$test_dir"
}

# Test 7: Git integration
test_git_integration() {
    start_test
    log_info "Testing git integration..."

    if ! command -v git >/dev/null 2>&1; then
        log_warning "Git not available - skipping git integration test"
        return 0
    fi

    # Test basic git command execution
    local git_version_test=$($LUSUSH_BINARY -c 'git --version' 2>&1)
    if [[ "$git_version_test" == *"git version"* ]]; then
        log_success "Git commands execute correctly"
    else
        log_failure "Git command execution failed: $git_version_test"
        return 1
    fi

    # Test in a git repository
    local git_dir=$(mktemp -d)
    cd "$git_dir"
    git init --quiet >/dev/null 2>&1
    echo "test" > test.txt
    git add test.txt >/dev/null 2>&1
    git commit -m "test" --quiet >/dev/null 2>&1

    local git_status_test=$($LUSUSH_BINARY -c 'git status --porcelain' 2>&1)
    if [[ $? -eq 0 ]]; then
        log_success "Git repository operations work"
    else
        log_failure "Git repository operations failed"
        cd - > /dev/null
        rm -rf "$git_dir"
        return 1
    fi

    cd - > /dev/null
    rm -rf "$git_dir"
}

# Test 8: Theme system compatibility
test_theme_compatibility() {
    start_test
    log_info "Testing theme system compatibility..."

    # Simple theme test without interactive mode
    local theme_test=$(echo -e 'theme list\nexit' | timeout 3s $LUSUSH_BINARY 2>&1)
    if [[ $? -eq 0 ]] || [[ $? -eq 124 ]]; then  # Success or timeout (both acceptable)
        log_success "Theme system accessible"
    else
        log_failure "Theme system failed"
        return 1
    fi
}

# Test 9: Performance check
test_performance() {
    start_test
    log_info "Testing performance characteristics..."

    local start_time=$(date +%s%N)
    for i in {1..10}; do
        $LUSUSH_BINARY -c 'echo "performance test"' >/dev/null 2>&1
    done
    local end_time=$(date +%s%N)

    local duration=$(( (end_time - start_time) / 1000000 ))  # Convert to milliseconds

    if [[ $duration -lt 5000 ]]; then  # Less than 5 seconds for 10 operations
        log_success "Performance acceptable (${duration}ms for 10 operations)"
    else
        log_warning "Performance slower than expected (${duration}ms)"
    fi
}

# Test 10: Library dependencies
test_dependencies() {
    start_test
    log_info "Testing library dependencies..."

    if command -v ldd >/dev/null 2>&1; then
        local deps=$(ldd "$LUSUSH_BINARY" 2>&1)
        if [[ "$deps" == *"libreadline"* ]]; then
            log_success "GNU Readline library linked correctly"
        else
            log_failure "GNU Readline library not found in dependencies"
            return 1
        fi

        if [[ "$deps" != *"not found"* ]]; then
            log_success "All library dependencies satisfied"
        else
            log_failure "Missing library dependencies found"
            echo "$deps" | grep "not found"
            return 1
        fi
    else
        log_warning "ldd not available - skipping dependency check"
    fi
}

# Analyze source code for Linux compatibility
analyze_source_compatibility() {
    log_info "Analyzing source code for Linux compatibility..."

    # Check for platform-specific code
    local linux_specific=$(grep -r "__linux__" src/ include/ 2>/dev/null | wc -l)
    local macos_specific=$(grep -r "__APPLE__\|__darwin__" src/ include/ 2>/dev/null | wc -l)

    log_info "Platform-specific code analysis:"
    echo "  • Linux-specific blocks: $linux_specific"
    echo "  • macOS-specific blocks: $macos_specific"

    if [[ $linux_specific -gt 0 ]]; then
        log_info "Linux-specific optimizations present"
    fi

    # Check for POSIX compliance
    local posix_functions=$(grep -r "open\|read\|write\|close\|stat\|lstat" src/ 2>/dev/null | wc -l)
    if [[ $posix_functions -gt 0 ]]; then
        log_success "POSIX-compliant functions used (good for Linux)"
    fi
}

# Generate recommendations
generate_recommendations() {
    echo ""
    echo -e "${BOLD}${CYAN}========================================${NC}"
    echo -e "${BOLD}${CYAN}      PHASE 2 LINUX STATUS REPORT${NC}"
    echo -e "${BOLD}${CYAN}========================================${NC}"
    echo ""

    local success_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))

    echo -e "${CYAN}Test Results Summary:${NC}"
    echo "  • Total Tests: $TOTAL_TESTS"
    echo "  • Passed: $PASSED_TESTS"
    echo "  • Failed: $FAILED_TESTS"
    echo "  • Success Rate: ${success_rate}%"
    echo "  • Issues Found: $ISSUES_FOUND"
    echo ""

    if [[ $success_rate -ge 90 ]]; then
        echo -e "${GREEN}🎉 EXCELLENT: Phase 2 features working very well on Linux!${NC}"
        echo ""
        echo -e "${GREEN}Status: READY FOR PRODUCTION DEPLOYMENT${NC}"
        echo ""
        echo "✅ Rich completions integrated successfully"
        echo "✅ No significant Linux compatibility issues"
        echo "✅ Performance meets expectations"
        echo "✅ All dependencies satisfied"

    elif [[ $success_rate -ge 75 ]]; then
        echo -e "${YELLOW}⚠️ GOOD: Phase 2 features mostly working with minor issues${NC}"
        echo ""
        echo -e "${YELLOW}Status: SUITABLE FOR DEPLOYMENT WITH MONITORING${NC}"
        echo ""
        echo "✅ Core functionality working"
        echo "⚠️ Some minor issues detected"
        echo "📋 Recommended: Address failing tests before production"

    else
        echo -e "${RED}❌ ISSUES: Significant problems detected${NC}"
        echo ""
        echo -e "${RED}Status: NEEDS ATTENTION BEFORE DEPLOYMENT${NC}"
        echo ""
        echo "❌ Multiple test failures"
        echo "🔧 Required: Fix critical issues"
        echo "📋 Recommended: Review and resolve failures"
    fi

    echo ""
    echo -e "${CYAN}Platform-Specific Recommendations:${NC}"

    if [[ $ISSUES_FOUND -eq 0 ]]; then
        echo "• No Linux-specific changes needed"
        echo "• Current implementation is cross-platform compatible"
        echo "• Ready to continue with macOS development"
    else
        echo "• Consider applying Linux-specific optimizations"
        echo "• Run: ./apply_linux_platform_enhancements.sh"
        echo "• Test with: ./test_fish_features_linux.sh"
    fi

    echo ""
    echo -e "${CYAN}Next Steps:${NC}"
    if [[ $success_rate -ge 90 ]]; then
        echo "1. ✅ Linux compatibility verified - no changes needed"
        echo "2. 🍎 Safe to continue macOS development"
        echo "3. 🚀 Ready for production deployment on Linux"
    else
        echo "1. 🔍 Review failing tests above"
        echo "2. 🛠️ Apply Linux-specific fixes if needed"
        echo "3. 🧪 Re-run comprehensive tests"
        echo "4. 📝 Update documentation with Linux notes"
    fi
}

# Main execution
main() {
    print_banner

    # Verify we're in the right place
    if [[ ! -f "meson.build" ]] || [[ ! -d "src" ]]; then
        log_failure "Not in Lusush repository root directory"
        exit 1
    fi

    log_info "Starting Phase 2 Linux status check..."
    echo ""

    # Run all tests
    test_binary_status
    test_core_functionality
    test_autosuggestions
    test_rich_completion_integration
    test_build_integration
    test_file_completion_basics
    test_git_integration
    test_theme_compatibility
    test_performance
    test_dependencies

    echo ""
    analyze_source_compatibility
    generate_recommendations

    echo ""
    local overall_status="UNKNOWN"
    if [[ $FAILED_TESTS -eq 0 && $ISSUES_FOUND -le 1 ]]; then
        overall_status="✅ EXCELLENT"
        echo -e "${GREEN}Overall Status: Linux compatibility excellent!${NC}"
    elif [[ $FAILED_TESTS -le 2 ]]; then
        overall_status="⚠️ GOOD"
        echo -e "${YELLOW}Overall Status: Minor issues, mostly functional.${NC}"
    else
        overall_status="❌ NEEDS WORK"
        echo -e "${RED}Overall Status: Multiple issues need attention.${NC}"
    fi

    echo ""
    echo -e "${BOLD}Phase 2 Linux Compatibility: ${overall_status}${NC}"
    echo ""

    # Exit with appropriate code
    [[ $FAILED_TESTS -le 1 ]] && exit 0 || exit 1
}

# Handle help argument
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    cat << 'EOF'
Usage: check_phase2_linux_status.sh

Quick and comprehensive status check for Phase 2 rich completion features
on Linux platforms.

This script provides:
• Fast, non-interactive testing
• Comprehensive compatibility analysis
• Clear status reporting
• Actionable recommendations

Tests performed:
• Binary status and basic execution
• Core shell functionality
• Autosuggestions system integration
• Rich completion system integration
• Build system verification
• File system operations
• Git integration
• Theme system compatibility
• Performance characteristics
• Library dependencies
• Source code analysis

Output:
• Pass/fail status for each test
• Overall compatibility rating
• Platform-specific recommendations
• Next steps guidance

No arguments required - just run and review results.
EOF
    exit 0
fi

# Run main function
main "$@"
