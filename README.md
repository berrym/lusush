# Lusush Shell

A POSIX-compliant shell with modern architecture, built in C.

## Version 0.6.0-dev

Lusush is a functional shell implementing core POSIX shell features with a modern, modular architecture. The shell successfully handles basic commands, variable operations, pipeline execution, and has partial control structure support.

## Current Status

**Working Features:**
- ✅ **Simple Commands**: Full execution of basic shell commands
- ✅ **Variable Assignment and Expansion**: Complete variable handling including arithmetic expansion  
- ✅ **Pipeline Execution**: Multi-command pipelines work reliably
- ✅ **String Processing**: Quoted strings and parameter expansion functional

**In Development:**
- ⚠️ **Control Structures**: Parser implemented, token boundary issues being resolved
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

The shell uses a modern three-phase architecture:
- **Tokenizer**: POSIX-compliant lexical analysis
- **Parser**: Recursive descent parser building AST
- **Executor**: Clean execution engine for parsed commands

This design provides clear separation of concerns and enables robust error handling and future extensibility.
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
# Run all tests
./test_modern_integration.sh

# Test specific components
./builddir/test_tokenizer_simple
./builddir/test_parser_modern
./builddir/test_executor_modern
```

## Current Limitations

- Control structures have parsing issues (being resolved)
- Nested control structures not yet supported
- Case statements and functions not implemented
- Some advanced parameter expansion patterns missing

See [Project Status](PROJECT_STATUS_CURRENT.md) for detailed limitation analysis.

## Contributing

Lusush follows modern C development practices with comprehensive testing and clear documentation. See the [Next Steps](NEXT_STEPS_CURRENT.md) document for current development priorities.

## License

MIT License - see LICENSE file for details.
