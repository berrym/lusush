# Lusush Shell User Manual

**Version**: 1.1.3  
**Date**: January 2025  
**Status**: Production Ready

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Basic Usage](#basic-usage)
4. [Themes](#themes)
5. [Git Integration](#git-integration)
6. [Tab Completion](#tab-completion)
7. [History Management](#history-management)
8. [Key Bindings](#key-bindings)
9. [Advanced Features](#advanced-features)
10. [Configuration](#configuration)
11. [Troubleshooting](#troubleshooting)
12. [FAQ](#faq)

## Introduction

Lusush is a modern, professional shell designed for developers and system administrators who need a reliable, feature-rich command-line environment. Built on the foundation of POSIX compliance, Lusush adds contemporary features like themed prompts, real-time git integration, and intelligent tab completion while maintaining full compatibility with standard shell operations.

### Key Features

- **Professional Themes**: 6 enterprise-grade visual themes
- **Git Integration**: Real-time branch and status display
- **Intelligent Completion**: Context-aware tab completion
- **POSIX Compliance**: Full compatibility with standard shell operations
- **Enterprise Ready**: Optimized for professional environments
- **Cross-Platform**: Linux, macOS, and BSD support

### System Requirements

- **Operating System**: Linux, macOS, or BSD
- **Memory**: 512MB RAM minimum (2GB recommended)
- **Disk Space**: 100MB available space
- **Terminal**: Any ANSI-compatible terminal emulator

## Getting Started

### Installation

1. **Download and Build**:
   ```bash
   git clone https://github.com/berrym/lusush.git
   cd lusush
   meson setup builddir
   ninja -C builddir
   ```

2. **Test the Installation**:
   ```bash
   ./builddir/lusush --version
   ```

3. **First Run**:
   ```bash
   ./builddir/lusush --enhanced-display -i
   ```

### Initial Setup

When you first run Lusush, it will:
- Initialize with default settings
- Set up history management
- Configure basic tab completion
- Apply the default theme

## Basic Usage

### Starting Lusush

```bash
# Standard mode
lusush

# Enhanced display mode (recommended)
lusush --enhanced-display -i

# Execute a single command
lusush -c "echo 'Hello, World!'"

# Interactive mode
lusush -i
```

### Command Execution

Lusush supports all standard shell operations:

```bash
# Basic commands
ls -la
pwd
cd /home/user

# Pipes and redirection
ls | grep txt > files.list
cat file.txt | sort | uniq

# Variables
export MY_VAR="value"
echo $MY_VAR
echo "Home is: $HOME"

# Multiline commands
for file in *.txt; do
    echo "Processing: $file"
    wc -l "$file"
done

# Conditional statements
if [ -f "config.txt" ]; then
    echo "Configuration found"
    source config.txt
else
    echo "No configuration file"
fi

# Background processes
long_running_command &
jobs
fg %1
```

### Command-Line Options

| Option | Description |
|--------|-------------|
| `-c "command"` | Execute command and exit |
| `-i` | Interactive mode |
| `--enhanced-display` | Enable enhanced display features |
| `--help` | Show help information |
| `--version` | Display version information |

## Themes

Lusush includes 6 professional themes designed for different work environments.

### Available Themes

1. **Corporate** - Professional theme for business environments
   ```
   [user@hostname] ~/project (main) $
   ```

2. **Dark** - Modern dark theme with bright accent colors
   ```
   ┌─[user@hostname]─[~/project] (main ✓)
   └─$
   ```

3. **Light** - Clean light theme with excellent readability
   ```
   user@hostname:~/project (main)$
   ```

4. **Minimal** - Ultra-minimal theme for distraction-free work
   ```
   ~/project $
   ```

5. **Colorful** - Vibrant colorful theme for creative workflows
   ```
   ● user@hostname ~/project (main ✓) ➜
   ```

6. **Classic** - Traditional shell appearance with basic colors
   ```
   user@hostname:~/project$
   ```

### Theme Management

```bash
# List all available themes
theme list

# Set active theme
theme set dark

# Get current theme
theme get

# Theme examples
theme set corporate    # For business environments
theme set colorful     # For creative work
theme set minimal      # For focused work
```

### Theme Customization

Themes automatically adapt to your environment:
- Git repository status
- Current working directory
- User and hostname information
- Terminal color capabilities

## Git Integration

Lusush provides seamless git integration that displays repository information directly in your prompt.

### Git Status Indicators

| Symbol | Meaning |
|--------|---------|
| `(main)` | Current branch name |
| `(main *)` | Modified files present |
| `(main +)` | Staged files present |
| `(main %)` | Untracked files present |
| `(main ✓)` | Clean working directory |
| `(main ↑1)` | 1 commit ahead of remote |
| `(main ↓2)` | 2 commits behind remote |
| `(main ↑1↓2)` | Diverged from remote |

### Git-Aware Commands

Git integration enhances several commands:

```bash
# Tab completion for git commands
git checkout <TAB>    # Shows available branches
git merge <TAB>       # Shows available branches
git push <TAB>        # Shows available remotes

# Automatic git status in prompt
cd /path/to/git/repo  # Prompt automatically shows git info
```

### Repository Navigation

```bash
# Clone and navigate
git clone https://github.com/user/repo.git
cd repo  # Prompt now shows git branch

# Branch operations
git checkout -b feature-branch  # Prompt updates immediately
git commit -m "Changes"         # Status indicators update
```

## Tab Completion

Lusush provides intelligent, context-aware tab completion that adapts to what you're typing.

### Basic Completion

```bash
# Command completion
ec<TAB>        # Expands to "echo"
ls -<TAB>      # Shows available options: -a, -l, -h, etc.

# File completion
cat myf<TAB>   # Completes to matching filenames
cd ~/Doc<TAB>  # Completes to ~/Documents/
```

### Context-Aware Completion

```bash
# Git commands
git checkout <TAB>     # Shows branch names
git push origin <TAB>  # Shows branch names
git merge <TAB>        # Shows available branches

# Directory-only completion for cd
cd <TAB>              # Only shows directories, not files

# Variable completion
echo $HO<TAB>         # Completes to $HOME
export PATH=$PATH:<TAB>  # Shows directory paths
```

### SSH Host Completion

```bash
# Completes known SSH hosts
ssh <TAB>             # Shows hosts from ~/.ssh/config and known_hosts
scp file.txt user@<TAB>  # Shows available hosts
```

### Advanced Completion Features

- **Smart Filtering**: Only relevant options shown
- **Fuzzy Matching**: Partial matches work intelligently
- **History Integration**: Recently used completions prioritized
- **Performance Optimized**: Fast completion even with large datasets

## History Management

Lusush provides powerful history management with intelligent features.

### Basic History Commands

```bash
# Show command history
history

# Show last 10 commands
history | tail -10

# Clear history
history clear

# Search history (interactive)
Ctrl+R  # Then type search term
```

### History Features

- **Automatic Deduplication**: Duplicate commands removed automatically
- **Persistent Storage**: History saved between sessions
- **Search Integration**: Fast history search with Ctrl+R
- **Size Management**: Configurable history size limits

### History Expansion

```bash
# Repeat last command
!!

# Use last argument
ls -la /some/long/path
cd !$  # Goes to /some/long/path

# Repeat command by number
!123   # Repeat command #123

# Search and repeat
!git   # Repeat last command starting with "git"
```

### Configuration

```bash
# Set history size (in lusushrc or environment)
export LUSUSH_HISTORY_SIZE=10000

# Enable/disable deduplication
export LUSUSH_HISTORY_DEDUPE=1
```

## Key Bindings

Lusush uses standard readline key bindings with some enhancements.

### Movement

| Key Combination | Action |
|-----------------|--------|
| `Ctrl+A` | Move to beginning of line |
| `Ctrl+E` | Move to end of line |
| `Alt+F` | Move forward one word |
| `Alt+B` | Move backward one word |
| `Arrow Keys` | Move cursor left/right, navigate history up/down |

### Editing

| Key Combination | Action |
|-----------------|--------|
| `Ctrl+D` | Delete character at cursor |
| `Ctrl+H` / `Backspace` | Delete character before cursor |
| `Ctrl+W` | Delete word before cursor |
| `Ctrl+U` | Delete from cursor to beginning of line |
| `Ctrl+K` | Delete from cursor to end of line |
| `Ctrl+Y` | Paste previously deleted text |

### History Navigation

| Key Combination | Action |
|-----------------|--------|
| `Up Arrow` | Previous command in history |
| `Down Arrow` | Next command in history |
| `Ctrl+R` | Reverse search through history |
| `Ctrl+G` | Cancel current operation |

### System Commands

| Key Combination | Action |
|-----------------|--------|
| `Ctrl+L` | Clear screen |
| `Ctrl+C` | Interrupt current command |
| `Ctrl+Z` | Suspend current command |
| `Tab` | Auto-completion |

## Advanced Features

### Multiline Commands

Lusush fully supports complex multiline shell constructs:

```bash
# For loops
for i in {1..5}; do
    echo "Number: $i"
    sleep 1
done

# Conditional statements
if [ -d "/etc" ]; then
    echo "System directory exists"
    ls -la /etc | head -5
else
    echo "System directory not found"
fi

# While loops
counter=1
while [ $counter -le 3 ]; do
    echo "Iteration: $counter"
    ((counter++))
done

# Function definitions
my_function() {
    echo "Function called with args: $@"
    return 0
}
my_function arg1 arg2
```

### Job Control

```bash
# Background processes
sleep 100 &
long_command &

# Job management
jobs              # List active jobs
fg %1             # Bring job 1 to foreground
bg %2             # Send job 2 to background
kill %1           # Terminate job 1
disown %1         # Remove job from shell's job table
```

### Aliases

```bash
# Create aliases
alias ll='ls -la'
alias grep='grep --color=always'
alias ..='cd ..'

# Use aliases
ll                # Runs 'ls -la'
grep pattern file # Uses colored grep

# List aliases
alias

# Remove aliases
unalias ll
```

### Variables and Environment

```bash
# Local variables
name="John"
count=42

# Environment variables
export PATH="$PATH:/usr/local/bin"
export EDITOR="nano"

# Special variables
echo $0    # Shell name
echo $#    # Number of arguments
echo $?    # Exit status of last command
echo $$    # Process ID of shell
```

## Configuration

### Environment Variables

Customize Lusush behavior with environment variables:

```bash
# Theme settings
export LUSUSH_THEME=dark

# History settings
export LUSUSH_HISTORY_SIZE=10000
export LUSUSH_HISTORY_DEDUPE=1

# Display settings
export LUSUSH_ENHANCED_DISPLAY=1

# Git integration
export LUSUSH_GIT_PROMPT=1
export LUSUSH_GIT_STATUS_TIMEOUT=2

# Performance tuning
export LUSUSH_COMPLETION_CACHE_SIZE=1000
```

### Configuration File

Create `~/.lusushrc` for persistent configuration:

```ini
[theme]
name = corporate
auto_detect_terminal = true

[history]
size = 10000
dedupe = true
save_on_exit = true

[completion]
enabled = true
fuzzy = false
case_sensitive = false
max_suggestions = 50

[git]
prompt_enabled = true
status_timeout = 2
branch_display = true

[display]
enhanced_mode = true
color_support = auto
```

### Startup Scripts

Add to your `~/.bashrc` or `~/.zshrc`:

```bash
# Auto-start Lusush with enhanced features
if command -v lusush &> /dev/null; then
    export LUSUSH_THEME=corporate
    export LUSUSH_ENHANCED_DISPLAY=1
    alias lsh='lusush --enhanced-display -i'
fi
```

## Troubleshooting

### Common Issues

#### 1. Colors Not Displaying

**Problem**: Themes and colors not showing in terminal.

**Solutions**:
```bash
# Check terminal color support
echo $TERM
echo $COLORTERM

# Test ANSI colors
echo -e '\033[1;32mGreen\033[0m \033[1;34mBlue\033[0m'

# Force color mode
export TERM=xterm-256color
lusush --enhanced-display -i
```

#### 2. Git Integration Not Working

**Problem**: Git branch not showing in prompt.

**Solutions**:
```bash
# Check git installation
git --version

# Verify you're in a git repository
git status

# Check git configuration
git config --list

# Enable git prompt explicitly
export LUSUSH_GIT_PROMPT=1
```

#### 3. Tab Completion Issues

**Problem**: Tab completion not working or slow.

**Solutions**:
```bash
# Check completion is enabled
echo "Tab completion test: ec" # Press TAB after 'ec'

# Clear completion cache
rm -rf ~/.lusush_completion_cache

# Reduce completion timeout
export LUSUSH_COMPLETION_TIMEOUT=1
```

#### 4. Performance Issues

**Problem**: Shell feels slow or unresponsive.

**Solutions**:
```bash
# Reduce history size
export LUSUSH_HISTORY_SIZE=1000

# Disable git status for large repositories
cd large_repo
export LUSUSH_GIT_PROMPT=0

# Use minimal theme
theme set minimal

# Check system resources
top | grep lusush
```

#### 5. Build Issues

**Problem**: Compilation fails during installation.

**Solutions**:
```bash
# Install required dependencies
# Ubuntu/Debian:
sudo apt-get install build-essential meson ninja-build libreadline-dev

# RHEL/CentOS/Fedora:
sudo dnf install gcc meson ninja-build readline-devel

# macOS:
brew install meson ninja readline

# Clean and rebuild
rm -rf builddir
meson setup builddir
ninja -C builddir
```

### Diagnostic Commands

```bash
# Check version and build info
lusush --version
lusush --build-info

# Test basic functionality
lusush -c "echo 'Basic test: OK'"

# Test themes
lusush -c "theme list"

# Test git integration
cd /path/to/git/repo
lusush -c "pwd"  # Should show git info in prompt

# Performance test
time lusush -c "for i in {1..100}; do echo \$i; done > /dev/null"
```

### Log Files

Check log files for detailed error information:

```bash
# System logs
journalctl -u lusush
tail -f /var/log/lusush.log

# User logs
tail -f ~/.lusush.log
```

### Getting Help

If you need additional help:

1. **Check Documentation**: `/usr/local/share/doc/lusush/`
2. **Visit GitHub**: https://github.com/berrym/lusush
3. **Report Issues**: https://github.com/berrym/lusush/issues
4. **Community Support**: Project discussions and forums

## FAQ

### General Questions

**Q: Is Lusush compatible with existing shell scripts?**

A: Yes, Lusush is fully POSIX-compliant and runs standard shell scripts without modification.

**Q: Can I use Lusush as my default shell?**

A: Yes, after installation, you can set Lusush as your default shell:
```bash
chsh -s /usr/local/bin/lusush
```

**Q: Does Lusush work with tmux/screen?**

A: Yes, Lusush works perfectly with terminal multiplexers. Themes and git integration work normally.

**Q: Is Lusush suitable for production environments?**

A: Absolutely. Lusush is designed for enterprise use with stability, security, and performance in mind.

### Features

**Q: How do I customize themes?**

A: Use the built-in theme system:
```bash
theme list        # See available themes
theme set dark    # Switch to dark theme
```

**Q: Can I disable git integration?**

A: Yes, set the environment variable:
```bash
export LUSUSH_GIT_PROMPT=0
```

**Q: How do I backup my configuration?**

A: Copy your configuration files:
```bash
cp ~/.lusushrc ~/backup/
cp ~/.lusush_history ~/backup/
```

### Performance

**Q: Why is Lusush using more memory than bash?**

A: Lusush includes additional features like git integration and enhanced completion, which require more memory but provide significantly better functionality.

**Q: How can I optimize Lusush for large repositories?**

A: For large git repositories, consider:
```bash
export LUSUSH_GIT_STATUS_TIMEOUT=1  # Reduce timeout
theme set minimal                   # Use minimal theme
```

**Q: Is there a way to profile performance?**

A: Yes, use the built-in profiling:
```bash
lusush --profile -c "your command here"
```

### Troubleshooting

**Q: What should I do if Lusush crashes?**

A: 
1. Check system logs for error messages
2. Try running with minimal configuration
3. Report the issue on GitHub with details

**Q: How do I reset Lusush to default settings?**

A: Remove configuration files and restart:
```bash
rm ~/.lusushrc ~/.lusush_history
lusush --enhanced-display -i
```

### Integration

**Q: Does Lusush work with IDEs?**

A: Yes, most IDEs can be configured to use Lusush as their integrated terminal shell.

**Q: Can I use Lusush in Docker containers?**

A: Yes, Lusush works well in containers. Add it to your Dockerfile:
```dockerfile
RUN git clone https://github.com/berrym/lusush.git && \
    cd lusush && \
    meson setup builddir && \
    ninja -C builddir && \
    ninja -C builddir install
```

---

## Appendix

### Quick Reference Card

**Theme Commands**:
- `theme list` - Show all themes
- `theme set <name>` - Change theme
- `theme get` - Show current theme

**Git Integration**:
- Automatic branch display in prompt
- Git status indicators (*, +, %, ✓)
- Git-aware tab completion

**Key Bindings**:
- `Ctrl+R` - History search
- `Ctrl+L` - Clear screen
- `Tab` - Auto-completion
- `Ctrl+A/E` - Line beginning/end

**Configuration**:
- `~/.lusushrc` - User configuration
- Environment variables for runtime settings
- `/etc/lusush/` - System-wide configuration

---

**Lusush Shell User Manual v1.1.3** - For the most up-to-date information, visit: https://github.com/berrym/lusush

*Happy shell computing!*