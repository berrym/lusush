# Lusush Shell

A POSIX-compliant shell with modern architecture. Version 0.7.0-dev

Lusush is a functional shell implementing core POSIX shell features with a modern, modular architecture. The shell successfully handles basic commands, variable operations, pipeline execution, and has partial control structure support.

## Current Status

**Working Features:**
- ✅ **Simple Commands**: Full execution of basic shell commands
- ✅ **Variable Assignment and Expansion**: Complete variable handling including arithmetic expansion  
- ✅ **Complete Modern Parameter Expansion**: Full POSIX parameter expansion specification (94% test success)
  - Default values: `${var:-default}`, `${var-default}`
  - Alternative values: `${var:+alternative}`, `${var+alternative}`
  - Length expansion: `${#var}`
  - Substring expansion: `${var:offset:length}`
  - Pattern matching: `${var#pattern}`, `${var##pattern}`, `${var%pattern}`, `${var%%pattern}`
  - Case conversion: `${var^}`, `${var,}`, `${var^^}`, `${var,,}`
- ✅ **Case Statements**: Complete POSIX-compliant case statement implementation (100% test success)
  - Exact pattern matching: `case word in pattern) commands ;; esac`
  - Wildcard patterns: `*` (any string), `?` (any character)
  - Multiple patterns: `pattern1|pattern2|pattern3) commands ;;`
  - Variable expansion in test words: `case $var in pattern) ...`
  - Variable expansion in patterns: `case word in $pattern) ...`
  - Multiple commands per case: `case word in pattern) cmd1; cmd2; cmd3 ;; esac`
- ✅ **Command Substitution**: Both modern `$(command)` and legacy backtick syntax
- ✅ **Logical Operators**: Full support for `&&` and `||` conditional execution
- ✅ **Quoted String Variable Expansion**: Full support for "$var" and "${var}" in double quotes
- ✅ **Pipeline Execution**: Multi-command pipelines work reliably
- ✅ **String Processing**: Proper distinction between literal ('...') and expandable ("...") strings
- ✅ **Modern Symbol Table**: POSIX-compliant variable scoping with complete integration
- ✅ **Control Structures**: FOR/WHILE loops, IF statements, and CASE statements with proper variable scoping

**Next Priorities:**
- 🔄 **Function Definitions**: POSIX-compliant function definition and calling
- 🔄 **Here Documents**: `<<` and `<<-` here document functionality
- 🔄 **Advanced I/O Redirection**: Complete redirection operators and file descriptor manipulation

## Quick Start

```bash
# Build
meson setup builddir
meson compile -C builddir

# Run shell
./builddir/lusush

# Test functionality
echo 'x=42; echo "Value: $x"' | ./builddir/lusush
echo 'echo hello | grep h' | ./builddir/lusush
echo 'result=$((5+1)); echo $result' | ./builddir/lusush
```

## Architecture

The shell uses a modern four-component architecture:
- **Tokenizer**: POSIX-compliant lexical analysis
- **Parser**: Recursive descent parser building AST
- **Executor**: Clean execution engine for parsed commands
- **Symbol Table**: POSIX-compliant variable scoping with proper isolation

This design provides clear separation of concerns, enables robust error handling, and supports proper variable scoping for nested contexts (loops, functions, subshells).
## Documentation

**Active Documentation:**
- **[Project Status](PROJECT_STATUS_CURRENT.md)** - Detailed technical status and capabilities
- **[Development Next Steps](NEXT_STEPS_CURRENT.md)** - Current priorities and roadmap  
- **[Documentation Index](docs/DOCUMENTATION_INDEX.md)** - Complete documentation overview

## Features

### Core Functionality
- POSIX-compliant command parsing with robust error handling
- Variable expansion with quote handling and arithmetic support
- Command substitution (both `$()` and backtick syntax)
- Pipeline processing with proper I/O redirection
- Built-in commands (echo, export, test, set, etc.)
- Interactive command line editing with history

### Command Examples
**Command Examples:**

**Basic Operations:**
```bash
# Variable assignment and expansion
name=world
echo "Hello $name"

# Arithmetic expansion  
result=$((5 + 3))
echo "Result: $result"

# Command substitution
date_str=$(date '+%Y-%m-%d')
echo "Today is $date_str"
```

**Case Statements:**
```bash
# File type detection
case "$filename" in
    *.txt) echo "Text file" ;;
    *.jpg|*.png) echo "Image file" ;;
    *) echo "Unknown type" ;;
esac

# User input validation
case "$answer" in
    y|yes|Y|YES) echo "Confirmed" ;;
    n|no|N|NO) echo "Cancelled" ;;
    *) echo "Invalid input" ;;
esac

# System detection
case "$(uname)" in
    Linux) echo "Linux system" ;;
    Darwin) echo "macOS system" ;;
    *) echo "Other system" ;;
esac
```

**Pipelines:**
```bash
echo "test data" | grep "test" | wc -l
ls -la | sort | head -10
```

**Advanced Parameter Expansion:**
```bash
# Default values
echo "${USER:-guest}"              # Use 'guest' if USER unset
echo "${CONFIG:-config.txt}"       # Default configuration file

# Alternative values  
echo "${DEBUG:+--verbose}"         # Add flag only if DEBUG set
echo "${name:+Hello $name}"        # Conditional greeting

# String operations
echo "${#password}"                # Get string length
echo "${filename:0:8}"             # Extract first 8 characters
echo "${path:5}"                   # Everything from position 5

# Pattern matching
echo "${filename%.*}"              # Remove file extension
echo "${path##*/}"                 # Get basename from path
echo "${url#*://}"                 # Remove protocol from URL
echo "${version%%.*}"              # Get major version number

# Case conversion
echo "${name^}"                    # Capitalize first character
echo "${text,,}"                   # Convert to all lowercase
echo "${const^^}"                  # Convert to all uppercase
echo "${word,}"                    # Lowercase first character

# Unset vs empty distinction
empty=""
echo "${empty-default}"            # "" (empty but set)
echo "${empty:-default}"           # "default" (empty)
echo "${empty+set}"                # "set" (variable exists)
```

## Building and Installation

Lusush uses the Meson build system:

```bash
# Setup build directory
meson setup builddir

# Compile
meson compile -C builddir

# Optional: Install
meson install -C builddir
```

## Testing

Run the comprehensive test suite:

```bash
# Run integration tests
./test_modern_integration.sh

# Test specific components
./builddir/test_tokenizer_simple
./builddir/test_parser_modern
./builddir/test_executor_modern
./builddir/test_symtable_modern

# Test case statements
./test_case_basic.sh

# Test comprehensive tokenizer
./builddir/test_tokenizer_comprehensive
```

## Current Limitations

- Function definitions not yet implemented
- Here documents not yet implemented
- Advanced I/O redirection features incomplete
- Special characters in parameter expansion (`:`, `@`, `?`) need refinement
- Complex command sequences in parameter expansion need improvement
- Minor edge cases in advanced parameter expansion scenarios

See [Project Status](PROJECT_STATUS_CURRENT.md) for detailed progress and technical architecture.

## Contributing

Lusush follows modern C development practices with comprehensive testing and clear documentation. See the [Next Steps](NEXT_STEPS_CURRENT.md) document for current development priorities.

## License

MIT License - see LICENSE file for details.
