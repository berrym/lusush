# Lusush Shell

A shell implementation with modern architecture targeting POSIX compliance. Version 1.0.0-dev

Lusush is a functional shell implementing many POSIX shell features with a modern, modular architecture. The shell successfully handles interactive commands, variable operations, pipeline execution, and control structures, with ongoing work toward complete POSIX compliance.

## Current Status: ~60-70% POSIX Compliant

**Interactive Features Complete (49/49 tests passing):**
- ✅ **Command Execution**: Full execution of basic shell commands
- ✅ **Variable Assignment and Expansion**: Complete variable handling including arithmetic expansion  
- ✅ **Quoting**: Proper single quote (literal) and double quote (expandable) handling
- ✅ **Arithmetic Expansion**: Full `$((expression))` support with operators and variables
- ✅ **Command Substitution**: Both `$(command)` and backtick syntax working
- ✅ **I/O Redirection**: Complete redirection system (>, <, >>, <<, <<<, 2>, &>, 2>&1)
- ✅ **Here Documents**: Multi-line input with variable expansion and tab stripping
- ✅ **Pipeline Execution**: Multi-command pipelines work reliably
- ✅ **Logical Operators**: Full support for `&&` and `||` conditional execution
- ✅ **Control Structures**: FOR/WHILE loops, IF statements, and CASE statements
- ✅ **Function Definitions**: Complete function definition and execution
- ✅ **Job Control**: Background processes with & operator
- ✅ **Globbing**: Pathname expansion with *, ?, and [...] patterns
- ✅ **Parameter Expansion**: Basic `${var:-default}` and similar patterns
- ✅ **Built-in Commands**: cd, pwd, echo, export, test, read, source, alias, and others

**Critical Missing POSIX Features:**
- ❌ **Signal Handling**: trap command not implemented
- ❌ **Script Execution**: Cannot run scripts with command line arguments
- ❌ **Positional Parameters**: $1, $2, $3, etc. not working
- ❌ **Special Variables**: $?, $$, $!, $#, $*, $@ not implemented
- ❌ **Control Flow Built-ins**: break, continue, return, shift not implemented
- ❌ **Process Management**: exec, wait commands missing
- ❌ **System Built-ins**: times, umask, ulimit, getopts missing
- ❌ **Tilde Expansion**: ~ not expanding to $HOME
- ❌ **Complete Job Control**: wait command and signal integration missing

**Current Development Priority:**
- 🔄 **Phase 1**: Implement positional parameters and special variables for script execution
  - Address remaining I/O redirection edge case in error handling
  - Minor refinements for advanced function error scenarios
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

# Test conditional logic
echo 'if [ -n "$HOME" ]; then echo "Home is set"; fi' | ./builddir/lusush

# Test functions with multiple variables
echo 'greet() { msg="Hello $1-$2"; echo "$msg"; }; greet John Doe' | ./builddir/lusush

# Test empty functions
echo 'empty() { }; empty; echo "Empty function works"' | ./builddir/lusush
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
- Built-in commands (echo, export, test, [, set, etc.)
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

**Conditional Logic:**
```bash
# Test conditions
if [ "$user" "=" "admin" ]; then
    echo "Admin access granted"
elif [ -n "$user" ]; then
    echo "User access: $user"
else
    echo "No user specified"
fi

# Test in functions
validate() {
    if [ -z "$1" ]; then
        echo "Error: No input provided"
        return 1
    fi
    echo "Valid input: $1"
}
```

**Functions:**
```bash
# Function definition and calling with multiple variables
calculate() {
    result=$((${1} + ${2}))
    echo "Sum of $1 and $2 is: $result"
}
calculate 5 10

# Function with quoted assignments and conditional logic
process_file() {
    filename="$1"
    message="Processing: $filename"
    if [ -n "$filename" ]; then
        echo "$message"
    else
        echo "No file specified"
    fi
}

# Empty functions work correctly
cleanup() { }
cleanup  # Executes silently
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
