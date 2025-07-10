# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.10] - 2025-01-10

### Fixed
- **Cursor Movement in Wrapped Lines (Critical Fix)**
  - Fixed cursor navigation issues with long commands that wrap to multiple lines
  - Resolved issue where Ctrl+A (home) and other cursor movements didn't work properly across wrapped lines
  - Enhanced cursor positioning logic in linenoise to handle multiline scenarios correctly
  - Added configurable multiline mode support with `multiline_mode` setting

### Added
- **Multiline Mode Configuration**
  - New `multiline_mode` configuration option in behavior settings
  - Configurable via `config set multiline_mode true/false`
  - Default enabled for better handling of long wrapped commands
  - Proper cursor movement across visual line boundaries

### Technical Details
- Enhanced cursor movement functions (moveLeft, moveRight, moveHome, moveEnd) to support multiline mode
- Improved single-line mode with full refresh for wrapped lines
- Added multiline_mode to configuration system with proper validation
- Applied multiline settings during shell initialization and configuration changes

### Code Quality
- Better separation between single-line and multiline cursor handling
- Improved terminal interaction for complex cursor positioning scenarios
- Enhanced configuration system extensibility

## [1.0.9] - 2025-01-10

### Fixed
- **Critical Multiline History Conversion Bug Fix**
  - Fixed missing semicolons between consecutive commands in multiline loop bodies
  - Resolved issue where `i=2\nwhile [ $i -le 5 ]\ndo\necho "$i"\ni=$(($i+1))\ndone` was incorrectly converted to `while [ $i -le 5 ]; do; echo "$i" i=$(($i+1)); done` instead of `i=2; while [ $i -le 5 ]; do; echo "$i"; i=$(($i+1)); done`
  - Added proper semicolon placement between regular commands and control structure starts
  - All multiline constructs now convert correctly to valid single-line history entries

### Technical Details
- Enhanced convert_multiline_for_history() function with additional semicolon placement rules
- Added semicolon insertion between regular commands and control structure starts
- Added semicolon insertion between control structure starts and regular commands
- Comprehensive test coverage for all semicolon placement scenarios
- Maintains backward compatibility with existing history conversion logic

### Code Quality
- Improved helper function categorization for line types
- Better separation of concerns in multiline conversion logic
- Enhanced error handling and edge case coverage
- Comprehensive testing validates all major shell construct patterns

## [1.0.8] - 2025-01-10

### Fixed
- **Multiline Construct History Conversion (Critical Fix)**
  - Fixed multiline shell constructs being incorrectly converted to invalid single-line history entries
  - Resolved "Expected DONE but got EOF" errors when recalling multiline commands from history
  - Proper semicolon placement in converted commands for valid shell syntax
  - Enhanced convert_multiline_for_history() function with intelligent parsing logic

- **Shell Construct Syntax Preservation**
  - For loops: "for i in 1 2 3; do; echo $i; done" (proper semicolon placement)
  - While loops: "while read line; do; echo $line; done" (correct control flow)
  - If statements: "if test; then; command; fi" (valid conditional syntax)
  - Complex nested constructs handled with proper delimiter placement
  - History recall now produces executable commands that work correctly

### Technical Details
- Implemented two-pass algorithm for multiline-to-single-line conversion
- Semicolons added before control keywords (do, then, else, elif) when needed
- Semicolons placed after control keywords when followed by commands
- Semicolons inserted before terminators (done, fi) when preceded by commands
- Enhanced line parsing with proper whitespace handling and empty line skipping
- Maintains compatibility with LUSUSH parser requirements for control structures

### Code Quality
- Comprehensive testing of all major shell construct types
- Validation scripts confirming proper conversion and execution
- Improved error handling in history conversion functions
- Better memory management in multiline processing logic

## [1.0.7] - 2025-01-10

### Added
- **Complete Termcap Integration System**
  - Full terminal capability detection and adaptation system
  - Enhanced terminal awareness across all shell components
  - Professional enterprise-grade terminal environment capabilities
  - Cross-platform terminal compatibility (Linux, macOS, BSD)

- **Responsive Template Engine**
  - Dynamic template rendering based on terminal size and capabilities
  - Automatic prompt simplification for narrow terminals
  - Terminal width-aware layouts and spacing adjustments
  - Real-time terminal state updates in template variables

- **Enhanced Line Editing Integration**
  - Improved linenoise integration with termcap for better terminal handling
  - Enhanced cursor positioning with terminal capability detection
  - Better column detection using termcap terminal information
  - Safer terminal operations with timeout protection and fallbacks

