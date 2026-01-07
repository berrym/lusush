# Lusush

**The shell that does what you mean.**

[![Version](https://img.shields.io/badge/version-1.4.0-blue)](https://github.com/lusush/lusush/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0+-blue)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/lusush/lusush)

---

## What is Lusush?

Lusush is an advanced interactive shell that combines the best of Bash and Zsh with capabilities no other shell offers:

- **Native Line Editor (LLE)** - Built from scratch, not a readline wrapper. Syntax highlighting, context-aware completions, and the editing experience we always wanted.
- **Integrated Debugger** - The only shell with GDB-like debugging built in. Set breakpoints, step through code, inspect variables.
- **Multi-Mode Architecture** - Run in POSIX, Bash, Zsh, or Lusush mode. One shell, full compatibility.
- **Extended Syntax** - Arrays, `[[ ]]`, process substitution, extended globbing, and parameter expansion done right.

## What's New in v1.4.0

This release is a major evolution. Lusush now includes:

| Feature | Description |
|---------|-------------|
| **LLE** | Complete line editor with Emacs keybindings, syntax highlighting, and 45 context-aware builtin completions |
| **Shell Modes** | POSIX, Bash, Zsh, or Lusush mode - switch with `set -o` |
| **Arrays** | Indexed and associative arrays with full expansion support |
| **Extended Tests** | `[[ ]]` with pattern matching and regex `=~` |
| **Process Substitution** | `<(cmd)` and `>(cmd)` |
| **Hook System** | `precmd`, `preexec`, `chpwd`, `periodic` hooks |
| **Glob Qualifiers** | `*(.)` for files, `*(/)` for directories |

See the [Changelog](CHANGELOG.md) for the complete list.

## Quick Start

```bash
# Build (no readline required)
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup build
ninja -C build

# Run
./build/lusush
```

## Features at a Glance

### LLE - Lusush Line Editor

Real-time syntax highlighting as you type. Context-aware completions for commands, files, and 45 shell builtins. History search with Ctrl-R. Multi-line editing that just works.

```bash
# Syntax highlighting shows:
# - Valid commands in green
# - Invalid commands in red
# - Strings, variables, paths in distinct colors
# - Errors before you press Enter
```

### Shell Modes

Choose your compatibility level:

```bash
set -o posix   # Strict POSIX sh
set -o bash    # Bash 5.x compatibility
set -o zsh     # Zsh compatibility
set -o lusush  # Default: curated best of both
```

### Extended Syntax

Arrays, arithmetic, extended tests, process substitution - all the features you expect from a modern shell:

```bash
# Arrays
files=(*.txt)
declare -A config
config[host]="localhost"

# Extended tests
[[ $response =~ ^[Yy] ]] && echo "Yes"

# Process substitution
diff <(sort file1) <(sort file2)

# Parameter expansion
echo "${name^^}"           # Uppercase
echo "${path##*/}"         # Basename
echo "${var:-default}"     # Default value
```

### Hook System

Run code at key moments in the shell lifecycle:

```bash
precmd() {
    # Runs before each prompt
    update_terminal_title
}

preexec() {
    # Runs before each command, receives command as $1
    echo "Running: $1"
}

chpwd() {
    # Runs after directory changes
    ls
}
```

### Integrated Debugger

The only shell where you can debug scripts interactively:

```bash
debug on
debug break add script.sh 15
source script.sh

# At breakpoint:
debug vars      # Show all variables
debug step      # Step to next line
debug continue  # Resume execution
```

## Documentation

| Document | Description |
|----------|-------------|
| [Getting Started](docs/GETTING_STARTED.md) | First-time user guide |
| [User Guide](docs/USER_GUIDE.md) | Complete feature reference |
| [LLE Guide](docs/LLE_GUIDE.md) | Line editor reference |
| [Extended Syntax](docs/EXTENDED_SYNTAX.md) | Arrays, `[[]]`, process sub |
| [Shell Modes](docs/SHELL_MODES.md) | POSIX/Bash/Zsh/Lusush modes |
| [Debugger Guide](docs/DEBUGGER_GUIDE.md) | Interactive debugging |
| [Builtins Reference](docs/BUILTIN_COMMANDS.md) | All 48 builtin commands |

## Building

### Requirements

- C11 compiler (GCC or Clang)
- Meson build system
- Ninja

No readline dependency. LLE provides all line editing functionality.

### Build Options

```bash
# Standard build
meson setup build
ninja -C build

# Debug build
meson setup build --buildtype=debug
ninja -C build

# Install system-wide
sudo ninja -C build install
```

### Platforms

- Linux (primary development platform)
- macOS (fully supported)
- BSD (supported)

## Development Status

Lusush is under active development. Current status:

| Component | Status |
|-----------|--------|
| Core Shell | Production Ready |
| LLE (Emacs mode) | Complete |
| LLE (Vi mode) | In Development |
| Extended Syntax | Complete |
| Shell Modes | Complete |
| Debugger | Production Ready |
| Plugin System | Foundation Complete |

## Contributing

Lusush maintains high standards:

- Clean, professional code (C11, no warnings)
- Zero memory leaks (verified with valgrind/leaks)
- Comprehensive tests
- Clear commit messages

See the development guidelines for details.

## License

GNU General Public License v3.0 or later. See [LICENSE](LICENSE).

---

**Lusush** - Advanced interactive shell with LLE, multi-mode compatibility, and integrated debugging.
