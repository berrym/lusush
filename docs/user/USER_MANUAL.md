# LUSUSH USER MANUAL

## Table of Contents
1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Getting Started](#getting-started)
4. [Basic Usage](#basic-usage)
5. [Advanced Features](#advanced-features)
6. [Configuration](#configuration)
7. [Completion System](#completion-system)
8. [Themes and Customization](#themes-and-customization)
9. [Network Integration](#network-integration)
10. [Built-in Commands](#built-in-commands)
11. [Troubleshooting](#troubleshooting)
12. [FAQ](#faq)

## Introduction

Lusush is a modern, POSIX-compliant shell that combines traditional shell functionality with enhanced user experience features. It provides intelligent tab completion, professional themes, network integration, and advanced command-line editing while maintaining full compatibility with POSIX shell scripts.

### Key Features

- **100% POSIX Compliance**: Full compatibility with POSIX shell standards
- **Enhanced Tab Completion**: Intelligent, categorized completion system
- **Professional Themes**: Corporate, dark, and light theme options
- **Network Integration**: SSH host completion and network-aware features
- **Advanced Line Editing**: Powerful command-line editing with history
- **Auto-correction**: Intelligent command correction suggestions
- **Git Integration**: Git-aware prompts with branch status
- **Customizable**: Extensive configuration options and user preferences

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
   git clone https://github.com/your-repo/lusush.git
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

### Basic Configuration

Create a configuration file:
```bash
# Create user configuration
mkdir -p ~/.config/lusush
cp /etc/lusush/lusushrc ~/.config/lusush/lusushrc
```

## Basic Usage

### Command Execution

Lusush supports all standard shell operations:

```bash
# Simple commands
ls -la
cd /home/user
pwd

# Pipelines
ps aux | grep python | head -5

# Redirection
echo "Hello World" > output.txt
cat < input.txt

# Command substitution
echo "Today is $(date)"
echo "Files: `ls | wc -l`"

# Variable assignment
name="John"
echo "Hello, $name"

# Arithmetic expansion
echo $((5 + 3))
result=$((10 * 2))
```

### Control Structures

```bash
# If statements
if [ "$USER" = "admin" ]; then
    echo "Admin user"
else
    echo "Regular user"
fi

# For loops
for file in *.txt; do
    echo "Processing $file"
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
# Function definition
greet() {
    echo "Hello, $1!"
}

# Function call
greet "World"

# Function with return value
add_numbers() {
    local sum=$(($1 + $2))
    echo $sum
}

result=$(add_numbers 5 10)
echo "Sum: $result"
```

## Advanced Features

### History Management

Lusush provides advanced history features:

```bash
# Search history with Ctrl+R
# Press Ctrl+R and type search term

# History expansion
!!          # Previous command
!n          # Command number n
!string     # Most recent command starting with string
^old^new    # Replace 'old' with 'new' in previous command
```

### Job Control

```bash
# Background jobs
long_running_command &

# List jobs
jobs

# Bring job to foreground
fg %1

# Send job to background
bg %1

# Kill job
kill %1
```

### Advanced Editing

Lusush supports extensive command-line editing:

| Key Binding | Action |
|-------------|--------|
| `Ctrl+A` | Move to beginning of line |
| `Ctrl+E` | Move to end of line |
| `Ctrl+U` | Delete from cursor to beginning |
| `Ctrl+K` | Delete from cursor to end |
| `Ctrl+W` | Delete word backward |
| `Alt+F` | Move forward one word |
| `Alt+B` | Move backward one word |
| `Ctrl+L` | Clear screen |
| `Ctrl+R` | Reverse history search |

## Configuration

### Configuration File

Lusush uses a configuration file located at:
- System-wide: `/etc/lusush/lusushrc`
- User-specific: `~/.config/lusush/lusushrc`

### Configuration Options

```bash
# ~/.config/lusush/lusushrc

# Shell behavior
set -o errexit              # Exit on error
set -o nounset              # Error on undefined variables
set -o pipefail             # Pipeline failure detection

# History settings
HISTSIZE=10000              # History size in memory
HISTFILESIZE=20000          # History file size
HISTCONTROL=ignoredups      # Ignore duplicate commands

# Prompt customization
PS1='lusush:\w\$ '          # Primary prompt
PS2='> '                    # Secondary prompt

# Completion settings
config set completion_enabled true    # Enable enhanced completion
config set history_no_dups true       # Prevent duplicate history entries

# Theme selection
theme set corporate         # Set corporate theme
```

### Environment Variables

Important environment variables:

```bash
# Shell identification
SHELL=/usr/local/bin/lusush

# History configuration
HISTFILE=~/.lusush_history
HISTSIZE=10000
HISTFILESIZE=20000

# Completion configuration
LUSUSH_COMPLETION_ENABLED=1
LUSUSH_COMPLETION_CATEGORY=1

# Theme configuration
LUSUSH_THEME=corporate
LUSUSH_THEME_COLORS=auto

# Network configuration
LUSUSH_NETWORK_ENABLED=1
LUSUSH_SSH_COMPLETION=1
```

## Completion System

### Overview

Lusush features an intelligent completion system that categorizes and displays completions based on context.

### Completion Modes

#### Enhanced Mode (Default)
- Single-line menu display
- Categorized completions
- Position tracking
- Smart navigation

#### Simple Mode
- Traditional completion lists
- Minimal display
- Basic cycling

### Switching Modes

```bash
# Check current mode
config get completion_enabled

# Toggle between modes
config set completion_enabled false   # Disable
config set completion_enabled true    # Enable

# View all options
config show
```

### Completion Categories

| Category | Description | Example |
|----------|-------------|---------|
| `[builtin]` | Shell built-in commands | `cd`, `echo`, `export` |
| `[file]` | Regular files | `document.txt`, `script.sh` |
| `[directory]` | Directories | `projects/`, `docs/` |
| `[command]` | External commands | `git`, `vim`, `python` |
| `[variable]` | Shell variables | `$HOME`, `$PATH` |

### Usage Examples

```bash
# Enhanced mode display
$ cat document.txt [2/4 file]

# Navigate with TAB
$ cd projects/ [1/3 directory]

# Builtin completion
$ export [2/2 builtin]
```

### Navigation Controls

| Key | Action |
|-----|--------|
| `TAB` | Next completion |
| `Ctrl+P` | Previous completion |
| `Ctrl+N` | Jump forward (large lists) |
| `ESC` | Cancel completion |
| `Enter` | Accept completion |

## Themes and Customization

### Available Themes

#### Corporate Theme
Professional appearance suitable for business environments:
```bash
theme set corporate
```

#### Dark Theme
Dark background with high contrast:
```bash
theme set dark
```

#### Light Theme
Light background with subtle colors:
```bash
theme set light
```

### Theme Commands

```bash
# List available themes
theme list

# Show current theme
theme show

# Get theme information
theme info corporate

# Show theme colors
theme colors

# Theme statistics
theme stats
```

### Custom Themes

Create custom themes by copying existing ones:
```bash
# Copy corporate theme
cp /etc/lusush/themes/corporate.theme ~/.config/lusush/themes/mytheme.theme

# Edit theme file
vim ~/.config/lusush/themes/mytheme.theme

# Apply custom theme
theme set mytheme
```

### Theme Configuration

Theme files use a simple key-value format:
```ini
# mytheme.theme
[colors]
primary_fg=#ffffff
primary_bg=#0066cc
secondary_fg=#cccccc
secondary_bg=#004499

[prompt]
show_git=true
show_time=false
show_user=true
show_host=true

[completion]
highlight_current=true
show_categories=true
```

## Network Integration

### SSH Host Completion

Lusush automatically completes SSH hostnames from:
- `~/.ssh/config`
- `~/.ssh/known_hosts`
- System-wide SSH configuration

```bash
# SSH completion examples
ssh user@hostname<TAB>      # Complete hostnames
scp file.txt host:<TAB>     # Complete remote paths
```

### Network Commands

```bash
# Network status
network status

# Network configuration
network config

# SSH host management
network hosts list
network hosts add hostname
network hosts remove hostname
```

### Network Configuration

```bash
# Enable/disable network features
config set network_enabled true

# Configure SSH completion
config set ssh_completion_enabled true

# Network timeout settings
config set network_timeout 5
```

## Built-in Commands

### Essential Commands

#### `cd` - Change Directory
```bash
cd /path/to/directory
cd ~                    # Home directory
cd -                    # Previous directory
cd ..                   # Parent directory
```

#### `pwd` - Print Working Directory
```bash
pwd                     # Show current directory
pwd -P                  # Show physical directory (resolve symlinks)
```

#### `echo` - Display Text
```bash
echo "Hello World"
echo -n "No newline"
echo -e "With\ttabs\nand\nnewlines"
```

#### `history` - Command History
```bash
history                 # Show command history
history 10              # Show last 10 commands
history -c              # Clear history
```

### Variable Management

#### `export` - Export Variables
```bash
export PATH="/usr/local/bin:$PATH"
export EDITOR=vim
export -p               # Show all exported variables
```

#### `unset` - Remove Variables
```bash
unset variable_name
unset -f function_name
```

#### `set` - Shell Options
```bash
set -o errexit          # Exit on error
set +o errexit          # Disable exit on error
set -o                  # Show all options
```

### Process Management

#### `jobs` - List Jobs
```bash
jobs                    # List active jobs
jobs -l                 # List with process IDs
jobs -p                 # Show process IDs only
```

#### `fg` - Foreground Job
```bash
fg                      # Bring last job to foreground
fg %1                   # Bring job 1 to foreground
```

#### `bg` - Background Job
```bash
bg                      # Send last job to background
bg %1                   # Send job 1 to background
```

### Utility Commands

#### `alias` - Command Aliases
```bash
alias ll='ls -la'
alias grep='grep --color=auto'
alias                   # Show all aliases
```

#### `type` - Command Type
```bash
type ls                 # Show command type
type -a ls              # Show all locations
type -t ls              # Show type only
```

#### `hash` - Command Hashing
```bash
hash                    # Show command hash table
hash -r                 # Reset hash table
hash command            # Add command to hash
```

### Lusush-Specific Commands

#### `theme` - Theme Management
```bash
theme list              # List available themes
theme set dark          # Set theme
theme show              # Show current theme
theme info corporate    # Get theme information
```

#### `config` - Configuration Management
```bash
config show             # Show all configuration
config show completion  # Show completion settings
config set completion_enabled true # Enable enhanced completion
config get completion_enabled      # Get option value
```

#### `network` - Network Features
```bash
network status          # Network status
network config          # Network configuration
network hosts list      # List SSH hosts
```

#### `clear` - Clear Screen
```bash
clear                   # Clear terminal screen
```

## Troubleshooting

### Common Issues

#### Completion Not Working
```bash
# Check if completion is enabled
config get completion_enabled

# Verify completion callback
echo $LUSUSH_COMPLETION_ENABLED

# Reset completion system
config set completion_enabled false
config set completion_enabled true
```

#### Theme Not Loading
```bash
# Check theme availability
theme list

# Verify theme file
ls ~/.config/lusush/themes/
ls /etc/lusush/themes/

# Reset to default theme
theme set corporate
```

#### History Not Saving
```bash
# Check history file permissions
ls -la ~/.lusush_history

# Verify history settings
echo $HISTFILE
echo $HISTSIZE

# Reset history file
rm ~/.lusush_history
touch ~/.lusush_history
```

#### Network Features Not Working
```bash
# Check network configuration
network status

# Verify SSH configuration
ls -la ~/.ssh/config
ls -la ~/.ssh/known_hosts

# Test network connectivity
ping google.com
```

### Performance Issues

#### Slow Startup
```bash
# Check configuration file
time lusush -c "exit"

# Disable network features temporarily
config set network_enabled false

# Reduce history size
config set history_size 1000
```

#### Slow Completion
```bash
# Switch to simple completion mode
config set completion_enabled false

# Check completion sources
ls -la ~/.ssh/
du -sh ~/.lusush_history

# Clear completion cache
hash -r
```

### Debug Mode

Enable debug mode for troubleshooting:
```bash
# Enable debug output
export LUSUSH_DEBUG=1

# Enable completion debug
export LUSUSH_DEBUG_COMPLETION=1

# Enable network debug
export LUSUSH_DEBUG_NETWORK=1

# Start with debug
lusush --debug
```

### Log Files

Check log files for errors:
```bash
# System logs
journalctl -u lusush

# User logs
tail -f ~/.config/lusush/lusush.log

# Debug logs
cat ~/.config/lusush/debug.log
```

## FAQ

### General Questions

**Q: Is Lusush compatible with my existing shell scripts?**
A: Yes, Lusush is 100% POSIX compliant and runs existing shell scripts without modification.

**Q: Can I use Lusush as my login shell?**
A: Yes, Lusush can be used as a login shell. Add it to `/etc/shells` and use `chsh` to set it as your default.

**Q: How do I migrate from bash/zsh to Lusush?**
A: Most bash/zsh configurations work with Lusush. Copy your `.bashrc` or `.zshrc` to `~/.config/lusush/lusushrc` and adjust as needed.

### Configuration Questions

**Q: Where should I put my configuration?**
A: User configuration goes in `~/.config/lusush/lusushrc`. System-wide configuration is in `/etc/lusush/lusushrc`.

**Q: How do I customize the prompt?**
A: Use the `PS1` variable in your configuration file or select a theme with `theme set`.

**Q: Can I disable enhanced features?**
A: Yes, use `config set completion_enabled false` to toggle features, or set `completion_enabled = false` in your ~/.lusushrc file.

### Feature Questions

**Q: How do I add custom completions?**
A: Create completion functions in `~/.config/lusush/completions/` directory.

**Q: Can I create custom themes?**
A: Yes, copy an existing theme to `~/.config/lusush/themes/` and modify it.

**Q: How do I add SSH hosts for completion?**
A: Add hosts to `~/.ssh/config` or use `network hosts add hostname`.

### Performance Questions

**Q: Why is startup slow?**
A: Check your configuration file for heavy operations. Disable network features if not needed.

**Q: Why is completion slow?**
A: Large completion sets or network lookups can slow completion. Try simple mode or adjust timeouts.

**Q: How do I improve performance?**
A: Reduce history size, disable unused features, and use simple completion mode for better performance.

### Compatibility Questions

**Q: Does Lusush work on macOS?**
A: Yes, Lusush supports macOS with proper dependencies installed.

**Q: What about Windows?**
A: Lusush works on Windows with WSL (Windows Subsystem for Linux).

**Q: Can I use Lusush in containers?**
A: Yes, Lusush works in Docker containers and other containerized environments.

---

## Getting Help

### Documentation
- User Manual: `/usr/share/doc/lusush/USER_MANUAL.md`
- Developer Guide: `/usr/share/doc/lusush/DEVELOPER_GUIDE.md`
- API Reference: `/usr/share/doc/lusush/API_REFERENCE.md`

### Community
- GitHub Issues: https://github.com/your-repo/lusush/issues
- Discussions: https://github.com/your-repo/lusush/discussions
- IRC: #lusush on irc.libera.chat

### Support
- Email: support@lusush.org
- Documentation: https://lusush.org/docs
- Examples: https://lusush.org/examples

---

*This manual covers Lusush version 1.0. For the latest documentation, visit https://lusush.org/docs*