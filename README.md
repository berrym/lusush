# LUSUSH Shell

A modern, POSIX-compliant shell with advanced features for professional development environments.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![POSIX Compliance](https://img.shields.io/badge/POSIX-100%25-blue.svg)]()

## Overview

LUSUSH is a feature-rich, production-ready shell designed for developers and system administrators who need reliable, efficient command-line functionality with modern conveniences. Built with a focus on POSIX compliance, performance, and professional terminal experience with advanced termcap integration for enterprise environments.

## Key Features

### Core Functionality
- **100% POSIX Compliance** - Full compatibility with standard shell operations
- **Advanced Line Editing** - Professional terminal editing with termcap integration
- **Intelligent Tab Completion** - Terminal-aware completion with responsive layouts
- **Command History** - Persistent history with search and navigation
- **Variable Management** - Dynamic template variables with real-time terminal state

### Terminal Intelligence (NEW in v1.0.7)
- **Complete Termcap Integration** - Full terminal capability detection and adaptation
- **Responsive Design** - Automatic adaptation to terminal size and capabilities
- **Platform Detection** - Specialized support for iTerm2, tmux, screen environments
- **Dynamic Templates** - Real-time terminal state updates in prompts and themes
- **Professional Experience** - Enterprise-grade terminal awareness

### Modern Enhancements
- **Professional Themes** - 6 enterprise-grade visual themes with terminal awareness
- **Network Integration** - SSH host completion and network diagnostics
- **Git Integration** - Git-aware prompts and branch information
- **Configuration Management** - INI-style configuration with runtime changes
- **Corporate Branding** - Professional startup branding and company identification

### Enterprise Features
- **Cross-Platform** - Enhanced compatibility on Linux, macOS, and BSD systems
- **Memory Efficient** - Optimized for low resource usage with minimal termcap overhead
- **Extensible** - Plugin architecture with terminal-aware capabilities
- **Secure** - Built with security best practices and safe terminal operations

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/berrym/lusush.git
cd lusush

# Build with Meson
meson setup builddir
ninja -C builddir

# Install (optional)
sudo ninja -C builddir install
```

### Basic Usage

```bash
# Start the shell
./builddir/lusush

# Configure features
config set hints_enabled true
config set theme_name corporate

# Explore built-in help
help
theme list
config show
```

## Documentation

### User Documentation
- [Installation Guide](docs/user/INSTALLATION.md) - Complete installation instructions
- [User Manual](docs/user/USER_MANUAL.md) - Comprehensive usage guide
- [Configuration Reference](docs/user/CONFIGURATION.md) - All configuration options

### Developer Documentation
- [Contributing Guide](docs/developer/CONTRIBUTING.md) - How to contribute to the project
- [Architecture Overview](docs/developer/ARCHITECTURE.md) - System design and structure
- [API Reference](docs/developer/API_REFERENCE.md) - Internal API documentation

### Production Deployment
- [Deployment Guide](docs/production/DEPLOYMENT.md) - Production setup and configuration
- [Maintenance Manual](docs/production/MAINTENANCE.md) - Ongoing maintenance procedures
- [Troubleshooting](docs/production/TROUBLESHOOTING.md) - Common issues and solutions

## Requirements

### Build Requirements
- C99-compatible compiler (GCC 4.9+ or Clang 3.4+)
- Meson build system (0.50.0+)
- Ninja build tool

### Runtime Requirements
- POSIX-compatible operating system
- Terminal with ANSI color support (recommended)
- 512KB RAM minimum

### Supported Platforms
- Linux (all major distributions)
- macOS (10.12+)
- FreeBSD, OpenBSD, NetBSD
- Other UNIX-like systems

## Configuration

LUSUSH uses an INI-style configuration system with runtime modification support:

```bash
# View current configuration
config show

# Enable features
config set hints_enabled true
config set git_prompt_enabled true

# Set theme
config set theme_name professional

# Save configuration
config save
```

## Testing

### Quick Verification
```bash
# Run POSIX compliance tests
./tests/compliance/test_posix_regression.sh

# Manual testing
./tests/manual/test_history_bottom_line.sh
```

### Complete Test Suite
```bash
# Run all tests
meson test -C builddir

# Specific test categories
./tests/compliance/  # POSIX compliance
./tests/manual/      # Manual verification
```

## Examples

### Basic Shell Operations
```bash
# Command execution
ls -la | grep "\.txt" | sort

# Variable operations
export MY_VAR="hello world"
echo $MY_VAR

# Control structures
for file in *.txt; do echo "Processing $file"; done
```

### Advanced Features
```bash
# Network completion
ssh user@<TAB>  # Shows available SSH hosts

# Git integration
cd /git/repo    # Prompt shows branch info

# Theme switching
theme set corporate  # Professional appearance
```

## Release History

- [v1.0.6](docs/releases/v1.0.6.md) - **Enhanced completion experience** with SSH completion bug fix
- [v1.0.5](docs/releases/v1.0.5.md) - **Interactive excellence** with complete hints system overhaul
- [v1.0.4](docs/releases/v1.0.4.md) - **Production-ready release** with all critical fixes
- [v1.0.3](docs/releases/v1.0.3.md) - Terminal editing fixes and hints configuration
- [v1.0.2](docs/releases/v1.0.2.md) - Enhanced themes and configuration system
- [v1.0.1](docs/releases/v1.0.1.md) - Initial production release
- [v1.0.0](docs/releases/v1.0.0.md) - First stable release

See [CHANGELOG.md](CHANGELOG.md) for complete version history.

## Contributing

We welcome contributions! Please see our [Contributing Guide](docs/developer/CONTRIBUTING.md) for details on:

- Code style and standards
- Development workflow
- Testing requirements
- Pull request process

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

### Community
- **Issues**: [GitHub Issues](https://github.com/berrym/lusush/issues)
- **Discussions**: [GitHub Discussions](https://github.com/berrym/lusush/discussions)

### Enterprise Support
For production deployments and enterprise support, see our [Deployment Guide](docs/production/DEPLOYMENT.md).

## Acknowledgments

- Built with [linenoise](https://github.com/antirez/linenoise) for line editing
- Inspired by modern shell design principles
- Thanks to all contributors and users

---

**LUSUSH** - A modern shell for modern developers.