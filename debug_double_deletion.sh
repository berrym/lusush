#!/bin/bash

# debug_double_deletion.sh
# Script to enable tracing and test the double deletion bug in Lusush Line Editor
# This script sets up the environment for comprehensive buffer state tracking
# to identify the root cause of the double-deletion bug in backspace operations.

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/builddir"
TRACE_LOG="/tmp/lle_buffer_trace.log"
SUMMARY_LOG="/tmp/lle_buffer_summary.log"
EXPORT_LOG="/tmp/lle_snapshots.csv"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

echo_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

echo_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [command] [options]

Commands:
    setup       - Build the system with tracing enabled
    clean       - Clean build directory and start fresh
    test        - Run interactive test to reproduce double deletion bug
    analyze     - Analyze trace logs from previous test run
    export      - Export trace data for external analysis
    help        - Show this help message

Test Options:
    --boundary-only     - Only trace boundary crossing operations
    --backspace-only    - Only trace backspace operations
    --verbose           - Enable verbose tracing output
    --min-length N      - Only trace if buffer length >= N
    --terminal-width N  - Test with specific terminal width (default: auto-detect)

Examples:
    $0 setup                           # Build with tracing support
    $0 test --boundary-only            # Test focusing on boundary crossings
    $0 test --verbose --min-length 30  # Verbose test for long lines
    $0 analyze                         # Analyze results from last test
    $0 export                          # Export data for further analysis

Environment Variables:
    LLE_TRACE_ENABLED=1          - Enable tracing system
    LLE_TRACE_VERBOSE=1          - Enable verbose output
    LLE_TRACE_BUFFER_OPS=1       - Trace buffer operations
    LLE_TRACE_DISPLAY_OPS=1      - Trace display operations
    LLE_TRACE_BOUNDARY_ONLY=1    - Only trace boundary crossing operations
    LLE_TRACE_BACKSPACE_ONLY=1   - Only trace backspace operations
    LLE_TRACE_MIN_LENGTH=N       - Only trace if buffer length >= N
    LLE_TRACE_AUTO_ANALYSIS=1    - Automatically analyze each operation

The tracing system logs all buffer state changes to files instead of stderr
to avoid contaminating the terminal display during interactive testing.
EOF
}

# Function to check prerequisites
check_prerequisites() {
    echo_info "Checking prerequisites..."

    if ! command -v meson &> /dev/null; then
        echo_error "Meson build system not found. Please install meson."
        exit 1
    fi

    if ! command -v ninja &> /dev/null; then
        echo_error "Ninja build tool not found. Please install ninja."
        exit 1
    fi

    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        echo_error "No C compiler found. Please install gcc or clang."
        exit 1
    fi

    echo_success "Prerequisites check passed"
}

# Function to clean build directory
clean_build() {
    echo_info "Cleaning build directory..."

    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo_success "Build directory cleaned"
    else
        echo_info "Build directory doesn't exist, nothing to clean"
    fi

    # Clean trace logs
    rm -f "$TRACE_LOG" "$SUMMARY_LOG" "$EXPORT_LOG"
    echo_success "Trace logs cleaned"
}

# Function to setup build with tracing
setup_build() {
    echo_info "Setting up build with tracing enabled..."

    cd "$SCRIPT_DIR"

    # Clean first
    clean_build

    # Setup meson build
    echo_info "Configuring meson build..."
    if ! meson setup builddir --buildtype=debug -Db_sanitize=address; then
        echo_error "Meson setup failed"
        exit 1
    fi

    # Build the project
    echo_info "Building Lusush with tracing support..."
    if ! meson compile -C builddir; then
        echo_error "Build failed"
        exit 1
    fi

    echo_success "Build completed successfully"
    echo_info "Binary location: $BUILD_DIR/lusush"
}

