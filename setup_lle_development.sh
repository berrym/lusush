#!/bin/bash

# Lusush Line Editor (LLE) Development Setup Script
# This script sets up the development environment for implementing the LLE

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the lusush directory
check_directory() {
    if [[ ! -f "setup_lle_development.sh" ]]; then
        print_error "This script must be run from the lusush project root directory"
        exit 1
    fi

    if [[ ! -d ".git" ]]; then
        print_error "This doesn't appear to be a git repository"
        exit 1
    fi

    print_success "Running from correct directory: $(pwd)"
}

# Create the directory structure for LLE
create_directory_structure() {
    print_status "Creating LLE directory structure..."

    # Source directories
    mkdir -p src/line_editor
    mkdir -p tests/line_editor
    mkdir -p docs/line_editor

    # Create placeholder files to ensure directories are tracked
    touch src/line_editor/.gitkeep
    touch tests/line_editor/.gitkeep
    touch docs/line_editor/.gitkeep

    print_success "Directory structure created"
}

# Set up git branches for LLE development
setup_git_branches() {
    print_status "Setting up git branches for LLE development..."

    # Get current branch
    current_branch=$(git rev-parse --abbrev-ref HEAD)
    print_status "Current branch: $current_branch"

    # Create feature branch if it doesn't exist
    if git show-ref --verify --quiet refs/heads/feature/lusush-line-editor; then
        print_warning "Branch 'feature/lusush-line-editor' already exists"
    else
        git checkout -b feature/lusush-line-editor
        print_success "Created branch 'feature/lusush-line-editor'"
    fi

    # Switch to the feature branch
    git checkout feature/lusush-line-editor
    print_success "Switched to LLE development branch"
}

# Create initial LLE files
create_initial_files() {
    print_status "Creating initial LLE files..."

    # Create basic header files with include guards
    cat > src/line_editor/text_buffer.h << 'EOF'
#ifndef LLE_TEXT_BUFFER_H
#define LLE_TEXT_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Lusush Line Editor - Text Buffer
 *
 * Core text manipulation and storage functionality.
 */

typedef struct {
    char *buffer;           // UTF-8 text buffer
    size_t length;          // Current text length (bytes)
    size_t capacity;        // Buffer capacity (bytes)
    size_t cursor_pos;      // Cursor position (byte offset)
    size_t char_count;      // Number of Unicode characters
} lle_text_buffer_t;

// Function declarations will be added as tasks are implemented

#endif // LLE_TEXT_BUFFER_H
EOF

    cat > src/line_editor/cursor_math.h << 'EOF'
#ifndef LLE_CURSOR_MATH_H
#define LLE_CURSOR_MATH_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Lusush Line Editor - Cursor Mathematics
 *
 * Mathematical framework for cursor positioning and calculations.
 */

typedef struct {
    size_t absolute_row;    // Terminal row (0-based)
    size_t absolute_col;    // Terminal column (0-based)
    size_t relative_row;    // Row relative to prompt start
    size_t relative_col;    // Column within current line
    bool at_boundary;       // At line wrap boundary
    bool valid;             // Position calculation valid
} lle_cursor_position_t;

typedef struct {
    size_t width;           // Terminal width in characters
    size_t height;          // Terminal height in characters
    size_t prompt_width;    // Width of current prompt
    size_t prompt_height;   // Height of current prompt
} lle_terminal_geometry_t;

// Function declarations will be added as tasks are implemented

#endif // LLE_CURSOR_MATH_H
EOF

    cat > src/line_editor/terminal_manager.h << 'EOF'
#ifndef LLE_TERMINAL_MANAGER_H
#define LLE_TERMINAL_MANAGER_H

#include <stdbool.h>
#include "cursor_math.h"

/**
 * Lusush Line Editor - Terminal Manager
 *
 * Terminal interface and capability management.
 */

typedef struct {
    lle_terminal_geometry_t geometry;
    bool capabilities_initialized;
    bool in_raw_mode;
    int stdin_fd;
    int stdout_fd;
} lle_terminal_manager_t;

// Function declarations will be added as tasks are implemented

#endif // LLE_TERMINAL_MANAGER_H
EOF

    # Create basic test framework
    cat > tests/line_editor/test_framework.h << 'EOF'
#ifndef LLE_TEST_FRAMEWORK_H
#define LLE_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Lusush Line Editor - Test Framework
 *
 * Simple testing framework for LLE components.
 */

// Test macros
#define LLE_TEST(name) void test_##name(void)

#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            abort(); \
        } \
    } while(0)

#define LLE_ASSERT_EQ(a, b) LLE_ASSERT((a) == (b))
#define LLE_ASSERT_STR_EQ(a, b) LLE_ASSERT(strcmp((a), (b)) == 0)
#define LLE_ASSERT_NOT_NULL(ptr) LLE_ASSERT((ptr) != NULL)
#define LLE_ASSERT_NULL(ptr) LLE_ASSERT((ptr) == NULL)

