# Changelog

All notable changes to the Lusush Shell project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.3.1] - 2025-10-01

### Added - Configuration System Modernization
- **Complete shell options integration with config system**
  - All 24 POSIX shell options accessible via modern interface
  - Dual interface support: `config set shell.errexit true` and traditional `set -e`
  - Perfect bidirectional synchronization between both interfaces
  - Mutual exclusivity handling for emacs/vi editing modes

- **Standardized dot notation configuration naming**
  - Migrated all config options to consistent namespace.option format
  - Professional organization: `history.enabled`, `completion.fuzzy`, `prompt.style`
  - Logical functional grouping: `behavior.*`, `network.*`, `scripts.*`, `shell.*`
  - 50+ options reorganized with enterprise-appropriate naming conventions

- **Comprehensive backward compatibility system**
  - Legacy underscore names supported with helpful deprecation warnings
  - Complete mapping table for seamless migration
  - Zero breaking changes for existing configurations
  - Gradual adoption path for professional environments

- **Enterprise-grade configuration documentation**
  - New comprehensive CONFIG_SYSTEM.md with complete feature coverage
  - Shell options integration examples and migration guides
  - Enterprise deployment patterns and security configurations
  - Professional troubleshooting and best practices documentation

### Improved - Repository Organization and Documentation
- **Complete documentation refresh** with current date (October 1, 2025)
  - All documentation updated to reflect 1.3.0-dev feature set
  - Cross-references verified and updated across all documents
  - Professional presentation suitable for enterprise environments
  - Beginner to expert progression in all major documentation

- **Repository cleanup and professional organization**
  - Removed development artifacts from repository root
  - Moved important documentation to docs/ directory
  - Clean root directory with only essential project files
  - Professional project structure suitable for enterprise evaluation

### Technical Improvements
- **Enhanced config.c with shell option integration functions**
  - Comprehensive mapping system for legacy option names
  - Professional deprecation warnings with migration guidance
  - Extended config show/get/set commands for shell options
  - Complete POSIX compatibility preserved for traditional set commands

- **Documentation accuracy verification**
  - All code examples tested with current Lusush build
  - Verified configuration examples across all documentation
  - Consistent naming conventions applied throughout
  - Professional error handling and user experience improvements

## [1.3.0] - 2025-09-30

### Added - Comprehensive POSIX Options Suite (24 Major Options)
- **Complete POSIX shell option implementation** - Enterprise-grade shell behavior control:
  - **Core execution options**: `-e` (errexit), `-x` (xtrace), `-n` (noexec), `-u` (nounset), `-v` (verbose)
  - **File system options**: `-f` (noglob), `-h` (hashall), `-C` (noclobber), `-a` (allexport)
  - **Job control options**: `-m` (monitor), `-b` (notify), `-t` (onecmd)
  - **Interactive options**: `ignoreeof`, `nolog`, `emacs`, `vi`, `posix`
  - **Advanced options**: `pipefail`, `histexpand`, `history`, `interactive-comments`, `braceexpand`
  - **Security options**: `physical`, `privileged`

### Added - Advanced Directory Navigation
- **Physical option** (`set -o physical`): Complete POSIX-compliant directory path handling
  - Logical mode (default): Preserves symlinked paths as navigated
  - Physical mode: Resolves all symbolic links to real directory paths
  - PWD and OLDPWD management with proper symtable integration
  - Logical path canonicalization with `.` and `..` resolution
  - Compatible with bash/zsh behavior specifications

### Added - Enterprise Security Features
- **Privileged option** (`set -o privileged`): Comprehensive restricted shell security
  - Command execution restrictions: Block commands containing `/` (absolute/relative paths)
  - Built-in command controls: Restrict `cd`, `exec`, `set` in privileged mode
  - Environment variable protection: Block modifications to `PATH`, `IFS`, `ENV`, `SHELL`
  - File system access controls: Block absolute path and parent directory redirections
  - Shell option lockdown: Prevent configuration tampering in secure environments
  - Enterprise-ready security for sandboxed environments and multi-tenant systems

