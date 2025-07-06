# Shell Script Configuration System

## Overview

LUSUSH provides a hybrid configuration system that combines modern structured configuration with traditional shell script execution. This design allows advanced users to migrate from bash/zsh while maintaining familiar workflow patterns, while also providing the benefits of a modern configuration system.

## Configuration Architecture

### Dual Configuration System

LUSUSH supports two complementary configuration approaches:

1. **Modern Config System** - Structured INI-style configuration (`.lusushrc`)
2. **Traditional Shell Scripts** - Executable shell scripts for advanced users

Both systems work together seamlessly, allowing users to choose their preferred configuration style or combine both approaches.

### Configuration File Hierarchy

#### For Login Shells

1. **System Profile** (`/etc/profile`) - System-wide POSIX profile
2. **User Profile** (`~/.profile`) - User POSIX profile (executed first)
3. **System LUSUSH Config** (`/etc/lusush/lusushrc`) - System-wide modern config
4. **User LUSUSH Config** (`~/.lusushrc`) - User modern config
5. **LUSUSH Login Script** (`~/.lusush_login`) - User login script
6. **LUSUSH RC Script** (`~/.lusushrc.sh`) - User interactive script

#### For Interactive (Non-Login) Shells

1. **System LUSUSH Config** (`/etc/lusush/lusushrc`) - System-wide modern config
2. **User LUSUSH Config** (`~/.lusushrc`) - User modern config
3. **LUSUSH RC Script** (`~/.lusushrc.sh`) - User interactive script

#### For Logout

1. **LUSUSH Logout Script** (`~/.lusush_logout`) - User logout cleanup

## Traditional Shell Scripts

### Supported Script Files

| File | Purpose | When Executed |
|------|---------|---------------|
| `~/.profile` | POSIX-compliant profile | Login shells |
| `~/.lusush_login` | LUSUSH login initialization | Login shells |
| `~/.lusushrc.sh` | Interactive shell configuration | Interactive shells |
| `~/.lusush_logout` | Logout cleanup | Login shell exit |

### Script Execution Control

Scripts can be enabled/disabled programmatically:

```bash
# Enable script execution (default)
config set script_execution true

# Disable script execution
config set script_execution false
```

### Script Features

#### Full Shell Command Support

Traditional scripts support all LUSUSH shell features:

```bash
#!/usr/bin/env lusush
# Example .lusushrc.sh

# Set shell options
set -o emacs
set +o noclobber

# Define aliases
alias ll='ls -alF'
alias la='ls -A'
alias ..='cd ..'

# Define functions
mkcd() {
    mkdir -p "$1" && cd "$1"
}

# Set environment variables
export EDITOR=vim
export PAGER=less
```

#### Modern Config Integration

Scripts can use modern config commands:

```bash
#!/usr/bin/env lusush
# Hybrid configuration example

# Traditional shell commands
alias git-log='git log --oneline --graph'
export DEVELOPMENT_MODE=1

# Modern config commands
config set completion_enabled true
config set theme_name corporate
theme set dark
```

#### Conditional Configuration

Scripts support conditional logic:

```bash
#!/usr/bin/env lusush
# Conditional configuration

# Set theme based on terminal
if [ "$TERM_PROGRAM" = "iTerm.app" ]; then
    theme set corporate
else
    theme set minimal
fi

# Load development tools in dev environment
if [ "$ENVIRONMENT" = "development" ]; then
    export DEBUG=1
    config set autocorrect_interactive true
fi
```

### Example Script Files

#### ~/.profile (POSIX Profile)

```bash
#!/bin/sh
# POSIX-compliant profile script

# Set PATH
export PATH="$HOME/bin:$HOME/.local/bin:$PATH"

# Set default editor
export EDITOR=vi
export PAGER=less

# Set locale
export LANG=en_US.UTF-8

# Load system profile
if [ -f /etc/profile ]; then
    . /etc/profile
fi

# Welcome message for login shells
if [ -n "$PS1" ]; then
    echo "Welcome to LUSUSH Shell"
fi
```

#### ~/.lusushrc.sh (Interactive Configuration)

```bash
#!/usr/bin/env lusush
# LUSUSH interactive shell configuration

# Modern aliases
alias projects='cd ~/Development'
alias serve='python -m http.server'

# Git aliases
alias gs='git status'
alias ga='git add'
alias gc='git commit'

# Functions
extract() {
    case "$1" in
        *.tar.gz) tar xzf "$1" ;;
        *.zip) unzip "$1" ;;
        *) echo "Unsupported format" ;;
    esac
}

# Configure modern features
config set completion_enabled true
config set fuzzy_completion true
config set spell_correction true
theme set corporate

# Load project-specific config
if [ -f "$PWD/.lusushrc.local" ]; then
    . "$PWD/.lusushrc.local"
fi
```

#### ~/.lusush_login (Login Initialization)

```bash
#!/usr/bin/env lusush
# LUSUSH login shell initialization

echo "Login session started: $(date)"

# Set login-specific environment
export LUSUSH_LOGIN_TIME=$(date +%s)
export LUSUSH_SESSION_ID="lusush_$$_$(date +%s)"

# Configure for login session
theme set corporate
config set history_size 50000
config set history_timestamps true

# Start SSH agent
if command -v ssh-agent >/dev/null 2>&1; then
    eval $(ssh-agent -s) >/dev/null
fi

# Display system info
echo "System: $(uname -s) $(uname -r)"
echo "Load: $(uptime | cut -d',' -f3-)"
```

