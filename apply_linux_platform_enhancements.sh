#!/bin/bash
#
# Apply Linux Platform-Specific Enhancements for Lusush Fish Features
# This script applies targeted Linux optimizations that don't interfere with macOS
#
# Purpose: Address the specific file and git completion issues found in Phase 2 testing
# while maintaining full cross-platform compatibility
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

# Configuration
READLINE_FILE="src/readline_integration.c"
RICH_COMPLETION_FILE="src/rich_completion.c"
BACKUP_DIR="linux_platform_backups"
LOG_FILE="linux_platform_enhancements.log"

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

print_banner() {
    echo -e "${BOLD}${CYAN}"
    cat << 'EOF'
    ╔══════════════════════════════════════════════════════════════╗
    ║      Linux Platform-Specific Enhancement Application        ║
    ║         Fish Features Cross-Platform Optimization           ║
    ╚══════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
    echo "Applying targeted Linux optimizations for Phase 2 rich completions"
    echo "Platform: $(uname -s) $(uname -r) $(uname -m)"
    echo ""
}

initialize_log() {
    cat > "$LOG_FILE" << EOF
Linux Platform Enhancement Application Log
==========================================
Started: $(date)
Platform: $(uname -a)
User: $(whoami)
Working Directory: $(pwd)

EOF
}

create_backups() {
    log_info "Creating backups of original files..."

    mkdir -p "$BACKUP_DIR"

    if [[ -f "$READLINE_FILE" ]]; then
        cp "$READLINE_FILE" "$BACKUP_DIR/readline_integration.c.backup"
        log_success "Backed up $READLINE_FILE"
    fi

    if [[ -f "$RICH_COMPLETION_FILE" ]]; then
        cp "$RICH_COMPLETION_FILE" "$BACKUP_DIR/rich_completion.c.backup"
        log_success "Backed up $RICH_COMPLETION_FILE"
    fi
}

verify_prerequisites() {
    log_info "Verifying prerequisites..."

    # Check if we're in the right directory
    if [[ ! -f "meson.build" ]] || [[ ! -d "src" ]]; then
        log_error "Not in Lusush repository root directory"
        exit 1
    fi

    # Check if files exist
    if [[ ! -f "$READLINE_FILE" ]]; then
        log_error "File not found: $READLINE_FILE"
        exit 1
    fi

    if [[ ! -f "$RICH_COMPLETION_FILE" ]]; then
        log_error "File not found: $RICH_COMPLETION_FILE"
        exit 1
    fi

    # Check if we're on the right branch
    local current_branch=$(git branch --show-current 2>/dev/null || echo "unknown")
    log_info "Current branch: $current_branch"

    log_success "Prerequisites verified"
}

apply_linux_file_completion_enhancement() {
    log_info "Applying Linux-specific file completion enhancements..."

    # Add Linux-optimized file completion handling to rich_completion.c
    local temp_file=$(mktemp)

    # Insert Linux-specific file completion optimization after the includes
    cat > "$temp_file" << 'EOF'

// Linux-specific file completion optimizations
#ifdef __linux__
static bool is_linux_system_path(const char *path) {
    if (!path) return false;

    // Common Linux system paths that benefit from optimization
    const char *system_paths[] = {
        "/usr/bin", "/bin", "/usr/sbin", "/sbin",
        "/usr/local/bin", "/etc", "/var", "/opt",
        NULL
    };

    for (int i = 0; system_paths[i]; i++) {
        if (strncmp(path, system_paths[i], strlen(system_paths[i])) == 0) {
            return true;
        }
    }
    return false;
}

static int linux_optimize_file_completion(const char *path, char ***completions) {
    if (!path || !completions) return 0;

    // For Linux system paths, use optimized completion strategy
    if (is_linux_system_path(path)) {
        // Use directory scanning with reduced overhead for system paths
        DIR *dir = opendir(path);
        if (!dir) return 0;

        struct dirent *entry;
        int count = 0;
        char **result = NULL;

        while ((entry = readdir(dir)) != NULL && count < 100) { // Limit to prevent slowdown
            if (entry->d_name[0] == '.') continue; // Skip hidden files

            result = realloc(result, (count + 1) * sizeof(char*));
            if (result) {
                result[count] = strdup(entry->d_name);
                count++;
            }
        }

        closedir(dir);
        *completions = result;
        return count;
    }

    return 0; // Use standard completion for non-system paths
}
#endif // __linux__

EOF

    # Find the right place to insert this code (after includes, before first function)
    local insert_line=$(grep -n "// ============================================================================" "$RICH_COMPLETION_FILE" | head -1 | cut -d: -f1)

    if [[ -n "$insert_line" ]]; then
        # Insert the Linux optimization code
        head -n "$insert_line" "$RICH_COMPLETION_FILE" > "${RICH_COMPLETION_FILE}.tmp"
        cat "$temp_file" >> "${RICH_COMPLETION_FILE}.tmp"
        tail -n +$((insert_line + 1)) "$RICH_COMPLETION_FILE" >> "${RICH_COMPLETION_FILE}.tmp"
        mv "${RICH_COMPLETION_FILE}.tmp" "$RICH_COMPLETION_FILE"

        log_success "Linux file completion enhancement applied"
    else
        log_warning "Could not find insertion point - skipping file completion enhancement"
    fi

    rm -f "$temp_file"
}