### Added - Professional Printf Implementation
- **Dynamic field width support**: Complete `%*s` and `%.*s` format specifiers
- **POSIX-compliant formatting**: Full compatibility with system printf behavior
- **Argument consumption**: Proper handling of width/precision arguments from parameter list
- **Enterprise-quality validation**: Comprehensive format type support and error handling

### Added - Advanced Redirection Features
- **Clobber override syntax**: Complete `>|` redirection for noclobber environments
- **Professional integration**: Seamless operation with shell options and redirection systems
- **POSIX compliance**: Full specification adherence with comprehensive error handling

### Added - Advanced Redirection and Pipeline Features
- **Clobber override syntax (`>|`)** - Complete noclobber environment support:
  - **Professional integration**: Seamless operation with existing redirection and shell option systems
  - **POSIX compliance**: Full specification adherence with comprehensive error handling
- **Pipeline failure detection (`-o pipefail`)** - Modern pipeline error handling:
  - **Professional implementation**: Pipeline failure detection with proper error propagation
  - **Enterprise functionality**: Robust error handling for complex pipeline operations

### Added - Professional Editing and History Management
- **Editing mode control** - Complete emacs/vi mode switching:
  - **Emacs mode**: Professional emacs-style command line editing with readline integration
  - **Vi mode**: Professional vi-style command line editing with mutual exclusivity
  - **Mode switching**: Runtime control with proper readline integration
- **History management enhancements** - Advanced history control:
  - **History expansion (`-o histexpand`)**: Enable history expansion (!! !n !string) with enterprise functionality
  - **History recording (`-o history`)**: Command history recording management with professional integration
  - **Function logging (`-o nolog`)**: Function definition history filtering with corruption fixes
  - **Interactive comments (`-o interactive-comments`)**: Enable # comments in interactive mode

### Added - POSIX Compliance and Strict Mode Features
- **Strict POSIX compliance mode (`-o posix`)** - Enterprise-grade strict mode:
  - **Function name validation**: Advanced function feature restrictions in strict mode
  - **Compliance behaviors**: Function name validation and history file selection
  - **Advanced feature restrictions**: Comprehensive behavior limitations for POSIX compliance
- **Brace expansion control (`-o braceexpand`)** - Complete brace expansion management:
  - **Pattern expansion**: Enable/disable brace expansion {a,b,c} with professional control
  - **Runtime configuration**: Dynamic control over expansion behavior

### Performance Enhancements
- **Sub-millisecond response times**: All POSIX options maintain optimal performance
- **Memory efficiency**: Enhanced buffer management with intelligent caching
- **Cross-platform compatibility**: Verified working on Linux, macOS, BSD systems
- **Zero regression policy**: All existing functionality preserved during enhancements

### Added - Comprehensive POSIX Shell Options Suite
- **Complete 24 major POSIX options** - Enterprise-grade shell behavior control:
  - **Short flag options**: `-a` (allexport), `-b` (notify), `-C` (noclobber), `-e` (errexit), `-f` (noglob), `-h` (hashall), `-m` (monitor), `-n` (noexec), `-t` (onecmd), `-u` (nounset), `-v` (verbose), `-x` (xtrace)
  - **Named options**: `ignoreeof`, `nolog`, `emacs`, `vi`, `posix`, `pipefail`, `histexpand`, `history`, `interactive-comments`, `braceexpand`, `physical`, `privileged`
  - **Professional implementation**: All options working seamlessly together with runtime control via `set -o`/`set +o`
  - **Zero regressions**: Complete preservation of existing functionality with enhanced capabilities

### Added - Directory Navigation and Security Features
- **Physical path navigation (`-o physical`)** - Enterprise directory security:
  - **Logical mode (default)**: Preserve symlinked paths with proper canonicalization of `.` and `..` components
  - **Physical mode**: Resolve all symbolic links for predictable, security-conscious navigation
  - **PWD/OLDPWD management**: Complete symtable integration with accurate path tracking
  - **POSIX compliance**: Full compatibility with bash/zsh behavior specifications

- **Restricted shell security (`-o privileged`)** - Professional security controls:
  - **Command execution restrictions**: Block commands containing `/` (absolute/relative paths)
  - **Built-in command controls**: Restrict dangerous commands (`cd`, `exec`, `set`) in privileged mode
  - **Environment variable protection**: Block modifications to `PATH`, `IFS`, `ENV`, `SHELL`
  - **File system access controls**: Prevent redirection to absolute paths and parent directories
  - **Enterprise-ready**: Suitable for sandboxed environments, containers, and multi-tenant systems

