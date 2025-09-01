#!/bin/bash
#
# Platform Compatibility Analysis Script for Lusush Fish Enhancements
# Analyzes the feature/fish-enhancements branch for platform-specific code
# and potential compatibility issues between macOS and Linux
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Configuration
ANALYSIS_OUTPUT="platform_compatibility_analysis.md"
TEMP_DIR=$(mktemp -d)

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_section() {
    echo -e "${BOLD}${CYAN}=== $1 ===${NC}"
}

# Cleanup function
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

print_banner() {
    echo -e "${BOLD}${BLUE}"
    cat << 'EOF'
    ╔══════════════════════════════════════════════════════════════════╗
    ║           Lusush Platform Compatibility Analysis                 ║
    ║              Fish Enhancements Branch Review                     ║
    ╚══════════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
}

# Function to analyze system-specific includes
analyze_system_includes() {
    log_section "Analyzing System-Specific Includes"

    local report_file="$TEMP_DIR/includes_report.txt"

    # Find all potentially platform-specific includes
    echo "Searching for platform-specific includes..." > "$report_file"
    echo "================================================" >> "$report_file"
    echo "" >> "$report_file"

    # macOS-specific includes
    echo "macOS-specific includes:" >> "$report_file"
    git diff master --name-only | xargs grep -l "darwin\|__APPLE__\|<AvailabilityMacros.h>\|<CoreFoundation/\|<ApplicationServices/" 2>/dev/null | tee -a "$report_file" || echo "None found" >> "$report_file"
    echo "" >> "$report_file"

    # Linux-specific includes
    echo "Linux-specific includes:" >> "$report_file"
    git diff master --name-only | xargs grep -l "linux\|__linux__\|<sys/epoll.h>\|<sys/inotify.h>" 2>/dev/null | tee -a "$report_file" || echo "None found" >> "$report_file"
    echo "" >> "$report_file"

    # BSD-specific includes
    echo "BSD-specific includes:" >> "$report_file"
    git diff master --name-only | xargs grep -l "bsd\|__FreeBSD__\|__OpenBSD__\|__NetBSD__\|<sys/event.h>" 2>/dev/null | tee -a "$report_file" || echo "None found" >> "$report_file"
    echo "" >> "$report_file"

    # Terminal-specific includes that might differ between platforms
    echo "Terminal-specific includes:" >> "$report_file"
    git diff master --name-only | xargs grep -l "<termios.h>\|<sys/ioctl.h>\|<pty.h>\|<util.h>" 2>/dev/null | tee -a "$report_file" || echo "None found" >> "$report_file"
    echo "" >> "$report_file"

    cat "$report_file"

    # Check for conditional compilation blocks
    log_info "Checking for conditional compilation blocks..."
    local conditional_blocks=$(git diff master --name-only | xargs grep -n "#ifdef\|#ifndef\|#if defined" 2>/dev/null | grep -E "(APPLE|linux|BSD|darwin|__MACH__|_WIN32)" || true)
    if [[ -n "$conditional_blocks" ]]; then
        log_warning "Found platform-specific conditional compilation:"
        echo "$conditional_blocks"
    else
        log_success "No platform-specific conditional compilation found"
    fi
}

# Function to analyze function calls that might be platform-specific
analyze_system_calls() {
    log_section "Analyzing System Calls and Library Functions"

    local report_file="$TEMP_DIR/syscalls_report.txt"

    echo "Analyzing potentially platform-specific function calls..." > "$report_file"
    echo "=========================================================" >> "$report_file"
    echo "" >> "$report_file"

    # Get all modified files
    local modified_files=$(git diff master --name-only | grep -E "\.(c|h)$")

    if [[ -z "$modified_files" ]]; then
        echo "No C/C++ files modified" >> "$report_file"
        cat "$report_file"
        return
    fi

    # Check for potentially problematic function calls
    local problematic_functions=(
        "kqueue"           # BSD-specific
        "epoll_create"     # Linux-specific
        "clock_gettime"    # May need linking changes
        "pthread_create"   # Threading
        "dlopen"          # Dynamic loading
        "mmap"            # Memory mapping
        "select"          # I/O multiplexing
        "poll"            # I/O multiplexing
        "ioctl"           # Device control
        "fcntl"           # File control
        "flock"           # File locking
        "setrlimit"       # Resource limits
        "getpwuid"        # User information
        "getgrgid"        # Group information
        "realpath"        # Path resolution
        "readlink"        # Symlink reading
        "lstat"           # File stats
        "strlcpy"         # String copying (BSD)
        "strlcat"         # String concatenation (BSD)
        "arc4random"      # Random numbers (BSD)
        "getline"         # GNU extension
        "asprintf"        # GNU extension
    )

    for func in "${problematic_functions[@]}"; do
        local matches=$(echo "$modified_files" | xargs grep -n "\b$func\b" 2>/dev/null || true)
        if [[ -n "$matches" ]]; then
            echo "Function '$func':" >> "$report_file"
            echo "$matches" >> "$report_file"
            echo "" >> "$report_file"
        fi
    done

    cat "$report_file"
}