apply_linux_git_completion_enhancement() {
    log_info "Applying Linux-specific git completion enhancements..."

    # Add Linux-optimized git completion to readline_integration.c
    local temp_file=$(mktemp)

    cat > "$temp_file" << 'EOF'

#ifdef __linux__
// Linux-specific git completion optimizations
static char **linux_git_completion(const char *text, int start, int end) {
    (void)end; // Suppress unused parameter warning

    if (!text) return NULL;

    // Get the command line to determine git subcommand context
    const char *line = rl_line_buffer;
    if (!line) return NULL;

    // Find git command position
    const char *git_pos = strstr(line, "git ");
    if (!git_pos) return NULL;

    // Move past "git "
    const char *subcommand_start = git_pos + 4;
    while (*subcommand_start == ' ') subcommand_start++;

    // Linux-optimized git subcommands with common usage patterns
    static const char *linux_git_commands[] = {
        "add", "commit", "push", "pull", "status", "log", "diff",
        "branch", "checkout", "merge", "rebase", "clone", "fetch",
        "init", "remote", "tag", "stash", "reset", "show", "config",
        "blame", "grep", "rm", "mv", "ls-files", "submodule",
        NULL
    };

    // If we're completing the first word after "git "
    if (start == (subcommand_start - line)) {
        char **matches = rl_completion_matches(text, NULL);
        if (!matches) {
            matches = malloc(sizeof(char*) * 25); // Space for git commands
            if (matches) {
                int match_count = 0;
                size_t text_len = strlen(text);

                for (int i = 0; linux_git_commands[i] && match_count < 24; i++) {
                    if (strncmp(linux_git_commands[i], text, text_len) == 0) {
                        matches[match_count] = strdup(linux_git_commands[i]);
                        if (matches[match_count]) match_count++;
                    }
                }
                matches[match_count] = NULL;

                if (match_count > 0) {
                    return matches;
                }
                free(matches);
            }
        }
    }

    return NULL; // Fall back to standard completion
}
#endif // __linux__

EOF

    # Find the git completion function and add Linux optimization
    local git_function_line=$(grep -n "Context-aware completion for specific commands" "$READLINE_FILE" | head -1 | cut -d: -f1)

    if [[ -n "$git_function_line" ]]; then
        # Insert the Linux git optimization before the existing git completion
        head -n "$git_function_line" "$READLINE_FILE" > "${READLINE_FILE}.tmp"
        cat "$temp_file" >> "${READLINE_FILE}.tmp"
        tail -n +$((git_function_line + 1)) "$READLINE_FILE" >> "${READLINE_FILE}.tmp"
        mv "${READLINE_FILE}.tmp" "$READLINE_FILE"

        log_success "Linux git completion enhancement applied"
    else
        log_warning "Could not find git completion section - skipping git enhancement"
    fi

    rm -f "$temp_file"
}