### Added - Advanced Shell Behavior Options
- **Exit behavior control**: 
  - **One command mode (`-t`)**: Exit after executing single command for automation
  - **Ignore EOF (`ignoreeof`)**: Interactive EOF handling with proper cleanup
  - **Async notification (`-b`)**: Background job completion notification
- **Development and debugging**:
  - **No function logging (`nolog`)**: Function definition history filtering with corruption prevention
  - **Interactive comments**: Enable `#` comments in interactive mode for better user experience

### Developer Experience
- **Comprehensive documentation**: All 24 POSIX options fully documented with examples
- **Professional error messages**: Clear, actionable error reporting for all restrictions
- **Enterprise debugging**: Enhanced debugging capabilities with option state visibility
- **Development velocity**: Maintained exceptional development momentum with proven "simple fixes first" pattern
- **Enhanced read builtin** - Complete POSIX option support:
  - `-p` (prompt): Display custom prompts before reading input
  - `-r` (raw mode): Disable backslash escape processing 
  - Framework for `-t` (timeout), `-n` (nchars), `-s` (silent) options
  - Professional error handling and usage messages
- **Enhanced test builtin** - Complete logical operator support:
  - `!` (negation): Full negation operator with proper precedence
  - `-a` (logical AND): Multi-expression AND operations
  - `-o` (logical OR): Multi-expression OR operations with correct precedence
  - Complex expression evaluation with recursive parsing
- **Enhanced type builtin** - Complete POSIX option compliance:
  - `-t` (type only): Output command type (builtin, file, function, alias)
  - `-p` (path only): Output executable path for external commands
  - `-a` (show all): Display all command locations including multiple PATH entries
  - Professional option parsing with -- terminator support

### Fixed - Critical Infrastructure Improvements
- **Tokenizer enhancement** - Fixed standalone `!` character parsing for test negation
- **Export system completion** - Added missing setenv() calls for proper environment synchronization
- **Option parsing consistency** - Standardized option handling across all enhanced built-ins

### Improved - Development and Quality
- **Professional error handling** - Comprehensive validation and user-friendly error messages
- **POSIX compliance advancement** - Significant progress toward complete POSIX shell compliance
- **Built-in command quality** - Enterprise-grade option support matching professional shells
- **Development methodology validation** - "Simple fixes first" approach proven effective across multiple enhancements

### Technical Details
- **Zero regressions** - All existing functionality preserved while adding new capabilities
- **Comprehensive testing** - Each enhancement thoroughly tested with edge cases and integration scenarios  
- **Professional documentation** - Complete usage messages and help text for all enhanced commands
- **Backward compatibility** - All changes maintain full compatibility with existing scripts and usage

## [1.2.5] - 2025-09-16

### Fixed
- **CRITICAL: Multiline parser issues completely resolved** - Fixed fundamental parser bugs that prevented production use
- **Multiline function definitions** - Functions with newlines now work correctly via stdin/pipe input
- **Multiline case statements** - Complex case statements with multiple patterns and commands now work
- **Here document support** - Complete implementation of << and <<- syntax with variable expansion
- **Function persistence** - Functions now persist correctly across commands using global executor
- **Context tracking** - Input system properly tracks compound command depth and multiline states

### Added  
- **Enhanced function parameter system** - Functions support `function name(param1, param2="default")` syntax
- **Advanced return values** - `return_value "string"` system for capturing function outputs
- **Function introspection** - `debug functions` and `debug function <name>` commands working
- **Comprehensive debugging** - Full debug command suite with breakpoints, tracing, profiling
- **Multiline input accumulation** - Proper handling of complete constructs via stdin/pipe
- **Here document detection** - Smart delimiter matching with whitespace handling