# Function to set up tracing environment
setup_tracing_env() {
    local boundary_only=false
    local backspace_only=false
    local verbose=false
    local min_length=0
    local auto_analysis=true

    # Parse options
    while [[ $# -gt 0 ]]; do
        case $1 in
            --boundary-only)
                boundary_only=true
                shift
                ;;
            --backspace-only)
                backspace_only=true
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --min-length)
                min_length="$2"
                shift 2
                ;;
            *)
                shift
                ;;
        esac
    done

    # Set environment variables for tracing
    export LLE_TRACE_ENABLED=1
    export LLE_TRACE_BUFFER_OPS=1
    export LLE_TRACE_DISPLAY_OPS=1
    export LLE_TRACE_AUTO_ANALYSIS=1
    export LLE_TRACE_LOG_PATH="$TRACE_LOG"

    if [ "$boundary_only" = true ]; then
        export LLE_TRACE_BOUNDARY_ONLY=1
        echo_info "Tracing configured: boundary crossing operations only"
    fi

    if [ "$backspace_only" = true ]; then
        export LLE_TRACE_BACKSPACE_ONLY=1
        echo_info "Tracing configured: backspace operations only"
    fi

    if [ "$verbose" = true ]; then
        export LLE_TRACE_VERBOSE=1
        echo_info "Tracing configured: verbose mode enabled"
    fi

    if [ "$min_length" -gt 0 ]; then
        export LLE_TRACE_MIN_LENGTH="$min_length"
        echo_info "Tracing configured: minimum buffer length = $min_length"
    fi

    echo_success "Tracing environment configured"
    echo_info "Trace log: $TRACE_LOG"
    echo_info "Summary log: $SUMMARY_LOG"
}

# Function to run interactive test
run_test() {
    echo_info "Running interactive double deletion test..."

    if [ ! -f "$BUILD_DIR/lusush" ]; then
        echo_error "Lusush binary not found. Please run '$0 setup' first."
        exit 1
    fi

    # Setup tracing environment
    setup_tracing_env "$@"

    # Clean previous logs
    rm -f "$TRACE_LOG" "$SUMMARY_LOG"

    echo ""
    echo_info "=========================================="
    echo_info "DOUBLE DELETION BUG REPRODUCTION TEST"
    echo_info "=========================================="
    echo ""
    echo_warning "Instructions for reproducing the bug:"
    echo "1. Type a long command that wraps around the terminal width"
    echo "2. Use backspace to delete characters across the line wrap boundary"
    echo "3. Watch for double character deletion behavior"
    echo "4. Press Ctrl+D to exit when you observe the bug"
    echo ""
    echo_info "Example commands to test:"
    echo "  echo 'This is a very long command that should definitely wrap around multiple lines to trigger the boundary crossing backspace bug'"
    echo "  ls -la /usr/local/bin && echo 'another long command to test wrapping behavior and backspace operations'"
    echo ""
    echo_warning "Terminal width: $(tput cols) columns"
    echo_warning "Test will be logged to: $TRACE_LOG"
    echo ""
    read -p "Press Enter to start the test (or Ctrl+C to cancel)..."

    # Run lusush with tracing enabled
    echo_info "Starting Lusush with tracing enabled..."
    "$BUILD_DIR/lusush" || true

    echo ""
    echo_success "Test completed!"
    echo_info "Analyzing results..."

    # Automatically analyze the results
    analyze_results
}