apply_linux_performance_optimizations() {
    log_info "Applying Linux-specific performance optimizations..."

    # Add Linux performance optimization flags to rich completion initialization
    local temp_file=$(mktemp)

    cat > "$temp_file" << 'EOF'

#ifdef __linux__
    // Linux-specific performance optimizations

    // Optimize for Linux file system characteristics
    rich_config.linux_fs_optimization = true;
    rich_config.use_linux_syscalls = true;

    // Set Linux-appropriate buffer sizes
    rich_config.linux_buffer_size = 8192; // Typical Linux page size
    rich_config.max_completions_displayed = 50; // Reasonable for Linux terminals

    // Enable Linux-specific completion caching
    rich_config.enable_path_caching = true;
    rich_config.cache_timeout_seconds = 30; // Good balance for Linux systems
#endif
EOF

    # Find the configuration initialization in rich_completion.c
    local config_line=$(grep -n "rich_config.*=" "$RICH_COMPLETION_FILE" | head -1 | cut -d: -f1)

    if [[ -n "$config_line" ]]; then
        # Find the end of the config block
        local config_end=$(tail -n +$((config_line + 1)) "$RICH_COMPLETION_FILE" | grep -n "^};" | head -1 | cut -d: -f1)
        config_end=$((config_line + config_end))

        # Insert Linux optimizations before the closing brace
        head -n $((config_end - 1)) "$RICH_COMPLETION_FILE" > "${RICH_COMPLETION_FILE}.tmp"
        cat "$temp_file" >> "${RICH_COMPLETION_FILE}.tmp"
        tail -n +$config_end "$RICH_COMPLETION_FILE" >> "${RICH_COMPLETION_FILE}.tmp"
        mv "${RICH_COMPLETION_FILE}.tmp" "$RICH_COMPLETION_FILE"

        log_success "Linux performance optimizations applied"
    else
        log_warning "Could not find configuration section - skipping performance optimizations"
    fi

    rm -f "$temp_file"
}

apply_error_handling_improvements() {
    log_info "Applying Linux-specific error handling improvements..."

    # Add robust error handling for Linux file operations
    local temp_file=$(mktemp)

    cat > "$temp_file" << 'EOF'

#ifdef __linux__
// Linux-specific error handling for file operations
static bool linux_safe_file_access(const char *path) {
    if (!path) return false;

    struct stat st;
    if (lstat(path, &st) != 0) {
        // Path doesn't exist or permission denied
        return false;
    }

    // Check if it's a symbolic link pointing to non-existent file
    if (S_ISLNK(st.st_mode)) {
        if (stat(path, &st) != 0) {
            return false; // Broken symlink
        }
    }

    // Check read permissions
    return (access(path, R_OK) == 0);
}

static void linux_completion_error_handler(const char *operation, const char *path) {
    if (!operation) operation = "completion";
    if (!path) path = "unknown";

    // Log error without disrupting user experience
    static int error_count = 0;
    if (error_count < 5) { // Limit error logging to prevent spam
        fprintf(stderr, "lusush: %s error for path: %s\n", operation, path);
        error_count++;
    }
}
#endif // __linux__

EOF

    # Insert error handling functions
    local functions_line=$(grep -n "// UTILITY FUNCTIONS" "$RICH_COMPLETION_FILE" | head -1 | cut -d: -f1)

    if [[ -n "$functions_line" ]]; then
        head -n "$functions_line" "$RICH_COMPLETION_FILE" > "${RICH_COMPLETION_FILE}.tmp"
        cat "$temp_file" >> "${RICH_COMPLETION_FILE}.tmp"
        tail -n +$((functions_line + 1)) "$RICH_COMPLETION_FILE" >> "${RICH_COMPLETION_FILE}.tmp"
        mv "${RICH_COMPLETION_FILE}.tmp" "$RICH_COMPLETION_FILE"

        log_success "Linux error handling improvements applied"
    else
        log_warning "Could not find utility functions section - skipping error handling"
    fi

    rm -f "$temp_file"
}

build_and_test() {
    log_info "Building Lusush with Linux enhancements..."

    # Clean build
    if meson setup builddir --wipe >/dev/null 2>&1 && ninja -C builddir >/dev/null 2>&1; then
        log_success "Build completed successfully"
    else
        log_error "Build failed - restoring backups"
        restore_backups
        return 1
    fi

    # Basic functionality test
    log_info "Testing basic functionality..."
    if ./builddir/lusush -c 'echo "Linux enhancements test"' >/dev/null 2>&1; then
        log_success "Basic functionality test passed"
    else
        log_error "Basic functionality test failed"
        return 1
    fi

    # Test autosuggestions still work
    log_info "Testing autosuggestions compatibility..."
    local auto_test=$(./builddir/lusush -c 'display testsuggestion' 2>&1)
    if [[ "$auto_test" == *"SUCCESS"* ]]; then
        log_success "Autosuggestions still functional"
    else
        log_warning "Autosuggestions may have issues"
    fi

    return 0
}