- **Advanced Completion System**
  - Terminal-aware completion positioning and display
  - Responsive completion menus that adapt to terminal width
  - Multi-column completion layouts for wide terminals
  - Simplified completion display for narrow terminals
  - Enhanced completion menu with terminal capability detection

- **Corporate Branding and Enterprise Features**
  - Professional startup branding with terminal size awareness
  - Centered company logos and information display
  - Environment-specific indicators (dev/staging/prod)
  - Department and project template variables
  - Professional color schemes with terminal-aware rendering

- **Dynamic Template Variables**
  - Real-time terminal capability variables (cols, rows, terminal, has_colors)
  - Platform detection variables (iterm2, tmux, screen)
  - Dynamic variable updates with current terminal state
  - Enhanced template context with terminal information

### Enhanced
- **Theme System Integration**
  - Terminal capability detection in theme initialization
  - Platform-specific theme optimizations for iTerm2, tmux, screen
  - Color support detection using termcap information
  - Responsive theme rendering based on terminal capabilities

- **Prompt System Improvements**
  - Terminal-aware prompt generation with capability detection
  - Color support fallbacks for terminals without color capabilities
  - Responsive prompt formatting based on terminal type
  - Enhanced prompt rendering with termcap integration

- **Shell Initialization**
  - Termcap initialization integrated into main shell startup
  - Proper terminal capability detection during shell initialization
  - Safe bottom margin creation for interactive shells
  - Enhanced terminal compatibility across different environments

### Technical Implementation
- **Core Architecture**
  - Termcap system integrated throughout shell components
  - Enhanced terminal information structure with comprehensive capabilities
  - Platform detection functions for specialized terminal handling
  - Safe terminal operations with proper error handling and timeouts

- **Memory Management**
  - Proper termcap cleanup on shell exit through signal handlers
  - Enhanced memory management in template variable systems
  - Dynamic variable allocation and cleanup in template contexts
  - Improved string buffer management with terminal-aware sizing

- **Cross-Platform Compatibility**
  - Enhanced macOS and iTerm2 support with specialized optimizations
  - Improved tmux and screen multiplexer compatibility
  - Better terminal emulator detection and adaptation
  - Consistent behavior across different Unix-like platforms

### Code Quality
- **Professional Implementation**
  - Clean separation of termcap functionality from core shell logic
  - Comprehensive error handling and graceful fallbacks
  - Extensive documentation of terminal capability features
  - Maintainable code structure with clear integration points

- **Performance Optimization**
  - Minimal performance overhead from termcap integration
  - Efficient terminal capability caching and reuse
  - Optimized template processing with responsive adjustments
  - Fast terminal detection with proper timeout handling

- **Reliability Improvements**
  - Enhanced error handling in terminal operations
  - Graceful degradation for limited terminal capabilities
  - Proper cleanup and resource management
  - Robust terminal state management throughout shell lifetime

## [1.0.6] - 2025-01-10

### Fixed
- **SSH Completion Bug (Critical Fix)**
  - Fixed SSH command disappearing during tab completion
  - Resolved issue where "ssh " + TAB would show only hostname instead of "ssh hostname"
  - Applied fix to all network commands: ssh, scp, rsync, sftp
  - Preserved command context during completion operations
  - Enhanced user experience for remote development workflows

- **macOS/iTerm2 Tab Completion**
  - Removed cursor position queries from line refresh functions that caused input interference
  - Fixed tab completion displaying cursor sequences instead of completions on macOS/iTerm2
  - Eliminated cursor position queries from history navigation that interfered with input handling
  - Maintained proper cursor handling through dedicated termcap module

### Technical Details
- Added complete_network_command_args_with_context() function in network completion system
- Modified completion callback to generate complete command lines instead of partial completions
- Implemented context-aware completion that preserves prefix and suffix around current word
- Enhanced memory management in completion string allocation and cleanup
- Modified refreshSingleLine() to prevent cursor position queries during line refresh
- Updated linenoiseEditHistoryNext() to avoid input stream interference
- Preserved legitimate cursor position handling in getCursorPosition() for terminal size detection
- Enhanced compatibility with macOS terminal emulators while maintaining cross-platform functionality

### Code Quality
- Comprehensive bug documentation and analysis
- Improved error handling in completion system
- Memory leak prevention in completion functions
- Enhanced string buffer management and bounds checking

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