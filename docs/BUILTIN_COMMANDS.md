# Lusush Built-in Commands Reference

**Complete guide to all built-in commands in Lusush v1.3.0**

---

## Table of Contents

1. [Overview](#overview)
2. [Core Shell Commands](#core-shell-commands)
3. [File and Directory Commands](#file-and-directory-commands)
4. [Variable and Environment Commands](#variable-and-environment-commands)
5. [Control Flow Commands](#control-flow-commands)
6. [Process and Job Management](#process-and-job-management)
7. [Unique Lusush Commands](#unique-lusush-commands)
8. [I/O and Text Processing](#io-and-text-processing)
9. [System Information Commands](#system-information-commands)
10. [Quick Reference](#quick-reference)

---

## Overview

Lusush implements a comprehensive set of POSIX-compliant built-in commands, enhanced with modern features and the unique integrated debugger. All commands maintain full POSIX compatibility while providing additional functionality where appropriate.

### 📚 **Command Categories**

- **POSIX Standard**: Full compliance with POSIX specifications
- **Enhanced**: POSIX-compliant with additional features
- **Lusush Unique**: Commands available only in Lusush (like `debug`)
- **Modern**: Traditional commands with improved usability

---

## Core Shell Commands

### `echo`
**Display text**

```bash
# Basic usage
echo "Hello, World!"
echo Hello World

# With variables
name="Alice"
echo "Hello, $name"

# Multiple arguments
echo one two three

# Escape sequences (with -e)
echo -e "Line 1\nLine 2\tTabbed"

# Suppress newline (with -n)
echo -n "No newline: "
echo "continues here"
```

**Options:**
- `-n`: Do not output trailing newline
- `-e`: Enable interpretation of backslash escapes
- `-E`: Disable interpretation of backslash escapes (default)

---

### `printf`
**Formatted output (Enhanced with POSIX compliance)**

```bash
# Basic formatting
printf "Hello, %s!\n" "World"

# Number formatting
printf "Number: %d, Float: %.2f\n" 42 3.14159

# Dynamic field width (Lusush enhancement)
printf "%*s\n" 10 "right"        # Right-align in 10 characters
printf "%-*s|\n" 10 "left"       # Left-align in 10 characters

# Dynamic precision
printf "%.*f\n" 3 3.14159        # 3 decimal places

# Multiple formats
printf "%-10s %5d %8.2f\n" "Item" 42 123.456
```

**Format Specifiers:**
- `%s`: String
- `%d`, `%i`: Integer
- `%f`: Float
- `%x`, `%X`: Hexadecimal
- `%o`: Octal
- `%c`: Character
- `%*s`: Dynamic width (Lusush enhancement)
- `%.*f`: Dynamic precision (Lusush enhancement)

---

### `read`
**Read input from user or file (Enhanced)**

```bash
# Basic input
read name
echo "Hello, $name"

# With prompt (Lusush enhancement)
read -p "Enter your name: " name

# Read multiple variables
read first last
echo "First: $first, Last: $last"

# Raw input (preserve backslashes)
read -r line

# Read from file
read line < file.txt

# Timeout (if supported)
read -t 5 -p "Enter within 5 seconds: " input
```

**Options:**
- `-p prompt`: Display prompt before reading
- `-r`: Raw mode (don't interpret backslash escapes)
- `-t timeout`: Timeout in seconds
- `-n count`: Read only specified number of characters

---

### `test` / `[`
**Evaluate conditional expressions (Enhanced)**

```bash
# File tests
test -f file.txt          # File exists and is regular file
test -d directory         # Directory exists
test -r file.txt          # File is readable
test -w file.txt          # File is writable
test -x script.sh         # File is executable

# String tests
test -z "$var"            # String is empty
test -n "$var"            # String is not empty
test "$a" = "$b"          # Strings are equal
test "$a" != "$b"         # Strings are not equal

# Numeric comparisons
test "$a" -eq "$b"        # Numbers are equal
test "$a" -ne "$b"        # Numbers are not equal
test "$a" -lt "$b"        # a less than b
test "$a" -le "$b"        # a less than or equal to b
test "$a" -gt "$b"        # a greater than b
test "$a" -ge "$b"        # a greater than or equal to b

# Logical operations (Lusush enhancement)
test ! -f file.txt        # NOT file exists
test -f file1.txt -a -f file2.txt    # AND operation
test -f file1.txt -o -f file2.txt    # OR operation

# Bracket notation
[ -f file.txt ] && echo "File exists"
[ "$name" = "Alice" ] && echo "Hello Alice"
```

---

### `type`
**Display command type (Enhanced with POSIX compliance)**

```bash
# Basic usage
type echo                 # Shows: echo is a shell builtin
type ls                   # Shows: ls is /bin/ls
type my_function          # Shows: my_function is a function

# Show all locations (-a option)
type -a echo              # Shows all instances

# Show only path (-p option)
type -p ls                # Shows: /bin/ls

# Show only type (-t option)
type -t echo              # Shows: builtin
type -t ls                # Shows: file
type -t my_function       # Shows: function
```

**Options:**
- `-a`: Show all locations of command
- `-p`: Show only the path (for external commands)
- `-t`: Show only the type (builtin, file, function, etc.)

---

## File and Directory Commands

### `cd`
**Change directory**

```bash
# Basic usage
cd /path/to/directory
cd ~                      # Home directory
cd -                      # Previous directory
cd                        # Home directory (no args)

# Relative paths
cd ..                     # Parent directory
cd ../..                  # Two levels up
cd ./subdir               # Subdirectory

# With error handling
cd /nonexistent 2>/dev/null || echo "Directory not found"
```

**Special Variables:**
- `PWD`: Current working directory
- `OLDPWD`: Previous working directory

---

### `pwd`
**Print working directory**

```bash
# Show current directory
pwd

# In scripts
current_dir=$(pwd)
echo "Working in: $current_dir"
```

---

### `mkdir`
**Create directories**

```bash
# Create single directory
mkdir newdir

# Create multiple directories
mkdir dir1 dir2 dir3

# Create parent directories (-p)
mkdir -p path/to/deep/directory

# With permissions
mkdir -m 755 publicdir
```

---

### `rmdir`
**Remove empty directories**

```bash
# Remove empty directory
rmdir emptydir

# Remove multiple empty directories
rmdir dir1 dir2 dir3

# Remove parent directories if empty (-p)
rmdir -p path/to/empty/dirs
```

---

## Variable and Environment Commands

### `set`
**Set shell options and positional parameters**

```bash
# Show all variables and functions
set

# Set positional parameters
set -- arg1 arg2 arg3
echo "First: $1, Second: $2, Third: $3"

# Clear positional parameters
set --

# Shell options (see SHELL_OPTIONS.md for complete reference)
set -e                    # Exit on error
set -u                    # Error on unset variables
set -x                    # Trace execution
set -o errexit            # Long form
set +e                    # Disable option

# Show option status
set -o
```

---

### `unset`
**Remove variables or functions**

```bash
# Remove variable
VAR="value"
unset VAR
echo $VAR                 # Empty

# Remove function
my_func() { echo "hello"; }
unset my_func
my_func                   # Command not found

# Remove multiple items
unset VAR1 VAR2 VAR3
```

---

### `export`
**Export variables to environment**

```bash
# Export existing variable
VAR="value"
export VAR

# Export and assign
export PATH="/usr/local/bin:$PATH"
export EDITOR="vim"

# Show all exported variables
export

# Remove from export (but keep variable)
export -n VAR
```

---

### `alias`
**Create command aliases**

```bash
# Create alias
alias ll='ls -la'
alias la='ls -A'
alias grep='grep --color=auto'

# Show all aliases
alias

# Show specific alias
alias ll

# Temporary disable alias
\ll                       # Run actual 'll' command, not alias
```

---

### `unalias`
**Remove aliases**

```bash
# Remove specific alias
unalias ll

# Remove all aliases
unalias -a
```

---

## Control Flow Commands

### `if` / `then` / `else` / `elif` / `fi`
**Conditional execution**

```bash
# Basic if statement
if [ -f "file.txt" ]; then
    echo "File exists"
fi

# If-else
if [ "$USER" = "root" ]; then
    echo "Running as root"
else
    echo "Running as regular user"
fi

# Multiple conditions
if [ -f "config.txt" ]; then
    echo "Config found"
elif [ -f "config.conf" ]; then
    echo "Alternative config found"
else
    echo "No config file found"
fi
```

---

### `for`
**Iterate over lists**

```bash
# Basic for loop
for item in apple banana cherry; do
    echo "Fruit: $item"
done

# File iteration
for file in *.txt; do
    echo "Processing: $file"
done

# Number sequence
for i in 1 2 3 4 5; do
    echo "Number: $i"
done

# With command substitution
for user in $(cat users.txt); do
    echo "User: $user"
done
```

---

### `while`
**Loop while condition is true**

```bash
# Basic while loop
counter=1
while [ $counter -le 5 ]; do
    echo "Count: $counter"
    counter=$((counter + 1))
done

# Read file line by line
while read line; do
    echo "Line: $line"
done < file.txt

# Infinite loop (use with caution)
while true; do
    echo "Running..."
    sleep 1
done
```

---

### `case`
**Pattern matching**

```bash
# Basic case statement
case "$1" in
    start)
        echo "Starting service"
        ;;
    stop)
        echo "Stopping service"
        ;;
    restart)
        echo "Restarting service"
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        ;;
esac

# Pattern matching
case "$filename" in
    *.txt)
        echo "Text file"
        ;;
    *.pdf)
        echo "PDF file"
        ;;
    *.jpg|*.png|*.gif)
        echo "Image file"
        ;;
    *)
        echo "Unknown file type"
        ;;
esac
```

---

## Process and Job Management

### `jobs`
**List active jobs**

```bash
# Show all jobs
jobs

# Show job PIDs
jobs -p

# Show running jobs only
jobs -r

# Show stopped jobs only
jobs -s
```

---

### `bg`
**Put jobs in background**

```bash
# Start job in background
sleep 100 &

# Put current job in background
sleep 100
# Press Ctrl+Z
bg

# Put specific job in background
bg %1
```

---

### `fg`
**Bring jobs to foreground**

```bash
# Bring most recent job to foreground
fg

# Bring specific job to foreground
fg %1
fg %job_name
```

---

### `kill`
**Terminate processes**

```bash
# Kill by PID
kill 1234

# Kill by job number
kill %1

# Kill with specific signal
kill -9 1234              # SIGKILL
kill -TERM 1234           # SIGTERM
kill -HUP 1234            # SIGHUP

# Kill all background jobs
kill $(jobs -p)
```

---

## Unique Lusush Commands

### `debug`
**Integrated debugger (Unique to Lusush)**

```bash
# Show debugger help
debug help

# Enable debugging
debug on                  # Basic level
debug on 2                # Verbose level
debug on 3                # Trace level

# Debug status
debug                     # Show current status

# Variable inspection
debug vars                # Show all variables
debug print VAR           # Show specific variable

# Execution control
debug step                # Step to next command
debug next                # Step over function calls
debug continue            # Continue execution

# Function analysis
debug functions           # List all functions
debug function name       # Show specific function

# Performance profiling
debug profile on          # Enable profiling
debug profile report      # Show performance data
debug profile reset       # Reset profiling data
debug profile off         # Disable profiling

# Execution tracing
debug trace on            # Enable execution tracing
debug trace off           # Disable tracing

# Advanced features
debug break add file line # Add breakpoint (when available)
debug break list          # List breakpoints
debug break remove id     # Remove breakpoint
debug break clear         # Clear all breakpoints
debug stack               # Show call stack
debug analyze script      # Analyze script for issues

# Disable debugging
debug off
```

**Debug Levels:**
- 0: None (disabled)
- 1: Basic debugging
- 2: Verbose debugging
- 3: Trace execution
- 4: Full profiling

---

### `config`
**Configuration management**

```bash
# Show all configuration
config list

# Get specific setting
config get theme.name
config get display.performance_monitoring

# Set configuration
config set theme.name dark
config set display.performance_monitoring true
config set display.optimization_level 2

# Reset to defaults
config reset

# Show configuration help
config help
```

**Common Configuration Options:**
- `theme.name`: Current theme (modern, dark, light, etc.)
- `autocorrect.enabled`: Enable "Did you mean" suggestions
- `display.performance_monitoring`: Enable performance monitoring
- `completion.enhanced`: Enhanced tab completion

---

### `theme`
**Professional Theme System**

```bash
# Show current theme and list all available themes
theme

# List all themes organized by category
theme list

# Set active theme (changes immediately)
theme set corporate        # Professional business theme
theme set dark            # Modern dark theme with bright accents
theme set light           # Clean light theme with excellent readability
theme set colorful        # Vibrant theme for creative workflows
theme set minimal         # Ultra-minimal for distraction-free work
theme set classic         # Traditional shell appearance

# Preview theme without applying
theme preview dark

# Show detailed information about a theme
theme info corporate

# Display color palette of active theme
theme colors

# Show theme system statistics
theme stats

# Complete help
theme help
```

**Available Themes by Category:**

**Professional:**
- `corporate`: Business-appropriate colors and professional styling

**Developer:**
- `dark`: Modern dark theme with bright accent colors for extended coding

**Minimal:**
- `light`: Clean light theme with excellent readability
- `minimal`: Ultra-minimal theme for distraction-free work

**Creative:**
- `colorful`: Vibrant colorful theme for creative workflows

**Classic:**
- `classic`: Traditional shell appearance with basic colors

**Git Integration:**
All themes include intelligent git-aware prompts that automatically display branch status, changes, and tracking information.

---

## I/O and Text Processing

### `cat`
**Display file contents**

```bash
# Display file
cat file.txt

# Display multiple files
cat file1.txt file2.txt

# Number lines (-n)
cat -n file.txt

# Show non-printing characters (-v)
cat -v file.txt

# Create file with heredoc
cat > newfile.txt << 'EOF'
Line 1
Line 2
Line 3
EOF
```

---

### `head`
**Display first lines of file**

```bash
# First 10 lines (default)
head file.txt

# First n lines
head -n 5 file.txt
head -5 file.txt

# Multiple files
head file1.txt file2.txt

# First n bytes
head -c 100 file.txt
```

---

### `tail`
**Display last lines of file**

```bash
# Last 10 lines (default)
tail file.txt

# Last n lines
tail -n 5 file.txt
tail -5 file.txt

# Follow file changes (-f)
tail -f logfile.txt

# Multiple files
tail file1.txt file2.txt
```

---

### `wc`
**Count lines, words, characters**

```bash
# Count everything
wc file.txt

# Count lines only (-l)
wc -l file.txt

# Count words only (-w)
wc -w file.txt

# Count characters only (-c)
wc -c file.txt

# Multiple files
wc *.txt

# From pipe
echo "hello world" | wc -w
```

---

## System Information Commands

### `uname`
**System information**

```bash
# System name
uname

# All information (-a)
uname -a

# Specific information
uname -s              # System name
uname -n              # Node name
uname -r              # Release
uname -v              # Version
uname -m              # Machine type
```

---

### `date`
**Display or set date**

```bash
# Current date and time
date

# Formatted output
date "+%Y-%m-%d"          # 2025-01-01
date "+%Y-%m-%d %H:%M:%S" # 2025-01-01 15:30:45
date "+%A, %B %d, %Y"     # Monday, January 01, 2025

# Specific timezone
TZ=UTC date

# Unix timestamp
date +%s
```

---

### `hostname`
**Display or set hostname**

```bash
# Show hostname
hostname

# Show fully qualified domain name
hostname -f

# Show IP address
hostname -i
```

---

### `whoami`
**Display current username**

```bash
# Current user
whoami

# Use in scripts
current_user=$(whoami)
echo "Running as: $current_user"
```

---

## Quick Reference

### Essential Commands Summary

| Category | Commands |
|----------|----------|
| **Core** | `echo`, `printf`, `read`, `test`, `type` |
| **Files** | `cd`, `pwd`, `mkdir`, `rmdir`, `cat`, `head`, `tail` |
| **Variables** | `set`, `unset`, `export`, `alias`, `unalias` |
| **Control** | `if`, `for`, `while`, `case` |
| **Jobs** | `jobs`, `bg`, `fg`, `kill` |
| **Unique** | `debug`, `config`, `theme` |
| **System** | `uname`, `date`, `hostname`, `whoami` |

### Lusush Enhancements

| Command | Enhancement |
|---------|-------------|
| `printf` | Dynamic field width (`%*s`) and precision (`%.*f`) |
| `read` | Prompt option (`-p`) |
| `test` | Logical operations (`-a`, `-o`, `!`) |
| `type` | Complete POSIX compliance (`-a`, `-p`, `-t`) |
| `debug` | **Unique**: Complete interactive debugger |
| `config` | **Unique**: Modern configuration system |
| `theme` | **Unique**: Professional theme management |

### Command Exit Codes

```bash
# Success
true                      # Always returns 0
echo "hello"              # Returns 0 on success

# Failure
false                     # Always returns 1
test -f nonexistent       # Returns 1 (false)

# Check exit code
command
echo $?                   # Shows exit code of last command

# Use in conditionals
if command; then
    echo "Command succeeded"
else
    echo "Command failed with code: $?"
fi
```

---

---

## display
**Layered Display System Interface**

### Usage
```bash
display status              # Show display system status  
display enable              # Enable layered display system
display disable             # Disable layered display system
display config              # Show detailed configuration
display stats               # Show performance statistics
display diagnostics         # Show comprehensive diagnostics
display help                # Show complete command reference
```

### Description
The `display` command provides an interface to Lusush's revolutionary layered display architecture. This system enables coordinated display management with universal prompt compatibility, real-time syntax highlighting, and intelligent layer combination.

**Note**: The underlying layered display controller architecture exists but full integration is planned for a future release. Currently operates in standard display mode.

### Environment Variables
- `LUSUSH_LAYERED_DISPLAY=1|0` - Enable/disable at startup
- `LUSUSH_DISPLAY_DEBUG=1|0` - Enable debug output  
- `LUSUSH_DISPLAY_OPTIMIZATION=0-4` - Set optimization level (0=disabled, 4=maximum)

### Examples
```bash
# Check system status
display status

# Show current configuration
display config

# Enable layered display system (when integration is complete)
display enable

# Performance diagnostics
display stats
display diagnostics
```

---

## Conclusion

Lusush provides a comprehensive set of built-in commands that combine POSIX compliance with modern enhancements. The unique `debug` command sets Lusush apart from all other shells, providing capabilities that simply aren't available anywhere else.

Key advantages:
- **Complete POSIX compliance**: All standard commands work as expected
- **Modern enhancements**: Improved usability without breaking compatibility
- **Unique capabilities**: Integrated debugger, configuration system, themes, layered display architecture
- **Professional quality**: Suitable for development, production, and education

Whether you're using Lusush for daily shell work, script development, or learning, these built-in commands provide everything needed for professional shell scripting and system administration.