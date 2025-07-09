# LUSUSH Release Documentation

This directory contains detailed release notes and version history for LUSUSH shell.

## Available Releases

### Current Stable Release
- **[v1.0.6](v1.0.6.md)** - **Enhanced completion experience** with SSH completion bug fix
  - Fixed SSH command disappearing during tab completion
  - Resolved command context preservation in network completions
  - Enhanced user experience for remote development workflows
  - Maintained full backward compatibility and performance
  - Applied fix to all network commands: ssh, scp, rsync, sftp

- **[v1.0.5](v1.0.5.md)** - **Interactive excellence** with complete hints system overhaul
  - Complete resolution of all hints system issues
  - Flawless terminal behavior at any cursor position
  - Perfect cursor positioning with git-aware prompts
  - Professional-grade interactive user experience
  - Zero regression with enhanced functionality

- **[v1.0.4](v1.0.4.md)** - **First production-ready release** with all critical fixes
  - Complete terminal editing fix suite
  - Professional repository organization
  - Enhanced configuration system
  - Enterprise-grade quality and documentation
  - 100% POSIX compliance maintained

### Previous Releases
- **[v1.0.3](v1.0.3.md)** - Terminal editing fixes and hints configuration
- **[v1.0.2](v1.0.2.md)** - Enhanced themes and configuration system
- **[v1.0.1](v1.0.1.md)** - Initial production release with core features
- **[v1.0.0](v1.0.0.md)** - First stable release

## Release Schedule

LUSUSH follows semantic versioning (SemVer) with the following release cadence:

- **Major releases**: Breaking changes, new architecture
- **Minor releases**: New features, enhancements
- **Patch releases**: Bug fixes, security updates

## Version History Summary

| Version | Release Date | Key Features |
|---------|--------------|--------------|
| v1.0.6  | 2025-01-10   | Enhanced completion experience, SSH completion bug fix |
| v1.0.5  | 2025-01-07   | Interactive excellence, complete hints system overhaul, perfect UX |
| v1.0.4  | 2025-01-07   | Production-ready release, repository cleanup, all critical fixes |
| v1.0.3  | 2025-01-07   | Terminal editing fixes, hints configuration |
| v1.0.2  | 2024-12-20   | Professional themes, enhanced configuration |
| v1.0.1  | 2024-12-15   | Core features, POSIX compliance |
| v1.0.0  | 2024-12-10   | Initial stable release |

## Upgrade Guide

### From v1.0.4 to v1.0.5
- No breaking changes
- Enhanced hints system with perfect interactive experience
- All terminal positioning and cursor issues resolved
- Maintained full backward compatibility

### From v1.0.3 to v1.0.4
- No breaking changes
- Enhanced terminal editing reliability
- Professional repository organization
- Improved user experience and documentation
- All existing configurations remain compatible

### From v1.0.2 to v1.0.3
- No breaking changes
- Enhanced terminal editing experience
- Improved configuration error messages
- All existing configurations remain compatible

### From v1.0.1 to v1.0.2
- Enhanced theme system
- New configuration options
- Backward compatible with existing setups

### From v1.0.0 to v1.0.1
- Additional built-in commands
- Improved performance
- Enhanced error handling

## Getting the Latest Release

```bash
# Check current version
lusush --version

# Download latest release
git clone https://github.com/berrym/lusush.git
cd lusush
git checkout v1.0.6

# Build and install
meson setup builddir
ninja -C builddir
sudo ninja -C builddir install
```

## Release Notes Format

Each release document includes:

- **Overview**: Summary of changes and improvements
- **New Features**: Detailed description of new functionality
- **Bug Fixes**: Issues resolved in this release
- **Performance**: Performance improvements and optimizations
- **Compatibility**: Backward compatibility information
- **Installation**: Specific installation instructions
- **Known Issues**: Any known limitations or issues

## Support Policy

- **Latest Release**: Full support with active development
- **Previous Minor**: Security and critical bug fixes
- **Older Releases**: Community support only

For support questions, please:
1. Check the appropriate release notes
2. Review the [troubleshooting guide](../production/TROUBLESHOOTING.md)
3. Submit an issue on GitHub if needed

## Contributing to Releases

- Release candidates are tagged as `-rc1`, `-rc2`, etc.
- Beta releases are available for testing new features
- All releases undergo comprehensive testing before publication

See our [contributing guide](../developer/CONTRIBUTING.md) for more information on the release process.