# Contributing to Lusush

Thank you for your interest in contributing to Lusush! This document provides guidelines and information for contributors.

## Table of Contents
1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [Development Setup](#development-setup)
4. [Development Workflow](#development-workflow)
5. [Coding Standards](#coding-standards)
6. [Testing Requirements](#testing-requirements)
7. [Submitting Changes](#submitting-changes)
8. [Documentation](#documentation)
9. [Issue Reporting](#issue-reporting)
10. [Community](#community)

## Code of Conduct

### Our Pledge
We are committed to making participation in the Lusush project a harassment-free experience for everyone, regardless of age, body size, disability, ethnicity, gender identity and expression, level of experience, nationality, personal appearance, race, religion, or sexual identity and orientation.

### Our Standards
Examples of behavior that contributes to creating a positive environment include:
- Using welcoming and inclusive language
- Being respectful of differing viewpoints and experiences
- Gracefully accepting constructive criticism
- Focusing on what is best for the community
- Showing empathy towards other community members

### Enforcement
Instances of abusive, harassing, or otherwise unacceptable behavior may be reported by contacting the project team at conduct@lusush.org.

## Getting Started

### Prerequisites
- **System**: Linux, macOS, or Unix-like system
- **Compiler**: GCC 7.0+ or Clang 6.0+
- **Build System**: Meson 0.55.0+ and Ninja 1.8.0+
- **Version Control**: Git
- **Editor**: Any text editor with C support (VS Code, Vim, Emacs, etc.)

### Skills Helpful for Contributing
- **C Programming**: Core language for shell implementation
- **Shell Scripting**: For tests and build scripts
- **POSIX Standards**: Understanding of shell specifications
- **Terminal/TTY Programming**: For line editing and completion features
- **Build Systems**: Experience with Meson/Ninja

## Development Setup

### 1. Fork and Clone
```bash
# Fork the repository on GitHub
# Then clone your fork
git clone https://github.com/YOUR_USERNAME/lusush.git
cd lusush

# Add upstream remote
git remote add upstream https://github.com/lusush/lusush.git
```

### 2. Install Dependencies
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential meson ninja-build git pkg-config
sudo apt install libreadline-dev libncurses-dev clang-format

# macOS
brew install meson ninja git pkg-config clang-format
brew install readline ncurses

# CentOS/RHEL/Fedora
sudo dnf install gcc meson ninja-build git pkgconfig clang
sudo dnf install readline-devel ncurses-devel
```

### 3. Build Development Version
```bash
# Configure debug build
meson setup builddir --buildtype=debugoptimized

# Build
ninja -C builddir

# Test installation
./builddir/lusush --version
```

### 4. Run Tests
```bash
# Run all tests
ninja -C builddir test

# Run specific test suites
./tests/compliance/test_posix_regression.sh
./tests/compliance/test_shell_compliance_comprehensive.sh
./tests/debug/verify_enhanced_features.sh
```

### 5. Set Up Development Environment
```bash
# Install pre-commit hooks (optional but recommended)
cp scripts/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit

# Configure git (if not already done)
git config user.name "Your Name"
git config user.email "your.email@example.com"
```

## Development Workflow

### 1. Mandatory Development Process
For each development cycle, you MUST follow this process:

```bash
# 1. Build verification
ninja -C builddir

# 2. Ensure regression tests pass
./tests/compliance/test_posix_regression.sh

# 3. Implement your changes
# ... edit files ...

# 4. Verify build success
ninja -C builddir

# 5. Confirm no regressions
./tests/compliance/test_posix_regression.sh

# 6. Run comprehensive validation
./tests/compliance/test_shell_compliance_comprehensive.sh

# 7. Check enhanced features
./tests/debug/verify_enhanced_features.sh

# 8. Apply code formatting
./tools/clang-format-all .

# 9. Commit changes
git add -A && git commit -m "description"

# 10. Update documentation as needed
```

**CRITICAL**: Use `ninja -C builddir` NOT `meson compile -C builddir`. The project uses ninja as the established build tool.

### 2. Branch Strategy
```bash
# Create feature branch
git checkout -b feature/your-feature-name

# Keep up to date with upstream
git fetch upstream
git rebase upstream/main

# Push changes
git push origin feature/your-feature-name
```

### 3. Commit Guidelines
- **Format**: Use conventional commits format
- **Scope**: Keep commits focused and atomic
- **Message**: Clear, descriptive commit messages
- **Testing**: Every commit should pass all tests

#### Commit Message Format
```
type(scope): description

[optional body]

[optional footer]
```

#### Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or modifying tests
- `chore`: Build system or auxiliary tool changes

#### Examples
```
feat(completion): add single-line menu completion system

Implement professional single-line menu that updates in place
without cluttering the screen. Includes categorization of
completion types and user-toggleable modes.

Fixes #123
```

## Coding Standards

### 1. Code Style
- **Language**: ISO C99 compliant
- **Formatting**: Use clang-format (run `./tools/clang-format-all .`)
- **Naming**: 
  - Functions: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_CASE`
  - Types: `snake_case_t`

### 2. Code Quality
```c
// Good: Clear, documented function
/**
 * Parse command line arguments and extract options
 * @param argc Number of arguments
 * @param argv Argument array
 * @return Parsed options structure or NULL on error
 */
static shell_options_t *parse_arguments(int argc, char **argv) {
    if (!argv || argc < 1) {
        return NULL;
    }
    
    // Implementation...
}

// Bad: Unclear, undocumented function
static void do_stuff(int a, char **b) {
    // No validation, unclear purpose
}
```

### 3. Error Handling
```c
// Good: Proper error handling
char *result = malloc(size);
if (!result) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return -1;
}

// Bad: No error checking
char *result = malloc(size);
strcpy(result, source);  // Potential segfault
```

### 4. Memory Management
```c
// Good: Proper cleanup
void cleanup_resources(resource_t *res) {
    if (res) {
        free(res->data);
        res->data = NULL;
        free(res);
    }
}

// Bad: Memory leaks
void bad_cleanup(resource_t *res) {
    free(res);  // Doesn't free res->data
}
```

### 5. POSIX Compliance
- **Standards**: Maintain 100% POSIX compliance
- **Portability**: Code must work on Linux, macOS, and Unix systems
- **Feature Tests**: Use feature test macros appropriately
- **System Calls**: Use POSIX system calls and library functions

## Testing Requirements

### 1. Test Categories
All changes must pass:
- **POSIX Regression Tests**: 49/49 tests must pass
- **Comprehensive Shell Tests**: 136/136 tests must pass  
- **Enhanced Features Tests**: 26/26 tests must pass

### 2. Adding New Tests
```bash
# For new features, add tests in appropriate directory
tests/
├── compliance/          # POSIX compliance tests
├── unit/               # Unit tests
├── integration/        # Integration tests
└── debug/              # Feature verification tests
```

### 3. Test Writing Guidelines
```bash
#!/bin/bash
# Example test script structure

test_description="Test new feature functionality"

# Setup
setup_test_environment() {
    # Prepare test data
}

# Test cases
test_basic_functionality() {
    result=$(echo "test command" | ./builddir/lusush)
    if [ "$result" = "expected output" ]; then
        echo "✓ PASSED: Basic functionality"
        return 0
    else
        echo "✗ FAILED: Basic functionality"
        return 1
    fi
}

# Cleanup
cleanup_test_environment() {
    # Clean up test data
}

# Run tests
main() {
    setup_test_environment
    test_basic_functionality
    exit_code=$?
    cleanup_test_environment
    exit $exit_code
}

main "$@"
```

### 4. Performance Testing
- **Benchmarks**: Include performance tests for significant changes
- **Memory**: Check for memory leaks with valgrind
- **Startup Time**: Ensure startup time remains under 100ms
- **Completion Speed**: Keep completion response under 50ms

## Submitting Changes

### 1. Before Submitting
- [ ] All tests pass
- [ ] Code is formatted (`./tools/clang-format-all .`)
- [ ] Documentation is updated
- [ ] Commit messages follow guidelines
- [ ] No compiler warnings
- [ ] POSIX compliance maintained

### 2. Pull Request Process
1. **Push to Fork**: Push your feature branch to your fork
2. **Create PR**: Open a pull request against the main branch
3. **Fill Template**: Use the provided PR template
4. **Address Feedback**: Respond to review comments
5. **Rebase if Needed**: Keep history clean

### 3. Pull Request Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] POSIX regression tests pass (49/49)
- [ ] Comprehensive shell tests pass (136/136)
- [ ] Enhanced features tests pass (26/26)
- [ ] New tests added for new functionality

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No new compiler warnings
- [ ] POSIX compliance maintained
```

### 4. Review Process
- **Automated Checks**: CI will run tests automatically
- **Code Review**: Maintainers will review code quality
- **Testing**: Additional testing on different platforms
- **Documentation**: Review of documentation changes
- **Approval**: Two maintainer approvals required

## Documentation

### 1. Code Documentation
- **Functions**: Document all public functions
- **Complex Logic**: Comment complex algorithms
- **APIs**: Document all public interfaces
- **Examples**: Provide usage examples

### 2. User Documentation
Update relevant user documentation:
- `docs/user/USER_MANUAL.md`
- `docs/user/FEATURES.md`
- `README.md` (if needed)

### 3. Developer Documentation
Update developer documentation:
- `docs/developer/API_REFERENCE.md`
- `docs/developer/ARCHITECTURE.md`
- This `CONTRIBUTING.md` file

## Issue Reporting

### 1. Bug Reports
Use the bug report template:

```markdown
**Bug Description**
Clear description of the bug

**Steps to Reproduce**
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

**Expected Behavior**
What you expected to happen

**Actual Behavior**
What actually happened

**Environment**
- OS: [e.g., Ubuntu 20.04]
- Lusush Version: [e.g., 1.0.0]
- Terminal: [e.g., GNOME Terminal]

**Additional Context**
Any other context about the problem
```

### 2. Feature Requests
Use the feature request template:

```markdown
**Feature Description**
Clear description of the feature

**Use Case**
Why is this feature needed?

**Proposed Solution**
How should this feature work?

**Alternatives Considered**
Other solutions you've considered

**Additional Context**
Any other context or screenshots
```

### 3. Security Issues
For security issues, please email security@lusush.org instead of creating a public issue.

## Community

### 1. Communication Channels
- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General discussion and questions
- **IRC**: #lusush on irc.libera.chat
- **Email**: dev@lusush.org for development discussions

### 2. Getting Help
- **Documentation**: Start with the user manual
- **Search Issues**: Check existing issues first
- **Ask Questions**: Use GitHub Discussions for questions
- **IRC**: Join #lusush for real-time help

### 3. Contributing Areas
We welcome contributions in:
- **Core Shell**: POSIX compliance and shell features
- **Completion System**: Tab completion improvements
- **Themes**: New themes and customization
- **Network Features**: SSH and network integration
- **Documentation**: User and developer documentation
- **Testing**: Test coverage and new test cases
- **Performance**: Optimization and benchmarking
- **Portability**: Support for new platforms

### 4. Maintainers
Current maintainers:
- Lead Maintainer: @lead-maintainer
- Core Team: @core-team
- Documentation: @docs-team

## Recognition

### 1. Contributors
All contributors are recognized in:
- `CONTRIBUTORS.md` file
- Release notes
- Project documentation

### 2. Significant Contributions
Major contributions may be recognized with:
- Commit access
- Maintainer status
- Special recognition in releases

## Development Resources

### 1. Useful References
- **POSIX Standard**: IEEE Std 1003.1-2017
- **Shell Grammar**: POSIX Shell Command Language
- **C Standards**: ISO/IEC 9899:1999 (C99)
- **Terminal Programming**: terminfo, ncurses documentation

### 2. Development Tools
- **Static Analysis**: Use `clang-static-analyzer`
- **Memory Checking**: Use `valgrind` for memory leak detection
- **Profiling**: Use `gprof` or `perf` for performance analysis
- **Debugging**: Use `gdb` for debugging

### 3. Learning Resources
- [Advanced Programming in the UNIX Environment](http://www.apuebook.com/)
- [The Linux Programming Interface](http://man7.org/tlpi/)
- [POSIX Programmer's Manual](http://pubs.opengroup.org/onlinepubs/9699919799/)

---

## License

By contributing to Lusush, you agree that your contributions will be licensed under the same license as the project (BSD 3-Clause License).

## Questions?

If you have questions about contributing, please:
1. Check this document first
2. Search existing issues and discussions
3. Ask in GitHub Discussions
4. Join #lusush on IRC
5. Email dev@lusush.org

Thank you for contributing to Lusush!