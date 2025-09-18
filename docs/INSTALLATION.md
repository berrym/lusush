# Lusush Installation Guide

**Version**: v1.3.0-dev  
**Last Updated**: January 17, 2025  
**Platforms**: Linux, macOS, BSD

This guide provides comprehensive installation instructions for Lusush - the world's first Shell Development Environment with integrated interactive debugging.

---

## 🎯 **Quick Install (Recommended)**

### **Prerequisites**
```bash
# Install build dependencies
# Ubuntu/Debian:
sudo apt update
sudo apt install build-essential meson ninja-build libreadline-dev git

# CentOS/RHEL/Fedora:
sudo dnf install gcc gcc-c++ meson ninja-build readline-devel git

# macOS:
brew install meson ninja readline git

# FreeBSD:
pkg install meson ninja readline git
```

### **Build and Install**
```bash
# Clone the repository
git clone https://github.com/lusush/lusush.git
cd lusush

# Configure build
meson setup builddir

# Build Lusush
ninja -C builddir

# Test the build
./builddir/lusush --version

# Install system-wide (optional)
sudo ninja -C builddir install
```

### **Verify Installation**
```bash
# Test basic functionality
echo 'echo "Hello from Lusush!"' | ./builddir/lusush

# Test debugging capabilities
echo 'debug help' | ./builddir/lusush

# Test enhanced display features
./builddir/lusush --enhanced-display
```

---

## 📋 **System Requirements**

### **Minimum Requirements**
- **OS**: Linux 3.x+, macOS 10.12+, FreeBSD 11+
- **RAM**: 64 MB available memory
- **Disk**: 50 MB free space
- **Dependencies**: GNU Readline 6.0+, standard C99 compiler

### **Recommended Requirements**
- **OS**: Recent Linux distribution, macOS 11+, FreeBSD 12+
- **RAM**: 128 MB available memory
- **Disk**: 100 MB free space
- **Terminal**: Modern terminal with 256-color support
- **Git**: For enhanced git integration features

### **Development Requirements** *(for building from source)*
- **Build System**: Meson 0.53+, Ninja 1.8+
- **Compiler**: GCC 7+ or Clang 8+
- **Libraries**: libreadline-dev, standard development headers

---

## 🐧 **Linux Installation**

### **Ubuntu/Debian Systems**
```bash
# Install dependencies
sudo apt update
sudo apt install build-essential meson ninja-build libreadline-dev git pkg-config

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Test installation
./builddir/lusush --version
echo 'debug help' | ./builddir/lusush

# Install system-wide
sudo ninja -C builddir install

# Add to PATH (if not installing system-wide)
echo 'export PATH="$HOME/lusush/builddir:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### **CentOS/RHEL/Fedora Systems**
```bash
# Install dependencies
sudo dnf update
sudo dnf groupinstall "Development Tools"
sudo dnf install meson ninja-build readline-devel git pkgconfig

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Test and install
./builddir/lusush --version
sudo ninja -C builddir install
```

### **Arch Linux**
```bash
# Install dependencies
sudo pacman -S base-devel meson ninja readline git pkgconf

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
sudo ninja -C builddir install
```

### **Alpine Linux**
```bash
# Install dependencies
apk add build-base meson ninja readline-dev git pkgconfig

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

---

## 🍎 **macOS Installation**

### **Using Homebrew (Recommended)**
```bash
# Install dependencies
brew install meson ninja readline git

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush

# Configure with Homebrew readline
meson setup builddir -Dreadline_path=$(brew --prefix readline)
ninja -C builddir

# Test
./builddir/lusush --version

# Install
ninja -C builddir install
```

### **Using MacPorts**
```bash
# Install dependencies
sudo port install meson ninja readline git

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
sudo ninja -C builddir install
```

### **macOS-Specific Configuration**
```bash
# If you encounter readline issues on macOS:
export LDFLAGS="-L$(brew --prefix readline)/lib"
export CPPFLAGS="-I$(brew --prefix readline)/include"
meson setup builddir --reconfigure
ninja -C builddir
```

---

## 🐡 **BSD Installation**

### **FreeBSD**
```bash
# Install dependencies
pkg install meson ninja readline git pkgconf

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

### **OpenBSD**
```bash
# Install dependencies
pkg_add meson ninja readline git

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

### **NetBSD**
```bash
# Install dependencies
pkgin install meson ninja readline git

# Clone and build
git clone https://github.com/lusush/lusush.git
cd lusush
meson setup builddir
ninja -C builddir

# Install
ninja -C builddir install
```

---

## ⚙️ **Build Configuration Options**

### **Standard Build Options**
```bash
# Debug build
meson setup builddir --buildtype=debug

# Release build (optimized)
meson setup builddir --buildtype=release

# Custom installation prefix
meson setup builddir --prefix=/usr/local

# Disable enhanced display features
meson setup builddir -Denhanced_display=false

# Static build
meson setup builddir -Dstatic=true
```

