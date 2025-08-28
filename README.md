# Lusush Shell

A modern, professional shell with advanced features including real-time syntax highlighting, comprehensive tab completion, git integration, and multiple themes.

## Features

### 🎨 Real-time Syntax Highlighting
- **Word-boundary triggered highlighting** - See syntax colors as you type
- **Comprehensive syntax support** - Keywords, built-ins, variables, strings, numbers, operators
- **Professional color scheme** - Enterprise-appropriate visual design
- **Zero performance impact** - Sub-millisecond response times

### 🚀 Enhanced User Experience  
- **Advanced tab completion** - Context-aware completion for commands, files, and git
- **Git integration** - Real-time branch and status display in prompts
- **Multiple themes** - 6 professional themes (dark, light, minimal, colorful, classic, corporate)
- **Multiline support** - Full support for complex shell constructs (`for`, `if`, `while`, etc.)

### 🔧 Professional Features
- **POSIX compliance** - Standard shell operations and compatibility
- **History management** - Intelligent history with deduplication
- **Cross-platform** - Linux, macOS, BSD support
- **Performance optimized** - Enterprise-scale ready
- **Memory safe** - Comprehensive error handling and resource management

## Quick Start

### Build Requirements
- C compiler (GCC or Clang)
- Meson build system
- GNU Readline library
- Git (for repository features)

### Installation

```bash
# Clone the repository
git clone https://github.com/berrym/lusush.git
cd lusush

# Setup build directory
meson setup builddir

# Build the shell
ninja -C builddir

# Install (optional)
sudo ninja -C builddir install
```

### Usage

```bash
# Run standard mode
./builddir/lusush

# Run with enhanced display mode (full syntax highlighting)
./builddir/lusush --enhanced-display

# Interactive mode
./builddir/lusush -i
```

## Syntax Highlighting

Lusush provides comprehensive real-time syntax highlighting:

- **Shell Keywords** (Blue): `if`, `then`, `else`, `for`, `while`, `do`, `done`, `case`, etc.
- **Built-in Commands** (Green): `echo`, `cd`, `pwd`, `export`, `grep`, `ls`, etc.
- **Variables** (Magenta): `$HOME`, `$PATH`, `${VAR}`, etc.
- **Strings** (Yellow): `"double quotes"`, `'single quotes'`
- **Numbers** (Cyan): `42`, `123.45`
- **Operators** (Red): `|`, `&`, `;`, `<`, `>`

Example with highlighting:
```bash
if test "$HOME" = "/home/user"; then
    echo "Found 123 files" | grep important;
    export PATH=$PATH:/usr/local/bin;
fi
```

## Themes

Switch between professional themes:

```bash
# Available themes
theme list

# Set theme
theme set dark
theme set light
theme set minimal
theme set colorful
theme set classic
theme set corporate
```

## Configuration

### Environment Variables
- `LUSUSH_ENHANCED_DISPLAY=1` - Enable enhanced display mode
- `LUSUSH_THEME=dark` - Set default theme
- `LUSUSH_HISTORY_SIZE=1000` - Set history size

### Command Line Options
- `--enhanced-display` - Enable full syntax highlighting
- `-i` - Interactive mode
- `--help` - Show help information
- `--version` - Show version information

## Advanced Features

### Git Integration
- Real-time git branch display in prompts
- Git status indicators (modified files, ahead/behind status)
- Git-aware tab completion

### Tab Completion
- **Context-aware**: Different completions based on command context
- **File completion**: Intelligent file and directory completion
- **Command completion**: Built-in and external command completion
- **Git completion**: Branch names, git subcommands

### History Management
- **Deduplication**: Automatic removal of duplicate entries
- **Search**: Reverse search with Ctrl+R
- **Persistence**: History saved across sessions

## Testing

Run the test suite:
```bash
# Quick verification tests
./quick_test.sh

# Full test suite
ninja -C builddir test
```

## Development

### Project Structure
```
lusush/
├── src/                    # Source code
│   ├── builtins/          # Built-in command implementations
│   ├── display/           # Display system components
│   ├── readline_integration.c  # Main readline integration
│   └── ...
├── include/               # Header files
├── tests/                 # Test suite
├── docs/                  # Documentation
└── builddir/             # Build output
```

### Key Components
- **Readline Integration**: GNU Readline wrapper with syntax highlighting
- **Theme System**: Professional theme management
- **Display Integration**: Enhanced display and prompt system
- **Completion System**: Advanced tab completion
- **Git Integration**: Real-time git status and branch display

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes with comprehensive tests
4. Ensure all tests pass: `ninja -C builddir test`
5. Submit a pull request

### Coding Standards
- C99 standard compliance
- Comprehensive error handling
- Memory safety (no leaks)
- Performance optimization
- Professional naming conventions

## License

[License details - see LICENSE file]

## Support

- **Issues**: Report bugs and feature requests via GitHub Issues
- **Documentation**: See `docs/` directory for detailed documentation
- **Performance**: Optimized for enterprise-scale usage

## Acknowledgments

- Built on GNU Readline for robust line editing
- Inspired by modern shell user experience design
- Designed for professional and enterprise environments

---

**Lusush Shell** - Professional shell with modern features and enterprise-grade reliability.