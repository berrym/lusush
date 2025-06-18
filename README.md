# lusush - A POSIX-Compliant Shell

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-0.1.8-blue.svg)](#)
[![C99](https://img.shields.io/badge/std-C99-green.svg)](#)

**lusush** is a modern, POSIX-compliant shell implementation written in C99. It provides essential shell functionality with a focus on compliance, performance, and maintainability.

## Features

### ✅ **Implemented Core Features**

#### **Command Execution**
- Built-in commands (22 builtins including `cd`, `echo`, `export`, `test`, etc.)
- External command execution with PATH resolution
- Command pipelines and basic redirection
- Variable assignment and expansion

#### **Control Flow Structures** 
- **If/elif/else/fi statements** - Complete conditional execution
- **For loops** - Iterate over lists with proper variable scoping
- **While loops** - Execute while condition is true
- **Until loops** - Execute until condition becomes true
- **Case statements** - Pattern matching (basic implementation)

#### **Word Expansion & Quoting**
- Variable expansion (`$VAR`, `${VAR}`)
- Command substitution (`` `cmd` ``, `$(cmd)`)
- Tilde expansion (`~`, `~/path`)
- Pathname expansion (globbing: `*`, `?`, `[...]`)
- Quote processing (single quotes, double quotes, escaping)
- Field splitting with IFS support

#### **Interactive Features**
- **Command-line completion** - Tab completion for commands, files, and variables
- **Command history** - Full history support with linenoise integration
- **Alias system** - Create and manage command aliases
- **Professional interface** - `--help`, `--version` options

#### **Advanced Features**
- **Arithmetic expansion** - Full arithmetic evaluation with operators
- **Modern parser** - Enhanced error recovery and multi-command body support
- **Symbol table** - Efficient variable and environment management
- **Signal handling** - Proper signal management for job control

### 🚧 **Current Development Priority: Parser Enhancement**

**Primary Focus** - Robust parsing foundation for all features:
- **Multi-line command parsing** - Better handling of commands spanning multiple lines
- **Nested construct support** - Proper parsing of deeply nested control structures  
- **Error recovery** - Enhanced parser error recovery and synchronization
- **Quote and escape handling** - Robust parsing of complex quoting scenarios
- **Command continuation** - Proper line continuation with backslash

**Secondary Features** (Dependent on parser improvements):
- **Function definitions** - User-defined shell functions
- **Heredoc support** - Here-document redirection (`<<`)
- **Enhanced pattern matching** - Full glob pattern support for case statements
- **Job control** - Background processes, job management (`jobs`, `fg`, `bg`)
- **Advanced redirection** - File descriptor manipulation

## Building

### Prerequisites

- **C99-compatible compiler** (GCC, Clang)
- **Meson build system** (0.55+)
- **POSIX-compliant system** (Linux, macOS, BSD)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd lusush

# Set up build directory
meson setup build

# Compile
meson compile -C build

# Run the shell
./build/lusush
```

### Build Options

```bash
# Debug build
meson setup build --buildtype=debug

# Release build  
meson setup build --buildtype=release

# Custom optimization
meson setup build -Doptimization=3
```

## Usage

### Interactive Mode

```bash
# Start the shell
./lusush

# Example usage
lusush$ echo "Hello, World!"
Hello, World!

lusush$ for i in 1 2 3; do echo "Number: $i"; done
Number: 1
Number: 2
Number: 3

lusush$ if test -f README.md; then echo "README exists"; fi
README exists
```

### Script Mode

```bash
# Execute a script file
./lusush script.sh

# Process stdin
echo 'echo "Hello from stdin"' | ./lusush
```

### Built-in Commands

| Command | Description | Example |
|---------|-------------|---------|
| `cd` | Change directory | `cd /home/user` |
| `echo` | Display text | `echo "Hello World"` |
| `export` | Export variables | `export PATH=/usr/bin` |
| `history` | Show command history | `history` |
| `alias` | Create aliases | `alias ll='ls -l'` |
| `test`/`[` | Test conditions | `test -f file.txt` |
| `read` | Read user input | `read name` |
| `source`/`.` | Execute script | `source ~/.bashrc` |
| `pwd` | Print working directory | `pwd` |
| `exit` | Exit shell | `exit 0` |

## Examples

### Control Flow

```bash
# If statement
if test $# -gt 0; then
    echo "Arguments provided: $*"
else
    echo "No arguments"
fi

# For loop with list
for file in *.txt; do
    echo "Processing: $file"
done

# While loop
count=1
while test $count -le 5; do
    echo "Count: $count"
    count=$((count + 1))
done

# Case statement
case $1 in
    start)
        echo "Starting service"
        ;;
    stop)
        echo "Stopping service"
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        ;;
esac
```

### Word Expansion

```bash
# Variable expansion
name="World"
echo "Hello, $name!"           # Hello, World!
echo "Hello, ${name}!"         # Hello, World!

# Command substitution
echo "Today is $(date)"        # Today is Tue Jun 17 2025
echo "Files: `ls *.txt`"       # Files: file1.txt file2.txt

# Pathname expansion
echo *.c                       # src/exec.c src/parser.c ...
echo test_[0-9].txt           # test_1.txt test_2.txt

# Tilde expansion
echo ~                         # /home/username
echo ~/Documents              # /home/username/Documents
```

## Architecture

### Core Components

```
lusush/
├── src/
│   ├── lusush.c          # Main shell loop
│   ├── parser.c          # Command parsing and AST generation  
│   ├── scanner.c         # Lexical analysis and tokenization
│   ├── exec.c            # Command execution engine
│   ├── wordexp.c         # Word expansion and substitution
│   ├── completion.c      # Tab completion system
│   └── builtins/         # Built-in command implementations
├── include/              # Header files
└── build system (meson.build)
```

### Design Principles

- **POSIX Compliance** - Adheres to POSIX shell standards
- **Modular Architecture** - Clean separation of concerns
- **Memory Safety** - Careful memory management with proper cleanup
- **Error Recovery** - Robust error handling and recovery mechanisms
- **Performance** - Optimized for interactive use and script execution

## Development

### Contributing

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature-name`
3. **Make changes** following the coding style
4. **Test thoroughly** with various shell constructs
5. **Submit a pull request**

### Code Style

- **C99 standard** compliance
- **4-space indentation**
- **LLVM-based clang-format** configuration (see `.clang-format`)
- **Descriptive variable names**
- **Comprehensive error handling**

### Testing

```bash
# Run basic functionality test
./build/lusush test.sh

# Manual testing
./build/lusush
lusush$ # Test various shell features
```

## Version History

- **v0.1.8** (2025-06-17) - Enhanced loop parsing and multi-command body support
- **v0.1.7** - Complete if/elif/else/fi control structures and version management
- **v0.1.6** - Major word expansion refactoring and feature enhancement  
- **v0.1.5** - POSIX control structures with modernized word expansion
- **v0.1.4** - Enhanced shell features and completion system

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **linenoise** - Lightweight readline alternative for command-line editing
- **libhashtable** - Efficient hash table implementation for symbol management
- **POSIX Shell Standards** - Reference implementation guidance

---

**lusush** - *A lightweight, POSIX-compliant shell for modern systems*