### **Feature-Specific Options**
```bash
# Build with specific readline path
meson setup builddir -Dreadline_path=/usr/local

# Disable debugging system (not recommended)
meson setup builddir -Ddebugger=false

# Enable additional profiling features
meson setup builddir -Dprofiling=true

# Cross-compilation example
meson setup builddir --cross-file=cross_file.txt
```

### **Reconfigure Existing Build**
```bash
# Reconfigure with new options
meson configure builddir --buildtype=release
meson configure builddir --prefix=/opt/lusush

# Clean and rebuild
ninja -C builddir clean
ninja -C builddir
```

---

## 🔍 **Troubleshooting**

### **Common Build Issues**

#### **Missing Dependencies**
```bash
# Error: "meson: command not found"
# Solution: Install meson and ninja

# Ubuntu/Debian:
sudo apt install meson ninja-build

# macOS:
brew install meson ninja
```

#### **Readline Issues**
```bash
# Error: "readline/readline.h: No such file"
# Solution: Install readline development headers

# Linux:
sudo apt install libreadline-dev    # Debian/Ubuntu
sudo dnf install readline-devel     # RHEL/Fedora

# macOS:
brew install readline
export LDFLAGS="-L$(brew --prefix readline)/lib"
export CPPFLAGS="-I$(brew --prefix readline)/include"
```

#### **Meson Version Issues**
```bash
# Error: "meson version too old"
# Solution: Install newer meson

# Using pip:
pip3 install --user meson ninja

# Or use system package manager to upgrade
```

### **Runtime Issues**

#### **Debugger Not Working**
```bash
# Test debugger functionality
echo 'debug help' | ./builddir/lusush

# If no output, check build configuration:
meson configure builddir | grep debugger

# Rebuild with debugger enabled:
meson configure builddir -Ddebugger=true
ninja -C builddir
```

#### **Enhanced Display Issues**
```bash
# Test enhanced display
./builddir/lusush --enhanced-display

# If features don't work, check terminal:
echo $TERM                    # Should support colors
tput colors                   # Should be >= 8

# Force enable enhanced display:
LUSUSH_ENHANCED_DISPLAY=1 ./builddir/lusush
```

#### **Permission Issues**
```bash
# If installation fails with permissions:
sudo ninja -C builddir install

# Or install to user directory:
meson configure builddir --prefix=$HOME/.local
ninja -C builddir install

# Add to PATH:
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
```

---

## 🚀 **Post-Installation Setup**

### **Shell Integration**
```bash
# Add lusush to available shells
echo "$(which lusush)" | sudo tee -a /etc/shells

# Set as default shell (optional)
chsh -s $(which lusush)

# Or use lusush for specific scripts:
#!/usr/bin/env lusush
```

### **Configuration**
```bash
# Create configuration directory
mkdir -p ~/.config/lusush

# Basic configuration
lusush -c "config show" > ~/.config/lusush/lusush.conf

# Theme configuration
lusush -c "theme list"
lusush -c "theme set dark"
```

### **Development Setup**
```bash
# Enable debugging by default
echo 'debug on' >> ~/.config/lusush/init.lsh

# Set preferred theme
echo 'theme set dark' >> ~/.config/lusush/init.lsh

# Enable enhanced display
echo 'display enhanced on' >> ~/.config/lusush/init.lsh
```

---

## 📋 **Verification Checklist**

### **Core Functionality**
- [ ] Basic shell commands work: `echo`, `ls`, `pwd`
- [ ] POSIX features work: `for` loops, `if` statements, functions
- [ ] Multiline input works: paste complex scripts
- [ ] Exit codes work correctly

### **Debugging System**
- [ ] Debug help works: `debug help`
- [ ] Breakpoints work: `debug break add file.sh 5`
- [ ] Variable inspection works: `debug vars`
- [ ] Loop debugging works: breakpoints inside loops

### **Enhanced Features**
- [ ] Git integration: prompt shows git status
- [ ] Themes work: `theme list` and `theme set`
- [ ] Syntax highlighting: commands appear colored
- [ ] Tab completion: enhanced completion available

### **Performance**
- [ ] Startup time < 100ms
- [ ] Command execution feels responsive
- [ ] No memory leaks in basic usage
- [ ] Stable under normal workloads

---

## 🆘 **Getting Help**

### **Documentation**
- **User Guide**: Complete usage documentation
- **Debugging Guide**: Interactive debugging manual
- **Configuration Guide**: Advanced configuration options
- **Examples**: Working examples and tutorials

### **Community Support**
- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Community Q&A and help
- **Wiki**: Community-contributed documentation

### **Professional Support**
For enterprise deployments and professional support:
- Email: support@lusush.project
- Priority support available for enterprise users

---

## 📝 **Development Status**

> **Important**: Lusush is under active development. While core debugging functionality and documented features work as expected, some advanced features may be partially implemented. This installation guide covers the current stable features.

### **Current Status**
- **Debugging System**: ✅ Production ready
- **POSIX Compliance**: ✅ 100% compatible  
- **Enhanced Display**: 🟡 In active development
- **Cross-Platform**: ✅ Linux, macOS, BSD support

---

**Installation Support**: If you encounter issues not covered in this guide, please file an issue on GitHub with your system details and build output.