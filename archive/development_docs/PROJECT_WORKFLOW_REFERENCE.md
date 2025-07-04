# LUSUSH PROJECT WORKFLOW REFERENCE

**Version**: 1.0.0-dev  
**Date**: December 2024  
**Status**: Production Ready Development

## Quick Start

### Build and Test
```bash
# Configure and build
meson setup builddir
ninja -C builddir

# Run POSIX compliance tests (mandatory)
./tests/compliance/test_posix_regression.sh

# Run comprehensive test suite
./tests/compliance/test_shell_compliance_comprehensive.sh

# Format code (mandatory before commits)
./tools/clang-format-all .
```

### Development Workflow
```bash
# 1. Make changes
vim src/file.c

# 2. Build and verify
ninja -C builddir

# 3. Test compliance (must pass)
./tests/compliance/test_posix_regression.sh

# 4. Format code
./tools/clang-format-all .

# 5. Commit with descriptive message
git add -A && git commit -m "Brief technical description"
```

## Testing Framework

### POSIX Regression Tests (Mandatory)
- **File**: `tests/compliance/test_posix_regression.sh`
- **Purpose**: Ensures no functionality regressions
- **Requirement**: Must pass 49/49 tests before any commit
- **Usage**: Run after every change

### Comprehensive Compliance Tests
- **File**: `tests/compliance/test_shell_compliance_comprehensive.sh`
- **Purpose**: Complete functionality validation across 12 categories
- **Coverage**: 100+ individual test cases
- **Usage**: Full validation for major changes

### Test Categories
1. Basic Command Execution
2. Variable Operations and Expansion
3. Arithmetic Expansion
4. Command Substitution
5. Control Structures
6. Function Operations
7. I/O Redirection and Pipes
8. Built-in Commands
9. Pattern Matching and Globbing
10. Error Handling and Edge Cases
11. Real-World Complex Scenarios
12. Performance and Stress Testing

## Code Quality Standards

### Mandatory Requirements
- **ISO C99 Compliance**: All code must compile with `gcc -std=c99 -pedantic`
- **Code Formatting**: Use `./tools/clang-format-all .` before every commit
- **No Memory Leaks**: Verify with valgrind for major changes
- **Test Compliance**: All commits must maintain 49/49 POSIX tests

### Code Organization
```
src/
├── lusush.c         # Main shell loop
├── parser.c         # Recursive descent parser
├── executor.c       # Command execution engine
├── tokenizer.c      # Lexical analysis
├── expand.c         # Parameter expansion
├── arithmetic.c     # Arithmetic evaluation
├── symtable.c       # Variable management
└── builtins/        # Built-in command implementations

tests/
├── compliance/      # Main test suites
├── debug/          # Debug and development tests
└── legacy/         # Historical test files

docs/
├── development/    # Technical documentation
└── achievements/   # Implementation milestones

tools/
└── clang-format-all # Code formatting script
```

## Commit Standards

### Commit Message Format
```
Brief technical description

Detailed explanation of what was implemented/fixed, including:
- Technical implementation details
- Root cause analysis (for fixes)
- Files modified
- Test results
- Impact on functionality

Always include:
- "All 49/49 POSIX regression tests maintained"
- Specific test improvements or changes
- Exit code and functionality verification
```

### Example Commit
```
Fix arithmetic expansion in quoted strings

Implemented proper arithmetic evaluation within double-quoted contexts
by enhancing the expand_quoted_string function to handle $((expr))
syntax correctly.

Technical implementation:
- Modified expand_quoted_string() in src/executor.c
- Added proper parentheses parsing for arithmetic detection
- Enhanced error handling for malformed expressions

Test results:
- All 49/49 POSIX regression tests maintained
- Arithmetic Expansion category: 95% to 100% improvement
- Overall compliance maintained at 99%

Files changed:
- src/executor.c: Enhanced quoted string arithmetic handling
```

## Development Priorities