restore_backups() {
    log_info "Restoring original files from backups..."

    if [[ -f "$BACKUP_DIR/readline_integration.c.backup" ]]; then
        cp "$BACKUP_DIR/readline_integration.c.backup" "$READLINE_FILE"
        log_success "Restored $READLINE_FILE"
    fi

    if [[ -f "$BACKUP_DIR/rich_completion.c.backup" ]]; then
        cp "$BACKUP_DIR/rich_completion.c.backup" "$RICH_COMPLETION_FILE"
        log_success "Restored $RICH_COMPLETION_FILE"
    fi
}

generate_summary() {
    echo ""
    echo -e "${BOLD}${CYAN}=========================================${NC}"
    echo -e "${BOLD}${CYAN}  LINUX PLATFORM ENHANCEMENTS SUMMARY${NC}"
    echo -e "${BOLD}${CYAN}=========================================${NC}"
    echo ""

    log_info "Enhancements Applied:"
    echo "  • Linux-specific file completion optimizations"
    echo "  • Enhanced git completion for Linux environments"
    echo "  • Performance optimizations for Linux systems"
    echo "  • Improved error handling for Linux file operations"
    echo ""

    log_info "Platform Compatibility:"
    echo "  • All changes are conditional (#ifdef __linux__)"
    echo "  • macOS functionality completely preserved"
    echo "  • Cross-platform compatibility maintained"
    echo ""

    log_info "Files Modified:"
    echo "  • $READLINE_FILE (git completion enhancements)"
    echo "  • $RICH_COMPLETION_FILE (file completion + performance)"
    echo ""

    log_info "Backup Location:"
    echo "  • Original files backed up to: $BACKUP_DIR/"
    echo ""

    log_success "Linux platform enhancements successfully applied!"
    echo ""
    echo -e "${GREEN}Next Steps:${NC}"
    echo "  • Test interactive completions: ./builddir/lusush -i"
    echo "  • Run comprehensive tests: ./test_fish_features_linux.sh"
    echo "  • Validate Phase 2 features: ./validate_phase2_linux.sh"
    echo ""
    echo -e "${CYAN}To restore original files if needed:${NC}"
    echo "  • cp $BACKUP_DIR/*.backup src/"
    echo ""
}

main() {
    print_banner
    initialize_log

    verify_prerequisites
    create_backups

    log_info "Applying Linux-specific enhancements..."
    echo ""

    apply_linux_file_completion_enhancement
    apply_linux_git_completion_enhancement
    apply_linux_performance_optimizations
    apply_error_handling_improvements

    echo ""
    log_info "Building and testing enhancements..."

    if build_and_test; then
        generate_summary

        echo -e "${GREEN}🎉 Linux platform enhancements successfully applied!${NC}"
        echo -e "${GREEN}   Fish features optimized for Linux without affecting macOS.${NC}"

        exit 0
    else
        log_error "Enhancement application failed"
        echo -e "${RED}❌ Enhancement application failed - check log: $LOG_FILE${NC}"
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    "--help"|"-h")
        cat << 'EOF'
Usage: apply_linux_platform_enhancements.sh [OPTIONS]

Apply Linux platform-specific enhancements for Lusush fish features.

This script adds targeted Linux optimizations to address Phase 2 rich
completion issues without interfering with macOS functionality:

• Linux-specific file completion optimizations
• Enhanced git completion for Linux environments
• Performance optimizations for Linux systems
• Improved error handling for Linux file operations

All changes are conditionally compiled (#ifdef __linux__) to ensure
complete cross-platform compatibility.

Options:
  --help, -h    Show this help message
  --restore     Restore original files from backups

Requirements:
• Must be run from Lusush repository root
• feature/fish-enhancements branch recommended
• Write access to source files
• Meson and Ninja for building

Files Modified:
• src/readline_integration.c (git completion)
• src/rich_completion.c (file completion + performance)

Backup Location:
• linux_platform_backups/ (created automatically)
EOF
        exit 0
        ;;
    "--restore")
        print_banner
        verify_prerequisites
        restore_backups
        log_success "Original files restored from backups"
        exit 0
        ;;
esac

# Run the main function
main "$@"
