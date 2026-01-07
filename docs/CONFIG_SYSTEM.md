# Configuration System

**Configuring Lusush v1.4.0**

---

## Table of Contents

1. [Overview](#overview)
2. [Config Command](#config-command)
3. [Configuration Sections](#configuration-sections)
4. [Startup Files](#startup-files)
5. [Environment Variables](#environment-variables)
6. [Best Practices](#best-practices)

---

## Overview

Lusush provides a modern configuration system with:

- **Runtime changes**: Modify settings without restart
- **Dual interface**: Modern `config` command + traditional syntax
- **Structured sections**: Logical organization of settings
- **Discoverability**: `config show` reveals all options

### Quick Start

```bash
# View all configuration
config show

# View specific section
config show shell
config show completion

# Get a value
config get shell.errexit

# Set a value (immediate effect)
config set shell.errexit true

# Save to file
config save
```

---

## Config Command

### config show

Display configuration:

```bash
config show              # All sections
config show shell        # Shell options only
config show completion   # Completion settings
config show display      # Display settings
config show history      # History settings
```

### config get

Get a specific value:

```bash
config get shell.errexit      # true or false
config get completion.enabled # true or false
config get history.size       # number
```

### config set

Set a value (takes effect immediately):

```bash
config set shell.errexit true
config set completion.enabled false
config set history.size 5000
```

Boolean values: `true`, `false`, `on`, `off`, `1`, `0`

### config save

Save current configuration:

```bash
config save              # Save to default file
```

### config reset

Reset to defaults:

```bash
config reset             # Reset all settings
```

---

## Configuration Sections

### shell - Shell Options

All 24 POSIX shell options plus mode options:

```bash
config show shell
```

| Option | Description |
|--------|-------------|
| `shell.errexit` | Exit on error (`set -e`) |
| `shell.nounset` | Error on unset variables (`set -u`) |
| `shell.xtrace` | Trace execution (`set -x`) |
| `shell.verbose` | Show input (`set -v`) |
| `shell.pipefail` | Pipeline failure detection |
| `shell.noclobber` | File overwrite protection (`set -C`) |
| `shell.noglob` | Disable globbing (`set -f`) |
| `shell.emacs` | Emacs editing mode |
| `shell.vi` | Vi editing mode |
| `shell.posix` | POSIX compliance mode |
| `shell.monitor` | Job control |
| `shell.hashall` | Hash command paths |

**Dual interface:**

```bash
# Modern syntax
config set shell.errexit true

# Traditional syntax
set -e

# Both stay synchronized
```

### completion - Tab Completion

```bash
config show completion
```

| Option | Default | Description |
|--------|---------|-------------|
| `completion.enabled` | true | Enable tab completion |
| `completion.case_sensitive` | false | Case-sensitive matching |
| `completion.menu_complete` | true | Show menu on first tab |
| `completion.show_types` | true | Show completion type indicators |
| `completion.show_descriptions` | true | Show descriptions |
| `completion.max_menu_height` | 10 | Maximum menu rows |

### display - Display Settings

```bash
config show display
```

| Option | Default | Description |
|--------|---------|-------------|
| `display.syntax_highlighting` | true | Enable syntax highlighting |
| `display.validate_commands` | true | Check if commands exist |
| `display.validate_paths` | true | Check if paths exist |

### history - Command History

```bash
config show history
```

| Option | Default | Description |
|--------|---------|-------------|
| `history.enabled` | true | Enable history |
| `history.size` | 10000 | Maximum entries |
| `history.file` | ~/.lusush_history | History file |
| `history.dedup_navigation` | false | Skip duplicates when navigating |
| `history.ignore_space` | true | Ignore lines starting with space |

### prompt - Prompt Configuration

```bash
config show prompt
```

| Option | Default | Description |
|--------|---------|-------------|
| `prompt.theme` | default | Active theme |
| `prompt.git_enabled` | true | Show git info |
| `prompt.show_user` | true | Show username |
| `prompt.show_host` | true | Show hostname |

### keybindings - Key Configuration

```bash
config show keybindings
```

Keybindings for LLE actions. See [LLE_GUIDE.md](LLE_GUIDE.md) for the full list of actions.

---

## Startup Files

Lusush reads configuration from these locations:

### Load Order

1. `/etc/lusush/lusushrc` - System-wide
2. `~/.lusushrc` - User configuration
3. `~/.config/lusush/config` - XDG user config

### File Format

```bash
# ~/.lusushrc

# Shell options
config set shell.emacs true
set -o pipefail

# Completion
config set completion.enabled true
config set completion.case_sensitive false

# Display
config set display.syntax_highlighting true

# Aliases
alias ll='ls -la'
alias gs='git status'
alias gd='git diff'

# Functions
mkcd() {
    mkdir -p "$1" && cd "$1"
}

# Hooks
precmd() {
    echo -ne "\033]0;${PWD}\007"
}

preexec() {
    echo "Running: $1"
}
```

### Login vs Non-Login

| Shell Type | Files Read |
|------------|------------|
| Login | `/etc/profile`, `~/.profile`, `~/.lusushrc` |
| Interactive | `~/.lusushrc` |
| Script | None (use explicit source) |

---

## Environment Variables

### Shell Variables

| Variable | Description |
|----------|-------------|
| `HOME` | User home directory |
| `PATH` | Command search path |
| `PWD` | Current directory |
| `OLDPWD` | Previous directory |
| `SHELL` | Current shell path |
| `USER` | Current username |
| `TERM` | Terminal type |

### Lusush Variables

| Variable | Description |
|----------|-------------|
| `LUSUSH_VERSION` | Version string |
| `LUSUSH_DEBUG` | Enable debug output |
| `LUSUSH_THEME` | Default theme |

### History Variables

| Variable | Description |
|----------|-------------|
| `HISTFILE` | History file path |
| `HISTSIZE` | History size |
| `HISTCONTROL` | History control flags |

### Hook Variables

| Variable | Description |
|----------|-------------|
| `PERIOD` | Periodic hook interval (seconds) |

---

## Best Practices

### Script Configuration

For scripts, set options explicitly:

```bash
#!/usr/bin/env lusush
set -euo pipefail

# Script body
```

Or with modern syntax:

```bash
#!/usr/bin/env lusush
config set shell.errexit true
config set shell.nounset true
config set shell.pipefail true

# Script body
```

### User Configuration

Recommended `~/.lusushrc`:

```bash
# Shell options
config set shell.emacs true

# Completion
config set completion.enabled true

# Display
config set display.syntax_highlighting true

# History
config set history.size 10000

# Useful aliases
alias ll='ls -la'
alias la='ls -A'
alias l='ls -CF'

# Git shortcuts
alias gs='git status'
alias ga='git add'
alias gc='git commit'
alias gp='git push'
alias gd='git diff'

# Navigation
alias ..='cd ..'
alias ...='cd ../..'

# Safety
alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'

# Custom functions
mkcd() {
    mkdir -p "$1" && cd "$1"
}
```

### Debugging Configuration

Check current settings:

```bash
# All settings
config show

# Specific section
config show shell

# Specific value
config get shell.errexit
```

---

## See Also

- [SHELL_OPTIONS.md](SHELL_OPTIONS.md) - Complete shell options reference
- [LLE_GUIDE.md](LLE_GUIDE.md) - Line editor configuration
- [HOOKS_AND_PLUGINS.md](HOOKS_AND_PLUGINS.md) - Hook system
- [USER_GUIDE.md](USER_GUIDE.md) - Complete reference
