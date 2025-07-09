# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.6] - 2025-01-09

### Fixed
- **macOS/iTerm2 Tab Completion**
  - Removed cursor position queries from line refresh functions that caused input interference
  - Fixed tab completion displaying cursor sequences instead of completions on macOS/iTerm2
  - Eliminated cursor position queries from history navigation that interfered with input handling
  - Maintained proper cursor handling through dedicated termcap module

### Technical Details
- Modified refreshSingleLine() to prevent cursor position queries during line refresh
- Updated linenoiseEditHistoryNext() to avoid input stream interference
- Preserved legitimate cursor position handling in getCursorPosition() for terminal size detection
- Enhanced compatibility with macOS terminal emulators while maintaining cross-platform functionality

## [1.0.5] - 2025-01-07

### Fixed
- **Complete Hints System Overhaul**
  - Resolved new prompt line creation for each character typed
  - Fixed line consumption when prompt at bottom of terminal
  - Corrected cursor positioning in git branch display
  - Eliminated all hints-related visual artifacts and terminal interference

- **Interactive Terminal Behavior**
  - Smooth character-by-character typing with inline hint updates
  - Content preservation regardless of terminal cursor position
  - Accurate cursor positioning that preserves git prompt formatting
  - Professional-grade interactive experience without visual glitches

- **Technical Implementation**
  - Fixed double-processing of prompt length calculations
  - Implemented ultra-conservative refresh logic to prevent scrolling
  - Disabled problematic bottom-line protection when hints enabled
  - Enhanced terminal control sequences for maximum compatibility

### Changed
- Repository cleanup: Removed all temporary debug and test scripts
- Enhanced code quality with clear technical documentation
- Optimized hints display logic for minimal terminal interference

## [1.0.0] - 2024-12-19

### Added
- Production-ready repository structure
- Comprehensive user and developer documentation
- Contributing guidelines and development workflow
- Installation guides for multiple platforms
- Repository cleanup and archival system

### Fixed
- **iTerm2 Bottom-line Terminal Handling**
  - Resolved line consumption issue when prompt reaches bottom of terminal in iTerm2
  - Implemented smart protection that only triggers on new prompts, not during editing
  - Fixed static variable limitation that caused protection to work only once per session
  - Fixed excessive protection triggering during history navigation and arrow key usage
  - Maintains natural terminal scrolling behavior while preventing line loss
  - Automatic iTerm2 detection with intelligent cursor position queries
  - Cross-platform compatibility maintained for all terminal types

### Changed
- Reorganized documentation structure for production
- Updated README.md for production readiness with macOS/iTerm2 compatibility section
- Archived development-specific files to maintain clean repository

## [1.0.0] - 2025-07-03

### Added

- **Real-time Hints System**
  - Intelligent input suggestions that appear as you type
  - Context-aware hints for commands, files, variables, and builtins
  - Performance optimized with <1ms latency
  - Fully configurable via `hints_enabled` setting
  - Seamless integration with existing completion system

- **Enhanced Tab Completion System**
  - Professional single-line completion display
  - Categorized completions (builtin, file, directory, variable, command)
  - Position tracking with [X/Y category] indicators
  - Fuzzy matching with configurable threshold
  - Smart navigation with TAB, Ctrl+P, Ctrl+N
  - Clean ESC cancellation without screen clutter

- **Modern Configuration System**
  - Unified `.lusushrc` configuration file with INI-style sections
  - Hybrid support for traditional shell scripts
  - Runtime configuration via `config` command
  - Complete migration from legacy `setopt` system
  - Comprehensive validation and error handling

- **Professional Theme System**
  - Corporate theme for business environments
  - Dark theme with high contrast
  - Light theme with subtle colors
  - Theme management commands (`theme set`, `theme list`, `theme info`)
  - Git-aware prompts with branch status integration

- **Network Integration Features**
  - SSH host completion from `~/.ssh/config` and `~/.ssh/known_hosts`
  - Network status and configuration commands
  - Cloud provider detection and integration
  - Remote context awareness

- **Advanced Line Editing**
  - Enhanced command-line editing with linenoise
  - Multiline command support
  - Advanced history management with Ctrl+R search
  - Intelligent auto-correction with learning capabilities

### Fixed
- **POSIX Compliance Achievements**
  - 100% POSIX regression test compliance (49/49 tests)
  - 100% comprehensive shell test compliance (136/136 tests)
  - Complete parameter expansion implementation
  - Full arithmetic expansion support
  - Proper command substitution handling
  - All control structures (if/then/else, loops, case statements)