### Improved
- **Shell compliance** - Improved from 70% to 85% (134/136 tests passing)
- **Test success rate** - Achieved 98.5% success rate on comprehensive tests
- **POSIX compatibility** - Maintained 85% comprehensive compliance (134/136 tests passing), 100% regression test success (49/49)
- **Documentation accuracy** - Complete overhaul with tested examples and verified capabilities
- **Cross-platform support** - Verified functionality on Linux, macOS, BSD systems

## [1.2.4] - 2025-09-15

### Fixed
- **Documentation accuracy restored** - All examples now tested and verified working

### Changed
- **README.md complete overhaul** - Professional documentation reflecting actual capabilities
- **Function examples updated** - All function syntax examples tested and verified
- **Capability claims accurate** - All percentage claims backed by test results
- **Marketing tone professional** - Removed hyperbole, focus on demonstrated capabilities

## [1.2.2] - 2025-09-11

### Added
- **Complete repositioning as "The Shell Development Environment"**
- **Advanced Scripting Guide** - Comprehensive 1000+ line guide for function systems and debugging
- **Configuration Mastery Guide** - Complete 800+ line enterprise configuration reference
- **Professional README.md** - Clear positioning emphasizing debugging, functions, and configuration

### Changed
- **Repository cleanup** - Removed all legacy test scripts, validation files, and outdated documentation
- **Clear project identity** - Established as the premier shell development environment
- **Documentation focus** - Streamlined to emphasize unique development capabilities

### Fixed
- **License consistency** - All files now correctly reference GPL-3.0+ license
- **Copyright consistency** - All files now properly attribute copyright to Michael Berry
- **Version consistency** - Updated all version references to 1.2.2
- **Version output** - Fixed `--version` and `-V` flags to show correct license and copyright
- **Author attribution** - Replaced "Lusush Development Team" references with "Michael Berry"
- **Theme authorship** - All built-in themes now properly attributed to Michael Berry
- **Configuration syntax** - Corrected all documentation to use actual config commands
- **Configuration options** - Updated guides to reflect real available configuration keys
- **Command examples** - Fixed `config set key value` syntax throughout documentation
- **Traditional configuration** - Added comprehensive documentation of shell script configuration methods
- **Development disclaimers** - Added important notes about active development and potential changes
- **Configuration documentation** - Fixed all config command syntax to match actual implementation
- **Development status notes** - Added important disclaimers about active development status

### Removed
- All legacy .sh test and validation scripts (14+ files)
- Outdated development documentation (PHASE*, LINUX*, PRODUCTION* files)
- Legacy directories (archive/, readline/, research/, scripts/, tools/)
- Development artifacts and assessment documents
- Redundant documentation in docs/ directory
- Incorrect MIT license references throughout codebase - RESOLVED: Updated to GPL-3.0+

## [Unreleased]
## [1.2.1] - 2025-01-16 (Enhanced)

### Added (Original Release)
- **Fish-like Autosuggestions** - Real-time command suggestions based on history with professional behavior
- **Robust Syntax Highlighting with Line Wrapping Support** - Complete implementation with universal length support
- **Terminal Dimension Detection** - Automatic terminal width/height detection for proper wrapping calculations
- **Multi-line Display Management** - Intelligent clearing and redrawing of wrapped command lines
- **Enhanced Buffer Management** - 256-character word buffers for handling long tokens safely
- **Professional Visual Experience** - Enterprise-grade syntax highlighting without display corruption

### Enhanced (January 2025 Improvements)
- **Autosuggestion Clearing on Continuation Prompts** - Fixed autosuggestions not clearing when transitioning to multiline constructs (`loop>`, `if>`, etc.)
- **History Navigation Artifacts** - Eliminated grey autosuggestion remnants during UP/DOWN arrow history navigation
- **UP Arrow Key Binding** - Properly bound UP arrow (`\e[A`) to custom history function with autosuggestion clearing
- **Enhanced Dismiss Function** - Added `lusush_dismiss_suggestion_with_display()` for proper state and visual clearing
- **Architectural Improvements** - Clean separation between autosuggestion logic and display handling
- **Professional Navigation Experience** - Smooth, artifact-free autosuggestion behavior in all scenarios

