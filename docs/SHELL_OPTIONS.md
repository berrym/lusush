# Lusush Shell Options Reference

**Complete guide to all 24 POSIX shell options implemented in Lusush v1.3.0**

---

## Table of Contents

1. [Overview](#overview)
2. [Basic Shell Options](#basic-shell-options)
3. [Named Options (set -o)](#named-options-set--o)
4. [Option Combinations](#option-combinations)
5. [Professional Workflows](#professional-workflows)
6. [Migration Guide](#migration-guide)

---

## Overview

Lusush implements all 24 major POSIX shell options, providing complete compatibility with professional shell scripting requirements. Options can be set using traditional short flags (`-e`) or named options (`set -o errexit`).

### ✅ **Production Status**
All 24 options are production-ready and have been comprehensively tested. Each option maintains full POSIX compliance while integrating seamlessly with Lusush's advanced features.

### 📖 **Usage Patterns**
```bash
# Short form (POSIX standard)
set -e                    # Enable errexit
set +e                    # Disable errexit

# Long form (POSIX standard)
set -o errexit            # Enable errexit
set +o errexit            # Disable errexit

# Modern config interface (NEW in v1.3.0)
config set shell.errexit true    # Enable errexit
config set shell.errexit false   # Disable errexit
config get shell.errexit         # Check current state
config show shell                # Show all 24 options

# Check current status
set -o                    # Show all option states (traditional)
config show shell         # Show all options with descriptions (modern)
```

---

## Basic Shell Options

### `-a` (allexport)
**Automatically export all variables**

```bash
# Traditional POSIX syntax
set -a                   # Enable automatic export
VAR="hello"              # Automatically exported
env | grep VAR           # Shows: VAR=hello
set +a                   # Disable automatic export

# Modern config syntax
config set shell.allexport true    # Enable automatic export
VAR="hello"              # Automatically exported
config set shell.allexport false   # Disable automatic export
```

**Use Cases:**
- Environment setup scripts
- Configuration file processing
- Docker container initialization

---

### `-b` (notify)
**Asynchronous background job notification**

```bash
# Traditional POSIX syntax
set -b                   # Enable background job notification
sleep 10 &               # Job runs in background
set +b                   # Disable notification

# Modern config syntax
config set shell.notify true       # Enable background job notification
sleep 10 &               # Job runs in background
config set shell.notify false      # Disable notification
```

**Use Cases:**
- Interactive sessions with background tasks
- Long-running process monitoring
- DevOps automation workflows

---

### `-C` (noclobber)
**File overwrite protection**

```bash
# Traditional POSIX syntax
set -C                   # Enable file protection
echo "content" > file.txt
echo "new" > file.txt    # Error: cannot overwrite
set +C                   # Disable protection

# Modern config syntax
config set shell.noclobber true    # Enable file protection
echo "content" > file.txt
echo "new" > file.txt    # Error: cannot overwrite
config set shell.noclobber false   # Disable protection

# Override protection when needed (both syntaxes)
echo "override" >| file.txt   # Force overwrite with >|
```

**Use Cases:**
- Preventing accidental file overwrites
- Safe scripting environments
- Production data protection

---

### `-e` (errexit)
**Exit immediately on command failure**

```bash
# Traditional POSIX syntax
set -e                   # Enable strict error handling
false                    # Script exits here
echo "Never reached"     # Not executed
set +e                   # Disable for conditional operations

# Modern config syntax
config set shell.errexit true      # Enable strict error handling
false                    # Script exits here
echo "Never reached"     # Not executed
config set shell.errexit false     # Disable for conditional operations

# Check current state
config get shell.errexit           # Returns: true/false
set -o | grep errexit               # Shows current state
```

**Use Cases:**
- Production deployment scripts
- Critical automation tasks
- Safe scripting practices

---

### `-f` (noglob)
**Disable pathname expansion**

```bash
# Enable glob protection
set -f
echo *.txt               # Prints literal "*.txt"

# Disable protection
set +f
echo *.txt               # Expands to matching files
```

**Use Cases:**
- Processing user input safely
- Avoiding unintended glob expansion
- Security-sensitive operations

---

### `-h` (hashall)
**Enable command path hashing**

```bash
# Enable command caching (default)
set -h
which ls                 # Path cached for faster access

# Disable caching
set +h
```

**Use Cases:**
- Performance optimization
- Dynamic PATH environments
- Standard shell behavior

---

### `-m` (monitor)
**Enable job control**

```bash
# Enable job control
set -m
sleep 100 &              # Background job
jobs                     # Show job status

# Disable job control
set +m
```

**Use Cases:**
- Interactive shell sessions
- Process management
- Background task coordination

---

### `-n` (noexec)
**Syntax check mode - read but don't execute**

```bash
# Check script syntax without execution
set -n
echo "This is not executed"
ls /nonexistent          # Syntax checked but not run

# Resume normal execution
set +n
```

**Use Cases:**
- Script validation before deployment
- Syntax checking in CI/CD pipelines
- Development workflow verification

---

### `-t` (onecmd)
**Exit after executing one command**

```bash
# Enable single command mode
set -t
echo "First command"     # Executed
echo "Second command"    # Not reached - shell exits
```

**Use Cases:**
- Automation scripts
- Single-purpose shell invocations
- Testing and validation

---

### `-u` (nounset)
**Treat unset variables as error**

```bash
# Enable unset variable detection
set -u
echo $UNDEFINED_VAR      # Error: variable not set

# Disable for optional variables
set +u
echo $OPTIONAL_VAR       # Prints empty string
```

**Use Cases:**
- Robust script development
- Preventing typos in variable names
- Production script safety

---

### `-v` (verbose)
**Display input lines as read**

```bash
# Enable verbose mode
set -v
echo "Hello"             # Shows: echo "Hello"
                         # Then: Hello

# Disable verbose mode
set +v
```

**Use Cases:**
- Script debugging
- Understanding command execution
- Educational demonstrations

---

### `-x` (xtrace)
**Trace command execution**

```bash
# Enable execution tracing
set -x
echo "Debug me"          # Shows: + echo "Debug me"
                         # Then: Debug me

# Disable tracing
set +x
```

**Use Cases:**
- Script debugging
- Troubleshooting automation
- Understanding execution flow

---

## Named Options (set -o)

### `ignoreeof`
**Interactive EOF handling**

```bash
# Enable EOF protection
set -o ignoreeof
# Ctrl+D won't exit shell immediately

# Disable protection
set +o ignoreeof
```

**Use Cases:**
- Interactive shell protection
- Preventing accidental exits
- User-friendly shell behavior

---

### `nolog`
**Function definition history control**

```bash
# Disable function logging
set -o nolog
function test_func() { echo "test"; }  # Not logged

# Enable function logging
set +o nolog
```

**Use Cases:**
- Clean history management
- Security in sensitive environments
- Reduced history clutter

---

### `emacs` / `vi`
**Command line editing modes**

```bash
# Enable emacs editing (default)
set -o emacs
# Emacs-style key bindings active

# Switch to vi editing
set -o vi
# Vi-style key bindings active
```

**Note:** These modes are mutually exclusive. Setting one automatically disables the other.

**Use Cases:**
- Editor preference matching
- Consistent editing experience
- Professional workflow integration

---

### `posix`
**Strict POSIX compliance mode**

```bash
# Enable strict POSIX behavior
set -o posix
# Enhanced compliance checking
# Stricter function name validation

# Disable for Lusush extensions
set +o posix
```

**Use Cases:**
- POSIX standard compliance
- Portable script development
- Enterprise requirements

---

### `pipefail`
**Pipeline failure detection**

```bash
# Enable pipeline failure detection
set -o pipefail
false | echo "success"   # Pipeline fails due to false

# Standard pipeline behavior
set +o pipefail
false | echo "success"   # Pipeline succeeds (last command)
```

**Use Cases:**
- Robust pipeline scripting
- Error detection in complex commands
- Production automation safety

---

### `histexpand`
**History expansion control**

```bash
# Enable history expansion (default)
set -o histexpand
!!                       # Repeat last command

# Disable history expansion
set +o histexpand
echo "!! literal"        # Prints literally
```

**Use Cases:**
- Interactive command repetition
- History-based workflows
- Security in automated scripts

---

### `history`
**Command history recording**

```bash
# Enable history recording (default)
set -o history
echo "recorded"          # Added to history

# Disable history recording
set +o history
echo "not recorded"      # Not added to history
```

**Use Cases:**
- Session management
- Security in sensitive operations
- History file control

---

### `interactive-comments`
**Interactive comment support**

```bash
# Enable interactive comments (default)
set -o interactive-comments
echo "test" # This comment is ignored

# Disable comments
set +o interactive-comments
```

**Use Cases:**
- Interactive documentation
- Command annotation
- Script portability

---

### `braceexpand`
**Brace expansion control**

```bash
# Enable brace expansion (default)
set -o braceexpand
echo {a,b,c}             # Expands to: a b c

# Disable brace expansion
set +o braceexpand
echo {a,b,c}             # Prints literally: {a,b,c}
```

**Use Cases:**
- Pattern generation
- File operations
- Sequence creation

---

### `physical`
**Physical directory path resolution**

```bash
# Enable physical paths
set -o physical
# cd follows physical directory structure
# PWD shows actual paths, not symlinks

# Use logical paths (default)
set +o physical
```

**Use Cases:**
- Symlink-aware navigation
- Security in shared environments
- Path resolution control

---

### `privileged`
**Security restrictions for restricted shell**

```bash
# Enable privileged mode
set -o privileged
# Restricted command execution
# Limited environment access
# Enhanced security controls

# Disable restrictions
set +o privileged
```

**Use Cases:**
- Multi-tenant environments
- Sandboxed execution
- Enterprise security requirements

---

## Option Combinations

### Strict Scripting Mode
```bash
# Ultimate safety configuration
set -euo pipefail
# -e: Exit on any error
# -u: Error on unset variables  
# -o pipefail: Pipeline failure detection
```

### Debug Mode
```bash
# Comprehensive debugging
set -xv
# -x: Trace execution
# -v: Show input lines
```

### Production Safety
```bash
# Safe production environment
set -eCu
# -e: Exit on error
# -C: Protect against overwrites
# -u: Catch unset variables
```

### Development Mode
```bash
# Development-friendly settings
set -xvh
# -x: Trace execution
# -v: Verbose input
# -h: Hash commands for speed
```

---

## Professional Workflows

### CI/CD Pipeline Script
```bash
#!/usr/bin/env lusush
# Production deployment script

# Strict error handling
set -euo pipefail

# Enable background job notification
set -b

# Protect against overwrites
set -C

# Script logic here
deploy_application() {
    echo "Deploying with strict error handling"
    # Deployment commands
}

deploy_application
```

### Interactive Development
```bash
#!/usr/bin/env lusush
# Development script with debugging

# Enable debugging features
set -xv

# Enable job control for background tasks
set -m

# Development logic here
test_feature() {
    echo "Testing with full visibility"
    # Test commands
}

test_feature
```

### Security-Conscious Script
```bash
#!/usr/bin/env lusush
# Security-focused script

# Enable privileged mode
set -o privileged

# Strict POSIX compliance
set -o posix

# Error handling
set -eu

# Secure operations here
secure_operation() {
    echo "Running in restricted mode"
    # Secure commands only
}

secure_operation
```

---

## Migration Guide

### From Bash
```bash
# Bash script
#!/bin/bash
set -euo pipefail

# Lusush equivalent
#!/usr/bin/env lusush
set -euo pipefail
# Works identically
```

### From Zsh
```bash
# Zsh script with options
#!/bin/zsh
setopt PIPE_FAIL
setopt ERR_EXIT

# Lusush equivalent
#!/usr/bin/env lusush
set -o pipefail
set -o errexit
```

### From POSIX sh
```bash
# Standard POSIX script
#!/bin/sh
set -e

# Lusush with enhancements
#!/usr/bin/env lusush
set -e
# Plus debugging capabilities:
debug on
```

---

## Quick Reference

### All 24 Options Summary

| Short | Long | Description |
|-------|------|-------------|
| `-a` | `allexport` | Automatic variable export |
| `-b` | `notify` | Background job notification |
| `-C` | `noclobber` | File overwrite protection |
| `-e` | `errexit` | Exit on command failure |
| `-f` | `noglob` | Disable pathname expansion |
| `-h` | `hashall` | Command path hashing |
| `-m` | `monitor` | Job control mode |
| `-n` | `noexec` | Syntax check only |
| `-t` | `onecmd` | Exit after one command |
| `-u` | `nounset` | Error on unset variables |
| `-v` | `verbose` | Display input lines |
| `-x` | `xtrace` | Trace command execution |
| | `ignoreeof` | Interactive EOF handling |
| | `nolog` | Function definition history control |
| | `emacs` | Emacs-style editing |
| | `vi` | Vi-style editing |
| | `posix` | Strict POSIX compliance |
| | `pipefail` | Pipeline failure detection |
| | `histexpand` | History expansion |
| | `history` | Command history recording |
| | `interactive-comments` | Comment support |
| | `braceexpand` | Brace expansion |
| | `physical` | Physical path resolution |
| | `privileged` | Security restrictions |

### Common Combinations

```bash
set -eu              # Strict error handling
set -euo pipefail    # Ultimate safety
set -xv              # Full debugging
set -h               # Performance optimization
set -C               # File protection
set -o posix         # POSIX compliance
set -o privileged    # Security mode
```

---

## Modern Configuration System Integration

### Dual Interface Support

Lusush uniquely provides **two equivalent interfaces** for shell options:

```bash
# Traditional POSIX (unchanged compatibility)
set -e                           # Enable errexit
set -o errexit                   # Enable errexit (long form)
set +o errexit                   # Disable errexit
set -o                          # Show all option states

# Modern config system (NEW in v1.3.0)
config set shell.errexit true   # Enable errexit
config set shell.errexit false  # Disable errexit  
config get shell.errexit        # Check current state
config show shell               # Show all 24 options with descriptions
```

### Perfect Bidirectional Synchronization

Both interfaces stay perfectly synchronized:

```bash
# Set via config system
config set shell.xtrace true
set -o | grep xtrace            # Shows: set -o xtrace

# Set via POSIX  
set -o verbose
config get shell.verbose       # Returns: true

# Mixed usage works seamlessly
config set shell.errexit true  # Modern interface
set +e                         # Traditional interface
config get shell.errexit      # Returns: false
```

### Discoverability Advantage

The modern config interface provides superior discoverability:

```bash
config show shell               # Lists all 24 options with descriptions
# Output shows:
#   shell.errexit = false  # Exit on command failure (set -e)
#   shell.xtrace = false   # Trace command execution (set -x)
#   shell.nounset = false  # Error on unset variables (set -u)
#   ... all 24 options with helpful descriptions
```

### Migration-Friendly

Perfect for gradual adoption:

```bash
#!/usr/bin/env lusush
# Existing scripts work unchanged
set -euo pipefail

# New development can use modern syntax
config set shell.errexit true
config set shell.nounset true  
config set shell.pipefail true

# Or mix both approaches as needed
set -x                         # Traditional
config set shell.verbose true  # Modern
```

## Conclusion

Lusush's comprehensive implementation of all 24 POSIX shell options provides complete compatibility with professional shell scripting requirements while offering a modern configuration interface that doesn't exist in any other shell.

The **unique dual interface approach** makes Lusush suitable for:
- **Existing workflows** - All traditional POSIX syntax works unchanged
- **Modern development** - Discoverable config system with descriptive help
- **Mixed environments** - Both interfaces work together seamlessly
- **Educational use** - `config show shell` reveals all available options
- **Enterprise deployment** - Centralized config management capabilities

The combination of traditional POSIX compliance, modern configuration management, and advanced features like the integrated debugger makes Lusush the most complete and professional shell environment available.

**For comprehensive configuration documentation, see [CONFIG_SYSTEM.md](CONFIG_SYSTEM.md)**