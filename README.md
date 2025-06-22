# Lusush Shell

A POSIX-compliant shell with modern architecture,**Command Examples:**
**Command Examples:**

**Basic Operations:**
```bash
# Variable assignment and expansion
name=world
echo "Hello $name"
echo "Hello ${name}"

# Advanced parameter expansion
echo "${USER:-guest}"              # Default value
echo "${DEBUG:+--verbose}"         # Alternative value
echo "${#filename}"                # String length
echo "${path:0:5}"                 # Substring

# Logical operators
test -f file && echo "exists" || echo "missing"
command1 && command2 || fallback_command

# Command substitution
date_str=$(date '+%Y-%m-%d')
echo "Today is $date_str"
echo `echo "Legacy backticks work too"`

# Complex combinations
echo "${name:+Hello ${name}}"
test -f "${CONFIG:-config.txt}" && echo "Config found"
```
``` Version 0.6.0-dev

Lusush is a functional shell implementing core POSIX shell features with a modern, modular architecture. The shell successfully handles basic commands, variable operations, pipeline execution, and has partial control structure support.

## Current Status

**Working Features:**
- ✅ **Simple Commands**: Full execution of basic shell commands
- ✅ **Variable Assignment and Expansion**: Complete variable handling including arithmetic expansion  
- ✅ **Advanced Parameter Expansion**: Complete POSIX-compliant parameter expansion system
  - Default values: `${var:-default}`, `${var-default}`
  - Alternative values: `${var:+alternative}`, `${var+alternative}`
  - Length expansion: `${#var}`
  - Substring expansion: `${var:offset:length}`
  - Pattern matching: `${var#pattern}`, `${var##pattern}`, `${var%pattern}`, `${var%%pattern}`
- ✅ **Command Substitution**: Both modern `$(command)` and legacy backtick syntax
- ✅ **Logical Operators**: Full support for `&&` and `||` conditional execution
- ✅ **Quoted String Variable Expansion**: Full support for "$var" and "${var}" in double quotes
- ✅ **Pipeline Execution**: Multi-command pipelines work reliably
- ✅ **String Processing**: Proper distinction between literal ('...') and expandable ("...") strings
- ✅ **Modern Symbol Table**: POSIX-compliant variable scoping with complete integration
- ✅ **Control Structures**: FOR/WHILE loops and IF statements with proper variable scoping

**In Development:**
- ⚠️ **Command Sequence Parsing**: Minor issue with assignment followed by FOR loop constructs
- ⚠️ **Advanced Features**: Case statements, functions, nested structures planned

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

# Pattern matching (NEW!)
echo "${filename%.*}"              # Remove file extension
echo "${path##*/}"                 # Get basename from path
echo "${url#*://}"                 # Remove protocol from URL
echo "${version%%.*}"              # Get major version number

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

# Test comprehensive tokenizer
./builddir/test_tokenizer_comprehensive
```

## Current Limitations

- Case statements and functions not yet implemented
- Case conversion features (`${var^}`, `${var,}`) not yet implemented
- Special characters in pattern matching (`:`, `@`, `?`) need refinement
- Minor edge cases in complex parameter expansion scenarios

See [Project Status](PROJECT_STATUS_CURRENT.md) for detailed progress and technical architecture.

## Contributing

Lusush follows modern C development practices with comprehensive testing and clear documentation. See the [Next Steps](NEXT_STEPS_CURRENT.md) document for current development priorities.

## License

MIT License - see LICENSE file for details.
