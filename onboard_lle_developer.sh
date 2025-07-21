#!/bin/bash

# Lusush Line Editor (LLE) Developer Onboarding Script
# This script instantly prepares any developer to start contributing to LLE development

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Print styled output
print_header() {
    echo -e "\n${BOLD}${CYAN}===============================================${NC}"
    echo -e "${BOLD}${CYAN}  $1${NC}"
    echo -e "${BOLD}${CYAN}===============================================${NC}\n"
}

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

print_task() {
    echo -e "${CYAN}📋 $1${NC}"
}

print_command() {
    echo -e "${YELLOW}$ $1${NC}"
}

# Check if running in the correct directory
check_environment() {
    print_header "Environment Check"

    if [[ ! -f "LLE_DEVELOPMENT_TASKS.md" ]]; then
        print_error "This script must be run from the lusush project root directory"
        print_error "Expected to find LLE_DEVELOPMENT_TASKS.md in current directory"
        exit 1
    fi

    if [[ ! -d ".git" ]]; then
        print_error "This doesn't appear to be a git repository"
        exit 1
    fi

    if [[ ! -f "meson.build" ]]; then
        print_error "Meson build file not found - this may not be the correct directory"
        exit 1
    fi

    print_success "Environment validated"
    print_status "Working directory: $(pwd)"
    print_status "Git repository: $(git remote get-url origin 2>/dev/null || echo 'local repository')"
}

# Display current project status
show_project_status() {
    print_header "Project Status"

    # Show current git branch
    current_branch=$(git rev-parse --abbrev-ref HEAD)
    print_status "Current branch: ${current_branch}"

    # Check if LLE branch exists
    if git show-ref --verify --quiet refs/heads/feature/lusush-line-editor; then
        print_success "LLE development branch exists"
    else
        print_warning "LLE development branch not found"
    fi

    # Show LLE progress if available
    if [[ -f "LLE_PROGRESS.md" ]]; then
        completed_tasks=$(grep -c "DONE" LLE_PROGRESS.md || echo "0")
        in_progress_tasks=$(grep -c "IN_PROGRESS" LLE_PROGRESS.md || echo "0")
        total_tasks=50
        remaining_tasks=$((total_tasks - completed_tasks - in_progress_tasks))

        echo ""
        print_status "LLE Development Progress:"
        echo "  ✅ Completed: ${completed_tasks}/${total_tasks}"
        echo "  🔄 In Progress: ${in_progress_tasks}"
        echo "  📋 Remaining: ${remaining_tasks}"

        completion_percent=$((completed_tasks * 100 / total_tasks))
        echo "  📊 Completion: ${completion_percent}%"
    else
        print_warning "LLE_PROGRESS.md not found - run setup_lle_development.sh first"
    fi
}

# Show available development tools
check_tools() {
    print_header "Development Tools Check"

    # Check essential tools
    tools=("git" "meson" "gcc" "clang" "make" "valgrind" "gdb")
    optional_tools=("perf" "clang-format" "cppcheck")

    echo "Essential tools:"
    for tool in "${tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            version=$(${tool} --version 2>/dev/null | head -n1 || echo "version unknown")
            print_success "${tool} - ${version}"
        else
            print_warning "${tool} - not found"
        fi
    done

    echo ""
    echo "Optional tools:"
    for tool in "${optional_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            version=$(${tool} --version 2>/dev/null | head -n1 || echo "available")
            print_success "${tool} - ${version}"
        else
            print_status "${tool} - not available (optional)"
        fi
    done
}

