# Installation Guide

**Building and installing Lusush v1.4.0**

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Requirements](#requirements)
3. [Platform-Specific Instructions](#platform-specific-instructions)
4. [Build Options](#build-options)
5. [Post-Installation](#post-installation)
6. [Troubleshooting](#troubleshooting)

---

## Quick Start

```bash
# Install dependencies (example for Ubuntu/Debian)
sudo apt install build-essential meson ninja-build

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Test
./builddir/lusush --version

# Install (optional)
sudo ninja -C builddir install
```

---

## Requirements

### Build Requirements

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| Meson | 0.53+ | 0.60+ |
| Ninja | 1.8+ | 1.10+ |
| C Compiler | GCC 7+ or Clang 8+ | GCC 11+ or Clang 14+ |
| pkg-config | Any | Latest |

### Runtime Requirements

| Requirement | Minimum | Notes |
|-------------|---------|-------|
| OS | Linux 3.x, macOS 10.12, FreeBSD 11 | Any modern Unix-like |
| Terminal | ANSI compatible | 256-color recommended |
| Memory | 64 MB | Typical usage ~5 MB |
| Disk | 10 MB | For binary and data |

### No Readline Required

Lusush uses LLE (Lusush Line Editor), a native line editor. GNU Readline is not required.

---

## Platform-Specific Instructions

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential meson ninja-build pkg-config git

# Build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
sudo ninja -C builddir install
```

### Fedora/RHEL/CentOS

```bash
# Install dependencies
sudo dnf install gcc meson ninja-build pkgconf git

# Build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
sudo ninja -C builddir install
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S base-devel meson ninja pkgconf git

# Build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
sudo ninja -C builddir install
```

### macOS

```bash
# Install dependencies (Homebrew)
brew install meson ninja git

# Build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

Or with MacPorts:

```bash
sudo port install meson ninja git
```

### FreeBSD

```bash
# Install dependencies
pkg install meson ninja pkgconf git

# Build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

### OpenBSD

```bash
# Install dependencies
pkg_add meson ninja git

# Build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

---

## Build Options

### Build Types

```bash
# Debug build (default)
meson setup builddir --buildtype=debug

# Release build (optimized)
meson setup builddir --buildtype=release

# Release with debug info
meson setup builddir --buildtype=debugoptimized
```

### Installation Prefix

```bash
# Custom prefix
meson setup builddir --prefix=/usr/local

# User-local installation
meson setup builddir --prefix=$HOME/.local
```

### Reconfiguring

```bash
# Change options on existing build
meson configure builddir --buildtype=release

# Or reconfigure entirely
meson setup builddir --reconfigure --buildtype=release
```

### Feature Options

```bash
# View all options
meson configure builddir
```

---

## Post-Installation

### Verify Installation

```bash
# Check version
lusush --version
# Output: lusush 1.4.0

# Test basic functionality
lusush -c 'echo "Hello from Lusush"'

# Test LLE
lusush -c 'display lle diagnostics'

# Test debugger
lusush -c 'debug help'
```

### Add to Available Shells

```bash
# Add to /etc/shells (requires root)
which lusush | sudo tee -a /etc/shells
```

### Set as Default Shell

```bash
# Change login shell
chsh -s $(which lusush)
```

### Create Configuration

```bash
# Create startup file
cat > ~/.lusushrc << 'EOF'
# Lusush startup configuration

# Shell options
config set shell.emacs true

# Aliases
alias ll='ls -la'
alias gs='git status'

# Hooks
precmd() {
    # Update terminal title
    echo -ne "\033]0;${PWD}\007"
}
EOF
```

### PATH Setup (User Installation)

If installed to `~/.local`:

```bash
# Add to shell profile
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.profile
source ~/.profile
```

---

## Troubleshooting

### Build Errors

**meson: command not found**

```bash
# Install via package manager
sudo apt install meson    # Debian/Ubuntu
sudo dnf install meson    # Fedora
brew install meson        # macOS

# Or via pip
pip3 install --user meson ninja
```

**Meson version too old**

```bash
# Install latest via pip
pip3 install --user --upgrade meson
```

**Compiler not found**

```bash
# Install build tools
sudo apt install build-essential  # Debian/Ubuntu
sudo dnf groupinstall "Development Tools"  # Fedora
xcode-select --install  # macOS
```

### Runtime Issues

**Command not found after install**

```bash
# Check installation location
which lusush

# If not found, check prefix
meson configure builddir | grep prefix

# Add to PATH if needed
export PATH="/usr/local/bin:$PATH"
```

**Display issues**

```bash
# Check terminal capabilities
echo $TERM
tput colors

# Ensure UTF-8 locale
export LANG=en_US.UTF-8
```

**Permission denied**

```bash
# System install requires root
sudo ninja -C builddir install

# Or install to user directory
meson setup builddir --prefix=$HOME/.local --reconfigure
ninja -C builddir install
```

### Platform-Specific Issues

**macOS: Command line tools missing**

```bash
xcode-select --install
```

**Linux: Old glibc**

Lusush requires glibc 2.17+. On older systems, consider building statically or using a container.

---

## Uninstalling

```bash
# If installed with ninja install
sudo ninja -C builddir uninstall

# Or manually
sudo rm /usr/local/bin/lusush
sudo rm -rf /usr/local/share/lusush
```

---

## Building from Release Tarball

```bash
# Download release
curl -LO https://github.com/lusush/lusush/releases/download/v1.4.0/lusush-1.4.0.tar.gz
tar xzf lusush-1.4.0.tar.gz
cd lusush-1.4.0

# Build
meson setup builddir
ninja -C builddir

# Install
sudo ninja -C builddir install
```

---

## See Also

- [GETTING_STARTED.md](GETTING_STARTED.md) - First steps after installation
- [USER_GUIDE.md](USER_GUIDE.md) - Complete feature reference
- [CONFIG_SYSTEM.md](CONFIG_SYSTEM.md) - Configuration options
