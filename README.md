# Lusush Shell

A POSIX-compliant shell with modern features, built in C.

## Version 0.2.1 - Complete POSIX Core Features

Lusush has achieved comprehensive POSIX shell compliance with robust parsing infrastructure, complete parameter expansion, enhanced builtins, and full command substitution support.

## Features

### Core Functionality
- **POSIX-compliant command parsing** with robust error handling
- **Multi-character operator support** (`&&`, `||`, `>>`, `<<`, etc.)
- **Logical operators with short-circuit evaluation** 
- **Multiple command separators** (semicolons, newlines, logical operators)
- **Pipeline processing** with proper distinction from logical operators
- **Variable expansion** with quote handling and field splitting
- **POSIX parameter expansion** (`${var=value}`, `${var:-default}`, `${var:+alternate}`, etc.)
- **Enhanced echo builtin** with escape sequence processing enabled by default
- **Comprehensive command substitution** (both `$()` modern and backtick legacy syntax)
- **Control structures** (if/then/else, for, while, until, case)
- **File redirection** (input, output, append)
- **Command completion** with linenoise integration

### Interactive Features
- **Command line editing** with history
- **Tab completion** for commands and files
- **Modern terminal interface** 
- **Built-in commands** (echo, export, source, test, read, eval, etc.)

## Building

Lusush uses the Meson build system:

```bash
meson setup builddir
meson compile -C builddir
```

## Usage

Run the shell:
```bash
./builddir/lusush
```

### Examples

**Command separation:**
```bash
echo first && echo second || echo backup
echo a; echo b; echo c
```

**Pipeline processing:**
```bash
echo "test data" | grep "test"
```

**Command substitution (both syntaxes):**
```bash
MODERN=$(date '+%Y-%m-%d')     # Modern syntax
LEGACY=`date '+%H:%M'`         # Legacy backtick syntax  
echo "Today is $MODERN at $LEGACY"
```

**Enhanced echo with escape sequences:**
```bash
echo "Line1\nLine2\tTabbed"    # Escape sequences enabled by default
echo -n "No newline"           # -n flag supported
```

**Control structures:**
```bash
for i in 1 2 3; do echo "Number: $i"; done
```

**Mixed operators (major achievement):**
```bash
echo "test" | grep "test" && echo "found" || echo "not found"
```

### Testing Current Capabilities

To test current lusush functionality, use the provided test script:

```bash
# Run current capabilities test
cat test-current-abilities.sh | ./builddir/lusush

# Note: Direct file execution may have issues, use piped input
```

The test script demonstrates:
- Variable assignment and expansion
- Command substitution with $() syntax  
- Arithmetic expansion
- Globbing and pathname expansion
- Mixed operator expressions (pipes + logical operators)

**Known limitations:**
- **🚨 CRITICAL: POSIX command-line options** - Missing essential options like `-c`, `-e`, `-x`, `-s`, `-i`
- **Advanced parameter expansion**: Pattern substitution `${var/pattern/replacement}` not implemented
- **Here documents**: `<<EOF` syntax not supported
- **Advanced redirection**: Some complex redirection patterns may need enhancement
- **Job control**: Background processes and job management not implemented

### Command-Line Options Gap

**CURRENTLY SUPPORTED**: Only `-h/--help` and `-v/--version` (non-POSIX convenience options)

**MISSING CRITICAL POSIX OPTIONS**:
- **`-c command_string`** - Execute command string (essential for automation)
- **`-s`** - Read commands from standard input
- **`-i`** - Interactive mode
- **`-l`** - Login shell behavior
- **`-e`** - Exit on error (`set -e`)
- **`-x`** - Trace execution (`set -x`)
- **`-n`** - Syntax check mode (`set -n`)
- **`-u`** - Unset variable error (`set -u`)

This represents a significant POSIX compliance gap that affects system integration and automation usage.

## Architecture

- **Mixed operator support** - Robust parsing of `cmd | pipe && logical` expressions
- **Multi-token pushback system** for sophisticated parsing
- **Unified word expansion** with proper POSIX semantics
- **Clean separation** between pipeline and logical operator processing
- **Robust scanner** with lookahead for multi-character operators
- **Professional codebase** with comprehensive error handling

## Development Status

Version 0.2.1 represents a major milestone with comprehensive POSIX parameter expansion, enhanced echo, and robust mixed operator parsing complete.

### What Works  
- All basic command separators and logical operators (&&, ||, ;)
- Variable assignment and expansion with proper quote handling
- Control structures with nested support (if/then/else, for, while, until, case)
- Simple and complex pipeline processing
- File redirection and basic I/O operations
- **Complete POSIX parameter expansion** - All `${var...}` patterns implemented
- **Enhanced echo builtin** - Escape sequences enabled by default
- **Command substitution** - Both `$()` and backtick syntax working
- **Mixed operator parsing** - `cmd | pipe && logical` expressions work perfectly

### Critical Development Priority
**POSIX Command-Line Options Implementation** - The most critical missing feature for system integration:
- Implement `-c command_string` for automation and script execution
- Add shell behavior flags (`-e`, `-x`, `-n`, `-u`) for robust scripting
- Complete POSIX option syntax (`-o`/`+o`) for full compliance

### Future Development Phases
- Advanced parameter expansion completion (pattern substitution)
- Advanced file descriptor management and here documents
- Background job control and process management
- Enhanced POSIX compliance testing and validation

## License

MIT License - see LICENSE file for details.

## Contributing

Lusush follows modern C development practices with comprehensive testing and documentation.

### Testing Current Capabilities

To test current lusush functionality, use the provided test script:

```bash
# Run current capabilities test
cat test-current-abilities.sh | ./builddir/lusush

# Note: Direct file execution may have issues, use piped input
```

The test script demonstrates:
- Variable assignment and expansion
- Command substitution with $() syntax  
- Arithmetic expansion
- Globbing and pathname expansion
- Mixed operator expressions (pipes + logical operators)

**Known limitations:**
- No shebang (`#!/bin/sh`) processing
- No comment (`#`) support in scripts
- `${var=value}` parameter expansion not implemented
- Backtick command substitution not supported
- Script file execution has issues (use piped input)
