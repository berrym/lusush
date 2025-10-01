# Lusush Configuration System Guide

**Complete guide to Lusush's modern, enterprise-grade configuration system**

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Shell Options Integration](#shell-options-integration)
4. [Configuration Sections](#configuration-sections)
5. [Commands Reference](#commands-reference)
6. [Advanced Features](#advanced-features)
7. [Enterprise Usage](#enterprise-usage)
8. [Migration Guide](#migration-guide)
9. [Best Practices](#best-practices)
10. [Troubleshooting](#troubleshooting)

---

## Overview

Lusush features a modern configuration system that provides:

- **Runtime configuration changes** with immediate effect
- **Dual interface support** - modern config system + traditional POSIX compatibility
- **Structured organization** with sections and comprehensive validation
- **Shell options integration** - all 24 POSIX options accessible via modern interface
- **Enterprise deployment** support with centralized management
- **Type checking and error handling** for reliability

### Why This Matters

Traditional shells require:
- Manual file editing for configuration changes
- Shell restarts to apply settings
- Memorizing cryptic option names
- No discovery mechanism for available options

Lusush provides:
- **Instant configuration changes**: `config set shell.errexit true`
- **Discoverability**: `config show shell` reveals all 24 shell options
- **Modern syntax**: `config set completion.enabled true`
- **Backward compatibility**: `set -e` still works exactly as before

---

## Quick Start

### Basic Usage
```bash
# Show all configuration
config show

# Show specific section
config show shell
config show completion  
config show prompt

# Set values (takes effect immediately)
config set shell.errexit true
config set completion.enabled true
config set prompt.theme dark

# Get values
config get shell.errexit
config get completion.enabled
```

### Shell Options Integration
```bash
# Modern discoverable interface
config show shell                    # List all 24 POSIX options
config set shell.errexit on          # Set with modern syntax
config set shell.xtrace true         # Enable command tracing
config get shell.posix               # Check POSIX compliance mode

# Traditional POSIX (still works perfectly)
set -e                               # Same as config set shell.errexit true
set -o xtrace                        # Same as config set shell.xtrace true
set +o errexit                       # Same as config set shell.errexit false

# Both interfaces stay synchronized
config set shell.verbose true       # Enable verbose mode
set -o                              # Shows verbose as enabled
```

---

## Shell Options Integration

Lusush uniquely provides **dual interfaces** for shell options - you can use either modern config syntax or traditional POSIX syntax. Both stay perfectly synchronized.

### Complete Shell Options Reference

| Config Name | POSIX Equivalent | Description |
|-------------|------------------|-------------|
| `shell.errexit` | `set -e` | Exit on command failure |
| `shell.xtrace` | `set -x` | Trace command execution |
| `shell.noexec` | `set -n` | Syntax check only |
| `shell.nounset` | `set -u` | Error on unset variables |
| `shell.verbose` | `set -v` | Display input lines |
| `shell.noglob` | `set -f` | Disable pathname expansion |
| `shell.hashall` | `set -h` | Command path hashing |
| `shell.monitor` | `set -m` | Job control mode |
| `shell.allexport` | `set -a` | Auto export variables |
| `shell.noclobber` | `set -C` | File overwrite protection |
| `shell.onecmd` | `set -t` | Exit after one command |
| `shell.notify` | `set -b` | Background job notification |
| `shell.ignoreeof` | `set -o ignoreeof` | Prevent exit on EOF |
| `shell.nolog` | `set -o nolog` | Function history control |
| `shell.emacs` | `set -o emacs` | Emacs-style editing |
| `shell.vi` | `set -o vi` | Vi-style editing |
| `shell.posix` | `set -o posix` | Strict POSIX compliance |
| `shell.pipefail` | `set -o pipefail` | Pipeline failure detection |
| `shell.histexpand` | `set -o histexpand` | History expansion |
| `shell.history` | `set -o history` | Command history recording |
| `shell.interactive-comments` | `set -o interactive-comments` | Interactive comments |
| `shell.braceexpand` | `set -o braceexpand` | Brace expansion |
| `shell.physical` | `set -o physical` | Physical directory paths |
| `shell.privileged` | `set -o privileged` | Security restrictions |

### Shell Options Examples

```bash
# Strict scripting mode (modern syntax)
config set shell.errexit true       # Exit on any error
config set shell.nounset true       # Error on unset variables
config set shell.pipefail true      # Pipeline failure detection

# Debug mode
config set shell.xtrace true        # Trace execution
config set shell.verbose true       # Show input lines

# Security mode
config set shell.privileged true    # Enable restrictions
config set shell.posix true         # Strict POSIX compliance

# Editing preferences
config set shell.vi true            # Switch to vi editing mode
config get shell.emacs              # Returns: false (mutually exclusive)

# File safety
config set shell.noclobber true     # Protect against overwrites
echo "test" > file.txt               # Creates file
echo "new" > file.txt                # Error: cannot overwrite
echo "override" >| file.txt          # Force overwrite with >|

# Job control
config set shell.monitor true       # Enable job control
config set shell.notify true        # Background job notifications
```

### Bidirectional Synchronization

The shell options integration maintains perfect synchronization:

```bash
# Set via config system
config set shell.errexit true
set -o                              # Shows errexit as enabled

# Set via POSIX
set -o xtrace  
config get shell.xtrace             # Returns: true

# Mixed usage works perfectly
config set shell.verbose true       # Modern interface
set +v                              # Traditional interface  
config get shell.verbose            # Returns: false
```

---

## Configuration Sections

### [history] - Command History Management
```bash
# View history settings
config show history

# Available options
config set history.enabled true          # Enable command history (default: true)
config set history.size 2000             # Max entries (default: 1000)
config set history.no_dups true          # Remove duplicates (default: true)
config set history.timestamps false      # Add timestamps (default: false)
config set history.file ~/.lusush_history # History file path
```

### [completion] - Tab Completion System
```bash
# View completion settings
config show completion

# Available options
config set completion.enabled true       # Enable tab completion (default: true)
config set completion.fuzzy true         # Fuzzy matching (default: true)
config set completion.threshold 60       # Fuzzy threshold 0-100 (default: 60)
config set completion.case_sensitive false # Case sensitivity (default: false)
config set completion.show_all false     # Show all matches (default: false)
config set completion.hints true         # Input hints (default: false)
```

### [prompt] - Prompt and Theme Configuration
```bash
# View prompt settings
config show prompt

# Available options
config set prompt.style git             # Style: normal/color/fancy/pro/git
config set prompt.theme dark            # Theme name
config set prompt.git_enabled true      # Git integration (default: true)
config set prompt.git_cache_timeout 5   # Git cache timeout seconds
config set prompt.theme_name corporate  # Active theme
config set prompt.theme_auto_detect_colors true # Auto-detect colors
config set prompt.theme_show_right_prompt true  # Right-side prompt
```

### [behavior] - Shell Behavior Settings  
```bash
# View behavior settings
config show behavior

# Available options
config set behavior.auto_cd false                 # Auto-cd to directories
config set behavior.spell_correction true         # Command spell checking (default: true)
config set behavior.autocorrect_max_suggestions 3 # Max suggestions (1-5)
config set behavior.autocorrect_threshold 40      # Similarity threshold (0-100)
config set behavior.autocorrect_interactive true  # Interactive prompts
config set behavior.confirm_exit false            # Confirm before exiting
config set behavior.multiline_mode true           # Multiline editing
config set behavior.colors_enabled true           # Color output
config set behavior.verbose_errors false          # Verbose error messages
config set behavior.debug_mode false              # Debug mode
```

### [network] - Network and SSH Configuration
```bash
# View network settings
config show network

# Available options
config set network.ssh_completion true   # SSH host completion (default: true)
config set network.cloud_discovery false # Cloud host discovery
config set network.cache_ssh_hosts true  # Cache hosts for performance
config set network.cache_timeout_minutes 5 # Cache timeout
config set network.show_remote_context true # Remote context in prompt
config set network.auto_detect_cloud true # Auto-detect cloud environment
config set network.max_completion_hosts 50 # Max hosts in completion
```

### [scripts] - Script Execution Control
```bash
# View script settings
config show scripts

# Available options
config set scripts.execution true        # Enable script execution (default: true)
```

### [shell] - POSIX Shell Options (Modern Interface)
```bash
# View all shell options
config show shell

# All 24 POSIX options available - see Shell Options Integration section above
# Examples:
config set shell.errexit true           # Exit on error
config set shell.xtrace true            # Trace execution  
config set shell.posix true             # POSIX compliance
config set shell.privileged true        # Security restrictions
```

---

## Commands Reference

### config show
**Display configuration values**

```bash
config show                    # Show all configuration
config show history            # Show history section only
config show completion         # Show completion section
config show prompt             # Show prompt section
config show behavior           # Show behavior section
config show network            # Show network section
config show scripts            # Show scripts section
config show shell              # Show shell options (all 24 POSIX options)
```

### config set
**Set configuration values (immediate effect)**

```bash
config set <key> <value>

# Examples
config set history.size 2000
config set completion.enabled true
config set shell.errexit on
config set prompt.theme dark

# Accepted boolean values: true/false, on/off, 1/0
config set shell.verbose true
config set shell.verbose on
config set shell.verbose 1
```

### config get
**Get specific configuration values**

```bash
config get <key>

# Examples
config get history.size          # Returns: 2000
config get shell.errexit         # Returns: true
config get completion.enabled    # Returns: true
```

### config reload
**Reload configuration files**

```bash
config reload
# Reloads configuration from files
```

---

## Advanced Features

### Configuration File Support

Lusush supports configuration files in standard locations:

```bash
# System-wide configuration
/etc/lusush/lusushrc

# User configuration  
~/.config/lusush/lusushrc
~/.lusushrc

# Project-specific configuration
./.lusush/config
```

### Configuration File Format

```ini
[history]
enabled = true
size = 2000
no_dups = true

[completion]
enabled = true
fuzzy = true
threshold = 60

[shell]
# Shell options can be configured in files too
errexit = true
xtrace = false
posix = false

[prompt]
style = git
theme = dark
git_enabled = true
```

### Runtime Integration

Configuration changes take effect immediately:

```bash
# Enable command tracing instantly
config set shell.xtrace true
echo "This will be traced"    # Shows: + echo "This will be traced"

# Change theme instantly
config set prompt.theme dark
# Prompt changes immediately
```

---

## Enterprise Usage

### Centralized Configuration Management

```bash
# Deploy organization-wide settings
sudo tee /etc/lusush/lusushrc << 'EOF'
[shell]
errexit = true
nounset = true
posix = true

[behavior]
spell_correction = true
confirm_exit = true

[network]
ssh_completion = true
cache_ssh_hosts = true
EOF
```

### Department-Specific Configurations

```bash
# Development team settings
[shell]
xtrace = false
verbose = false
debug_mode = false

[completion]
fuzzy = true
hints = true

# Production team settings  
[shell]
errexit = true
nounset = true
privileged = true

[behavior]
confirm_exit = true
verbose_errors = true
```

### Security Configurations

```bash
# High-security environment
config set shell.privileged true    # Enable security restrictions
config set shell.posix true         # Strict POSIX compliance
config set shell.errexit true       # Fail on errors
config set shell.nounset true       # Catch unset variables

# Verify security settings
config show shell | grep -E "(privileged|posix|errexit|nounset)"
```

---

## Migration Guide

### From Bash
```bash
# Bash script with options
#!/bin/bash
set -euo pipefail

# Lusush modern equivalent
#!/usr/bin/env lusush
config set shell.errexit true
config set shell.nounset true  
config set shell.pipefail true

# Or keep traditional syntax (works identically)
#!/usr/bin/env lusush
set -euo pipefail
```

### From Other Shells
```bash
# Zsh setopt commands
setopt PIPE_FAIL
setopt ERR_EXIT

# Lusush config equivalents
config set shell.pipefail true
config set shell.errexit true

# Or traditional POSIX syntax
set -o pipefail
set -o errexit
```

### Discovering Available Options

```bash
# See what's available in your installation
config show                    # All sections and options
config show shell             # All 24 shell options
config show completion        # All completion options
config show prompt            # All prompt options

# This reveals the full capabilities of your Lusush installation
```

---

## Best Practices

### 1. Use Modern Syntax for New Development
```bash
# Preferred for new scripts - discoverable and self-documenting
config set shell.errexit true
config set shell.nounset true
config set shell.pipefail true

# Rather than cryptic traditional syntax
# set -euo pipefail
```

### 2. Document Configuration Changes
```bash
#!/usr/bin/env lusush
# Enable strict error handling for production deployment
config set shell.errexit true   # Exit on any command failure
config set shell.nounset true   # Error on undefined variables
config set shell.pipefail true  # Detect pipeline failures

# Enable debugging features for development
config set shell.xtrace true    # Trace command execution
config set shell.verbose true   # Show input lines
```

### 3. Verify Configuration State
```bash
# At start of critical scripts, verify expected configuration
verify_config() {
    if [ "$(config get shell.errexit)" != "true" ]; then
        echo "ERROR: errexit must be enabled for this script"
        exit 1
    fi
}
```

### 4. Use Section-Based Organization
```bash
# Configure by functional area
config show completion     # Review completion settings
config show prompt         # Review prompt settings  
config show shell          # Review shell behavior
```

### 5. Environment-Specific Configurations
```bash
# Development environment
if [ "$ENVIRONMENT" = "development" ]; then
    config set shell.xtrace true
    config set shell.verbose true
    config set debug_mode true
fi

# Production environment
if [ "$ENVIRONMENT" = "production" ]; then
    config set shell.errexit true
    config set shell.privileged true
    config set shell.posix true
fi
```

---

## Troubleshooting

### Common Issues

**Q: Configuration changes don't take effect**
```bash
# Verify the setting was applied
config get shell.errexit

# Check for typos in option names
config show shell | grep errexit

# Use exact option names as shown in config show
```

**Q: Shell options not working as expected**
```bash
# Check current shell option state
config show shell

# Verify both modern and traditional interfaces
config get shell.errexit
set -o | grep errexit

# Both should show the same state
```

**Q: Unknown configuration key errors**
```bash
# See available options
config show                    # All available options
config show shell             # All shell options

# Check spelling
config show | grep -i "option_name"
```

**Q: Tab completion not working with config command**
```bash
# Verify completion is enabled
config get completion.enabled

# Enable if disabled
config set completion.enabled true
```

### Debugging Configuration

```bash
# Enable debug mode for detailed information
config set behavior.debug_mode true

# Check configuration file loading
config reload

# Verify configuration state
config show > current_config.txt
```

### Getting Help

```bash
# Show command usage
config

# Show available sections
config show

# Show specific section options
config show shell
config show completion
config show prompt
```

---

## Backward Compatibility

Lusush maintains full backward compatibility with the old underscore naming convention. When you use an old name, you'll get a deprecation warning but the command will work:

```bash
# Old naming (deprecated but still works)
config set completion_enabled true
# Warning: 'completion_enabled' is deprecated, use 'completion.enabled' instead
# Set completion.enabled = true

config get history_size
# Warning: 'history_size' is deprecated, use 'history.size' instead
# 1000

# New naming (recommended)
config set completion.enabled true        # Clean, no warnings
config get history.size                   # Clean, no warnings
```

### Complete Legacy Mapping

| Old Name (Deprecated) | New Name (Recommended) |
|-----------------------|------------------------|
| `history_enabled` | `history.enabled` |
| `history_size` | `history.size` |
| `completion_enabled` | `completion.enabled` |
| `fuzzy_completion` | `completion.fuzzy` |
| `prompt_style` | `prompt.style` |
| `git_prompt_enabled` | `prompt.git_enabled` |
| `auto_cd` | `behavior.auto_cd` |
| `spell_correction` | `behavior.spell_correction` |
| `ssh_completion_enabled` | `network.ssh_completion` |
| `script_execution` | `scripts.execution` |

**Migration Strategy:**
1. **Immediate**: All old names continue working with deprecation warnings
2. **Recommended**: Update scripts to use new dot notation for cleaner configuration
3. **Future**: Old names will be supported indefinitely for compatibility

---

## Conclusion

Lusush's configuration system provides a modern, discoverable interface while maintaining complete backward compatibility with traditional POSIX shell syntax. Key benefits:

- **Modern UX**: `config set shell.errexit true` vs cryptic `set -e`
- **Discoverability**: `config show shell` reveals all 24 options
- **Perfect compatibility**: Traditional `set -e` still works exactly as before
- **Bidirectional sync**: Both interfaces stay synchronized
- **Enterprise ready**: Centralized management and security features

The shell options integration is particularly powerful - you get modern configuration management for all 24 POSIX options while preserving complete backward compatibility for existing scripts and workflows.

**Key Benefits:**
- **Modern Namespacing**: Clean `section.option` syntax (e.g., `completion.enabled`)
- **Full Compatibility**: Old underscore names still work with deprecation warnings
- **Professional Organization**: Logical grouping by functional area
- **Enterprise Ready**: Centralized management with consistent naming
- **Migration Friendly**: Gradual adoption path with zero breaking changes

Whether you're developing new scripts (use modern syntax) or maintaining existing ones (traditional syntax still works perfectly), Lusush's configuration system adapts to your workflow while providing enterprise-grade management capabilities.

---

**For more information:**
- [Shell Options Reference](SHELL_OPTIONS.md) - Complete POSIX options documentation
- [Built-in Commands](BUILTIN_COMMANDS.md) - All built-in command documentation  
- [Getting Started Guide](GETTING_STARTED.md) - Beginner tutorial
- [User Guide](USER_GUIDE.md) - Comprehensive user documentation