# Function to analyze trace results
analyze_results() {
    echo_info "Analyzing trace results..."

    if [ ! -f "$TRACE_LOG" ]; then
        echo_error "Trace log not found: $TRACE_LOG"
        echo_info "Please run '$0 test' first to generate trace data"
        exit 1
    fi

    if [ ! -f "$SUMMARY_LOG" ]; then
        echo_warning "Summary log not found, trace may not have completed properly"
    fi

    echo_info "Trace log size: $(wc -l < "$TRACE_LOG") lines"

    # Check for critical events
    echo ""
    echo_info "=== CRITICAL EVENTS ANALYSIS ==="

    local double_deletions=$(grep -c "DOUBLE DELETION" "$TRACE_LOG" 2>/dev/null || echo "0")
    local boundary_crossings=$(grep -c "BOUNDARY_CROSSING" "$TRACE_LOG" 2>/dev/null || echo "0")
    local buffer_errors=$(grep -c "BUFFER.*ERROR\|CONSISTENCY.*ERROR" "$TRACE_LOG" 2>/dev/null || echo "0")
    local backspace_operations=$(grep -c "CMD_BACKSPACE_ENTRY" "$TRACE_LOG" 2>/dev/null || echo "0")

    echo "Double deletions detected: $double_deletions"
    echo "Boundary crossings: $boundary_crossings"
    echo "Buffer errors: $buffer_errors"
    echo "Total backspace operations: $backspace_operations"

    if [ "$double_deletions" -gt 0 ]; then
        echo ""
        echo_error "🚨 DOUBLE DELETION BUG CONFIRMED! 🚨"
        echo_info "Found $double_deletions instances of double deletion"
        echo ""
        echo_info "Critical events from trace log:"
        grep -n "DOUBLE DELETION\|CRITICAL.*ERROR" "$TRACE_LOG" | head -10
    else
        echo ""
        echo_success "No double deletions detected in this session"
        if [ "$boundary_crossings" -gt 0 ]; then
            echo_info "However, $boundary_crossings boundary crossings occurred"
            echo_info "The bug may be intermittent or require specific conditions"
        fi
    fi

    # Show buffer consistency issues
    if [ "$buffer_errors" -gt 0 ]; then
        echo ""
        echo_warning "Buffer consistency errors detected:"
        grep -n "CONSISTENCY.*ERROR\|BUFFER.*ERROR" "$TRACE_LOG" | head -5
    fi

    # Show summary if available
    if [ -f "$SUMMARY_LOG" ]; then
        echo ""
        echo_info "=== SESSION SUMMARY ==="
        cat "$SUMMARY_LOG"
    fi

    echo ""
    echo_info "=== NEXT STEPS ==="
    if [ "$double_deletions" -gt 0 ]; then
        echo "1. Review the trace log for the exact sequence of operations"
        echo "2. Focus on operations between BEFORE_TEXT_BACKSPACE and AFTER_TEXT_BACKSPACE"
        echo "3. Check if buffer modifications occur during display updates"
        echo "4. Export data for detailed analysis: $0 export"
    else
        echo "1. Try reproducing with different terminal widths: --terminal-width N"
        echo "2. Test with longer commands: --min-length 50"
        echo "3. Use boundary-only tracing: --boundary-only"
        echo "4. Repeat test with more aggressive backspace operations"
    fi

    echo ""
    echo_info "Full trace log: $TRACE_LOG"
    echo_info "Summary log: $SUMMARY_LOG"
}

# Function to export trace data
export_data() {
    echo_info "Exporting trace data for external analysis..."

    if [ ! -f "$TRACE_LOG" ]; then
        echo_error "Trace log not found: $TRACE_LOG"
        exit 1
    fi

    # Create CSV export (simplified version since we don't have the full export function)
    echo "Creating CSV export..."
    {
        echo "sequence,timestamp,operation,buffer_length,cursor_pos,boundary,fallback,content"
        grep "^\[" "$TRACE_LOG" | sed 's/\[//g' | sed 's/\]//g' | while read -r line; do
            # Parse trace line and convert to CSV format
            # This is a simplified parser - the full implementation would be in the C code
            echo "$line" | awk -F' ' 'BEGIN{OFS=","} {
                seq=$1; timestamp=$2; operation=$3
                # Extract key=value pairs
                for(i=4; i<=NF; i++) {
                    if($i ~ /len=/) { gsub(/len=/, "", $i); len=$i }
                    if($i ~ /cursor=/) { gsub(/cursor=/, "", $i); cursor=$i }
                    if($i ~ /boundary=/) { gsub(/boundary=/, "", $i); boundary=$i }
                    if($i ~ /fallback=/) { gsub(/fallback=/, "", $i); fallback=$i }
                    if($i ~ /content=/) {
                        content=""
                        for(j=i; j<=NF; j++) content=content " " $j
                        gsub(/content=./, "", content)
                        gsub(/..$/, "", content)
                    }
                }
                print seq, timestamp, operation, len, cursor, boundary, fallback, "\"" content "\""
            }'
        done
    } > "$EXPORT_LOG"

    echo_success "Data exported to: $EXPORT_LOG"
    echo_info "You can now analyze this data with spreadsheet software or custom scripts"

    # Show some basic statistics
    local total_lines=$(wc -l < "$EXPORT_LOG")
    echo_info "Exported $((total_lines - 1)) trace entries"

    # Show first few critical entries
    echo ""
    echo_info "Sample critical entries:"
    head -10 "$EXPORT_LOG"
}

# Main command dispatcher
main() {
    local command="${1:-help}"
    shift || true

    case "$command" in
        setup)
            check_prerequisites
            setup_build
            ;;
        clean)
            clean_build
            ;;
        test)
            check_prerequisites
            run_test "$@"
            ;;
        analyze)
            analyze_results
            ;;
        export)
            export_data
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            echo_error "Unknown command: $command"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

# Handle Ctrl+C gracefully
trap 'echo ""; echo_info "Operation cancelled by user"; exit 130' INT

# Run main function with all arguments
main "$@"
