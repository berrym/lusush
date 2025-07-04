# LUSUSH INSTALLATION GUIDE

## Overview

This guide provides comprehensive installation instructions for Lusush, a modern POSIX-compliant shell with enhanced features. Choose the installation method that best fits your system and requirements.

## System Requirements

### Minimum Requirements
- **Operating System**: Linux, macOS, or Unix-like system
- **Architecture**: 64-bit (x86_64, ARM64)
- **Memory**: 128MB RAM minimum
- **Storage**: 50MB free disk space
- **Compiler**: GCC 7.0+ or Clang 6.0+

### Recommended Requirements
- **Memory**: 512MB RAM or more
- **Storage**: 200MB free disk space
- **Terminal**: ANSI color support
- **Network**: Internet connection for enhanced features

### Dependencies

#### Build Dependencies
- **meson** (0.55.0+)
- **ninja** (1.8.0+)
- **gcc** or **clang**
- **make**
- **git**
- **pkg-config**

#### Runtime Dependencies
- **libc** (glibc 2.17+ or musl)
- **libreadline** (optional, for enhanced editing)
- **ncurses** (for terminal handling)

## Installation Methods

### Method 1: Package Manager (Recommended)

#### Ubuntu/Debian
```bash
# Add Lusush repository
curl -fsSL https://lusush.org/install/ubuntu.key | sudo apt-key add -
echo "deb https://lusush.org/apt stable main" | sudo tee /etc/apt/sources.list.d/lusush.list

# Update package list
sudo apt update

# Install Lusush
sudo apt install lusush

# Install additional tools (optional)
sudo apt install lusush-completion lusush-themes lusush-docs
```

#### CentOS/RHEL/Fedora
```bash
# Add Lusush repository
sudo rpm --import https://lusush.org/install/rpm.key
sudo tee /etc/yum.repos.d/lusush.repo <<EOF
[lusush]
name=Lusush Repository
baseurl=https://lusush.org/rpm/stable/
enabled=1
gpgcheck=1
gpgkey=https://lusush.org/install/rpm.key
EOF

# Install Lusush
sudo dnf install lusush  # Fedora
sudo yum install lusush  # CentOS/RHEL

# Install additional packages (optional)
sudo dnf install lusush-completion lusush-themes lusush-docs
```

#### macOS (Homebrew)
```bash
# Add Lusush tap
brew tap lusush/lusush

# Install Lusush
brew install lusush

# Install additional components (optional)
brew install lusush-completion lusush-themes
```

#### macOS (MacPorts)
```bash
# Install Lusush
sudo port install lusush

# Install additional components (optional)
sudo port install lusush-completion lusush-themes
```

#### Arch Linux
```bash
# Install from AUR
yay -S lusush

# Or using makepkg
git clone https://aur.archlinux.org/lusush.git
cd lusush
makepkg -si

# Install additional packages
yay -S lusush-completion lusush-themes
```

### Method 2: Build from Source

#### Step 1: Install Build Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential meson ninja-build git pkg-config
sudo apt install libreadline-dev libncurses-dev  # Optional dependencies
```

**CentOS/RHEL:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install meson ninja-build git pkgconfig
sudo yum install readline-devel ncurses-devel  # Optional dependencies
```

**macOS:**
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install dependencies via Homebrew
brew install meson ninja git pkg-config
brew install readline ncurses  # Optional dependencies
```

#### Step 2: Download Source Code

```bash
# Clone the repository
git clone https://github.com/lusush/lusush.git
cd lusush

# Checkout latest stable version
git checkout v1.0.0

# Or clone specific release
git clone --branch v1.0.0 https://github.com/lusush/lusush.git
```

#### Step 3: Configure Build

```bash
# Configure build (release mode)
meson setup builddir --buildtype=release

# Configure with custom prefix
meson setup builddir --buildtype=release --prefix=/usr/local

# Configure with debug symbols
meson setup builddir --buildtype=debugoptimized

# Configure with all features
meson setup builddir \
  --buildtype=release \
  --prefix=/usr/local \
  -Denable-network=true \
  -Denable-themes=true \
  -Denable-completion=true \
  -Denable-readline=true
```

#### Step 4: Build

```bash
# Build Lusush
ninja -C builddir

# Build with verbose output
ninja -C builddir -v

# Build with parallel jobs
ninja -C builddir -j $(nproc)
```

#### Step 5: Test (Optional but Recommended)

```bash
# Run test suite
ninja -C builddir test

