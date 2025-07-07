# LUSUSH USER MANUAL v1.0.0

## Table of Contents
1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Getting Started](#getting-started)
4. [Basic Usage](#basic-usage)
5. [Configuration System](#configuration-system)
6. [Completion and Hints](#completion-and-hints)
7. [Themes and Customization](#themes-and-customization)
8. [Network Integration](#network-integration)
9. [Built-in Commands](#built-in-commands)
10. [Advanced Features](#advanced-features)
11. [Troubleshooting](#troubleshooting)
12. [FAQ](#faq)

## Introduction

Lusush is a modern, POSIX-compliant shell that combines traditional shell functionality with enhanced user experience features. It provides intelligent tab completion with real-time hints, professional themes, network integration, and advanced command-line editing while maintaining full compatibility with POSIX shell scripts.

### Key Features

- **100% POSIX Compliance**: Full compatibility with POSIX shell standards
- **Real-time Hints**: Intelligent suggestions that appear as you type
- **Enhanced Tab Completion**: Context-aware completion with fuzzy matching
- **Professional Themes**: Corporate, dark, light, colorful, minimal, and classic themes
- **Network Integration**: SSH host completion and network-aware features
- **Advanced Line Editing**: Powerful command-line editing with history
- **Modern Configuration**: Unified configuration system with INI-style sections
- **Git Integration**: Git-aware prompts with branch status
- **Hybrid Configuration**: Support for both modern config and traditional scripts

### System Requirements

- Linux, macOS, or Unix-like operating system
- Terminal with ANSI color support
- 64-bit architecture (recommended)
- Minimum 50MB disk space
- Internet connection (for network features)

## Installation

### Building from Source

1. **Prerequisites**:
   ```bash
   # Ubuntu/Debian
   sudo apt install build-essential meson ninja-build git
   
   # macOS
   brew install meson ninja git
   
   # CentOS/RHEL
   sudo yum install gcc meson ninja-build git
   ```

2. **Download and Build**:
   ```bash
   git clone https://github.com/berrym/lusush.git
   cd lusush
   meson setup builddir --buildtype=release
   ninja -C builddir
   ```

3. **Install**:
   ```bash
   sudo ninja -C builddir install
   ```

### Verification

Test your installation:
```bash
lusush --version
lusush -c "echo 'Hello, Lusush!'"
```

## Getting Started

### First Launch

Start Lusush by typing:
```bash
lusush
```

You'll see the default prompt:
```
lusush:~$ 
```

### Setting as Default Shell

To make Lusush your default shell:
```bash
# Add lusush to /etc/shells (if not already present)
echo "$(which lusush)" | sudo tee -a /etc/shells

# Change your default shell
chsh -s $(which lusush)
```

### Initial Configuration

Lusush works out of the box with sensible defaults. For customization, create a configuration file:

```bash
# Create your configuration file
cat > ~/.lusushrc << 'EOF'
[history]
history_enabled = true
history_size = 10000
history_no_dups = true

[completion]
completion_enabled = true
fuzzy_completion = true
hints_enabled = true

[prompt]
prompt_style = git
prompt_theme = default

[theme]
theme_name = corporate
EOF
```

## Basic Usage

### Command Execution

Lusush executes commands just like any POSIX shell:

```bash
# Basic commands
ls -la
pwd
cd /path/to/directory
echo "Hello World"

# Command chaining
ls && pwd || echo "Error"

# Pipes and redirection
ls | grep ".txt" > output.txt
cat < input.txt
```

### Variables and Environment

```bash
# Set variables
name="John Doe"
export PATH="/usr/local/bin:$PATH"

# Use variables
echo $name
echo "Hello, $name!"

# View environment
env
config show
```

### Job Control

```bash
# Background jobs
long_running_command &

# List jobs
jobs

# Bring to foreground
fg %1

# Send to background
bg %1
```

## Configuration System

### Modern Configuration (.lusushrc)

Lusush uses a modern INI-style configuration system:

```ini
[history]
history_enabled = true
history_size = 1000
history_no_dups = true
history_timestamps = false
history_file = ~/.lusush_history

[completion]
completion_enabled = true
fuzzy_completion = true
completion_threshold = 60
completion_case_sensitive = false
hints_enabled = true

[prompt]
prompt_style = git
prompt_theme = default
git_prompt_enabled = true
git_cache_timeout = 5

[theme]
theme_name = corporate
theme_auto_detect_colors = true

[behavior]
auto_cd = false
spell_correction = true
confirm_exit = false

[network]
ssh_completion_enabled = true
cache_ssh_hosts = true
```

### Configuration Commands

```bash
# View all configuration
config show

# View specific section
config show completion

# Get specific value
config get hints_enabled

# Set configuration value
config set completion_enabled true
config set theme_name dark

# Save configuration
config save
```

### Traditional Script Support

Lusush also supports traditional shell scripts:

```bash
# ~/.profile - Login script
export EDITOR=vim
export PAGER=less

# ~/.lusushrc.sh - Interactive shell configuration
alias ll='ls -la'
alias grep='grep --color=auto'

# ~/.lusush_login - Login-specific initialization
echo "Welcome to Lusush!"

# ~/.lusush_logout - Logout cleanup
echo "Goodbye!"
```

## Completion and Hints

### Tab Completion

Lusush provides intelligent tab completion:

```bash
# Command completion
ec<TAB>        # Expands to echo, ed, etc.

# File completion
ls test<TAB>   # Shows test files

# Variable completion
echo $HO<TAB>  # Expands to $HOME

# SSH completion
ssh user@<TAB> # Shows SSH hosts from ~/.ssh/config
```

### Real-time Hints

Hints appear as you type, showing likely completions:

```bash
# As you type 'ec', you see:
$ ec█ho [text...]

# As you type 'cd', you see:
$ cd█ [directory]

# As you type 'echo $HO', you see:
$ echo $HO█ME
```

### Completion Configuration

```bash
# Enable/disable completion
config set completion_enabled true

# Enable/disable hints
config set hints_enabled true

# Configure fuzzy matching
config set fuzzy_completion true
config set completion_threshold 70

# Case sensitivity
config set completion_case_sensitive false
```

## Themes and Customization

### Available Themes

Lusush includes several professional themes:

```bash
# Corporate theme (default)
theme set corporate

# Dark theme
theme set dark

# Light theme
theme set light

# Colorful theme
theme set colorful

# Minimal theme
theme set minimal

# Classic theme
theme set classic

# List all themes
theme list
```

### Theme Information

```bash
# View current theme
theme info

# View theme details
theme info corporate

# Theme statistics
theme stats

# Color schemes
theme colors
```

### Custom Prompts

Configure your prompt style:

```bash
# Git-aware prompt (default)
config set prompt_style git

# Simple prompt
config set prompt_style normal

# Colorful prompt
config set prompt_style color

# Professional prompt
config set prompt_style pro
```

## Network Integration

### SSH Host Completion

Lusush automatically reads SSH hosts from your configuration:

```bash
# SSH completion from ~/.ssh/config
ssh user@<TAB>

# SCP completion
scp file.txt user@<TAB>:

# SFTP completion
sftp user@<TAB>
```

### Network Commands

```bash
# View network status
network status

# SSH host management
network hosts list
network hosts cache
network hosts clear

# Network configuration
network config
network diagnostics
```

### Network Configuration

```bash
# Enable SSH completion
config set ssh_completion_enabled true

# Cache SSH hosts
config set cache_ssh_hosts true

# Cloud discovery
config set cloud_discovery_enabled false
```

## Built-in Commands

### Core POSIX Commands

All 28 required POSIX built-in commands are implemented:

```bash
# Navigation
cd [directory]          # Change directory
pwd                     # Print working directory

# Variables
export VAR=value        # Export variable
unset VAR              # Unset variable
set                    # Set shell options
readonly VAR=value     # Create read-only variable

# I/O
echo [text]            # Print text
printf format [args]   # Formatted output
read [var]             # Read input

# Control
test expression        # Test conditions
[ expression ]         # Test conditions (alias)
true                   # Return success
false                  # Return failure
:                      # No-op command

# Process control
exec command           # Replace shell
exit [code]            # Exit shell
trap signal handler    # Set signal handler
wait [pid]             # Wait for process

# Utilities
type command           # Show command type
hash                   # Hash table utilities
umask [mask]           # Set file creation mask
ulimit [options]       # Set resource limits
times                  # Show process times
getopts optstring var  # Parse options

# Job control
jobs                   # List jobs
fg [job]               # Foreground job
bg [job]               # Background job

# Functions
return [code]          # Return from function
local var=value        # Local variable
shift [n]              # Shift parameters

# Control flow
break [n]              # Break from loop
continue [n]           # Continue loop

# File operations
source file            # Execute script
. file                 # Execute script (alias)
```

### Enhanced Commands

Lusush adds several enhanced commands:

```bash
# Configuration management
config show [section]   # Show configuration
config set key value    # Set configuration
config get key          # Get configuration value

# Theme management
theme set name          # Set theme
theme list              # List themes
theme info [name]       # Theme information
theme colors            # Color schemes

# Network integration
network status          # Network status
network hosts           # SSH host management
network config          # Network configuration
network diagnostics     # Network diagnostics

# Development tools
debug                   # Debug utilities
dump                    # Dump symbol table
```

### Command Examples

```bash
# File operations
ls -la                 # List files
cat file.txt           # Display file
cp source dest         # Copy file
mv old new            # Move file
rm file               # Remove file
mkdir dir             # Create directory

# Text processing
grep pattern file      # Search patterns
sed 's/old/new/' file  # Stream editor
awk '{print $1}' file  # Text processor

# System information
ps aux                 # Process list
df -h                  # Disk usage
free -m                # Memory usage
who                    # Logged in users
```

## Advanced Features

### History Management

```bash
# View history
history

# Search history
history | grep command

# History expansion
!!                     # Previous command
!n                     # Command number n
!string                # Last command starting with string

# Configuration
config set history_enabled true
config set history_size 10000
config set history_no_dups true
```

### Aliases

```bash
# Create aliases
alias ll='ls -la'
alias grep='grep --color=auto'
alias ..='cd ..'

# View aliases
alias

# Remove aliases
unalias ll
```

### Functions

```bash
# Define functions
function myfunction() {
    echo "Hello from function"
    return 0
}

# Use functions
myfunction
```

### Auto-correction

```bash
# Enable spell correction
config set spell_correction true

# Example: typo correction
$ eco "hello"
Did you mean: echo "hello"? (y/n)
```

### Git Integration

```bash
# Git-aware prompts show:
# - Current branch
# - Dirty state
# - Ahead/behind status

# Enable git prompts
config set git_prompt_enabled true
config set prompt_style git
```

## Troubleshooting

### Common Issues

#### Completion Not Working

```bash
# Check completion settings
config get completion_enabled

# Enable completion
config set completion_enabled true

# Verify PATH
echo $PATH
```

#### Hints Not Appearing

```bash
# Check hints setting
config get hints_enabled

# Enable hints
config set hints_enabled true

# Verify completion is enabled
config get completion_enabled
```

#### Theme Issues

```bash
# Check current theme
theme info

# Reset to default
theme set corporate

# Verify color support
config get theme_auto_detect_colors
```

#### Performance Issues

```bash
# Check completion threshold
config get completion_threshold

# Increase threshold
config set completion_threshold 80

# Disable hints if needed
config set hints_enabled false
```

### Debug Information

```bash
# Show version information
lusush --version

# Show configuration
config show

# Debug utilities
debug

# Check shell features
help
```

### Getting Help

```bash
# Built-in help
help

# Command-specific help
help cd
help config
help theme

# Manual pages
man lusush
```

## FAQ

### General Questions

**Q: Is Lusush POSIX compliant?**
A: Yes, Lusush is 100% POSIX compliant and passes all POSIX compliance tests.

**Q: Can I use my existing shell scripts?**
A: Yes, all POSIX shell scripts will work without modification.

**Q: How do I migrate from bash/zsh?**
A: Most bash/zsh scripts will work. Use the configuration system instead of shell-specific options.

### Configuration Questions

**Q: Where is the configuration file?**
A: `~/.lusushrc` for the main configuration, plus traditional scripts like `~/.lusushrc.sh`.

**Q: How do I disable features?**
A: Use `config set feature_name false` to disable specific features.

**Q: Can I use both modern and traditional configuration?**
A: Yes, Lusush supports both INI-style configuration and traditional shell scripts.

### Performance Questions

**Q: Does Lusush impact performance?**
A: No, Lusush is optimized for performance with minimal overhead.

**Q: Can I disable resource-intensive features?**
A: Yes, you can disable completion, hints, or themes if needed.

### Compatibility Questions

**Q: Does Lusush work on macOS?**
A: Yes, Lusush works on Linux, macOS, and Unix-like systems.

**Q: Can I use Lusush in scripts?**
A: Yes, use `#!/usr/bin/env lusush` or `#!/usr/local/bin/lusush` in your scripts.

### Support

For additional support:
- Documentation: `docs/` directory
- Issues: GitHub issue tracker
- Community: Project discussions

---

*This manual covers Lusush v1.0.0. For the latest features and updates, refer to the project documentation and changelog.*