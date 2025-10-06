# Lusush User Guide

**Version**: 1.3.0  
**Last Updated**: October 2024

Welcome to Lusush, the advanced interactive shell designed for professional environments. This guide covers all features available in the production v1.3.0 release.

## Table of Contents

- [Getting Started](#getting-started)
- [Basic Usage](#basic-usage)
- [Shell Features](#shell-features)
- [Theme System](#theme-system)
- [Display System](#display-system)
- [Configuration](#configuration)
- [POSIX Compliance](#posix-compliance)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)

## Getting Started

### Starting Lusush

```bash
# Interactive shell
lusush

# Execute a command
lusush -c "echo 'Hello, World!'"

# Run a script
lusush script.sh

# Force interactive mode
lusush -i
```

### Command Line Options

```bash
lusush [OPTIONS] [SCRIPT] [ARGUMENTS...]

Options:
  -c COMMAND    Execute command string and exit
  -i            Force interactive mode
  -l            Act as login shell
  -s            Read commands from standard input
  -e            Exit on error (set -e)
  -x            Trace execution (set -x)
  -u            Treat unset variables as error (set -u)
  -v            Verbose mode (set -v)
  -f            Disable pathname expansion (set -f)
  --help        Show help message
  --version     Show version information
```

## Basic Usage

### Command Execution

Lusush executes commands just like any POSIX shell:

```bash
# Simple commands
echo "Hello, World!"
ls -la
cd /home/user

# Pipes and redirection
ls | grep ".txt"
echo "content" > file.txt
cat < input.txt

# Background jobs
long_running_command &
```

### Variables

```bash
# Set variables
name="John"
count=42

# Use variables
echo "Hello, $name"
echo "Count: ${count}"

# Environment variables
export PATH="$PATH:/new/path"
```

### Control Structures

```bash
# If statements
if [ "$name" = "John" ]; then
    echo "Hello John"
else
    echo "Hello stranger"
fi

# For loops
for file in *.txt; do
    echo "Processing: $file"
done

# While loops
counter=1
while [ $counter -le 5 ]; do
    echo "Count: $counter"
    counter=$((counter + 1))
done

# Case statements
case "$1" in
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

### Functions

```bash
# Define functions
greet() {
    echo "Hello, $1!"
}

# Call functions
greet "Alice"

# Functions with return values
add() {
    local result=$(($1 + $2))
    echo $result
}

sum=$(add 5 3)
echo "Sum: $sum"
```

## Shell Features

### Command History

```bash
# Search history
history | grep "command"

# Execute previous command
!!

# Execute command from history
!123

# History expansion
!cd    # Last command starting with 'cd'
```

### Tab Completion

Lusush provides intelligent tab completion:

```bash
# File completion
cat /etc/pas<TAB>      # Completes to /etc/passwd

# Command completion
ech<TAB>               # Completes to echo

# Variable completion
echo $HO<TAB>          # Completes to $HOME

# Git-aware completion (in git repositories)
git che<TAB>           # Completes to checkout
git checkout mai<TAB>  # Completes to main
```

### Aliases

```bash
# Create aliases
alias ll='ls -la'
alias grep='grep --color=auto'

# Use aliases
ll
grep "pattern" file.txt

# Remove aliases
unalias ll

# List all aliases
alias
```

### Job Control

```bash
# Run command in background
long_command &

# List jobs
jobs

# Bring job to foreground
fg %1

# Send job to background
bg %1

# Kill job
kill %1
```

## Theme System

Lusush includes a professional theme system with 6 built-in themes.

### Available Themes

- **default**: Clean, minimalist design
- **corporate**: Professional business appearance
- **dark**: High contrast dark theme
- **colorful**: Vibrant development theme
- **minimal**: Ultra-clean minimal design
- **classic**: Traditional shell appearance

### Theme Commands

```bash
# List all available themes
theme list

# Show current theme
theme show

# Switch themes
theme set dark
theme set corporate
theme set minimal

# Get theme information
theme info dark

# Show theme colors
theme colors
```

### Git Integration

All themes include intelligent git integration:

```bash
# In a git repository, prompts show:
[user@host] ~/project (main) $           # Current branch
[user@host] ~/project (main ✓) $         # Clean repository
[user@host] ~/project (feature *) $      # Uncommitted changes
[user@host] ~/project (main +2) $        # Staged changes
```

### Symbol Compatibility

Themes automatically adapt to terminal capabilities:

```bash
# Set symbol mode
theme symbols unicode    # Use Unicode symbols
theme symbols ascii      # Use ASCII symbols
theme symbols auto       # Auto-detect (default)
```

## Display System

Lusush uses an advanced layered display architecture for optimal performance.

### Display Commands

```bash
# Show system status
display status

# Show performance statistics
display stats

# Show configuration
display config

# Show system diagnostics
display diagnostics

# Show help
display help
```

### Performance Monitoring

```bash
# Initialize performance monitoring
display performance init

# Show performance report
display performance report

# Show detailed performance report
display performance report detail

# Show memory usage
display performance memory

# Show layer-specific performance
display performance layers
```

## Configuration

### Configuration File

Lusush reads configuration from `~/.lusushrc`:

```ini
# Theme settings
theme.name = "dark"
theme.symbol_mode = "auto"

# Performance settings
display.performance_monitoring = true
display.optimization_level = 2

# Shell behavior
shell.history_size = 1000
shell.interactive_comments = true

# Completion settings
completion.enabled = true
completion.case_sensitive = false
```

### Environment Variables

```bash
# Display settings
export LUSUSH_DISPLAY_DEBUG=1           # Enable debug output
export LUSUSH_DISPLAY_OPTIMIZATION=2    # Set optimization level

# Theme settings
export LUSUSH_THEME=dark                # Default theme
```

### Configuration Commands

```bash
# Show all configuration
config show

# Show specific section
config show shell
config show theme

# Set configuration values
config set theme.name dark
config set display.optimization_level 3

# Get configuration values
config get theme.name

# Save configuration
config save

# Reset to defaults
config reset
```

## POSIX Compliance

Lusush is fully POSIX compliant and supports all standard shell options.

### Shell Options

```bash
# Error handling
set -e          # Exit on error (errexit)
set -u          # Error on unset variables (nounset)
set +e          # Disable exit on error

# Debugging and tracing
set -x          # Trace execution (xtrace)
set -v          # Verbose input (verbose)
set +x          # Disable tracing

# Globbing and expansion
set -f          # Disable pathname expansion (noglob)
set +f          # Enable pathname expansion

# Other options
set -m          # Enable job control (monitor)
set -h          # Remember command locations (hashall)
set -C          # Don't overwrite files (noclobber)

# Show all options
set -o
```

### Long Option Names

```bash
# Using long option names
set -o errexit     # Same as set -e
set -o nounset     # Same as set -u
set -o xtrace      # Same as set -x
set -o verbose     # Same as set -v
set -o noglob      # Same as set -f
set -o monitor     # Same as set -m
set -o noclobber   # Same as set -C
```

### Built-in Commands

Lusush provides all standard POSIX built-in commands:

```bash
# File operations
cd, pwd, mkdir, rmdir

# Text processing
echo, printf, read

# Variable operations
export, unset, readonly

# Process control
exec, exit, return, wait

# Shell state
set, unset, alias, unalias

# Utility
type, which, command, test, [
```

## Performance

### Performance Features

- **Layered Display Architecture**: Optimized display system
- **Memory Pool Management**: Efficient memory allocation
- **Intelligent Caching**: Smart caching for frequently used operations
- **Performance Monitoring**: Real-time performance metrics

### Performance Tuning

```bash
# Optimization levels (0-4)
config set display.optimization_level 4    # Maximum optimization
config set display.optimization_level 0    # Basic functionality

# Enable performance monitoring
config set display.performance_monitoring true

# View performance statistics
display performance report
```

### Performance Monitoring

```bash
# Initialize monitoring
display performance init

# Run some commands
echo "test"
ls
pwd

# Check results
display performance report
```

Sample output:
```
=== Enhanced Performance Report ===
Cache Performance:
  Operations: 24 total (12 hits, 12 misses)
  Hit Rate: 50.0% (Target: >75.0%)
Display Timing:
  Operations: 18 measured
  Average: 0.05ms (Target: <50.0ms) ✓
Memory Pool Performance:
  Pool allocations: 45 (100.0% hit rate)
  Malloc fallbacks: 0
  Active allocations: 23
  Pool memory usage: 1842 bytes
  Avg allocation time: 48 ns
  Pool efficiency: EXCELLENT ✓
```

## Troubleshooting

### Common Issues

#### Configuration Problems

```bash
# Check configuration syntax
config validate

# Show current configuration
config show

# Reset to defaults
config reset
```

#### Performance Issues

```bash
# Check system performance
display performance report

# Show diagnostics
display diagnostics

# Reduce optimization level
config set display.optimization_level 1
```

#### Theme Issues

```bash
# Check theme compatibility
theme info

# Use ASCII symbols for compatibility
theme symbols ascii

# Switch to basic theme
theme set classic
```

#### Display Issues

```bash
# Check display status
display status

# Show display configuration
display config

# Check terminal capabilities
echo $TERM
tput colors
```

### Debug Information

```bash
# Show system information
lusush --version

# Show build information
config show system

# Show environment
env | grep LUSUSH
```

### Getting Help

```bash
# Command help
help
help set
help theme

# Manual pages
man lusush

# Version information
lusush --version
```

### Performance Troubleshooting

```bash
# Monitor performance
display performance init
# Run problematic commands
display performance report

# Check memory usage
display performance memory

# Show diagnostics
display diagnostics
```

## Advanced Usage

### Scripting Best Practices

```bash
#!/usr/bin/env lusush

# Enable strict mode
set -euo pipefail

# Use functions for reusable code
log() {
    echo "[$(date)] $*" >&2
}

# Proper error handling
if ! command -v git >/dev/null 2>&1; then
    log "ERROR: git is required"
    exit 1
fi

# Use local variables in functions
process_files() {
    local dir="$1"
    local pattern="$2"
    
    find "$dir" -name "$pattern" | while read -r file; do
        log "Processing: $file"
        # Process file
    done
}
```

### Integration with Other Tools

```bash
# Git integration
if git rev-parse --git-dir > /dev/null 2>&1; then
    echo "In git repository: $(git branch --show-current)"
fi

# SSH integration
if [ -n "$SSH_CONNECTION" ]; then
    echo "Connected via SSH"
fi

# Screen/tmux integration
if [ -n "$STY" ] || [ -n "$TMUX" ]; then
    echo "Running in screen/tmux"
fi
```

### Custom Prompt Configuration

While lusush uses themes for prompt styling, you can customize behavior:

```bash
# Configure git integration
config set theme.git_enabled true
config set theme.show_branch true
config set theme.show_status true

# Configure symbols
config set theme.symbol_mode auto
```

This completes the Lusush User Guide. For more technical details, see the documentation in the `docs/` directory.