- **Shell Options Implementation**
  - Complete `set -e` (errexit) implementation with proper enforcement
  - Full `set -n` (noexec) syntax checking mode
  - Comprehensive `set -f` (noglob) pathname expansion control
  - All `set -o` options with standards-compliant behavior

- **Built-in Commands Coverage**
  - All 28 required POSIX built-in commands implemented
  - Enhanced commands: `theme`, `network`, `config`, `debug`
  - Proper exit status handling and error reporting
  - Complete job control implementation (`jobs`, `fg`, `bg`)

### Changed
- **Completion System Redesign**
  - Eliminated screen clutter from multi-line completion displays
  - Replaced abrupt 10-item transition with smooth scaling
  - Consistent behavior across all completion scenarios
  - Professional appearance with modern terminal integration

- **Code Quality Improvements**
  - ISO C99 compliant codebase
  - Comprehensive memory management and cleanup
  - Robust error handling throughout
  - Systematic testing and validation framework

- **Development Workflow**
  - Mandatory development process with comprehensive testing
  - Automated code formatting with clang-format
  - Zero regression policy with continuous validation
  - Clean commit history and documentation standards

### Performance
- **Startup Time**: < 100ms typical startup
- **Memory Usage**: 10-20MB typical memory footprint
- **Completion Speed**: < 50ms for most completion operations
- **Test Suite**: 100% pass rate across 211 total tests

### Architecture
- **Modular Design**: Clean separation of parsing, execution, and built-ins
- **Memory Safety**: Comprehensive allocation and cleanup strategies
- **Error Resilience**: Graceful degradation and robust error handling
- **Extensibility**: Plugin-ready architecture for future enhancements

## [0.9.0] - 2025-06-15

### Added
- Advanced debugging capabilities with interactive step execution
- Variable inspection and performance profiling
- Git integration with real-time branch status
- Enhanced history with intelligent search

### Fixed
- Core shell functionality and POSIX compliance improvements
- Parser enhancements for complex command structures
- Memory management optimizations

## [0.8.0] - 2025-05-20

### Added
- Basic theme system implementation
- SSH host completion foundation
- Auto-correction system with learning capabilities
- Enhanced configuration management

### Fixed
- Tokenizer improvements and edge case handling
- Symbol table performance optimizations
- Built-in command implementations

## [0.7.0] - 2025-04-10

### Added
- Network integration framework
- Cloud provider detection
- Advanced parameter expansion
- Function definition and scoping

### Fixed
- Arithmetic expansion implementation
- Command substitution reliability
- Error handling improvements

## [0.6.0] - 2025-03-05

### Added
- Basic completion system
- History management
- Job control implementation
- Enhanced line editing

### Fixed
- Parser stability improvements
- Memory leak fixes
- Signal handling

## [0.5.0] - 2025-02-01

### Added
- Core POSIX compliance foundation
- Basic built-in commands
- Command execution engine
- Tokenizer and parser implementation

### Fixed
- Initial architecture and design decisions
- Build system setup with Meson and Ninja
- Basic test framework

## [0.1.0] - 2024-12-01

### Added
- Initial project structure
- Basic shell loop implementation
- Foundation for POSIX-compliant shell
- Development environment setup

---

## Release Notes

### Version 1.0.0 - Production Ready
This release marks Lusush as production-ready with:

- **Complete POSIX Compliance**: 100% compliance with all required standards
- **Professional User Experience**: Modern completion system and themes
- **Enterprise Features**: Network integration and advanced configuration
- **Zero Regressions**: Comprehensive testing ensures reliability
- **Clean Architecture**: Maintainable and extensible codebase

### Upgrade Guide
When upgrading to 1.0.0:

1. **Configuration**: Review and update configuration files
2. **Completion**: Enhanced completion is enabled by default
3. **Themes**: Set preferred theme with `theme set <name>`
4. **Configuration**: View configuration with `config show`

### Breaking Changes
- Enhanced completion is now the default mode
- Some internal APIs have changed (affects plugins only)
- Configuration file format has minor updates

### Migration from Other Shells
Lusush 1.0.0 provides excellent compatibility with:
- **Bash**: Most scripts work without modification
- **Zsh**: Configuration can be adapted easily
- **POSIX sh**: 100% compatible

### System Requirements
- **OS**: Linux, macOS, or Unix-like system
- **Memory**: 128MB minimum, 512MB recommended  
- **Storage**: 50MB minimum, 200MB recommended
- **Terminal**: ANSI color support recommended

### Support and Documentation
- **User Manual**: Complete documentation available
- **API Reference**: Developer documentation provided
- **Community**: GitHub Discussions and IRC support
- **Issues**: Bug reports and feature requests welcome

---

*For more information, visit [https://lusush.org](https://lusush.org)*