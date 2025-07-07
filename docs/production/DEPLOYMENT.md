# LUSUSH Production Deployment Guide

## Overview

This guide provides comprehensive instructions for deploying LUSUSH shell in production environments. LUSUSH is designed for enterprise use with robust POSIX compliance, advanced features, and reliable performance.

## System Requirements

### Minimum Requirements
- **OS**: Linux, macOS, or BSD-based system
- **RAM**: 512MB available memory
- **Storage**: 100MB available disk space
- **Compiler**: C99-compatible compiler (GCC 4.9+ or Clang 3.4+)
- **Build Tools**: Meson 0.50.0+, Ninja

### Recommended Requirements
- **RAM**: 2GB or more for optimal performance
- **Storage**: 500MB for full installation with documentation
- **Terminal**: ANSI color support for enhanced features
- **Network**: Internet connection for SSH completion features

### Supported Platforms
- **Linux**: All major distributions (Ubuntu, CentOS, Fedora, Debian, RHEL)
- **macOS**: 10.12 (Sierra) and later
- **BSD**: FreeBSD, OpenBSD, NetBSD
- **Unix**: AIX, Solaris (with appropriate build tools)

## Installation Methods

### Method 1: Build from Source (Recommended)

```bash
# Clone the repository
git clone https://github.com/berrym/lusush.git
cd lusush

# Configure build
meson setup builddir --buildtype=release --prefix=/usr/local

# Compile
ninja -C builddir

# Run tests (recommended)
ninja -C builddir test
./tests/compliance/test_posix_regression.sh

# Install
sudo ninja -C builddir install
```

### Method 2: Package Installation

```bash
# Create distribution package
ninja -C builddir dist

# Install from package (example for RPM-based systems)
sudo rpm -ivh lusush-*.rpm

# Or for DEB-based systems
sudo dpkg -i lusush_*.deb
```

## Configuration

### System-wide Configuration

Create `/etc/lusush/lusushrc`:

```ini
[core]
posix_mode = true
strict_compliance = true

[history]
history_enabled = true
history_size = 10000
history_no_dups = true

[completion]
completion_enabled = true
fuzzy_completion = false
hints_enabled = false

[prompt]
prompt_style = professional
git_prompt_enabled = true

[security]
restrict_eval = true
safe_mode = true
```

### User Configuration Template

Create template at `/etc/skel/.lusushrc`:

```ini
[completion]
completion_enabled = true
hints_enabled = false

[prompt]
prompt_style = git
theme_name = corporate

[history]
history_enabled = true
history_no_dups = true
```

### Environment Variables

Add to system environment:

```bash
# /etc/environment or /etc/profile.d/lusush.sh
export LUSUSH_CONFIG_DIR="/etc/lusush"
export LUSUSH_PLUGIN_DIR="/usr/local/lib/lusush/plugins"
```

## Security Configuration

### File Permissions

```bash
# Set proper permissions
sudo chmod 755 /usr/local/bin/lusush
sudo chmod 644 /etc/lusush/lusushrc
sudo chmod 755 /etc/lusush
```

### SELinux Configuration (if applicable)

```bash
# Create SELinux policy for LUSUSH
sudo setsebool -P use_lusush_shell on
sudo restorecon -R /usr/local/bin/lusush
```

### Audit Configuration

```bash
# Add audit rules for shell execution
echo "-w /usr/local/bin/lusush -p x -k shell_exec" >> /etc/audit/rules.d/lusush.rules
sudo service auditd restart
```

## Performance Tuning

### Memory Optimization

```ini
# In system lusushrc
[performance]
history_size = 5000
completion_cache_size = 1000
max_completion_items = 50
```

### Network Optimization

```ini
[network]
ssh_timeout = 5
dns_cache_timeout = 300
network_completion_enabled = true
```

## Monitoring and Logging

### System Integration

```bash
# Create systemd service for monitoring
sudo tee /etc/systemd/system/lusush-monitor.service << EOF
[Unit]
Description=LUSUSH Shell Monitoring
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/lusush-monitor
Restart=always
User=lusush-monitor

[Install]
WantedBy=multi-user.target
EOF
```

### Log Configuration

```bash
# Configure rsyslog for LUSUSH
echo "if \$programname == 'lusush' then /var/log/lusush.log" >> /etc/rsyslog.d/50-lusush.conf
sudo systemctl restart rsyslog
```

### Monitoring Metrics

Key metrics to monitor:
- Shell startup time
- Memory usage per session
- Command execution latency
- Error rates
- User session duration

## Backup and Recovery

### Configuration Backup

```bash
#!/bin/bash
# backup-lusush-config.sh
tar -czf "lusush-config-$(date +%Y%m%d).tar.gz" \
    /etc/lusush/ \
    /etc/skel/.lusushrc \
    /usr/local/bin/lusush \
    /usr/local/lib/lusush/
```

### Recovery Procedures

```bash
# Restore from backup
tar -xzf lusush-config-YYYYMMDD.tar.gz -C /

# Verify installation
lusush --version
lusush -c "echo 'Recovery test successful'"
```