// Test runner
void lle_run_all_tests(void);

#endif // LLE_TEST_FRAMEWORK_H
EOF

    # Create initial progress tracking file
    cat > LLE_PROGRESS.md << 'EOF'
# LLE Development Progress

## Phase 1: Foundation (Weeks 1-2)
- [ ] LLE-001: Basic Text Buffer Structure (2h) - TODO
- [ ] LLE-002: Text Buffer Initialization (3h) - TODO
- [ ] LLE-003: Basic Text Insertion (4h) - TODO
- [ ] LLE-004: Basic Text Deletion (3h) - TODO
- [ ] LLE-005: Cursor Movement (3h) - TODO
- [ ] LLE-006: Cursor Position Structure (2h) - TODO
- [ ] LLE-007: Basic Cursor Position Calculation (4h) - TODO
- [ ] LLE-008: Prompt Geometry Calculation (3h) - TODO
- [ ] LLE-009: Terminal Manager Structure (2h) - TODO
- [ ] LLE-010: Terminal Initialization (4h) - TODO
- [ ] LLE-011: Basic Terminal Output (3h) - TODO
- [ ] LLE-012: Test Framework Setup (3h) - TODO
- [ ] LLE-013: Text Buffer Tests (4h) - TODO
- [ ] LLE-014: Cursor Math Tests (3h) - TODO

## Phase 2: Core Functionality (Weeks 3-4)
- [ ] LLE-015: Prompt Structure Definition (2h) - TODO
- [ ] LLE-016: Prompt Parsing (4h) - TODO
- [ ] LLE-017: Prompt Rendering (4h) - TODO
- [ ] LLE-018: Multiline Input Display (4h) - TODO
- [ ] LLE-019: Theme Interface Definition (2h) - TODO
- [ ] LLE-020: Basic Theme Application (3h) - TODO
- [ ] LLE-021: Key Input Handling (2h) - TODO
- [ ] LLE-022: Key Event Processing (4h) - TODO
- [ ] LLE-023: Basic Editing Commands (4h) - TODO
- [ ] LLE-024: History Structure (2h) - TODO
- [ ] LLE-025: History Management (4h) - TODO
- [ ] LLE-026: History Navigation (3h) - TODO

## Phase 3: Advanced Features (Weeks 5-6)
- [ ] LLE-027: UTF-8 Text Handling (3h) - TODO
- [ ] LLE-028: Unicode Cursor Movement (3h) - TODO
- [ ] LLE-029: Completion Framework (3h) - TODO
- [ ] LLE-030: Basic File Completion (4h) - TODO
- [ ] LLE-031: Completion Display (4h) - TODO
- [ ] LLE-032: Undo Stack Structure (2h) - TODO
- [ ] LLE-033: Undo Operation Recording (4h) - TODO
- [ ] LLE-034: Undo/Redo Execution (4h) - TODO
- [ ] LLE-035: Syntax Highlighting Framework (3h) - TODO
- [ ] LLE-036: Basic Shell Syntax (4h) - TODO
- [ ] LLE-037: Syntax Display Integration (3h) - TODO

## Phase 4: Integration & Polish (Weeks 7-8)
- [ ] LLE-038: Core Line Editor API (3h) - TODO
- [ ] LLE-039: Line Editor Implementation (4h) - TODO
- [ ] LLE-040: Input Event Loop (4h) - TODO
- [ ] LLE-041: Replace Linenoise Integration (4h) - TODO
- [ ] LLE-042: Theme System Integration (3h) - TODO
- [ ] LLE-043: Configuration Integration (3h) - TODO
- [ ] LLE-044: Display Optimization (4h) - TODO
- [ ] LLE-045: Memory Optimization (3h) - TODO
- [ ] LLE-046: Comprehensive Integration Tests (4h) - TODO
- [ ] LLE-047: Performance Benchmarks (3h) - TODO
- [ ] LLE-048: API Documentation (4h) - TODO
- [ ] LLE-049: User Documentation (3h) - TODO
- [ ] LLE-050: Final Integration and Testing (4h) - TODO

## Summary
- Tasks Completed: 0/50
- Hours Completed: 0/160
- Current Phase: 1/4
- Estimated Completion: Week 4
- Current Task: LLE-001

## Notes
Development started on: $(date)
Last updated: $(date)
EOF

    print_success "Initial LLE files created"
}