### Improved
- **Removed All Length-Based Safety Restrictions** - No more artificial limits on command length
- **Enhanced Token Recognition** - Commands, keywords, strings, variables, operators of any length
- **Cursor Position Mathematics** - Accurate positioning for wrapped lines and multi-line scenarios
- **Enhanced Key Bindings** - Both UP and DOWN arrow keys now use enhanced clearing functions
- **Zero Regressions** - All existing functionality preserved while fixing edge cases
- **ANSI Escape Sequence Mastery** - Proper save/restore cursor, clear lines, move cursor operations
- **Performance Optimization** - Sub-millisecond highlighting with intelligent wrapping calculations

## [1.2.2] - 2025-01-16

### Enhanced
- **Autosuggestion System** - Improved clearing behavior and navigation experience
- **History Navigation Artifacts** - Eliminated grey autosuggestion remnants during UP/DOWN arrow history navigation
- **Enhanced Key Bindings** - Proper UP/DOWN arrow key handling with autosuggestion clearing
- **Professional Navigation** - Smooth, artifact-free autosuggestion behavior in all scenarios
- **Display Integration** - Removed unnecessary startup messages for cleaner user experience

### Fixed
- **Autosuggestion Clearing on Continuation Prompts** - Fixed autosuggestions not clearing when transitioning to multiline constructs (`loop>`, `if>`, etc.)
- **UP Arrow Key Binding** - Properly bound UP arrow (`\e[A`) to custom history function with autosuggestion clearing
- **Enhanced Dismiss Function** - Added proper state and visual clearing for autosuggestions
- **Startup Messages** - Removed verbose enhanced display mode messages

### Technical Improvements
- **Clean Architecture** - Improved separation between autosuggestion logic and display handling
- **Version Consistency** - Updated all version references to 1.2.2
- **Code Quality** - Enhanced error handling and state management

### Fixed
- **70-character cursor position limit** - Removed, now handles unlimited cursor positions
- **50-character string limit** - Removed, strings of any length now highlighted
- **32-character word limit** - Removed, commands and tokens of any length supported
- **20-character string wrapping limit** - Removed, proper line wrapping for all string lengths
- **15-character variable limit** - Removed, variables of any length now highlighted
- **12-character word wrapping limit** - Removed, words of any length properly wrapped and highlighted
- **Complex construct blocking** - `for`, `while`, `if`, `case` statements now fully highlighted

### Technical Achievements
- **Universal Terminal Compatibility** - Works with any terminal width without corruption
- **Memory Safety** - Enhanced buffer management prevents overflows with long tokens
- **Display Stability** - Zero corruption or positioning issues with wrapped lines
- **Cross-platform Consistency** - Reliable behavior across all Unix-like systems

## [1.2.0] - 2025-01-10

### Added
- **Real-time syntax highlighting** - Comprehensive word-boundary triggered highlighting
- **Enhanced syntax detection** - Keywords, built-ins, variables, strings, numbers, operators
- **Professional color scheme** - Enterprise-appropriate visual design
- **Unix rub out highlighting technique** - Clean cursor management without display corruption
- **Comprehensive word boundary detection** - Uses existing lusush helper functions
- **Enhanced display mode** - `--enhanced-display` flag for full syntax highlighting
- **String literal highlighting** - Support for both single and double quotes
- **Variable highlighting** - `$VAR` and `${VAR}` syntax detection
- **Number highlighting** - Integer and decimal number detection
- **Operator highlighting** - Pipes, redirections, and command separators

### Improved
- **Performance optimization** - Sub-millisecond syntax highlighting response
- **Memory management** - Zero memory leaks with comprehensive resource handling
- **Error handling** - Robust fallback mechanisms for all display operations
- **Cross-platform compatibility** - Verified on Linux, macOS, and BSD systems
- **Documentation** - Comprehensive user and developer documentation

### Changed
- **Removed Ferrari engine branding** - Replaced with professional terminology
- **Enhanced readline integration** - Improved custom getc function for word boundaries
- **Syntax detection architecture** - Leverages existing lusush detection functions

### Technical Details
- Uses `lusush_is_shell_keyword()` for keyword detection
- Uses `lusush_is_shell_builtin()` for built-in command detection  
- Uses `lusush_is_word_separator()` for proper word boundary detection
- Implements ANSI cursor save/restore for clean highlighting
- Maintains full POSIX compliance and existing functionality