## Multi-Environment Deployment

### Development Environment

```ini
[development]
debug_mode = true
verbose_logging = true
hints_enabled = true
auto_correction = true
```

### Staging Environment

```ini
[staging]
debug_mode = false
verbose_logging = true
hints_enabled = false
performance_monitoring = true
```

### Production Environment

```ini
[production]
debug_mode = false
verbose_logging = false
hints_enabled = false
strict_compliance = true
performance_monitoring = true
audit_logging = true
```

## Testing Deployment

### Verification Script

```bash
#!/bin/bash
# verify-lusush-deployment.sh

echo "Testing LUSUSH deployment..."

# Test installation
if ! command -v lusush &> /dev/null; then
    echo "ERROR: lusush not found in PATH"
    exit 1
fi

# Test POSIX compliance
echo "Running POSIX compliance tests..."
cd /usr/local/share/lusush/tests
./compliance/test_posix_regression.sh

# Test basic functionality
echo "Testing basic functionality..."
lusush -c "echo 'Hello from LUSUSH'; exit 0"

# Test configuration loading
echo "Testing configuration..."
lusush -c "config show; exit 0"

echo "Deployment verification completed successfully!"
```

## Maintenance

### Regular Updates

```bash
# Update script
#!/bin/bash
git pull origin master
meson setup builddir --buildtype=release --reconfigure
ninja -C builddir
sudo ninja -C builddir install
systemctl restart lusush-dependent-services
```

### Health Checks

```bash
# Health check script
#!/bin/bash
# Check shell availability
pgrep -f lusush > /dev/null || echo "WARNING: No LUSUSH processes found"

# Check configuration
lusush -c "config show" > /dev/null || echo "ERROR: Configuration invalid"

# Check logs for errors
grep -i error /var/log/lusush.log | tail -10
```

### Performance Monitoring

```bash
# Performance monitoring script
#!/bin/bash
echo "LUSUSH Performance Report"
echo "========================"
echo "Active sessions: $(pgrep -f lusush | wc -l)"
echo "Memory usage: $(pgrep -f lusush | xargs ps -o pid,vsz,rss | awk 'NR>1 {sum+=$3} END {print sum "KB"}')"
echo "Average startup time: $(for i in {1..10}; do time lusush -c exit 2>&1; done | grep real | awk '{print $2}' | sort -n | awk '{a[NR]=$0} END {print a[int(NR/2)]}')"
```

## Troubleshooting

### Common Issues

1. **Permission Denied**
   ```bash
   # Fix permissions
   sudo chmod +x /usr/local/bin/lusush
   ```

2. **Configuration Not Loading**
   ```bash
   # Check configuration file syntax
   lusush --check-config
   ```

3. **Performance Issues**
   ```bash
   # Reduce history size
   config set history_size 1000
   ```

### Diagnostic Commands

```bash
# System information
lusush --version
lusush --build-info

# Configuration check
lusush --check-config

# Performance profiling
lusush --profile -c "complex command here"
```

## Security Considerations

### Access Control

- Implement proper user permissions
- Use sudo for administrative tasks
- Restrict shell access for service accounts
- Monitor shell usage through audit logs

### Network Security

- Configure firewall rules for SSH completion
- Use secure SSH configurations
- Implement network access controls
- Monitor network-related shell activities

### File System Security

- Secure configuration directories
- Implement file integrity monitoring
- Use read-only file systems where appropriate
- Regular security audits

## Support and Escalation

### Internal Support

1. Check logs: `/var/log/lusush.log`
2. Run diagnostics: `lusush --diagnose`
3. Review configuration: `config show`
4. Test POSIX compliance: `./tests/compliance/test_posix_regression.sh`

### External Support

- GitHub Issues: [https://github.com/berrym/lusush/issues](https://github.com/berrym/lusush/issues)
- Documentation: `/usr/local/share/doc/lusush/`
- Community Support: Project discussions and forums

## Appendix

### A. Sample systemd Service

```ini
[Unit]
Description=LUSUSH Shell Session
After=network.target

[Service]
Type=forking
ExecStart=/usr/local/bin/lusush
User=%i
Environment=HOME=/home/%i
WorkingDirectory=/home/%i

[Install]
WantedBy=multi-user.target
```

### B. Sample Monitoring Script

```bash
#!/bin/bash
# lusush-health-monitor.sh
while true; do
    if ! pgrep -f lusush > /dev/null; then
        logger "WARNING: No LUSUSH processes detected"
    fi
    sleep 60
done
```

### C. Configuration Migration Script

```bash
#!/bin/bash
# migrate-shell-config.sh
# Migrates from bash/zsh to LUSUSH
if [ -f ~/.bashrc ]; then
    echo "# Migrated from bash" >> ~/.lusushrc
    grep "^export" ~/.bashrc >> ~/.lusushrc
fi
```

---

This deployment guide provides comprehensive instructions for production deployment of LUSUSH shell. For additional support, consult the [troubleshooting guide](TROUBLESHOOTING.md) or contact the development team.