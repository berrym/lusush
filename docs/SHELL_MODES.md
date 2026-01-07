# Shell Modes

**Running Lusush in different compatibility modes**

Lusush can operate in four distinct modes, each providing different feature sets and behaviors. This allows you to run scripts from different shell environments and choose the right balance between features and portability.

---

## Table of Contents

1. [Overview](#overview)
2. [Lusush Mode](#lusush-mode)
3. [POSIX Mode](#posix-mode)
4. [Bash Mode](#bash-mode)
5. [Zsh Mode](#zsh-mode)
6. [Feature Matrix](#feature-matrix)
7. [Switching Modes](#switching-modes)
8. [Shebang Detection](#shebang-detection)
9. [Per-Feature Overrides](#per-feature-overrides)
10. [Best Practices](#best-practices)

---

## Overview

| Mode | Purpose | Feature Set |
|------|---------|-------------|
| **Lusush** | Default interactive mode | All features enabled |
| **POSIX** | Maximum portability | POSIX sh standard only |
| **Bash** | Bash script compatibility | Bash 4.x features |
| **Zsh** | Zsh script compatibility | Zsh features |

Modes affect:
- Available syntax (arrays, `[[]]`, etc.)
- Command behavior
- Option names and defaults
- Error handling

Modes do NOT affect:
- LLE (always available interactively)
- Integrated debugger
- Configuration system
- Theme system

---

## Lusush Mode

**The default mode. Use this for interactive sessions and Lusush-native scripts.**

Lusush mode combines the best features from Bash, Zsh, and adds Lusush-specific capabilities.

### Enabling

```bash
set -o lusush           # Enable
set +o lusush           # Disable (switches to POSIX)
```

### Features

Everything works in Lusush mode:

**From Bash**
- Indexed arrays: `arr=(a b c)`
- Associative arrays: `declare -A map`
- Extended test: `[[ ]]`
- Process substitution: `<()`, `>()`
- Extended parameter expansion
- Arithmetic: `(())`, `let`
- `select` loop
- Case fall-through: `;&`, `;;&`
- `time` keyword

**From Zsh**
- Glob qualifiers: `*(.)`, `*(/)`, `*(@)`
- Hook functions: `precmd`, `preexec`, `chpwd`, `periodic`
- Hook arrays: `precmd_functions`, etc.
- Extended globbing patterns

**Lusush-Specific**
- Integrated debugging: `debug on`
- Plugin system foundation
- Enhanced display system
- Configuration interface

### When to Use

- Interactive shell sessions
- Scripts that will only run in Lusush
- Scripts that need debugging features
- Scripts that use hooks

### Example Script

```bash
#!/usr/bin/env lusush

# Uses Lusush-specific features
declare -A config
config[debug]=true
config[verbose]=false

precmd() {
    [[ ${config[debug]} == true ]] && debug vars
}

files=(*.txt)
for f in "${files[@]}"; do
    [[ -f $f ]] && process "$f"
done
```

---

## POSIX Mode

**Maximum portability. Scripts run on any POSIX-compliant shell.**

POSIX mode restricts Lusush to the POSIX shell specification. Use this for scripts that must run on different systems and shells.

### Enabling

```bash
set -o posix            # Enable
set +o posix            # Disable
lusush --posix script.sh    # Run script in POSIX mode
```

### Restrictions

POSIX mode disables:

| Feature | Alternative |
|---------|-------------|
| Arrays | Use positional parameters or string parsing |
| `[[` extended test | Use `[` (test) |
| Process substitution | Use temporary files or pipes |
| Extended globbing | Use standard globs |
| `(())` arithmetic command | Use `$(())` expansion |
| Here-strings `<<<` | Use here-documents `<<` |
| `select` loop | Write your own menu |
| Case fall-through | Structure cases differently |
| Glob qualifiers | Use `find` or test in loop |

### What Still Works

POSIX mode retains:
- Standard control flow: `if`, `for`, `while`, `case`, `until`
- Functions (POSIX syntax)
- Standard parameter expansion
- Pipes, redirections
- Command substitution `$()`
- Arithmetic expansion `$(())`
- All POSIX builtins
- Here-documents

### When to Use

- Scripts for multiple platforms
- System scripts that may run on minimal shells
- Scripts in `/etc` that use `/bin/sh`
- Maximum compatibility requirements

### Example Script

```bash
#!/usr/bin/env lusush
set -o posix

# POSIX-compliant: no arrays, no [[]]
files=$(ls *.txt 2>/dev/null)
for f in $files; do
    if [ -f "$f" ]; then
        echo "Processing: $f"
    fi
done

# Arithmetic with $(()), not (())
count=0
while [ $count -lt 10 ]; do
    count=$((count + 1))
done
```

---

## Bash Mode

**Bash 4.x compatibility. Run existing Bash scripts.**

Bash mode provides compatibility with Bash scripts, enabling Bash-specific features while maintaining Bash semantics.

### Enabling

```bash
set -o bash             # Enable
set +o bash             # Disable
lusush --bash script.sh     # Run script in Bash mode
```

### Features

Bash mode enables:

- Indexed arrays
- Associative arrays (`declare -A`)
- Extended test `[[]]`
- Process substitution `<()`, `>()`
- Arithmetic command `(())`
- Extended globbing (`shopt -s extglob`)
- `select` loop
- Case fall-through `;&`, `;;&`
- `time` keyword
- Nameref variables (`local -n`)
- Extended parameter expansion
- Here-strings `<<<`

### Bash-Specific Behaviors

- `shopt` for shell options
- `BASH_*` variables populated
- Bash-style word splitting
- Bash-style pathname expansion

### What's Not Included

Bash mode does NOT include:
- Zsh glob qualifiers
- Lusush hook system
- Zsh-specific parameter expansions
- Plugin system

### When to Use

- Running existing Bash scripts
- Writing scripts for both Bash and Lusush
- Teams migrating from Bash

### Example Script

```bash
#!/usr/bin/env lusush
set -o bash

# Standard Bash script
declare -a files=()
declare -A counts

while IFS= read -r -d '' file; do
    files+=("$file")
done < <(find . -name "*.txt" -print0)

for file in "${files[@]}"; do
    ext="${file##*.}"
    ((counts[$ext]++))
done

for ext in "${!counts[@]}"; do
    echo "$ext: ${counts[$ext]}"
done
```

---

## Zsh Mode

**Zsh compatibility. Run existing Zsh scripts.**

Zsh mode enables Zsh-specific features and behaviors.

### Enabling

```bash
set -o zsh              # Enable
set +o zsh              # Disable
lusush --zsh script.sh      # Run script in Zsh mode
```

### Features

Zsh mode includes everything in Bash mode, plus:

- Glob qualifiers: `*(.)`, `*(/)`, `*(@)`, `*(*)`
- Extended glob patterns
- Zsh-style parameter expansion
- Hook functions: `precmd`, `preexec`, `chpwd`
- Hook arrays
- Zsh option names

### Zsh-Specific Behaviors

- Zsh-style word splitting (different from Bash)
- Glob qualifiers always available
- Zsh option name compatibility

### What's Not Included

- Full Zsh module system
- Zsh completion system (uses LLE's)
- Some esoteric Zsh features

### When to Use

- Running existing Zsh scripts
- Scripts using glob qualifiers
- Scripts using Zsh hooks
- Teams migrating from Zsh

### Example Script

```bash
#!/usr/bin/env lusush
set -o zsh

# Zsh-style script
# Only regular files, sorted by modification time
for file in *(.) ; do
    echo "Regular file: $file"
done

# Only directories
for dir in *(/) ; do
    echo "Directory: $dir"
done

# Hook function
precmd() {
    echo "About to show prompt"
}
```

---

## Feature Matrix

| Feature | POSIX | Bash | Zsh | Lusush |
|---------|:-----:|:----:|:---:|:------:|
| **Syntax** |
| Basic shell syntax | Yes | Yes | Yes | Yes |
| Arrays (indexed) | - | Yes | Yes | Yes |
| Arrays (associative) | - | Yes | Yes | Yes |
| `[[ ]]` extended test | - | Yes | Yes | Yes |
| Process substitution | - | Yes | Yes | Yes |
| `(( ))` arithmetic | - | Yes | Yes | Yes |
| `let` builtin | - | Yes | Yes | Yes |
| Here-strings `<<<` | - | Yes | Yes | Yes |
| Case fall-through | - | Yes | Yes | Yes |
| `select` loop | - | Yes | Yes | Yes |
| `time` keyword | - | Yes | Yes | Yes |
| Extended globbing | - | Yes | Yes | Yes |
| Glob qualifiers | - | - | Yes | Yes |
| Nameref `local -n` | - | Yes | Yes | Yes |
| **Parameter Expansion** |
| POSIX forms | Yes | Yes | Yes | Yes |
| Case modification | - | Yes | Yes | Yes |
| Pattern substitution | - | Yes | Yes | Yes |
| Indirect `${!var}` | - | Yes | Yes | Yes |
| Transformations `${var@Q}` | - | Yes | Yes | Yes |
| **Functions** |
| Basic functions | Yes | Yes | Yes | Yes |
| `local` variables | Partial | Yes | Yes | Yes |
| Anonymous functions | - | - | Yes | Yes |
| **Hooks** |
| precmd | - | - | Yes | Yes |
| preexec | - | - | Yes | Yes |
| chpwd | - | - | Yes | Yes |
| periodic | - | - | - | Yes |
| Hook arrays | - | - | Yes | Yes |
| **Other** |
| LLE editor | Yes | Yes | Yes | Yes |
| Debugger | Yes | Yes | Yes | Yes |
| Config system | Yes | Yes | Yes | Yes |
| Plugin system | - | - | - | Yes |

---

## Switching Modes

### Interactive

```bash
# Check current mode
set -o | grep -E "posix|bash|zsh|lusush"

# Switch modes
set -o lusush
set -o posix
set -o bash
set -o zsh
```

### In Scripts

```bash
#!/usr/bin/env lusush
set -o bash   # Script runs in Bash mode
```

### Command Line

```bash
lusush --posix script.sh
lusush --bash script.sh
lusush --zsh script.sh
lusush script.sh         # Lusush mode (default)
```

### Config File

```bash
# In ~/.lusushrc
set -o lusush  # Default mode for interactive sessions
```

---

## Shebang Detection

Lusush can detect the intended mode from script shebangs:

| Shebang | Detected Mode |
|---------|---------------|
| `#!/bin/sh` | POSIX |
| `#!/usr/bin/env sh` | POSIX |
| `#!/bin/bash` | Bash |
| `#!/usr/bin/env bash` | Bash |
| `#!/bin/zsh` | Zsh |
| `#!/usr/bin/env zsh` | Zsh |
| `#!/usr/bin/env lusush` | Lusush |
| `#!/usr/bin/env lusush --posix` | POSIX |
| `#!/usr/bin/env lusush --bash` | Bash |

When sourcing scripts or running without explicit mode, Lusush examines the shebang and adjusts behavior accordingly.

---

## Per-Feature Overrides

You can enable or disable individual features regardless of mode:

```bash
# In POSIX mode, but enable arrays
set -o posix
shopt -s arrays         # Enable arrays only

# In Lusush mode, but disable process substitution
set -o lusush
shopt -u procsub        # Disable process substitution

# Check feature status
shopt                   # List all features
shopt arrays            # Check specific feature
```

### Available Feature Flags

```bash
shopt -s extglob        # Extended globbing
shopt -s arrays         # Indexed arrays
shopt -s assoc_arrays   # Associative arrays
shopt -s procsub        # Process substitution
shopt -s exttest        # [[ ]] extended test
shopt -s globqual       # Glob qualifiers
```

---

## Best Practices

### Choosing a Mode

1. **Interactive work**: Use Lusush mode (default)
2. **New scripts for Lusush**: Use Lusush mode
3. **Scripts for multiple systems**: Use POSIX mode
4. **Existing Bash scripts**: Use Bash mode
5. **Existing Zsh scripts**: Use Zsh mode

### Script Portability

```bash
#!/usr/bin/env lusush
# Explicitly set mode at script start
set -o posix  # or bash, zsh, lusush

# Script continues in that mode
```

### Testing Across Modes

```bash
# Test script in different modes
lusush --posix script.sh && echo "POSIX OK"
lusush --bash script.sh && echo "Bash OK"
lusush script.sh && echo "Lusush OK"
```

### Gradual Migration

When migrating scripts:

1. Start in compatibility mode (bash or zsh)
2. Test thoroughly
3. Gradually adopt Lusush features
4. Eventually switch to Lusush mode if desired

### Documentation

Always document which mode your script requires:

```bash
#!/usr/bin/env lusush
# requires: lusush mode (uses hooks)
# or
#!/usr/bin/env lusush --posix
# compatible: any POSIX shell
```

---

## See Also

- [USER_GUIDE.md](USER_GUIDE.md) - Complete feature reference
- [EXTENDED_SYNTAX.md](EXTENDED_SYNTAX.md) - Extended syntax details
- [SHELL_OPTIONS.md](SHELL_OPTIONS.md) - All shell options
- [FEATURE_COMPARISON.md](FEATURE_COMPARISON.md) - Comparison with other shells
