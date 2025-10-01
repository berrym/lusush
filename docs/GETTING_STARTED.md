# Getting Started with Lusush

**A comprehensive guide to using the only shell with integrated debugging capabilities**

---

## Table of Contents

1. [What is Lusush?](#what-is-lusush)
2. [Installation](#installation)
3. [First Steps](#first-steps)
4. [Basic Shell Usage](#basic-shell-usage)
5. [Your First Debugging Session](#your-first-debugging-session)
6. [Essential Features](#essential-features)
7. [Configuration and Customization](#configuration-and-customization)
8. [Moving from Other Shells](#moving-from-other-shells)
9. [Next Steps](#next-steps)

---

## What is Lusush?

Lusush is a modern, POSIX-compliant shell that adds something no other shell offers: **integrated interactive debugging**. While it works like any standard shell for daily tasks, it provides unique capabilities that make script development and troubleshooting dramatically easier.

### 🔍 **Unique Features**
- **Integrated Debugger**: Debug scripts interactively without external tools
- **Complete POSIX Compliance**: All 24 major shell options implemented
- **Modern User Experience**: Themes, autosuggestions, and smart corrections
- **Enterprise Security**: Privileged mode and advanced security features

### 👥 **Who Should Use Lusush**
- **Developers**: Who write and debug shell scripts
- **DevOps Engineers**: Managing automation and deployment scripts
- **System Administrators**: Working with system scripts
- **Students**: Learning shell scripting with visual debugging
- **Anyone**: Who wants a better shell experience

---

## Installation

### Prerequisites

Make sure you have the required build tools:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential meson ninja-build libreadline-dev

# CentOS/RHEL/Fedora
sudo dnf install gcc meson ninja-build readline-devel

# macOS (with Homebrew)
brew install meson ninja readline
```

### Build from Source

```bash
# 1. Clone the repository
git clone https://github.com/lusush/lusush.git
cd lusush

# 2. Build with Meson
meson setup builddir
ninja -C builddir

# 3. Test the build
./builddir/lusush --version
```

### Verify Installation

```bash
# Check that Lusush works
./builddir/lusush --help

# Test basic functionality
echo 'echo "Hello, Lusush!"' | ./builddir/lusush
```

---

## First Steps

### Starting Lusush

```bash
# Start interactive session
./builddir/lusush

# You'll see a prompt like:
[user@hostname] /current/directory $ 
```

### Basic Commands

Try these basic commands to get familiar:

```bash
# Current directory
pwd

# List files
ls

# Create a directory
mkdir test_lusush
cd test_lusush

# Create a simple file
echo "Hello, World!" > hello.txt
cat hello.txt
```

### Getting Help

```bash
# Shell help
help

# Command-specific help
help echo
help set

# Debugger help
debug help
```

---

## Basic Shell Usage

### Variables and Environment

```bash
# Set a variable
name="Alice"
echo "Hello, $name"

# Export to environment
export PATH_BACKUP="$PATH"
echo $PATH_BACKUP

# Check all variables
set | head -10
```

### Basic Scripting

Create your first script:

```bash
# Create a script file
cat > first_script.sh << 'EOF'
#!/usr/bin/env lusush

# Simple script demonstration
name="World"
echo "Hello, $name!"

# Basic loop
for i in 1 2 3; do
    echo "Count: $i"
done

# Conditional
if [ -f "hello.txt" ]; then
    echo "File exists!"
else
    echo "File not found"
fi
EOF

# Make it executable
chmod +x first_script.sh

# Run it
./first_script.sh
```

### POSIX Shell Options

Lusush implements all 24 POSIX shell options:

```bash
# See all options
set -o

# Enable strict error handling
set -e
set -u

# Enable tracing (great for debugging)
set -x
echo "This command will be traced"
set +x

# Check current status
set -o | grep -E "(errexit|nounset|xtrace)"
```

---

## Your First Debugging Session

This is where Lusush truly shines. No other shell can do this!

### Enable the Debugger

```bash
# Show debugger commands
debug help

# Enable debugging
debug on
```

### Debug a Simple Script

```bash
# Create a debug example
cat > debug_example.sh << 'EOF'
#!/usr/bin/env lusush

echo "Starting debug example"
counter=1
max=5

while [ $counter -le $max ]; do
    echo "Processing item $counter"
    result=$((counter * 2))
    echo "Result: $result"
    counter=$((counter + 1))
done

echo "Processing complete"
EOF

chmod +x debug_example.sh
```

### Interactive Debugging Session

```bash
# Enable debugging
debug on 2

# Run the script - you'll see detailed output
./debug_example.sh

# Inspect variables during execution
debug vars

# Look at specific variables
debug print counter
debug print result
debug print max
```

### Example Debug Output

```bash
$ debug on 2
[DEBUG] Debug mode enabled (level: 2)
[DEBUG] Debug session started at: 23592.704952486
Debug mode enabled

$ counter=1
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: counter=1

$ debug print counter
[DEBUG] VARIABLE: counter
[DEBUG] Value: '1'
[DEBUG] Scope: global
[DEBUG] Type: string
```

### Advanced Debugging Features

```bash
# Enable execution tracing
debug trace on
echo "This command is traced"
debug trace off

# Enable performance profiling
debug profile on
# ... run some commands ...
debug profile report
debug profile off

# Analyze script structure
debug functions

# Turn off debugging
debug off
```

---

## Essential Features

### Themes and Appearance

```bash
# List available themes
theme list

# Switch to dark theme
theme set dark

# See current theme
theme show
```

### Autosuggestions and Corrections

```bash
# Enable smart features
config set autocorrect.enabled true
config set autosuggestions.enabled true

# Try typing a wrong command - Lusush will suggest corrections
ehco "hello"    # Suggests: "echo"

# Check configuration
config list
```

### Git Integration

If you're in a git repository:

```bash
cd /path/to/git/repo

# The prompt will show git status
[user@hostname] /path/to/repo (main ✓) $ 

# Make some changes and see status updates
echo "change" >> README.md
# Prompt shows: (main +1)

git add README.md
# Prompt shows: (main ↑1)
```

### Tab Completion

```bash
# Complete commands
de<TAB>          # Completes to "debug"

# Complete options
debug <TAB>      # Shows debug subcommands

# Complete file names
cat hel<TAB>     # Completes to hello.txt

# Complete config options
config set <TAB> # Shows available config options
```

---

## Configuration and Customization

### Configuration System

```bash
# View all configuration
config list

# Set options
config set autocorrect.enabled true
config set autosuggestions.enabled true
config set theme.name dark

# Get specific setting
config get theme.name

# Reset to defaults
config reset
```

### Shell Behavior

```bash
# Professional script settings
set -eu                    # Strict error handling
set -o pipefail           # Pipeline failure detection

# Development settings
set -xv                   # Trace and verbose mode

# Interactive settings
set -o emacs              # Emacs-style editing
# or
set -o vi                 # Vi-style editing

# Check all settings
set -o
```

### Creating Your Profile

```bash
# Create a custom startup script
cat > ~/.lusushrc << 'EOF'
# Lusush startup configuration

# Enable helpful options
set -h                    # Hash commands for speed
config set autocorrect.enabled true
config set autosuggestions.enabled true

# Set preferred theme
theme set modern

# Custom aliases
alias ll='ls -la'
alias la='ls -A'
alias l='ls -CF'

# Development shortcuts
alias debug-on='debug on 2'
alias debug-off='debug off'

# Welcome message
echo "Lusush loaded with custom configuration"
EOF
```

---

## Moving from Other Shells

### From Bash

Most bash scripts work directly in Lusush:

```bash
#!/usr/bin/env lusush
# This bash script works in Lusush
set -euo pipefail

function my_function() {
    local var="$1"
    echo "Processing: $var"
}

my_function "test"
```

**Bonus**: Add debugging to your existing bash scripts:

```bash
#!/usr/bin/env lusush
# Your existing bash script
set -euo pipefail

# Add debugging capability
debug on 1    # Enable basic debugging

# Your existing code works unchanged
# ... rest of script ...

debug off     # Clean exit
```

### From Zsh

```bash
# Zsh habits that work in Lusush
setopt() {
    # Zsh: setopt PIPE_FAIL
    # Lusush equivalent:
    set -o pipefail
}

# Most zsh scripts need minimal changes
```

### From Fish

```bash
# Fish users will appreciate Lusush's modern features
# Similar autosuggestions and syntax highlighting
# Plus the unique debugging capabilities
```

---

## Next Steps

### Learn Advanced Debugging

```bash
# Try breakpoints (when available)
debug break add myscript.sh 15
debug break list

# Use step-by-step execution
debug step
debug continue

# Performance analysis
debug profile on
# ... run performance-critical code ...
debug profile report
```

### Explore POSIX Compliance

```bash
# Learn all 24 shell options
set -o                    # See current settings

# Try strict POSIX mode
set -o posix

# Security features
set -o privileged        # Restricted shell mode

# See the complete reference
# Read: docs/SHELL_OPTIONS.md
```

### Customize Your Environment

```bash
# Explore themes
theme list
theme set <theme_name>

# Configure advanced features
config list              # See all options
config set key value     # Customize behavior

# Create custom functions
my_debug() {
    debug on 2
    "$@"                 # Run the passed command
    debug off
}

# Use it
my_debug ./myscript.sh
```

### Read More Documentation

- **[Debugger Guide](DEBUGGER_GUIDE.md)** - Complete debugging reference
- **[Shell Options](SHELL_OPTIONS.md)** - All 24 POSIX options explained
- **[Built-in Commands](BUILTIN_COMMANDS.md)** - Command reference
- **[Configuration Guide](CONFIGURATION.md)** - Advanced customization
- **[Security Features](SECURITY.md)** - Enterprise security options

---

## Common First-Time Questions

### Q: Why choose Lusush over bash/zsh/fish?
**A:** Lusush is the only shell with integrated debugging. If you ever write shell scripts, this capability alone makes development dramatically easier. Plus, it maintains full POSIX compliance.

### Q: Will my existing scripts work?
**A:** Yes! Lusush is POSIX-compliant, so standard shell scripts work without changes. You can gradually add debugging capabilities as needed.

### Q: Is it ready for production?
**A:** Yes. All core functionality and the debugger are production-ready. Lusush has been comprehensively tested and validated.

### Q: How do I learn the debugger?
**A:** Start with `debug help`, then try `debug on` and `debug vars` with simple commands. The [Debugger Guide](DEBUGGER_GUIDE.md) has comprehensive examples.

### Q: Can I use it as my daily shell?
**A:** Absolutely! Lusush works as a full replacement for bash, zsh, or other shells, with the added benefit of debugging capabilities when you need them.

---

## Getting Help

### Built-in Help

```bash
help              # General help
debug help        # Debugger help
config help       # Configuration help
theme help        # Theme help
```

### Documentation

- Complete documentation in the `docs/` directory
- All features are documented with examples
- Professional deployment guides available

### Community

- **Issues**: Report bugs or ask questions on GitHub
- **Discussions**: Community support and tips
- **Documentation**: Comprehensive guides for all features

---

## Welcome to Better Shell Scripting

You now have the only shell with integrated debugging capabilities. This unique feature will transform how you develop, troubleshoot, and understand shell scripts.

**Start experimenting with:**
1. Basic shell usage with helpful features
2. Simple debugging with `debug on` and `debug vars`
3. Customization with themes and configuration
4. Gradual migration of your existing scripts

**Remember:** Lusush does everything other shells do, plus interactive debugging that no other shell offers. You're now equipped with professional-grade shell scripting capabilities that simply aren't available anywhere else.

Happy scripting! 🚀