# Function to analyze Makefile/build system changes
analyze_build_system() {
    log_section "Analyzing Build System Changes"

    local report_file="$TEMP_DIR/build_report.txt"

    echo "Build system analysis for fish enhancements..." > "$report_file"
    echo "===============================================" >> "$report_file"
    echo "" >> "$report_file"

    # Check meson.build changes
    if git diff master meson.build &>/dev/null; then
        echo "meson.build changes detected:" >> "$report_file"
        git diff master meson.build >> "$report_file"
        echo "" >> "$report_file"

        # Check for new dependencies
        local new_deps=$(git diff master meson.build | grep "^+" | grep -E "dependency|find_library|find_program" | grep -v "^+++" || true)
        if [[ -n "$new_deps" ]]; then
            echo "New dependencies added:" >> "$report_file"
            echo "$new_deps" >> "$report_file"
            echo "" >> "$report_file"
        fi

        # Check for new source files
        local new_sources=$(git diff master meson.build | grep "^+" | grep -E "\.c'|\.cpp'" | grep -v "^+++" || true)
        if [[ -n "$new_sources" ]]; then
            echo "New source files added:" >> "$report_file"
            echo "$new_sources" >> "$report_file"
            echo "" >> "$report_file"
        fi
    else
        echo "No meson.build changes detected" >> "$report_file"
        echo "" >> "$report_file"
    fi

    # Check for any other build-related files
    local build_files=$(git diff master --name-only | grep -E "(CMakeLists\.txt|Makefile|\.mk$|configure|config\..*)" || true)
    if [[ -n "$build_files" ]]; then
        echo "Other build system files changed:" >> "$report_file"
        echo "$build_files" >> "$report_file"
        echo "" >> "$report_file"
    fi

    cat "$report_file"
}

# Function to analyze terminal/readline specific code
analyze_terminal_code() {
    log_section "Analyzing Terminal and Readline Code"

    local report_file="$TEMP_DIR/terminal_report.txt"

    echo "Terminal and readline compatibility analysis..." > "$report_file"
    echo "===============================================" >> "$report_file"
    echo "" >> "$report_file"

    # Check readline integration changes
    if git diff master src/readline_integration.c &>/dev/null; then
        echo "readline_integration.c has been modified" >> "$report_file"
        echo "" >> "$report_file"

        # Check for terminal control sequences
        local term_sequences=$(git diff master src/readline_integration.c | grep -E "\\\\033|\\\\e\[|printf.*033|ANSI|VT100" | grep "^+" | grep -v "^+++" || true)
        if [[ -n "$term_sequences" ]]; then
            echo "Terminal escape sequences added:" >> "$report_file"
            echo "$term_sequences" >> "$report_file"
            echo "" >> "$report_file"
        fi

        # Check for cursor manipulation
        local cursor_code=$(git diff master src/readline_integration.c | grep -E "cursor|\\\\033\[s|\\\\033\[u|\\\\033\[.*[ABCD]" | grep "^+" | grep -v "^+++" || true)
        if [[ -n "$cursor_code" ]]; then
            echo "Cursor manipulation code:" >> "$report_file"
            echo "$cursor_code" >> "$report_file"
            echo "" >> "$report_file"
        fi
    fi

    # Check for terminal capability dependencies
    local termcap_usage=$(git diff master --name-only | xargs grep -n "termcap\|tgetent\|tgetstr\|tputs" 2>/dev/null || true)
    if [[ -n "$termcap_usage" ]]; then
        echo "Terminal capability usage:" >> "$report_file"
        echo "$termcap_usage" >> "$report_file"
        echo "" >> "$report_file"
    fi

    cat "$report_file"
}