# Show next task to work on
show_next_task() {
    print_header "Next Development Task"

    if [[ -f "LLE_PROGRESS.md" ]]; then
        # Find first TODO task
        next_task=$(grep -n "TODO" LLE_PROGRESS.md | head -n1 | cut -d: -f2 | sed 's/^[[:space:]]*//')

        if [[ -n "$next_task" ]]; then
            print_task "Next task to work on:"
            echo "  ${next_task}"

            # Extract task ID
            task_id=$(echo "$next_task" | grep -o "LLE-[0-9]\{3\}" | head -n1)

            if [[ -n "$task_id" ]]; then
                echo ""
                print_status "To start working on this task:"
                print_command "git checkout feature/lusush-line-editor"
                print_command "git checkout -b task/${task_id,,}-description"
                print_command "# Read task details in LLE_DEVELOPMENT_TASKS.md"
                print_command "# Implement, test, and commit"
                print_command "scripts/complete_task.sh ${task_id#LLE-}"
            fi
        else
            print_success "All tasks completed! 🎉"
        fi
    else
        print_warning "Run setup_lle_development.sh to initialize task tracking"
    fi
}

# Show essential commands
show_commands() {
    print_header "Essential Commands"

    echo "🔧 Build Commands:"
    print_command "scripts/lle_build.sh setup      # Initialize build directory"
    print_command "scripts/lle_build.sh build      # Build LLE components"
    print_command "scripts/lle_build.sh test       # Run all tests"
    print_command "scripts/lle_build.sh benchmark  # Performance tests"
    print_command "scripts/lle_build.sh clean      # Clean build"

    echo ""
    echo "📋 Development Commands:"
    print_command "scripts/update_progress.sh      # Check progress"
    print_command "scripts/complete_task.sh XXX    # Complete task XXX"
    print_command "git checkout -b task/lle-XXX-desc # Start new task"

    echo ""
    echo "🧪 Testing Commands:"
    print_command "meson test -C builddir test_text_buffer  # Specific test"
    print_command "meson test -C builddir --verbose         # Verbose output"
    print_command "valgrind --leak-check=full builddir/lusush # Memory check"

    echo ""
    echo "🐛 Debug Commands:"
    print_command "export LLE_DEBUG=1               # Enable debug output"
    print_command "gdb builddir/lusush              # Debug with GDB"
    print_command "perf record builddir/lusush      # Performance profiling"
}

# Show key files to read
show_key_files() {
    print_header "Key Files for Development"

    echo "📋 Task & Progress Files:"
    echo "  📄 LLE_DEVELOPMENT_TASKS.md     - Complete task breakdown (50 tasks)"
    echo "  📊 LLE_PROGRESS.md              - Current progress tracking"
    echo "  📋 LLE_DEVELOPMENT_WORKFLOW.md  - Git workflow and quality standards"

    echo ""
    echo "🤖 AI Assistant Files:"
    echo "  🎯 AI_CONTEXT.md                - Instant context for AI assistants"
    echo "  📖 LLE_AI_DEVELOPMENT_GUIDE.md  - Comprehensive AI development guide"
    echo "  ⚙️  .cursorrules                 - Code standards and patterns"

    echo ""
    echo "📚 Reference Files:"
    echo "  🔍 LINE_EDITOR_STRATEGIC_ANALYSIS.md - Why we're building LLE"
    echo "  📁 src/line_editor/             - LLE source code directory"
    echo "  🧪 tests/line_editor/           - LLE test suite"
    echo "  📜 scripts/lle_build.sh         - Build helper script"

    echo ""
    echo "📖 Quick Start Reading Order:"
    echo "  1️⃣  AI_CONTEXT.md (5 min)        - Instant overview"
    echo "  2️⃣  LLE_PROGRESS.md (2 min)      - Current status"
    echo "  3️⃣  Next task in LLE_DEVELOPMENT_TASKS.md (10 min)"
    echo "  4️⃣  .cursorrules (5 min)         - Code standards"
}

# Check build system status
check_build_system() {
    print_header "Build System Status"

    if [[ -d "builddir" ]]; then
        print_success "Build directory exists"

        if [[ -f "builddir/build.ninja" ]]; then
            print_success "Meson build configured"
        else
            print_warning "Build directory exists but not configured"
            print_status "Run: scripts/lle_build.sh setup"
        fi
    else
        print_status "Build directory not found"
        print_status "Run: scripts/lle_build.sh setup"
    fi

    # Check if LLE build files exist
    if [[ -f "src/line_editor/meson.build" ]]; then
        print_success "LLE build configuration exists"
    else
        print_warning "LLE build configuration missing"
        print_status "Run: ./setup_lle_development.sh"
    fi

    # Check for helper scripts
    if [[ -f "scripts/lle_build.sh" ]]; then
        print_success "LLE build helper script available"
    else
        print_warning "LLE build helper script missing"
        print_status "Run: ./setup_lle_development.sh"
    fi
}