### Testing
- All regression tests pass (8/8)
- Zero crashes or memory issues detected
- Performance benchmarks maintained
- Cross-platform compatibility verified

## [1.1.3] - 2025-01-10

### Added
- **Display integration system** - Layered display architecture
- **Enhanced display modes** - Advanced prompt and display management
- **Git integration improvements** - Real-time branch and status display
- **Professional themes** - 6 enterprise-grade theme options
- **Advanced tab completion** - Context-aware completion system

### Improved
- **Stability enhancements** - Resolved critical display issues
- **Memory safety** - Comprehensive leak prevention
- **Performance optimization** - Sub-millisecond response times
- **Error handling** - Professional error reporting

### Fixed
- **Display corruption issues** - Clean terminal output
- **Memory leaks** - Comprehensive resource management
- **Recursion protection** - Safe function call handling

## [1.1.2] - 2024-12-15

### Added
- **Advanced tab completion** - Git-aware and context-sensitive
- **SSH host completion** - Integration with SSH configuration
- **Directory-only completion** - Smart completion for cd command
- **Completion performance optimization** - Faster completion responses

### Improved
- **Tab completion reliability** - More robust completion system
- **Completion display** - Better formatting and presentation
- **Performance** - Optimized completion algorithms

## [1.1.1] - 2024-12-01

### Added
- **Professional theme system** - Multiple enterprise-grade themes
- **Git branch integration** - Real-time git status in prompts
- **Theme switching** - Dynamic theme changes with `theme` command
- **Multi-line prompt support** - Complex prompt layouts

### Improved
- **Prompt generation** - More robust and flexible prompt system
- **Color management** - Professional color schemes
- **Git integration** - Better git status detection and display

### Fixed
- **Prompt display issues** - Clean multi-line prompt rendering
- **Color handling** - Consistent color application

## [1.1.0] - 2024-11-15

### Added
- **GNU Readline integration** - Full readline support with history
- **History management** - Persistent command history with deduplication
- **Key bindings** - Standard shell key bindings and shortcuts
- **Interactive mode improvements** - Enhanced user experience

### Improved
- **Line editing** - Full readline editing capabilities
- **History navigation** - Arrow keys and search functionality
- **Input handling** - More responsive command input

### Fixed
- **Memory management** - Proper cleanup and resource handling
- **Stability issues** - Resolved crashes and error conditions

## [1.0.0] - 2024-10-01

### Added
- **Initial stable release** - Complete shell functionality
- **POSIX compliance** - Full standard shell operations
- **Built-in commands** - Essential shell built-ins implemented
- **Command execution** - Reliable command processing
- **Multiline support** - Complex shell constructs (if, for, while, case)
- **Job control** - Background and foreground process management
- **Variable management** - Shell variable operations
- **Configuration system** - Customizable shell behavior

### Core Features
- **Command parsing** - Robust command line parsing
- **Process management** - Process creation and control
- **I/O redirection** - Standard input/output redirection
- **Pipe support** - Command pipelining
- **Signal handling** - Proper signal management
- **Error handling** - Comprehensive error reporting

### Build System
- **Meson build system** - Modern build configuration
- **Cross-platform support** - Linux, macOS, BSD compatibility
- **Package management** - Easy installation and deployment

---

## Release Notes

### Version 1.2.0 Highlights

This release represents a major advancement in shell user experience with the addition of comprehensive real-time syntax highlighting. Key achievements:

- **Professional syntax highlighting** that rivals modern IDEs
- **Zero performance impact** on shell operations
- **Enterprise-ready visual design** appropriate for business environments
- **Comprehensive syntax support** covering all major shell elements
- **Rock-solid stability** with extensive testing and validation

### Upgrade Notes

- All existing configurations remain compatible
- New `--enhanced-display` flag enables full syntax highlighting
- Standard mode behavior unchanged for backward compatibility
- All themes work with new syntax highlighting features

### Development Focus

Version 1.2.0 focuses on:
- User experience improvements
- Visual feedback and modern interface design
- Performance optimization
- Production-ready stability
- Professional appearance for enterprise use

---

## Contributing

See [CONTRIBUTING.md] for information on how to contribute to Lusush Shell.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.