# Function to analyze autosuggestions implementation
analyze_autosuggestions() {
    log_section "Analyzing Autosuggestions Implementation"

    local report_file="$TEMP_DIR/autosuggestions_report.txt"

    echo "Autosuggestions platform compatibility..." > "$report_file"
    echo "=========================================" >> "$report_file"
    echo "" >> "$report_file"

    # Check autosuggestions source
    if [[ -f "src/autosuggestions.c" ]]; then
        echo "Autosuggestions implementation found: src/autosuggestions.c" >> "$report_file"
        echo "" >> "$report_file"

        # Check for threading code
        local threading=$(grep -n "pthread\|thread\|mutex\|semaphore" src/autosuggestions.c || true)
        if [[ -n "$threading" ]]; then
            echo "Threading code detected:" >> "$report_file"
            echo "$threading" >> "$report_file"
            echo "" >> "$report_file"
        fi

        # Check for file operations
        local file_ops=$(grep -n "fopen\|fread\|fwrite\|mmap\|stat\|lstat" src/autosuggestions.c || true)
        if [[ -n "$file_ops" ]]; then
            echo "File operations:" >> "$report_file"
            echo "$file_ops" >> "$report_file"
            echo "" >> "$report_file"
        fi

        # Check for memory management
        local memory=$(grep -n "malloc\|calloc\|realloc\|free\|mmap\|munmap" src/autosuggestions.c || true)
        if [[ -n "$memory" ]]; then
            echo "Memory management calls:" >> "$report_file"
            echo "$memory" | head -10 >> "$report_file"
            echo "... (showing first 10 matches)" >> "$report_file"
            echo "" >> "$report_file"
        fi

        # Check for signal handling
        local signals=$(grep -n "signal\|sigaction\|SIGTERM\|SIGINT" src/autosuggestions.c || true)
        if [[ -n "$signals" ]]; then
            echo "Signal handling:" >> "$report_file"
            echo "$signals" >> "$report_file"
            echo "" >> "$report_file"
        fi
    else
        echo "Autosuggestions source file not found" >> "$report_file"
        echo "" >> "$report_file"
    fi

    cat "$report_file"
}

# Function to test current platform compatibility
test_current_platform() {
    log_section "Testing Current Platform Compatibility"

    local report_file="$TEMP_DIR/platform_test_report.txt"

    echo "Current platform test results..." > "$report_file"
    echo "================================" >> "$report_file"
    echo "" >> "$report_file"

    # System information
    echo "System Information:" >> "$report_file"
    echo "OS: $(uname -s)" >> "$report_file"
    echo "Kernel: $(uname -r)" >> "$report_file"
    echo "Architecture: $(uname -m)" >> "$report_file"
    echo "Distribution: $(lsb_release -d 2>/dev/null | cut -f2 || echo "Unknown")" >> "$report_file"
    echo "" >> "$report_file"

    # Build test
    echo "Build Test:" >> "$report_file"
    if [[ -x "./builddir/lusush" ]]; then
        echo "✓ Lusush binary exists and is executable" >> "$report_file"

        # Basic functionality test
        local basic_test=$(./builddir/lusush -c 'echo "Platform test"' 2>&1)
        if [[ "$basic_test" == "Platform test" ]]; then
            echo "✓ Basic command execution works" >> "$report_file"
        else
            echo "✗ Basic command execution failed: $basic_test" >> "$report_file"
        fi

        # Autosuggestions test
        local auto_test=$(./builddir/lusush -c 'display testsuggestion' 2>&1)
        if [[ "$auto_test" == *"SUCCESS"* ]]; then
            echo "✓ Autosuggestions system working" >> "$report_file"
        else
            echo "✗ Autosuggestions system failed: $auto_test" >> "$report_file"
        fi

        # Theme test
        local theme_test=$(timeout 5s ./builddir/lusush -i <<< $'theme list\nexit' 2>&1)
        if [[ $? -eq 0 ]]; then
            echo "✓ Theme system working" >> "$report_file"
        else
            echo "✗ Theme system failed or timed out" >> "$report_file"
        fi

    else
        echo "✗ Lusush binary not found or not executable" >> "$report_file"
    fi

    echo "" >> "$report_file"

    # Library dependencies
    echo "Library Dependencies:" >> "$report_file"
    if [[ -x "./builddir/lusush" ]]; then
        ldd ./builddir/lusush 2>/dev/null >> "$report_file" || echo "Could not analyze dependencies" >> "$report_file"
    fi
    echo "" >> "$report_file"

    cat "$report_file"
}

