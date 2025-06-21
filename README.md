# Lusush Shell

A POSIX-compliant shell with modern architecture,**Command Examples:**

**Basic Operations:**
```bash
# Variable assignment and expansion
name=world
echo "Hello $name"
echo "Hello ${name}"

# Quoted string expansion
echo "Current user: $USER, path: $PATH"
for item in one two three; do
    echo "Processing item: $item"
done

# Arithmetic expansion  
result=$((5 + 3))
echo "Result: $result"
echo "Calculation: $((result * 2))"

# Command substitution
date_str=$(date '+%Y-%m-%d')
echo "Today is $date_str"
``` Version 0.6.0-dev

Lusush is a functional shell implementing core POSIX shell features with a modern, modular architecture. The shell successfully handles basic commands, variable operations, pipeline execution, and has partial control structure support.

## Current Status

**Working Features:**
- ✅ **Simple Commands**: Full execution of basic shell commands
- ✅ **Variable Assignment and Expansion**: Complete variable handling including arithmetic expansion  
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

**Parameter Expansion:**
```bash
echo "${USER:-default_user}"
echo "${PATH:+path_is_set}"
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

- Minor command sequence parsing issue (assignment + FOR loop combinations)
- Case statements and functions not yet implemented
- Some advanced parameter expansion patterns missing

See [Project Status](PROJECT_STATUS_CURRENT.md) for detailed progress and technical architecture.

## Contributing

Lusush follows modern C development practices with comprehensive testing and clear documentation. See the [Next Steps](NEXT_STEPS_CURRENT.md) document for current development priorities.

## License

MIT License - see LICENSE file for details.