# Run specific tests
./tests/compliance/test_posix_regression.sh
./tests/compliance/test_shell_compliance_comprehensive.sh
./tests/debug/verify_enhanced_features.sh

# Manual testing
./builddir/lusush -c "echo 'Hello, Lusush!'"
```

#### Step 6: Install

```bash
# Install to system
sudo ninja -C builddir install

# Install to custom location
DESTDIR=/tmp/lusush-install ninja -C builddir install

# Install with custom prefix
meson setup builddir --prefix=$HOME/.local
ninja -C builddir install
```

### Method 3: Binary Release

#### Download Binary
```bash
# Download latest release
curl -L https://github.com/lusush/lusush/releases/latest/download/lusush-linux-x86_64.tar.gz -o lusush.tar.gz

# Extract
tar -xzf lusush.tar.gz
cd lusush-1.0.0

# Install
sudo ./install.sh
```

#### Verify Installation
```bash
# Check installation
lusush --version
which lusush

# Test basic functionality
lusush -c "echo 'Installation successful!'"
```

## Post-Installation Setup

### 1. Add to System Shells

```bash
# Check if lusush is in /etc/shells
grep lusush /etc/shells

# Add if not present
echo "$(which lusush)" | sudo tee -a /etc/shells
```

### 2. Set as Default Shell (Optional)

```bash
# Change default shell for current user
chsh -s $(which lusush)

# Or specify path directly
chsh -s /usr/local/bin/lusush
```

### 3. Create Configuration Directory

```bash
# Create user configuration directory
mkdir -p ~/.config/lusush

# Create system configuration directory (if installing system-wide)
sudo mkdir -p /etc/lusush
```

### 4. Initial Configuration

```bash
# Copy default configuration
cp /usr/share/lusush/lusushrc ~/.config/lusush/lusushrc

# Or create basic configuration
cat > ~/.config/lusush/lusushrc << 'EOF'
# Lusush Configuration File

# History settings
HISTSIZE=10000
HISTFILESIZE=20000
HISTFILE=~/.lusush_history

# Enable enhanced features
setopt -b ENHANCED_COMPLETION
setopt -b MULTILINE_EDIT

# Set theme
theme set corporate

# Custom aliases
alias ll='ls -la'
alias grep='grep --color=auto'
alias ..='cd ..'

