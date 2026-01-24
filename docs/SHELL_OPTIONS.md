# Shell Options Reference

**All shell options in Lush v1.4.0**

---

## Table of Contents

1. [Overview](#overview)
2. [Setting Options](#setting-options)
3. [POSIX Options](#posix-options)
4. [Shell Mode Options](#shell-mode-options)
5. [Feature Options](#feature-options)
6. [Common Combinations](#common-combinations)
7. [Quick Reference](#quick-reference)

---

## Overview

Lush supports all 24 POSIX shell options plus additional options for shell modes and feature control.

### Checking Options

```bash
# List all options
set -o

# Check specific option
set -o | grep errexit

# Modern config interface
config show shell
config get shell.errexit
```

---

## Setting Options

### Traditional Syntax

```bash
# Enable with -
set -e                    # Short form
set -o errexit            # Long form

# Disable with +
set +e                    # Short form
set +o errexit            # Long form

# Multiple options
set -eu                   # errexit and nounset
set -o errexit -o nounset # Long form
```

### Config Interface

```bash
# Set option
config set shell.errexit true
config set shell.errexit false

# Get current value
config get shell.errexit

# Show all
config show shell
```

Both interfaces stay synchronized.

---

## POSIX Options

### Error Handling

#### `errexit` (`-e`)

Exit immediately when a command fails.

```bash
set -e
false           # Script exits here
echo "Not run"  # Never executed
```

Exceptions: Commands in `if`, `while`, `until`, `&&`, `||`.

#### `nounset` (`-u`)

Error on unset variables.

```bash
set -u
echo "$UNDEFINED"  # Error: UNDEFINED: unbound variable
```

Use `${var:-default}` for optional variables.

#### `pipefail`

Pipeline fails if any command fails.

```bash
set -o pipefail
false | true    # Pipeline exits 1, not 0
```

Without pipefail, pipeline exit status is the last command's.

### Input/Output

#### `noclobber` (`-C`)

Prevent overwriting files with `>`.

```bash
set -C
echo x > file   # Creates file
echo y > file   # Error: file exists

echo z >| file  # Force with >|
```

#### `noglob` (`-f`)

Disable pathname expansion (globbing).

```bash
set -f
echo *.txt      # Prints literal "*.txt"
```

#### `allexport` (`-a`)

Automatically export all variables.

```bash
set -a
MY_VAR=value    # Automatically exported
```

### Job Control

#### `monitor` (`-m`)

Enable job control.

```bash
set -m
sleep 100 &     # Background job
jobs            # List jobs
fg              # Foreground
```

#### `notify` (`-b`)

Report background job status immediately.

```bash
set -b
sleep 1 &       # Notification when done
```

### Debugging

#### `xtrace` (`-x`)

Print commands before execution.

```bash
set -x
echo hello      # Prints: + echo hello
                #         hello
```

#### `verbose` (`-v`)

Print input lines as read.

```bash
set -v
echo hello      # Prints: echo hello
                #         hello
```

#### `noexec` (`-n`)

Read commands but don't execute.

```bash
set -n
echo hello      # Parsed but not executed
```

### Other POSIX Options

| Option | Short | Description |
|--------|-------|-------------|
| `hashall` | `-h` | Hash command paths for faster lookup |
| `ignoreeof` | | Require `exit` to leave shell |
| `nolog` | | Don't save function definitions in history |
| `onecmd` | `-t` | Exit after one command |
| `physical` | | Use physical paths (resolve symlinks) |
| `privileged` | `-p` | Restrict operations in setuid scripts |

---

## Shell Mode Options

Lush supports four shell modes, set with `set -o`:

### `lush` (Default)

All features enabled. Best for interactive use and Lush scripts.

```bash
set -o lush   # Enable (usually default)
```

Features:
- Arrays, `[[]]`, process substitution
- Extended globbing, glob qualifiers
- Hook system
- Full LLE

### `posix`

Strict POSIX compliance.

```bash
set -o posix    # Enable
```

Disables:
- Arrays
- `[[]]` extended test
- Process substitution
- Extended globbing

### `bash`

Bash compatibility mode.

```bash
set -o bash     # Enable
```

Enables:
- Arrays, `[[]]`, process substitution
- Bash-style features

### `zsh`

Zsh compatibility mode.

```bash
set -o zsh      # Enable
```

Enables:
- All Bash features
- Glob qualifiers
- Zsh-style hooks

### Modes at Startup

```bash
lush              # Lush mode (default)
lush --posix      # POSIX mode
lush --bash       # Bash mode
lush --zsh        # Zsh mode
```

In scripts:

```bash
#!/usr/bin/env lush
set -o posix    # POSIX mode for this script
```

---

## Feature Options

### Editing Mode

#### `emacs`

Emacs-style line editing (default).

```bash
set -o emacs
```

#### `vi`

Vi-style line editing (in development).

```bash
set -o vi
```

These are mutually exclusive.

### History

#### `history`

Enable command history.

```bash
set -o history      # Enable
set +o history      # Disable
```

#### `histexpand`

Enable `!` history expansion.

```bash
set -o histexpand   # Enable !! and !n
set +o histexpand   # Literal ! in strings
```

### Interactive Features

#### `interactive-comments`

Allow `#` comments in interactive mode.

```bash
set -o interactive-comments  # Enable (default)
echo hello  # This is a comment
```

#### `braceexpand`

Enable brace expansion.

```bash
set -o braceexpand  # Enable (default)
echo {a,b,c}        # a b c
```

---

## Common Combinations

### Strict Mode

Standard for robust scripts:

```bash
set -euo pipefail
```

- `-e`: Exit on error
- `-u`: Error on unset variables
- `-o pipefail`: Pipeline fails properly

### Debug Mode

Full visibility into execution:

```bash
set -xv
```

- `-x`: Trace execution
- `-v`: Show input

### Safe Mode

Prevent accidents:

```bash
set -euC
```

- `-e`: Exit on error
- `-u`: Catch typos
- `-C`: Don't overwrite files

### Production Script Header

```bash
#!/usr/bin/env lush
set -euo pipefail

# Error handler
trap 'echo "Error on line $LINENO" >&2; exit 1' ERR
```

---

## Quick Reference

### All Options

| Option | Short | Default | Description |
|--------|-------|---------|-------------|
| `allexport` | `-a` | off | Export all variables |
| `braceexpand` | | on | Brace expansion |
| `emacs` | | on | Emacs editing |
| `errexit` | `-e` | off | Exit on error |
| `hashall` | `-h` | on | Hash commands |
| `histexpand` | | on | History expansion |
| `history` | | on | Command history |
| `ignoreeof` | | off | Require exit command |
| `interactive-comments` | | on | Allow # comments |
| `monitor` | `-m` | on* | Job control |
| `noclobber` | `-C` | off | Protect files |
| `noexec` | `-n` | off | Don't execute |
| `noglob` | `-f` | off | Disable globbing |
| `nolog` | | off | Don't log functions |
| `notify` | `-b` | off | Immediate job notify |
| `nounset` | `-u` | off | Error on unset |
| `onecmd` | `-t` | off | Exit after one command |
| `physical` | | off | Physical paths |
| `pipefail` | | off | Pipeline failure |
| `posix` | | off | POSIX mode |
| `privileged` | `-p` | off | Restricted mode |
| `verbose` | `-v` | off | Show input |
| `vi` | | off | Vi editing |
| `xtrace` | `-x` | off | Trace execution |

*`monitor` is on for interactive shells only.

### Mode Options

| Option | Description |
|--------|-------------|
| `lush` | All features (default) |
| `posix` | Strict POSIX compliance |
| `bash` | Bash compatibility |
| `zsh` | Zsh compatibility |

### Config Interface

```bash
config show shell              # All options
config get shell.OPTION        # Get value
config set shell.OPTION VALUE  # Set value
```

---

## See Also

- [SHELL_MODES.md](SHELL_MODES.md) - Mode documentation
- [CONFIG_SYSTEM.md](CONFIG_SYSTEM.md) - Configuration system
- [USER_GUIDE.md](USER_GUIDE.md) - Complete reference