#### ~/.lusush_logout (Logout Cleanup)

```bash
#!/usr/bin/env lusush
# LUSUSH logout cleanup

echo "Logout: $(date)"

# Save history
history -w

# Clean up SSH agent
if [ -n "$SSH_AGENT_PID" ]; then
    ssh-agent -k >/dev/null 2>&1
fi

# Clean up temporary files
rm -rf ~/.lusush_tmp

# Log session stats
if [ -n "$LUSUSH_LOGIN_TIME" ]; then
    duration=$(($(date +%s) - $LUSUSH_LOGIN_TIME))
    echo "Session duration: ${duration}s"
fi
```

## Integration with Modern Config

### Coexistence Strategy

Both configuration systems work together:

1. **Modern config** provides structured, validated settings
2. **Shell scripts** provide flexible, programmatic configuration
3. **Settings can be mixed** - use both approaches as needed
4. **Scripts can read/modify config** using `config` commands

### Configuration Precedence

1. System-wide settings (lowest priority)
2. User `.lusushrc` file settings
3. Shell script modifications (highest priority)

### Best Practices

#### For Basic Users

Use the modern config system (`.lusushrc`):

```ini
[completion]
completion_enabled = true
fuzzy_completion = true

[theme]
theme_name = corporate
```

#### For Advanced Users

Combine both approaches:

```bash
#!/usr/bin/env lusush
# ~/.lusushrc.sh

# Complex logic using shell scripts
if [ "$(hostname)" = "workstation" ]; then
    theme set corporate
    config set autocorrect_interactive false
else
    theme set minimal
    config set autocorrect_interactive true
fi

# Traditional aliases and functions
alias build='make -j$(nproc)'
backup() {
    cp "$1" "$1.backup.$(date +%Y%m%d)"
}
```

#### For Migration from Bash/Zsh

1. **Start with existing `.profile`** - Works as-is
2. **Convert `.bashrc`/`.zshrc`** to `.lusushrc.sh`
3. **Gradually adopt modern config** for structured settings
4. **Use hybrid approach** for maximum flexibility

## Migration Guide

### From Bash

1. Copy `.profile` → Works unchanged
2. Copy `.bashrc` → Rename to `.lusushrc.sh`
3. Update bash-specific syntax:
   - `shopt` → `config set`
   - `PS1` → `theme set`
   - Complex prompts → Use theme system

### From Zsh

1. Copy `.profile` → Works unchanged
2. Copy `.zshrc` → Rename to `.lusushrc.sh`
3. Update zsh-specific syntax:
   - `setopt` → `config set`
   - Prompt themes → `theme set`
   - Completion → Use modern completion system

### Example Migration

**Original .bashrc:**
```bash
# Bash configuration
export EDITOR=vim
alias ll='ls -l'
shopt -s histappend
PS1='\u@\h:\w\$ '
```

**Converted .lusushrc.sh:**
```bash
#!/usr/bin/env lusush
# LUSUSH configuration

# Keep traditional commands
export EDITOR=vim
alias ll='ls -l'

# Convert bash-specific features
config set history_enabled true
config set history_no_dups true

# Use modern theming
theme set corporate
```

## Advanced Features

### Modular Configuration

Organize scripts in directories:

```bash
# ~/.lusushrc.sh
for script in ~/.lusush.d/*.sh; do
    [ -r "$script" ] && . "$script"
done
```

### Environment-Specific Configuration

```bash
# ~/.lusushrc.sh
case "$HOSTNAME" in
    dev-*)
        . ~/.lusush.d/development.sh
        ;;
    prod-*)
        . ~/.lusush.d/production.sh
        ;;
esac
```

### Project-Specific Configuration

```bash
# ~/.lusushrc.sh
if [ -f "$PWD/.lusushrc.local" ]; then
    . "$PWD/.lusushrc.local"
fi
```

## Configuration Commands

### Script Control

```bash
# Check if script execution is enabled
config get script_execution

# Enable script execution
config set script_execution true

# Disable script execution
config set script_execution false
```

### Manual Script Execution

```bash
# Execute specific script
source ~/.lusushrc.sh

# Execute login scripts
config exec login

# Execute logout scripts
config exec logout
```

## Troubleshooting

### Common Issues

1. **Scripts not executing**
   - Check script permissions: `chmod +x ~/.lusushrc.sh`
   - Verify script execution is enabled: `config get script_execution`

2. **Syntax errors**
   - Use `#!/usr/bin/env lusush` shebang
   - Test scripts: `lusush -c '. ~/.lusushrc.sh'`

3. **Config conflicts**
   - Scripts override config file settings
   - Use `config show` to see effective settings

### Debugging

```bash
# Debug script execution
set -x
. ~/.lusushrc.sh
set +x

# Check configuration
config show
config get completion_enabled
```

## Conclusion

LUSUSH's hybrid configuration system provides the best of both worlds:

- **Familiar workflow** for users migrating from bash/zsh
- **Modern features** for structured configuration
- **Flexibility** to combine both approaches
- **Compatibility** with existing shell scripts

This design ensures that advanced shell users can maintain their preferred configuration style while benefiting from LUSUSH's modern shell features and improvements.