# Custom prompt
PS1='lusush:\w\$ '
EOF
```

### 5. Install Completions (Optional)

```bash
# System-wide completions
sudo mkdir -p /etc/lusush/completions
sudo cp completions/* /etc/lusush/completions/

# User-specific completions
mkdir -p ~/.config/lusush/completions
cp completions/* ~/.config/lusush/completions/
```

### 6. Install Themes (Optional)

```bash
# System-wide themes
sudo mkdir -p /etc/lusush/themes
sudo cp themes/* /etc/lusush/themes/

# User-specific themes
mkdir -p ~/.config/lusush/themes
cp themes/* ~/.config/lusush/themes/
```

## Configuration Options

### Build Configuration

Common meson configuration options:

```bash
# Feature toggles
-Denable-network=true/false      # Network integration
-Denable-themes=true/false       # Theme support
-Denable-completion=true/false   # Enhanced completion
-Denable-readline=true/false     # Readline support
-Denable-history=true/false      # History management
-Denable-git=true/false          # Git integration

# Build options
--buildtype=release              # Release build
--buildtype=debug                # Debug build
--buildtype=debugoptimized       # Debug with optimizations
--prefix=/usr/local              # Install prefix
--sysconfdir=/etc                # System configuration directory
--localstatedir=/var             # Variable data directory
```

### Runtime Configuration

Environment variables for runtime configuration:

```bash
# Feature control
export LUSUSH_ENHANCED=1                # Enable enhanced features
export LUSUSH_COMPLETION_ENABLED=1      # Enable completion
export LUSUSH_THEMES_ENABLED=1          # Enable themes
export LUSUSH_NETWORK_ENABLED=1         # Enable network features

# Paths
export LUSUSH_CONFIG_DIR=~/.config/lusush
export LUSUSH_SYSTEM_CONFIG_DIR=/etc/lusush
export LUSUSH_DATA_DIR=~/.local/share/lusush

# Debug options
export LUSUSH_DEBUG=1                    # Enable debug output
export LUSUSH_DEBUG_COMPLETION=1        # Debug completion
export LUSUSH_DEBUG_NETWORK=1           # Debug network features
```

## Verification

### Basic Verification

```bash
# Check version
lusush --version

# Check installation paths
which lusush
ls -la $(which lusush)

# Test basic functionality
lusush -c "echo 'Hello, World!'"
lusush -c "ls /tmp"
```

### Feature Verification

```bash
# Test completion
lusush -c "echo 'Type: ls <TAB>' and press Enter"

# Test themes
lusush -c "theme list"

# Test network features
lusush -c "network status"

# Test POSIX compliance
lusush -c "echo \$((2 + 2))"
```

### Comprehensive Testing

```bash
# Run test suite (if available)
lusush-test

# Or run manual tests
cd /usr/share/lusush/tests
./run_tests.sh
```

## Troubleshooting

### Common Issues

#### Build Issues

**Error: meson not found**
```bash
# Install meson
sudo apt install meson  # Ubuntu/Debian
sudo yum install meson  # CentOS/RHEL
brew install meson      # macOS
```

**Error: ninja not found**
```bash
# Install ninja
sudo apt install ninja-build  # Ubuntu/Debian
sudo yum install ninja-build  # CentOS/RHEL
brew install ninja            # macOS
```

**Error: compiler not found**
```bash
# Install compiler
sudo apt install build-essential  # Ubuntu/Debian
sudo yum groupinstall "Development Tools"  # CentOS/RHEL
xcode-select --install  # macOS
```

#### Runtime Issues

**Error: lusush not found**
```bash
# Check PATH
echo $PATH
which lusush

# Add to PATH if needed
export PATH="/usr/local/bin:$PATH"
```

**Error: permission denied**
```bash
# Check permissions
ls -la $(which lusush)

# Fix permissions
sudo chmod +x $(which lusush)
```

**Error: shared library not found**
```bash
# Check library dependencies
ldd $(which lusush)

# Update library cache
sudo ldconfig
```

#### Configuration Issues

**Error: configuration file not found**
```bash
# Create configuration directory
mkdir -p ~/.config/lusush

# Copy default configuration
cp /usr/share/lusush/lusushrc ~/.config/lusush/lusushrc
```

**Error: theme not found**
```bash
# Check theme directory
ls ~/.config/lusush/themes/
ls /etc/lusush/themes/

# Install default themes
lusush-install-themes
```

### Performance Issues

**Slow startup**
```bash
# Check configuration file
time lusush -c "exit"

# Disable features temporarily
export LUSUSH_NETWORK_ENABLED=0
export LUSUSH_THEMES_ENABLED=0
```

**High memory usage**
```bash
# Reduce history size
export HISTSIZE=1000
export HISTFILESIZE=2000

# Disable completion caching
export LUSUSH_COMPLETION_CACHE=0
```

## Uninstallation

### Package Manager

```bash
# Ubuntu/Debian
sudo apt remove lusush lusush-completion lusush-themes lusush-docs

# CentOS/RHEL/Fedora
sudo dnf remove lusush lusush-completion lusush-themes lusush-docs

# macOS (Homebrew)
brew uninstall lusush lusush-completion lusush-themes
```

### Manual Removal

```bash
# Remove binary
sudo rm -f /usr/local/bin/lusush

# Remove system configuration
sudo rm -rf /etc/lusush

# Remove system documentation
sudo rm -rf /usr/share/lusush
sudo rm -rf /usr/share/doc/lusush

# Remove user configuration
rm -rf ~/.config/lusush
rm -rf ~/.local/share/lusush
rm -f ~/.lusush_history
```

### Restore Previous Shell

```bash
# Change back to previous shell
chsh -s /bin/bash

# Or use default shell
chsh -s $(getent passwd $USER | cut -d: -f7)
```

## Support

### Getting Help

- **Documentation**: `/usr/share/doc/lusush/`
- **Online Manual**: https://lusush.org/docs
- **GitHub Issues**: https://github.com/lusush/lusush/issues
- **Community Forum**: https://forum.lusush.org

### Reporting Issues

When reporting installation issues, please include:

1. **System Information**:
   ```bash
   uname -a
   lsb_release -a  # Linux
   sw_vers         # macOS
   ```

2. **Installation Method**: Package manager, source build, or binary

3. **Error Messages**: Full error output with context

4. **Build Configuration**: If building from source

5. **Environment**: Relevant environment variables

### Contributing

- **Source Code**: https://github.com/lusush/lusush
- **Contributing Guide**: https://github.com/lusush/lusush/blob/main/CONTRIBUTING.md
- **Development Setup**: https://github.com/lusush/lusush/blob/main/docs/DEVELOPMENT.md

---

*This installation guide covers Lusush version 1.0. For the latest instructions, visit https://lusush.org/install*