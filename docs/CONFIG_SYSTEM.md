# Configuration System

**Configuring Lusush v1.5.0**

---

## Table of Contents

1. [Overview](#overview)
2. [Configuration Files](#configuration-files)
3. [Config Command](#config-command)
4. [setopt/unsetopt Commands](#setoptunsetopt-commands)
5. [Configuration Sections](#configuration-sections)
6. [Shell Mode and Features](#shell-mode-and-features)
7. [Migrating from Legacy Format](#migrating-from-legacy-format)
8. [Environment Variables](#environment-variables)
9. [Best Practices](#best-practices)

---

## Overview

Lusush v1.5.0 introduces a unified configuration system with:

- **TOML format**: Human-readable, structured configuration
- **XDG compliance**: Config stored in `~/.config/lusush/`
- **Unified registry**: Single source of truth for all settings
- **Bidirectional sync**: Changes via commands sync to config files
- **setopt/unsetopt**: Zsh-style feature control

### Quick Start

```bash
# View all configuration
config show

# View specific section
config show shell

# Get/set a value
config get display.syntax_highlighting
config set display.syntax_highlighting false

# Enable/disable features
setopt extended_glob
unsetopt extended_glob

# Save configuration
config save

# Show config file location
config path
```

---

## Configuration Files

### XDG-Compliant Location (v1.5.0+)

Lusush uses the XDG Base Directory specification:

```
~/.config/lusush/
├── config.toml          # Main configuration (TOML format)
└── config.sh            # Optional shell script (sourced after config.toml)
```

The path respects `$XDG_CONFIG_HOME` if set.

### TOML Configuration Format

The primary configuration file uses TOML format:

```toml
# ~/.config/lusush/config.toml
# Lusush Shell Configuration

[shell]
mode = "lusush"
errexit = false
nounset = false
xtrace = false
pipefail = false

[shell.features]
# Only non-default feature overrides appear here
extended_glob = true

[history]
enabled = true
size = 10000
no_dups = true
timestamps = true

[display]
syntax_highlighting = true
autosuggestions = true
transient_prompt = true

[completion]
enabled = true
fuzzy = true
case_sensitive = false

[prompt]
theme = "default"
```

### Optional Shell Script

For advanced configuration that requires shell commands, create `config.sh`:

```bash
# ~/.config/lusush/config.sh
# Sourced after config.toml - for aliases, functions, and complex setup

# Aliases
alias ll='ls -la'
alias gs='git status'

# Functions
mkcd() {
    mkdir -p "$1" && cd "$1"
}

# Hook functions
precmd() {
    echo -ne "\033]0;${PWD}\007"
}
```

### Legacy Format (~/.lusushrc)

For backward compatibility, Lusush still reads the legacy format:

```bash
# ~/.lusushrc (legacy - will be migrated)
config set shell.errexit true
config set completion.enabled true
alias ll='ls -la'
```

If `~/.lusushrc` exists but no XDG config, Lusush will:
1. Load the legacy config
2. Print a migration notice
3. On `config save`, write to the new XDG location

---

## Config Command

### config show

Display configuration:

```bash
config show              # All sections
config show shell        # Shell options only
config show display      # Display settings
config show history      # History settings
config show completion   # Completion settings
```

### config get / config set

Get or set specific values:

```bash
config get shell.errexit
config set shell.errexit true

config get display.syntax_highlighting
config set display.syntax_highlighting false
```

Boolean values: `true`, `false`, `on`, `off`, `1`, `0`

### config save

Save current configuration to TOML file:

```bash
config save              # Save to ~/.config/lusush/config.toml
```

### config path

Show configuration file paths and status:

```bash
config path
```

Output:
```
Configuration paths:
  User config: /home/user/.config/lusush/config.toml
  System config: /etc/lusush/lusushrc
  XDG directory: /home/user/.config/lusush
  Legacy path: /home/user/.lusushrc
  Format: TOML
  Status: XDG config active
```

### config migrate

Explicitly migrate legacy config to XDG location:

```bash
config migrate           # Convert ~/.lusushrc to ~/.config/lusush/config.toml
```

### config reset

Reset to defaults:

```bash
config reset             # Reset all settings
```

---

## setopt/unsetopt Commands

Lusush v1.5.0 introduces Zsh-style `setopt` and `unsetopt` commands for controlling shell features:

### setopt

Enable options or list current settings:

```bash
setopt                   # List all options with current state
setopt extended_glob     # Enable extended globbing
setopt extglob           # Short alias also works
setopt -p                # Print in re-usable format
setopt -q extended_glob  # Query silently (exit status only)
```

### unsetopt

Disable options:

```bash
unsetopt extended_glob   # Disable extended globbing
unsetopt extglob         # Short alias also works
```

### Available Options

| Option | Aliases | Description |
|--------|---------|-------------|
| `indexed_arrays` | `arrays` | Enable indexed arrays |
| `associative_arrays` | `assoc` | Enable associative arrays |
| `extended_glob` | `extglob` | Extended glob patterns |
| `null_glob` | `nullglob` | No match = empty result |
| `dot_glob` | `dotglob` | Include dotfiles in globs |
| `glob_qualifiers` | `globqual` | Zsh-style qualifiers |
| `process_substitution` | `procsub` | `<(cmd)` and `>(cmd)` |
| `brace_expansion` | `braces` | `{a,b,c}` expansion |
| `extended_test` | `exttest` | `[[ ]]` extended test |
| `regex_match` | `regex` | `=~` regex matching |
| `case_modification` | `casemod` | `${var^^}`, `${var,,}` |
| `hook_functions` | `hooks` | precmd, preexec, chpwd |

Tab completion is available for option names.

### Persistence

Options set via `setopt`/`unsetopt` are saved when you run `config save`:

```bash
setopt extended_glob
config save              # Saves to [shell.features] in config.toml
```

---

## Configuration Sections

### [shell] - Shell Options

All 24 POSIX shell options:

| Option | Description | set -o equivalent |
|--------|-------------|-------------------|
| `errexit` | Exit on error | `set -e` |
| `nounset` | Error on unset variables | `set -u` |
| `xtrace` | Trace execution | `set -x` |
| `verbose` | Print input lines | `set -v` |
| `pipefail` | Pipeline failure detection | `set -o pipefail` |
| `noclobber` | Prevent file overwrite | `set -C` |
| `noglob` | Disable pathname expansion | `set -f` |
| `emacs` | Emacs editing mode | `set -o emacs` |
| `vi` | Vi editing mode | `set -o vi` |
| `posix` | POSIX compliance mode | `set -o posix` |

**Dual interface** - these stay synchronized:

```bash
config set shell.errexit true
set -e
# Both methods update the same setting
```

### [shell.features] - Feature Flags

Override individual features regardless of shell mode:

```toml
[shell.features]
extended_glob = true
process_substitution = false
```

Or use `setopt`/`unsetopt`:

```bash
setopt extended_glob
unsetopt process_substitution
```

### [display] - Display Settings

| Option | Default | Description |
|--------|---------|-------------|
| `syntax_highlighting` | true | Syntax highlighting for commands |
| `autosuggestions` | true | Fish-style autosuggestions |
| `transient_prompt` | true | Simplify prompts in scrollback |

### [history] - Command History

| Option | Default | Description |
|--------|---------|-------------|
| `enabled` | true | Enable command history |
| `size` | 10000 | Maximum history entries |
| `no_dups` | true | Ignore duplicate entries |
| `timestamps` | true | Record command timestamps |

### [completion] - Tab Completion

| Option | Default | Description |
|--------|---------|-------------|
| `enabled` | true | Enable tab completion |
| `fuzzy` | true | Fuzzy matching |
| `case_sensitive` | false | Case-sensitive matching |

### [prompt] - Prompt Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `theme` | "default" | Active prompt theme |

---

## Shell Mode and Features

### Shell Mode

Control which shell compatibility mode is active:

```bash
config get shell.mode           # lusush (default)
config set shell.mode bash      # Bash compatibility
config set shell.mode zsh       # Zsh compatibility
config set shell.mode posix     # Strict POSIX
```

Or use `set -o`:

```bash
set -o bash
set -o zsh
set -o posix
set -o lusush
```

### Feature Defaults by Mode

| Feature | POSIX | Bash | Zsh | Lusush |
|---------|-------|------|-----|--------|
| `extended_glob` | off | on | on | on |
| `process_substitution` | off | on | on | on |
| `brace_expansion` | off | on | on | on |
| `glob_qualifiers` | off | off | on | on |
| `hook_functions` | off | off | on | on |

### Overriding Features

Use `setopt`/`unsetopt` or `config set shell.features.*` to override defaults:

```bash
# In POSIX mode but want extended globs
set -o posix
setopt extended_glob
```

---

## Migrating from Legacy Format

### Automatic Migration

If you have an existing `~/.lusushrc`:

1. Lusush loads it on startup
2. Displays: `Loading configuration from ~/.lusushrc (legacy location)`
3. Displays: `Run 'config save' to migrate to ~/.config/lusush/config.toml`

Run `config save` to migrate:

```bash
config save
# Output: Configuration saved to /home/user/.config/lusush/config.toml
# Output: You may remove the old ~/.lusushrc file
```

### Manual Migration

Use `config migrate` for explicit migration:

```bash
config migrate
```

### What Gets Migrated

- All `config set` values → TOML sections
- Shell options → `[shell]` section
- Feature flags → `[shell.features]` section

### What Stays in config.sh

Move these to `~/.config/lusush/config.sh`:

- Aliases
- Functions
- Hook functions (precmd, preexec)
- Complex shell logic

---

## Environment Variables

### Shell Variables

| Variable | Description |
|----------|-------------|
| `XDG_CONFIG_HOME` | Base config directory (default: ~/.config) |
| `HOME` | User home directory |
| `PATH` | Command search path |
| `SHELL` | Current shell path |
| `TERM` | Terminal type |

### Lusush Variables

| Variable | Description |
|----------|-------------|
| `LUSUSH_VERSION` | Version string |
| `LUSUSH_DEBUG` | Enable debug output |

---

## Best Practices

### Recommended config.toml

```toml
# ~/.config/lusush/config.toml

[shell]
mode = "lusush"
pipefail = true

[shell.features]
extended_glob = true

[display]
syntax_highlighting = true
autosuggestions = true
transient_prompt = true

[history]
size = 10000
no_dups = true

[completion]
enabled = true
fuzzy = true
```

### Recommended config.sh

```bash
# ~/.config/lusush/config.sh

# Aliases
alias ll='ls -la'
alias la='ls -A'
alias gs='git status'
alias gd='git diff'

# Navigation
alias ..='cd ..'
alias ...='cd ../..'

# Safety
alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'

# Functions
mkcd() {
    mkdir -p "$1" && cd "$1"
}

# Hooks
precmd() {
    # Update terminal title
    echo -ne "\033]0;${PWD##*/}\007"
}
```

### For Scripts

Set options explicitly at the start:

```bash
#!/usr/bin/env lusush
set -euo pipefail

# Script body
```

---

## See Also

- [SHELL_OPTIONS.md](SHELL_OPTIONS.md) - Complete shell options reference
- [SHELL_MODES.md](SHELL_MODES.md) - Shell compatibility modes
- [LLE_GUIDE.md](LLE_GUIDE.md) - Line editor configuration
- [BUILTIN_COMMANDS.md](BUILTIN_COMMANDS.md) - All builtin commands