# Update build system to include LLE
update_build_system() {
    print_status "Updating build system for LLE..."

    # Check if meson.build exists
    if [[ -f "meson.build" ]]; then
        # Create a separate meson.build for line_editor if not already present
        if [[ ! -f "src/line_editor/meson.build" ]]; then
            cat > src/line_editor/meson.build << 'EOF'
# Lusush Line Editor (LLE) Build Configuration

lle_sources = [
  'text_buffer.c',
  'cursor_math.c',
  'terminal_manager.c',
  'prompt.c',
  'display.c',
  'input_handler.c',
  'edit_commands.c',
  'history.c',
  'completion.c',
  'theme_integration.c',
  'undo.c',
  'syntax.c',
  'unicode.c',
  'config.c',
  'line_editor.c'
]

lle_headers = [
  'text_buffer.h',
  'cursor_math.h',
  'terminal_manager.h',
  'prompt.h',
  'display.h',
  'input_handler.h',
  'edit_commands.h',
  'history.h',
  'completion.h',
  'theme_integration.h',
  'undo.h',
  'syntax.h',
  'unicode.h',
  'config.h',
  'line_editor.h'
]

# Create line editor static library
lle_lib = static_library('lle',
                        lle_sources,
                        include_directories: inc,
                        dependencies: [],
                        install: false)

# Line editor dependency for main lusush build
lle_dep = declare_dependency(link_with: lle_lib,
                            include_directories: include_directories('.'))
EOF
            print_success "Created src/line_editor/meson.build"
        else
            print_warning "src/line_editor/meson.build already exists"
        fi

        # Create test meson.build if not already present
        if [[ ! -f "tests/line_editor/meson.build" ]]; then
            cat > tests/line_editor/meson.build << 'EOF'
# LLE Test Configuration

lle_test_sources = [
  'test_text_buffer.c',
  'test_cursor_math.c',
  'test_terminal_manager.c',
  'test_prompt.c',
  'test_display.c',
  'test_history.c',
  'test_completion.c',
  'test_unicode.c',
  'test_integration.c'
]

# Individual component tests
foreach test_src : lle_test_sources
  test_name = test_src.split('.')[0]
  test_exe = executable(test_name,
                       test_src,
                       dependencies: [lle_dep],
                       include_directories: inc,
                       install: false)
  test(test_name, test_exe)
endforeach

# Performance benchmark
benchmark_exe = executable('lle_benchmark',
                          'benchmark.c',
                          dependencies: [lle_dep],
                          include_directories: inc,
                          c_args: ['-O2'],
                          install: false)
EOF
            print_success "Created tests/line_editor/meson.build"
        else
            print_warning "tests/line_editor/meson.build already exists"
        fi

        # Create helper script for meson commands
        cat > scripts/lle_build.sh << 'EOF'
#!/bin/bash
# LLE Meson Build Helper Script

BUILDDIR=${BUILDDIR:-builddir}

case "$1" in
    "setup")
        echo "Setting up build directory..."
        meson setup $BUILDDIR
        ;;
    "build")
        echo "Building LLE components..."
        meson compile -C $BUILDDIR
        ;;
    "test")
        echo "Running LLE tests..."
        meson test -C $BUILDDIR --suite lle
        ;;
    "test-verbose")
        echo "Running LLE tests (verbose)..."
        meson test -C $BUILDDIR --suite lle --verbose
        ;;
    "benchmark")
        echo "Running LLE benchmarks..."
        $BUILDDIR/tests/line_editor/lle_benchmark
        ;;
    "clean")
        echo "Cleaning build directory..."
        rm -rf $BUILDDIR
        ;;
    "reconfigure")
        echo "Reconfiguring build..."
        meson setup --reconfigure $BUILDDIR
        ;;
    *)
        echo "Usage: $0 {setup|build|test|test-verbose|benchmark|clean|reconfigure}"
        echo ""
        echo "Commands:"
        echo "  setup         - Initialize meson build directory"
        echo "  build         - Build LLE components"
        echo "  test          - Run LLE tests"
        echo "  test-verbose  - Run LLE tests with verbose output"
        echo "  benchmark     - Run performance benchmarks"
        echo "  clean         - Clean build directory"
        echo "  reconfigure   - Reconfigure build system"
        exit 1
        ;;
esac
EOF
        chmod +x scripts/lle_build.sh
        print_success "Created scripts/lle_build.sh helper script"

    else
        print_warning "No meson.build found - you'll need to integrate LLE with your build system manually"
    fi
}

# Set up git hooks for development
setup_git_hooks() {
    print_status "Setting up git hooks for LLE development..."

    # Create pre-commit hook
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# LLE Development Pre-commit Hook

echo "Running LLE pre-commit checks..."

# Check if any LLE files are being committed
if git diff --cached --name-only | grep -q "src/line_editor\|tests/line_editor"; then
    echo "LLE files detected, running quick validation..."

    # Check for basic compilation (if build system is available)
    if [ -f "meson.build" ] && [ -d "builddir" ]; then
        if meson compile -C builddir 2>/dev/null; then
            echo "✓ LLE files compile successfully"
        else
            echo "⚠ Could not verify LLE compilation"
        fi
    else
        echo "⚠ Could not verify LLE compilation (no build directory)"
    fi

    # Check for TODO markers in committed code
    if git diff --cached | grep -q "TODO\|FIXME\|XXX"; then
        echo "⚠ Warning: TODO/FIXME markers found in committed code"
    fi
fi

echo "Pre-commit checks complete"
exit 0
EOF

    chmod +x .git/hooks/pre-commit
    print_success "Git pre-commit hook installed"
}

