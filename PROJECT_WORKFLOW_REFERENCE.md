# LUSUSH SHELL PROJECT WORKFLOW REFERENCE

**Document Type**: Clickable Text-Only Project Management Reference  
**Purpose**: Maintain familiarity with project status and established development workflow  
**Last Updated**: December 2024  
**Status**: Active Development - POSIX Compliance Phase Complete

## QUICK PROJECT STATUS OVERVIEW

**Current Achievement Level**: 85% POSIX Compliant Shell  
**Regression Tests**: 49/49 passing  
**Major Features**: All core functionality complete  
**Recent Achievement**: Complete command substitution enhancement with nested quotes support

## ESTABLISHED DEVELOPMENT WORKFLOW

### STANDARD DEVELOPMENT CYCLE
1. **Identify Issue**: Through regression tests, diagnostic scripts, or feature gaps
2. **Implement Solution**: Make targeted, surgical changes to resolve specific issues
3. **Test Implementation**: Verify fix works and run regression test suite
4. **Format Codebase**: Execute `./clang-format-all .` on entire codebase
5. **Create Text-Only Git Commit**: No emojis, no symbols, descriptive technical content
6. **Update Core Documentation**: Reflect changes in PROJECT_STATUS_CURRENT.md and README.md
7. **Validate Completion**: Ensure no regressions and document achievement

### REQUIRED COMMANDS FOR EACH CYCLE
```
# Build and test
ninja -C builddir
./test_posix_regression.sh

# Format entire codebase
./clang-format-all .

# Standard git workflow
git add -A
git commit -m "Text-only commit message describing technical achievement"

# Verify no regressions
ninja -C builddir
./test_posix_regression.sh | tail -n 5
```

### COMMIT MESSAGE STANDARDS
- **Format**: Text-only, no emojis, no symbols
- **Content**: Technical achievement description
- **Structure**: Title line, blank line, detailed technical content
- **Focus**: What was fixed, how it was implemented, test results

## CORE PROJECT FILES TO MAINTAIN

### PRIMARY DOCUMENTATION
- **PROJECT_STATUS_CURRENT.md**: Comprehensive project status and achievements
- **README.md**: High-level project overview and feature list
- **ACHIEVEMENT_SUMMARY_DECEMBER_2024.md**: Monthly achievement tracking

### CRITICAL SOURCE FILES
- **src/executor.c**: Main execution engine and command processing
- **src/tokenizer.c**: Input parsing and token generation
- **src/parser.c**: AST generation and syntax analysis
- **src/symtable.c**: Variable and symbol management
- **include/*.h**: All header files for API consistency

### ESSENTIAL TEST FILES
- **test_posix_regression.sh**: 49 regression tests for POSIX compliance
- **test-comprehensive.sh**: Feature demonstration and validation
- **test_broken_features_diagnosis.sh**: Edge case identification

## RECENT MAJOR ACHIEVEMENTS

### December 2024 - Command Substitution Enhancement
**Technical Focus**: Nested quotes and backtick command substitution
**Files Modified**: src/tokenizer.c, src/executor.c
**Result**: Complete command substitution functionality for both modern and legacy syntax
**Validation**: All regression tests maintained, complex edge cases resolved

### December 2024 - Enhanced Symbol Table System  
**Technical Focus**: Performance improvement and POSIX compliance
**Files Modified**: src/symtable.c, include/symtable.h
**Result**: 3-4x performance improvement with maintained API compatibility
**Validation**: Fixed critical unset variable bug, achieved 49/49 regression test compliance

### December 2024 - Core Edge Case Resolution
**Technical Focus**: Parameter expansion and arithmetic variable resolution
**Files Modified**: src/executor.c, src/arithmetic.c
**Result**: Multiple parameter expansions and arithmetic variables working correctly
**Validation**: No regressions, enhanced functionality across all test suites

## CURRENT PROJECT STRUCTURE

### BUILD SYSTEM
- **meson.build**: Primary build configuration
- **builddir/**: Compiled binaries and build artifacts
- **clang-format-all**: Code formatting automation script

### SOURCE ORGANIZATION
- **src/**: Core implementation files
- **include/**: Header files and API definitions
- **src/builtins/**: Shell builtin command implementations
- **docs/**: Technical documentation and specifications

### TESTING FRAMEWORK
- **Regression Tests**: 49 POSIX compliance tests with 100% pass rate
- **Diagnostic Tests**: Edge case identification and validation
- **Feature Tests**: Comprehensive functionality demonstration
- **Debug Scripts**: Targeted issue analysis and resolution

## QUALITY ASSURANCE STANDARDS

### CODE QUALITY REQUIREMENTS
- **Formatting**: clang-format applied to entire codebase before commits
- **Compilation**: Clean ninja build with minimal warnings
- **Testing**: All 49 regression tests must pass
- **Documentation**: Core docs updated to reflect all changes

### REGRESSION PREVENTION
- **Test Suite**: Execute full regression suite after each change
- **Edge Case Validation**: Run diagnostic tests for complex scenarios
- **API Compatibility**: Maintain existing interfaces and functionality
- **Performance Monitoring**: Ensure no performance degradation

## DEVELOPMENT PRIORITIES

### COMPLETED PHASES
- **Phase 1**: Core Shell Foundation - Variable operations, command execution
- **Phase 2**: Advanced Features - Functions, control structures, I/O redirection  
- **Phase 3**: Edge Case Resolution - Command substitution, parameter expansion
- **Phase 4**: Performance Enhancement - Symbol table optimization

### ONGOING MAINTENANCE
- **Regression Prevention**: Maintain 49/49 test compliance
- **Code Quality**: Consistent formatting and documentation standards
- **API Stability**: Preserve existing interfaces and functionality
- **Documentation**: Keep core documentation synchronized with codebase

## WORKFLOW CHECKLIST FOR NEW WORK

### PRE-DEVELOPMENT
- [ ] Review current PROJECT_STATUS_CURRENT.md
- [ ] Identify specific issue or enhancement target
- [ ] Understand impact on existing functionality
- [ ] Plan surgical, targeted implementation approach

### DURING DEVELOPMENT  
- [ ] Make focused changes to resolve specific issues
- [ ] Test changes with relevant test scripts
- [ ] Verify no regressions with ninja build
- [ ] Run regression test suite to confirm 49/49 passing

### POST-DEVELOPMENT
- [ ] Execute `./clang-format-all .` on entire codebase
- [ ] Create text-only git commit with technical description
- [ ] Update PROJECT_STATUS_CURRENT.md with achievement details
- [ ] Update README.md if major functionality added
- [ ] Final regression test validation

### VALIDATION
- [ ] Clean ninja build with minimal warnings
- [ ] All 49 POSIX regression tests passing
- [ ] No functional regressions in existing features
- [ ] Documentation accurately reflects current state

## CONTACT AND REFERENCE

### TECHNICAL ARCHITECTURE
- **Language**: ISO C99 compliant implementation
- **Build System**: Meson with ninja backend
- **Dependencies**: libhashtable for optimized symbol management
- **Standards**: POSIX shell specification compliance target

### KEY METRICS
- **POSIX Compliance**: 85% achieved, 49/49 regression tests passing
- **Code Quality**: clang-format enforced, minimal compiler warnings
- **Performance**: 3-4x improvement in symbol table operations
- **Reliability**: Comprehensive test coverage with edge case validation

This reference document serves as the authoritative guide for maintaining
consistency with the established Lusush Shell development workflow and
quality standards. All development work should follow these patterns to
ensure continued project success and maintainability.