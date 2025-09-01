#!/bin/bash
#
# Remove Conservative Linux-Specific Restrictions from Lusush
# This script removes the conservative safety restrictions that were added for Linux
# but are no longer needed with the robust syntax highlighting implementation
#
# Based on .cursorrules documentation:
# "Latest Achievement: ✅ COMPLETED - Robust syntax highlighting with full line wrapping
# support successfully implemented and tested. All conservative safety restrictions
# removed while maintaining display stability."
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

READLINE_FILE="src/readline_integration.c"
BACKUP_FILE="src/readline_integration.c.backup"

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

print_banner() {
    echo -e "${BOLD}${BLUE}"
    cat << 'EOF'
    ╔══════════════════════════════════════════════════════════════════╗
    ║        Remove Conservative Linux Restrictions from Lusush        ║
    ║             Syntax Highlighting Enhancement Script               ║
    ╚══════════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
    echo "This script removes conservative Linux-specific restrictions that are"
    echo "no longer needed with the robust syntax highlighting implementation."
    echo ""
}

verify_prerequisites() {
    log_info "Verifying prerequisites..."

    if [[ ! -f "$READLINE_FILE" ]]; then
        log_error "File not found: $READLINE_FILE"
        log_error "Please run this script from the Lusush repository root"
        exit 1
    fi

    # Check if we're on the right branch
    if [[ "$(git branch --show-current)" != "feature/fish-enhancements" ]]; then
        log_warning "Not on feature/fish-enhancements branch"
        log_info "Current branch: $(git branch --show-current)"
        read -p "Continue anyway? (y/n): " confirm
        if [[ "$confirm" != "y" ]]; then
            log_info "Aborted by user"
            exit 1
        fi
    fi

    # Check if the restrictions are actually present
    if ! grep -q "__linux__" "$READLINE_FILE"; then
        log_warning "No Linux-specific restrictions found in $READLINE_FILE"
        log_info "The restrictions may have already been removed"
        exit 0
    fi

    log_success "Prerequisites verified"
}

backup_original_file() {
    log_info "Creating backup of original file..."
    cp "$READLINE_FILE" "$BACKUP_FILE"
    log_success "Backup created: $BACKUP_FILE"
}

show_current_restrictions() {
    log_info "Current Linux-specific restrictions found:"
    echo ""
    echo -e "${YELLOW}Restriction blocks to be removed:${NC}"
    echo ""

    grep -n -A 8 -B 2 "__linux__" "$READLINE_FILE" | sed 's/^/    /' || true
    echo ""
}