### Current Status
- **Overall Compliance**: 95% (Production Ready)
- **Test Success Rate**: 99% (135/136 tests)
- **Perfect Categories**: 9/12 at 100% completion
- **POSIX Compliance**: 100% (49/49 regression tests)

### Enhancement Opportunities
1. **Real-World Scenarios**: 80% → 100% (1 test remaining)
2. **Error Handling**: 85% → 100% (1 test remaining)
3. **Performance Stress**: 75% → 100% (1 test remaining)

### Focus Areas
- **Edge Case Refinement**: Address remaining test failures
- **Performance Optimization**: Enhance execution speed for large scripts
- **Error Message Quality**: Improve user-facing error reporting
- **Documentation**: Expand user guides and examples

## Build System

### Meson Configuration
```bash
# Standard build
meson setup builddir

# Debug build
meson setup builddir -Dbuildtype=debug

# Release build
meson setup builddir -Dbuildtype=release
```

### Build Targets
```bash
# Compile everything
ninja -C builddir

# Compile specific target
ninja -C builddir lusush

# Install (optional)
meson install -C builddir
```

## Testing Best Practices

### Before Making Changes
1. Run `./tests/compliance/test_posix_regression.sh` to establish baseline
2. Note current compliance scores from comprehensive tests
3. Ensure clean build with `ninja -C builddir`

### During Development
1. Build frequently: `ninja -C builddir`
2. Test specific functionality being modified
3. Use debug scripts in `tests/debug/` for focused testing

### Before Committing
1. **Mandatory**: `./tests/compliance/test_posix_regression.sh` must pass 49/49
2. Run comprehensive tests to check for improvements
3. Format code: `./tools/clang-format-all .`
4. Verify clean build with no new warnings

### Debugging Tools
```bash
# Debug specific functionality
./tests/debug/debug_*.sh

# Test specific shell features
echo 'command' | ./builddir/lusush

# Memory leak checking
valgrind ./builddir/lusush
```

## Repository Structure

### Essential Files (Root)
- `README.md` - Project overview and quick start
- `LICENSE` - MIT license
- `meson.build` - Build configuration
- `PROJECT_STATUS_CURRENT.md` - Current development status
- `COMPREHENSIVE_TEST_SUITE.md` - Testing framework documentation

### Source Code
- `src/` - All source code implementations
- `include/` - Header files and interfaces

### Testing
- `tests/compliance/` - Essential test suites
- `tests/debug/` - Development and debugging tests
- `tests/legacy/` - Historical test files

### Documentation
- `docs/development/` - Technical and architectural documentation
- `docs/achievements/` - Implementation milestone documentation

### Tools
- `tools/` - Development tools and utilities
- `.clang-format` - Code formatting configuration

## Performance Guidelines

### Memory Management
- Always free allocated memory
- Use proper error handling and cleanup
- Check for memory leaks with valgrind
- Avoid unnecessary allocations in hot paths

### Code Efficiency
- Prefer simple, readable solutions
- Optimize only when necessary
- Profile before optimizing
- Maintain code clarity

### Error Handling
- Check all system call return values
- Provide meaningful error messages
- Clean up resources on error paths
- Follow POSIX error code conventions

## Integration Guidelines

### Adding New Features
1. Design with existing architecture in mind
2. Maintain backward compatibility
3. Add comprehensive tests
4. Update documentation
5. Follow existing code patterns

### Modifying Existing Features
1. Understand current implementation thoroughly
2. Identify potential impact areas
3. Test all related functionality
4. Maintain or improve test coverage
5. Document changes and rationale

### Bug Fixes
1. Identify root cause completely
2. Create test that reproduces the issue
3. Implement minimal, targeted fix
4. Verify fix doesn't break other functionality
5. Update tests to prevent regression

---

**Remember**: Lusush is production-ready software. All changes must maintain the high quality standards that have achieved 99% test success rate and 95% POSIX compliance.