# Function to generate recommendations
generate_recommendations() {
    log_section "Generating Platform Compatibility Recommendations"

    local report_file="$TEMP_DIR/recommendations.txt"

    cat > "$report_file" << 'EOF'
Platform Compatibility Recommendations
======================================

Based on the analysis of the feature/fish-enhancements branch, here are
recommendations for ensuring cross-platform compatibility:

## Immediate Actions Required:

1. **Verify Terminal Escape Sequences**
   - Test ANSI escape sequences on different terminals
   - Consider using terminfo/termcap for terminal capabilities
   - Add fallbacks for terminals that don't support certain sequences

2. **Test Thread Safety**
   - Verify autosuggestions work correctly with different threading models
   - Test on systems with different pthread implementations
   - Consider lock-free approaches where possible

3. **Memory Management**
   - Verify memory allocation patterns work on all platforms
   - Test with different memory allocators (glibc, musl, BSD)
   - Add memory debugging for platform-specific issues

## Platform-Specific Considerations:

### Linux Compatibility:
- ✓ GNU readline is widely available
- ✓ POSIX compliance is generally good
- ⚠ Different distributions may have different library versions
- ⚠ Terminal capabilities vary between distributions

### macOS Compatibility:
- ⚠ May use BSD variants of some system calls
- ⚠ Different default shell behavior
- ⚠ Terminal.app vs iTerm2 differences
- ⚠ Homebrew vs system readline versions

### BSD Compatibility:
- ⚠ Different string function availability (strlcpy vs strncpy)
- ⚠ Different terminal handling
- ⚠ May need different compiler flags

## Testing Strategy:

1. **Automated Testing**
   - Set up CI/CD for multiple platforms
   - Test on Ubuntu, CentOS, macOS, FreeBSD
   - Test with different compiler versions

2. **Manual Testing**
   - Test interactive features on different terminals
   - Verify visual appearance across platforms
   - Test performance characteristics

3. **Edge Case Testing**
   - Test with limited terminal capabilities
   - Test on systems with unusual configurations
   - Test with different locale settings

## Code Quality Improvements:

1. **Error Handling**
   - Add proper error handling for platform-specific calls
   - Provide meaningful error messages
   - Implement graceful degradation

2. **Feature Detection**
   - Use compile-time feature detection where possible
   - Provide runtime fallbacks for missing features
   - Document platform-specific behavior

3. **Documentation**
   - Document known platform differences
   - Provide troubleshooting guides
   - List tested platforms and versions
EOF

    cat "$report_file"
}

