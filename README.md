# Lusush Shell

A POSIX-compliant shell with modern features, built in C.

## Version 0.2.0 - Core Parser Robustness Complete

Lusush has reached a major architectural milestone with robust parsing infrastructure and full support for logical operators, command separators, and pipeline processing.

## Features

### Core Functionality
- **POSIX-compliant command parsing** with robust error handling
- **Multi-character operator support** (`&&`, `||`, `>>`, `<<`, etc.)
- **Logical operators with short-circuit evaluation** 
- **Multiple command separators** (semicolons, newlines, logical operators)
- **Pipeline processing** with proper distinction from logical operators
- **Variable expansion** with quote handling and field splitting
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

**Variable expansion:**
```bash
VAR="hello world"; echo "$VAR"
```

**Control structures:**
```bash
for i in 1 2 3; do echo "Number: $i"; done
```

## Architecture

- **Multi-token pushback system** for sophisticated parsing
- **Unified word expansion** with proper POSIX semantics
- **Clean separation** between pipeline and logical operator processing
- **Robust scanner** with lookahead for multi-character operators
- **Professional codebase** with comprehensive error handling

## Development Status

Version 0.2.0 represents a major milestone with all core parsing functionality complete and ready for advanced features.

### What Works
- All basic command separators and logical operators
- Variable assignment and expansion with proper quote handling
- Control structures with nested support
- Simple and complex pipeline processing
- File redirection and basic I/O operations

### Next Development Phase
- Complex mixed operator expressions
- Advanced file descriptor management
- Background job control
- Enhanced POSIX compliance features

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