# Suggest development environment setup
suggest_editor_setup() {
    print_header "Development Environment Suggestions"

    echo "🔧 Recommended Editor Setup:"
    echo ""
    echo "📝 For VS Code/Cursor:"
    echo "  • Install C/C++ extension"
    echo "  • Install Meson extension"
    echo "  • Load workspace from lusush root directory"
    echo "  • AI assistant will read .cursorrules automatically"
    echo ""
    echo "⚙️  For Vim/Neovim:"
    echo "  • Install LSP support (clangd)"
    echo "  • Set up build system integration"
    echo "  • Reference .cursorrules for coding standards"
    echo ""
    echo "🔍 For any editor with AI:"
    echo "  • Load AI_CONTEXT.md first for instant context"
    echo "  • Reference LLE_AI_DEVELOPMENT_GUIDE.md for detailed guidance"
    echo "  • Always check LLE_PROGRESS.md for current task"
    echo ""
    echo "💡 Pro Tips:"
    echo "  • Use 'git log --oneline' to see recent LLE commits"
    echo "  • Run tests frequently during development"
    echo "  • Follow the mathematical correctness principle"
    echo "  • Write tests alongside implementation"
}

# Show performance and quality standards
show_standards() {
    print_header "Performance & Quality Standards"

    echo "⚡ Performance Requirements:"
    echo "  • Character insertion: < 1ms"
    echo "  • Cursor movement: < 1ms"
    echo "  • Display update: < 5ms"
    echo "  • History search: < 10ms"
    echo "  • Tab completion: < 50ms"
    echo ""
    echo "💾 Memory Requirements:"
    echo "  • Base memory: < 1MB"
    echo "  • Per-character overhead: < 50 bytes"
    echo "  • No memory leaks (Valgrind clean)"
    echo ""
    echo "🎯 Quality Standards:"
    echo "  • 100% test coverage for public functions"
    echo "  • Mathematical correctness for cursor positioning"
    echo "  • UTF-8 support throughout"
    echo "  • Cross-terminal compatibility"
    echo "  • Integration with Lusush theme system"
    echo ""
    echo "🧪 Testing Requirements:"
    echo "  • Unit tests for every function"
    echo "  • Integration tests for component interactions"
    echo "  • Edge case and error condition testing"
    echo "  • Performance validation"
    echo "  • Memory leak detection"
}

# Main function
main() {
    print_header "Lusush Line Editor Developer Onboarding"

    check_environment
    show_project_status
    check_tools
    check_build_system
    show_next_task
    show_commands
    show_key_files
    suggest_editor_setup
    show_standards

    print_header "Ready to Develop!"

    echo "🎯 You're all set to contribute to LLE development!"
    echo ""
    echo "🚀 Quick Start Steps:"
    echo "  1. Read AI_CONTEXT.md for instant overview"
    echo "  2. Check next task in LLE_PROGRESS.md"
    echo "  3. Set up build system: scripts/lle_build.sh setup"
    echo "  4. Start coding following .cursorrules standards"
    echo "  5. Write tests and run: scripts/lle_build.sh test"
    echo ""
    echo "📞 Need Help?"
    echo "  • Check LLE_AI_DEVELOPMENT_GUIDE.md for detailed guidance"
    echo "  • Use AI assistance with context from .cursorrules"
    echo "  • Review existing code in src/line_editor/ for patterns"
    echo "  • Run diagnostics if stuck: scripts/lle_build.sh test"
    echo ""
    print_success "Happy coding! 🚀"
}

# Run main function with all output
main "$@"