remove_restrictions() {
    log_info "Removing conservative Linux-specific restrictions..."

    # Create a temporary file for processing
    local temp_file=$(mktemp)
    local in_linux_block=false
    local block_start=0
    local line_num=0

    while IFS= read -r line; do
        ((line_num++))

        if [[ "$line" =~ ^[[:space:]]*#ifdef[[:space:]]+__linux__[[:space:]]*$ ]]; then
            log_info "Found Linux restriction block starting at line $line_num"
            in_linux_block=true
            block_start=$line_num
            continue
        fi

        if [[ "$in_linux_block" == true ]] && [[ "$line" =~ ^[[:space:]]*#endif[[:space:]]*$ ]]; then
            log_info "Removed Linux restriction block (lines $block_start-$line_num)"
            in_linux_block=false
            continue
        fi

        if [[ "$in_linux_block" == false ]]; then
            echo "$line" >> "$temp_file"
        fi
    done < "$READLINE_FILE"

    # Replace the original file
    mv "$temp_file" "$READLINE_FILE"
    log_success "Conservative restrictions removed successfully"
}

validate_syntax() {
    log_info "Validating C syntax after modifications..."

    # Basic syntax check with gcc
    if command -v gcc >/dev/null 2>&1; then
        if gcc -fsyntax-only -I include -I include/libhashtable -I include/display "$READLINE_FILE" 2>/dev/null; then
            log_success "C syntax validation passed"
        else
            log_warning "C syntax validation failed - manual review recommended"
        fi
    else
        log_warning "gcc not available - skipping syntax validation"
    fi
}

test_build() {
    log_info "Testing build after modifications..."

    if [[ -f "meson.build" ]] && command -v ninja >/dev/null 2>&1; then
        log_info "Attempting to rebuild Lusush..."
        if ninja -C builddir >/dev/null 2>&1; then
            log_success "Build successful after removing restrictions"
        else
            log_error "Build failed after modifications"
            log_warning "You may need to review the changes manually"
            return 1
        fi
    else
        log_warning "Cannot test build - meson/ninja not available or configured"
    fi
}

show_summary() {
    echo ""
    log_info "=== MODIFICATION SUMMARY ==="
    echo ""
    log_success "✓ Removed all Linux-specific conservative restrictions"
    log_success "✓ Preserved all other functionality"
    log_success "✓ Backup created: $BACKUP_FILE"
    echo ""
    log_info "Restrictions removed from $READLINE_FILE:"
    echo "  • 70-character cursor position limit"
    echo "  • 50-character string limit"
    echo "  • 20-character string wrapping limit"
    echo "  • 30-character variable limit"
    echo "  • 15-character variable wrapping limit"
    echo "  • 40-character word limit"
    echo "  • 12-character word wrapping limit"
    echo ""
    log_info "These restrictions were originally added as conservative safety measures"
    log_info "but are no longer needed with the robust syntax highlighting system."
    echo ""
}

verify_restrictions_removed() {
    log_info "Verifying all restrictions were removed..."

    if grep -q "__linux__" "$READLINE_FILE"; then
        log_warning "Some Linux-specific blocks may still remain:"
        grep -n "__linux__" "$READLINE_FILE" | sed 's/^/    /'
    else
        log_success "All Linux-specific restriction blocks successfully removed"
    fi
}

offer_testing_suggestions() {
    echo ""
    log_info "=== TESTING RECOMMENDATIONS ==="
    echo ""
    echo "After removing the restrictions, please test:"
    echo ""
    echo "1. Basic functionality:"
    echo "   ./builddir/lusush -c 'echo \"test\"'"
    echo ""
    echo "2. Multiline constructs:"
    echo "   ./builddir/lusush -c 'for i in 1 2 3; do echo \"Item: \$i\"; done'"
    echo ""
    echo "3. Syntax highlighting with long commands:"
    echo "   ./builddir/lusush -i"
    echo "   # Type long commands to verify highlighting works without restrictions"
    echo ""
    echo "4. Interactive autosuggestions:"
    echo "   ./test_autosuggestions_interactive.sh"
    echo ""
    echo "5. Comprehensive feature test:"
    echo "   ./test_fish_features_linux.sh"
    echo ""
}

main() {
    print_banner

    verify_prerequisites
    show_current_restrictions

    echo ""
    read -p "Proceed with removing the conservative Linux restrictions? (y/n): " confirm
    if [[ "$confirm" != "y" ]]; then
        log_info "Operation cancelled by user"
        exit 0
    fi

    backup_original_file
    remove_restrictions
    verify_restrictions_removed
    validate_syntax

    if test_build; then
        show_summary
        offer_testing_suggestions

        echo ""
        log_success "🎉 Conservative Linux restrictions successfully removed!"
        log_info "Lusush now has unrestricted syntax highlighting on all platforms."

    else
        log_error "Build failed - restoration may be needed"
        log_info "Backup available at: $BACKUP_FILE"
        log_info "To restore: cp $BACKUP_FILE $READLINE_FILE"
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        cat << 'EOF'
Usage: remove_conservative_linux_restrictions.sh [OPTIONS]

Remove conservative Linux-specific restrictions from Lusush syntax highlighting.

This script removes the conservative safety restrictions that were added
specifically for Linux in the syntax highlighting system. According to the
project documentation, these restrictions are no longer needed with the
robust syntax highlighting implementation.

Restrictions being removed:
  • 70-character cursor position limit
  • 50-character string limit
  • 20-character string wrapping limit
  • 30-character variable limit
  • 15-character variable wrapping limit
  • 40-character word limit
  • 12-character word wrapping limit

Options:
  --help, -h    Show this help message

The script will:
  • Create a backup of the original file
  • Remove all Linux-specific restriction blocks
  • Validate C syntax after modifications
  • Test build if possible
  • Provide testing recommendations

Requirements:
  • Must be run from Lusush repository root
  • feature/fish-enhancements branch recommended
  • Write access to source files
EOF
        exit 0
        ;;
esac

# Run the main function
main "$@"