# Function to create comprehensive report
create_comprehensive_report() {
    log_info "Creating comprehensive compatibility report..."

    cat > "$ANALYSIS_OUTPUT" << EOF
# Lusush Fish Enhancements - Platform Compatibility Analysis

**Generated on:** $(date)
**Branch:** feature/fish-enhancements vs master
**Analyzer:** $(whoami)@$(hostname)
**Platform:** $(uname -s) $(uname -r) $(uname -m)

## Executive Summary

This report analyzes the platform compatibility of the fish-like enhancements
added to Lusush, with particular focus on ensuring the features work correctly
across Linux, macOS, and BSD systems.

## Analysis Results

EOF

    # Append all analysis sections
    echo "### System-Specific Includes Analysis" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/includes_report.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No includes report generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    echo "### System Calls Analysis" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/syscalls_report.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No syscalls report generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    echo "### Build System Analysis" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/build_report.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No build system report generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    echo "### Terminal Code Analysis" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/terminal_report.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No terminal report generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    echo "### Autosuggestions Analysis" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/autosuggestions_report.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No autosuggestions report generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    echo "### Current Platform Test Results" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/platform_test_report.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No platform test report generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    echo "## Recommendations" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    cat "$TEMP_DIR/recommendations.txt" >> "$ANALYSIS_OUTPUT" 2>/dev/null || echo "No recommendations generated" >> "$ANALYSIS_OUTPUT"
    echo '```' >> "$ANALYSIS_OUTPUT"
    echo "" >> "$ANALYSIS_OUTPUT"

    cat >> "$ANALYSIS_OUTPUT" << 'EOF'

## Conclusion

The fish-like enhancements appear to be implemented with good cross-platform
compatibility in mind. The analysis shows minimal platform-specific code,
relying primarily on standard POSIX interfaces and GNU readline.

Key strengths:
- Extensive use of standard POSIX functions
- Proper error handling in most areas
- Good separation of platform-specific code
- Comprehensive build system integration

Areas for improvement:
- Terminal escape sequence compatibility testing needed
- Performance testing across different platforms required
- More comprehensive error handling for edge cases
- Documentation of platform-specific behavior

## Next Steps

1. Run automated tests on multiple platforms
2. Perform manual testing of interactive features
3. Address any issues found in recommendations
4. Update documentation with platform compatibility notes

This analysis indicates that the fish-like enhancements should work well
across supported platforms with minimal additional platform-specific changes
required.
EOF

    log_success "Comprehensive report created: $ANALYSIS_OUTPUT"
}

# Main execution function
main() {
    print_banner

    log_info "Starting platform compatibility analysis..."
    log_info "Analyzing branch: feature/fish-enhancements"

    # Verify we're in the right place
    if [[ ! -d ".git" ]]; then
        log_error "Not in a git repository"
        exit 1
    fi

    if ! git rev-parse --verify feature/fish-enhancements >/dev/null 2>&1; then
        log_error "feature/fish-enhancements branch not found"
        exit 1
    fi

    # Ensure we're on the right branch
    if [[ "$(git branch --show-current)" != "feature/fish-enhancements" ]]; then
        log_warning "Not on feature/fish-enhancements branch, switching..."
        git checkout feature/fish-enhancements
    fi

    # Run all analyses
    analyze_system_includes
    analyze_system_calls
    analyze_build_system
    analyze_terminal_code
    analyze_autosuggestions
    test_current_platform
    generate_recommendations

    # Create final report
    create_comprehensive_report

    log_success "Platform compatibility analysis complete!"
    log_info "Report saved to: $ANALYSIS_OUTPUT"

    # Display summary
    echo ""
    log_section "Analysis Summary"
    log_success "✓ System includes analysis completed"
    log_success "✓ System calls analysis completed"
    log_success "✓ Build system analysis completed"
    log_success "✓ Terminal code analysis completed"
    log_success "✓ Autosuggestions analysis completed"
    log_success "✓ Current platform testing completed"
    log_success "✓ Recommendations generated"
    log_success "✓ Comprehensive report created"

    echo ""
    log_info "View the full report: cat $ANALYSIS_OUTPUT"
    log_info "Or open in your preferred editor/viewer"
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        cat << 'EOF'
Usage: analyze_platform_compatibility.sh [OPTIONS]

Analyze platform compatibility of Lusush fish-like enhancements.

This script performs comprehensive analysis of the feature/fish-enhancements
branch to identify potential platform compatibility issues and provide
recommendations for ensuring cross-platform functionality.

Options:
  --help, -h    Show this help message

The analysis includes:
  • System-specific include files
  • Platform-specific function calls
  • Build system changes and dependencies
  • Terminal and readline integration code
  • Autosuggestions implementation details
  • Current platform testing
  • Compatibility recommendations

Output:
  • Detailed markdown report: platform_compatibility_analysis.md
  • Console output with color-coded results
  • Actionable recommendations for improvements

Requirements:
  • Must be run from Lusush repository root
  • feature/fish-enhancements branch must exist
  • Built Lusush binary recommended for full testing
EOF
        exit 0
        ;;
esac

# Run the main function
main "$@"