# Create helpful development scripts
create_dev_scripts() {
    print_status "Creating development helper scripts..."

    # Task completion script
    cat > scripts/complete_task.sh << 'EOF'
#!/bin/bash
# Script to help complete LLE development tasks

if [ $# -ne 1 ]; then
    echo "Usage: $0 <task-number>"
    echo "Example: $0 001"
    exit 1
fi

TASK_NUM=$1
TASK_BRANCH="task/lle-${TASK_NUM}"

echo "Completing LLE-${TASK_NUM}..."

# Run tests
if [ -f "scripts/lle_build.sh" ]; then
    if scripts/lle_build.sh test; then
        echo "✓ Tests passed"
    else
        echo "✗ Tests failed - fix before completing task"
        exit 1
    fi
else
    echo "⚠ Build script not found - manually verify tests pass"
fi

# Update progress file
sed -i "s/LLE-${TASK_NUM}:.*TODO/LLE-${TASK_NUM}: $(date '+%Y-%m-%d') - DONE/" LLE_PROGRESS.md

# Commit changes
git add .
git commit -m "LLE-${TASK_NUM}: Complete task implementation

$(git diff --cached --stat)"

echo "✓ Task LLE-${TASK_NUM} completed and committed"
EOF

    chmod +x scripts/complete_task.sh

    # Progress update script
    cat > scripts/update_progress.sh << 'EOF'
#!/bin/bash
# Script to update LLE development progress

echo "LLE Development Progress Summary"
echo "================================"

TOTAL_TASKS=50
COMPLETED_TASKS=$(grep -c "DONE" LLE_PROGRESS.md)
IN_PROGRESS_TASKS=$(grep -c "IN_PROGRESS" LLE_PROGRESS.md)
TODO_TASKS=$((TOTAL_TASKS - COMPLETED_TASKS - IN_PROGRESS_TASKS))

echo "Tasks Completed: $COMPLETED_TASKS/$TOTAL_TASKS"
echo "Tasks In Progress: $IN_PROGRESS_TASKS"
echo "Tasks Remaining: $TODO_TASKS"

COMPLETION_PERCENT=$((COMPLETED_TASKS * 100 / TOTAL_TASKS))
echo "Completion: $COMPLETION_PERCENT%"

# Update progress file summary
sed -i "/## Summary/,/## Notes/{
    s/Tasks Completed: .*/Tasks Completed: $COMPLETED_TASKS\/$TOTAL_TASKS/
    s/Last updated: .*/Last updated: $(date)/
}" LLE_PROGRESS.md

echo ""
echo "Next tasks to work on:"
grep -n "TODO" LLE_PROGRESS.md | head -3
EOF

    chmod +x scripts/update_progress.sh

    mkdir -p scripts
    print_success "Development helper scripts created"
}

# Main setup function
main() {
    echo "==============================================="
    echo "  Lusush Line Editor Development Setup"
    echo "==============================================="
    echo ""

    check_directory
    create_directory_structure
    setup_git_branches
    create_initial_files
    update_build_system
    setup_git_hooks
    create_dev_scripts

    echo ""
    print_success "LLE development environment setup complete!"
    echo ""
    echo "Next steps:"
    echo "1. Review the task breakdown in LLE_DEVELOPMENT_TASKS.md"
    echo "2. Start with task LLE-001: Basic Text Buffer Structure"
    echo "3. Use 'scripts/update_progress.sh' to track your progress"
    echo "4. Use 'scripts/complete_task.sh <number>' to complete tasks"
    echo ""
    echo "Build commands:"
    echo "• scripts/lle_build.sh setup    - Initialize build directory"
    echo "• scripts/lle_build.sh build    - Build LLE components"
    echo "• scripts/lle_build.sh test     - Run LLE tests"
    echo "• scripts/lle_build.sh benchmark - Run performance tests"
    echo ""
    echo "Integration notes:"
    echo "• LLE will be built as a static library (liblle.a)"
    echo "• Main meson.build will need updating to link with LLE"
    echo "• Linenoise calls will be gradually replaced with LLE API"
    echo ""
    echo "Current branch: $(git rev-parse --abbrev-ref HEAD)"
    echo "Ready to begin LLE development!"
}

# Run main function
main "$@"
