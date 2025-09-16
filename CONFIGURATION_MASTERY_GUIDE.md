# Configuration Mastery Guide for Lusush

> **Master the most sophisticated shell configuration system available**

Lusush's configuration system is enterprise-grade, providing runtime configuration changes, validation, sectioned organization, and seamless integration with all shell features. This guide covers everything from basic configuration to advanced enterprise deployment patterns.

> **⚠️ Development Status Note**: Lusush is under active development and has undergone significant evolution. While most configuration options documented here are fully functional, some advanced features may be in various stages of integration. Future versions may introduce changes to configuration syntax, option names, or behaviors as the shell continues to mature. Always test configuration changes in your environment and refer to `config show` for the current state of available options.

## 🎯 Why Configuration Mastery Matters

Traditional shells offer limited configuration options, often requiring shell restarts or manual file editing. Lusush provides a **professional configuration management system** with:

- **Runtime configuration changes** with immediate effect
- **Structured organization** with sections and validation
- **Enterprise deployment** support with centralized management
- **Integration with debugging and development tools**
- **Type checking and error handling** for reliability

## 🏗️ Configuration System Architecture

### Configuration Hierarchy

Lusush uses a layered configuration approach:

```
1. System-wide defaults     (/etc/lusush/lusush.conf)
2. User defaults           (~/.config/lusush/lusush.conf)
3. Project-specific        (./.lusush/config)
4. Runtime overrides       (config set commands)
5. Environment variables   (LUSUSH_* variables)
```

### Configuration Sections

```ini
[history]
size = 1000
no_duplicates = true
save_on_exit = true

[completion]
git_branches = true
ssh_hosts = true
case_sensitive = false

[prompt]
show_git_status = true
theme = dark
multiline = false

[behavior]
auto_cd = true
glob_star = true
history_expansion = true

[debug]
default_level = 1
profile_memory = false
trace_functions = false

[network]
ssh_completion = true
known_hosts_file = ~/.ssh/known_hosts
git_timeout = 5
```

## 🚀 Basic Configuration Commands

### Core Configuration Operations

```bash
# View current configuration
config show                 # Show all current settings
config show history         # Show history section only
config show completion      # Show completion section only
config show prompt          # Show prompt section only

# Modify configuration
config set history_size 2000           # Set value with immediate effect
config set completion_enabled true     # Enable tab completion
config set theme_name dark              # Change theme instantly

# Get specific values
config get history_size     # Get specific setting value
config get theme_name       # Get current theme name

# Reload and save
config reload               # Reload configuration files
config save                 # Save current configuration
```

> **💡 Pro Tip**: Use `config show` to see all currently available options in your version of Lusush. Configuration capabilities continue to expand with each release.

### Configuration Validation

```bash
# Lusush validates configuration changes automatically
config set history_size "invalid"      # ERROR: Invalid integer value
config set theme_name "nonexistent"    # ERROR: Unknown theme
config set completion_threshold -1     # ERROR: Value must be positive

# Reload and save configuration
config reload               # Reload all configuration files
config save                 # Save current configuration to file
```

## 📊 Configuration Categories Deep Dive

### History Management Configuration

```bash
# History size and behavior
config set history_enabled true                 # Enable command history
config set history_size 5000                    # Maximum history entries
config set history_no_dups true                 # Remove duplicate entries
config set history_timestamps false             # Add timestamps to history
config set history_file ~/.lusush_history       # Custom history file path

# Example history configuration
config show history                              # Show all history settings
```

**Available History Options:**
- `history_enabled` (bool) - Enable command history
- `history_size` (int) - Maximum history entries
- `history_no_dups` (bool) - Remove duplicate history entries
- `history_timestamps` (bool) - Add timestamps to history
- `history_file` (string) - History file path

### Tab Completion System

```bash
# Core completion settings
config set completion_enabled true              # Enable tab completion
config set fuzzy_completion true                # Enable fuzzy matching
config set completion_threshold 70              # Fuzzy matching threshold (0-100)
config set completion_case_sensitive false      # Case-insensitive completion
config set completion_show_all false            # Show all completions
config set hints_enabled true                   # Enable input hints

# Example completion configuration
config show completion                           # Show all completion settings
```

**Available Completion Options:**
- `completion_enabled` (bool) - Enable tab completion
- `fuzzy_completion` (bool) - Enable fuzzy matching in completion
- `completion_threshold` (int) - Fuzzy matching threshold (0-100)
- `completion_case_sensitive` (bool) - Case sensitive completion
- `completion_show_all` (bool) - Show all completions
- `hints_enabled` (bool) - Enable input hints

### Prompt and Theme Configuration

```bash
# Core prompt settings
config set prompt_style git                     # Prompt style (normal, color, fancy, pro, git)
config set prompt_theme default                 # Prompt color theme
config set git_prompt_enabled true              # Enable git-aware prompts
config set git_cache_timeout 5                  # Git status cache timeout (seconds)
config set prompt_format ""                     # Custom prompt format string

# Theme settings
config set theme_name corporate                 # Active theme name
config set theme_auto_detect_colors true        # Auto-detect terminal colors
config set theme_fallback_basic false           # Fallback to basic colors
config set theme_show_right_prompt true         # Show right-side prompt
config set theme_enable_animations false        # Enable theme animations
config set theme_enable_icons false             # Enable theme icons

# Corporate branding (for corporate theme)
config set theme_corporate_company "ACME Corp"  # Company name
config set theme_corporate_department "DevOps"  # Department name
config set theme_corporate_project "WebApp"     # Project name
config set theme_corporate_environment "prod"   # Environment name
config set theme_show_company true              # Show company in prompt
config set theme_show_department false          # Show department in prompt

# Example prompt configuration
config show prompt                              # Show all prompt settings
```

**Available Prompt/Theme Options:**
- `prompt_style` (string) - Prompt style (normal, color, fancy, pro, git)
- `prompt_theme` (string) - Prompt color theme
- `git_prompt_enabled` (bool) - Enable git-aware prompts
- `git_cache_timeout` (int) - Git status cache timeout
- `prompt_format` (string) - Custom prompt format string
- `theme_name` (string) - Active theme name (corporate, dark, light, minimal, colorful, classic)
- `theme_auto_detect_colors` (bool) - Auto-detect terminal color support
- `theme_fallback_basic` (bool) - Fallback to basic colors if needed
- `theme_corporate_*` (string) - Corporate branding options
- `theme_show_*` (bool) - Display options for corporate theme

### Shell Behavior Configuration

```bash
# Core behavior settings
config set auto_cd false                        # cd into directories by typing name
config set spell_correction true                # Enable command spell correction
config set confirm_exit false                   # Confirm before exiting shell
config set tab_width 4                          # Tab width for display
config set colors_enabled true                  # Enable color output
config set color_scheme default                 # Color scheme to use
config set verbose_errors false                 # Verbose error messages
config set debug_mode false                     # Enable debug mode
config set no_word_expand false                 # Disable word expansion
config set multiline_mode true                  # Enable multiline input mode

# Auto-correction settings
config set autocorrect_max_suggestions 3        # Max correction suggestions
config set autocorrect_threshold 60             # Correction threshold (0-100)
config set autocorrect_interactive true         # Interactive corrections
config set autocorrect_learn_history true       # Learn from history
config set autocorrect_builtins true            # Correct built-in commands
config set autocorrect_external true            # Correct external commands
config set autocorrect_case_sensitive false     # Case-sensitive corrections

# Script execution control
config set script_execution true                # Allow script execution

# Example behavior configuration
config show behavior                             # Show behavior settings (if section exists)
```

**Available Behavior Options:**
- `auto_cd` (bool) - Change to directory by typing name
- `spell_correction` (bool) - Enable command spell correction
- `confirm_exit` (bool) - Confirm before exiting shell
- `tab_width` (int) - Tab width for display
- `colors_enabled` (bool) - Enable color output
- `color_scheme` (string) - Color scheme to use
- `verbose_errors` (bool) - Show verbose error messages
- `debug_mode` (bool) - Enable debug mode
- `no_word_expand` (bool) - Disable word expansion
- `multiline_mode` (bool) - Enable multiline input mode
- `autocorrect_*` (various) - Auto-correction settings
- `script_execution` (bool) - Allow script execution

### Network and SSH Configuration

```bash
# Network features
config set ssh_completion_enabled true          # Enable SSH host completion
config set cloud_discovery_enabled false       # Enable cloud provider discovery
config set cache_ssh_hosts true                 # Cache SSH host information
config set cache_timeout_minutes 5              # Cache timeout in minutes
config set show_remote_context true             # Show remote context in prompt
config set auto_detect_cloud true               # Auto-detect cloud environments
config set max_completion_hosts 50              # Maximum hosts to show in completion

# Example network configuration
config show network                             # Show network settings (if section exists)
```

**Available Network Options:**
- `ssh_completion_enabled` (bool) - Enable SSH host completion
- `cloud_discovery_enabled` (bool) - Enable cloud provider discovery
- `cache_ssh_hosts` (bool) - Cache SSH host information
- `cache_timeout_minutes` (int) - Cache timeout in minutes
- `show_remote_context` (bool) - Show remote context in prompt
- `auto_detect_cloud` (bool) - Auto-detect cloud environments
- `max_completion_hosts` (int) - Maximum hosts to show in completion

## 🏢 Enterprise Configuration Patterns

### Centralized Configuration Management

```bash
# System administrator setup
# file: /etc/lusush/enterprise.conf

[enterprise]
# Corporate standards
config_server = "https://config.company.com/lusush"
update_interval = 3600              # Check for updates hourly
mandatory_settings = "security,logging,audit"

[security]
# Security policies
disable_history_sharing = true
encrypt_history = true
audit_commands = true
restricted_commands = "rm,chmod,chown"

[logging]
# Centralized logging
log_server = "logs.company.com:514"
log_level = "INFO"
audit_log = "/var/log/lusush/audit.log"

[corporate_theme]
# Standardized appearance
primary_color = "#003366"           # Corporate blue
secondary_color = "#666666"         # Corporate gray
show_company_info = true
```

### Department-Specific Configurations

```bash
# Development team configuration
# file: ~/.config/lusush/dev-team.conf

[completion]
git_branches = true
git_remotes = true
docker_containers = true
kubernetes_contexts = true

[debug]
default_level = 2
auto_profile = true
trace_functions = true

[development]
syntax_check = true
performance_warnings = true
code_quality_checks = true

# Operations team configuration  
# file: ~/.config/lusush/ops-team.conf

[completion]
ssh_hosts = true
known_hosts_file = /etc/ssh/ssh_known_hosts
aws_profiles = true
terraform_workspaces = true

[prompt]
show_aws_profile = true
show_kubernetes_context = true
show_terraform_workspace = true

[behavior]
confirm_destructive_operations = true
audit_all_commands = true
```

### Project-Specific Configuration

```bash
# Project root: .lusush/config
[project]
name = "microservice-api"
environment = "development"

[completion]
# Project-specific completions
custom_commands = "./scripts/completions.sh"
makefile_targets = true

[debug]
# Enhanced debugging for this project
default_level = 3
breakpoint_file = ".lusush/breakpoints"
profile_output = "logs/performance.profile"

[deployment]
# Project deployment settings
staging_host = "staging.api.company.com"
production_host = "api.company.com"
health_check_url = "/health"
```

## 🔧 Advanced Configuration Techniques

### Dynamic Configuration Loading

```bash
function load_environment_config() {
    local environment=$1
    local config_file="configs/${environment}.conf"
    
    if [ ! -f "$config_file" ]; then
        echo "Configuration file not found: $config_file" >&2
        return 1
    fi
    
    echo "Loading configuration for environment: $environment"
    
    # Load configuration sections
    while IFS='=' read -r key value; do
        # Skip comments and empty lines
        case "$key" in
            \#*) continue ;;
            "") continue ;;
        esac
        
        # Apply configuration
        config set "$key" "$value"
        echo "Set $key = $value"
    done < "$config_file"
    
    # Validate loaded configuration
    config validate || {
        echo "Configuration validation failed" >&2
        return 1
    }
    
    echo "Environment configuration loaded successfully"
}

# Usage
load_environment_config "production"
load_environment_config "development"
```

### Configuration Templates and Profiles

```bash
function create_config_profile() {
    local profile_name=$1
    local template_file="templates/${profile_name}.template"
    local output_file="profiles/${profile_name}.conf"
    
    # Configuration profiles for different roles
    case "$profile_name" in
        developer)
            cat > "$output_file" << 'EOF'
[debug]
default_level = 2
auto_profile = true
trace_functions = true

[completion]
git_branches = true
docker_containers = true

[prompt]
theme = dark
show_git_status = true
EOF
            ;;
            
        devops)
            cat > "$output_file" << 'EOF'
[completion]
ssh_hosts = true
aws_profiles = true
kubernetes_contexts = true

[prompt]
theme = corporate
show_aws_profile = true
show_kubernetes_context = true

[security]
audit_commands = true
confirm_destructive = true
EOF
            ;;
            
        executive)
            cat > "$output_file" << 'EOF'
[prompt]
theme = minimal
show_git_status = false
show_timestamp = true

[behavior]
auto_cd = false
history_expansion = false

[completion]
menu_threshold = 5
EOF
            ;;
    esac
    
    echo "Created configuration profile: $profile_name"
    echo "Apply with: config load $output_file"
}
```

### Configuration Validation and Testing

```bash
function validate_enterprise_config() {
    local config_file=$1
    local errors=""
    
    echo "Validating enterprise configuration..."
    
    # Check required sections
    local required_sections="security logging audit"
    for section in $required_sections; do
        if ! config_has_section "$config_file" "$section"; then
            errors="$errors Missing required section: $section\n"
        fi
    done
    
    # Check security requirements
    local audit_enabled
    audit_enabled=$(config_get "$config_file" "security.audit_commands")
    if [ "$audit_enabled" != "true" ]; then
        errors="$errors Security audit must be enabled\n"
    fi
    
    # Check logging configuration
    local log_level
    log_level=$(config_get "$config_file" "logging.level")
    if [ "$log_level" != "info" ] && [ "$log_level" != "warn" ] && [ "$log_level" != "error" ]; then
        errors="$errors Invalid logging level: $log_level\n"
    fi
    
    # Report validation results
    if [ -z "$errors" ]; then
        echo "✅ Configuration validation passed"
        return 0
    else
        echo "❌ Configuration validation failed:"
        printf '  - %s\n' "${errors[@]}"
        return 1
    fi
}

function config_has_section() {
    local file=$1
    local section=$2
    grep -q "^\[$section\]" "$file"
}

function config_get() {
    local file=$1
    local key=$2
    local section="${key%.*}"
    local property="${key##*.}"
    
    awk -F'=' -v section="[$section]" -v prop="$property" '
        $0 == section { in_section = 1; next }
        /^\[/ { in_section = 0; next }
        in_section && $1 == prop { print $2; exit }
    ' "$file"
}
```

## 🚀 Integration with Development Workflow

### Build System Integration

```bash
# Makefile integration
# file: Makefile

.PHONY: configure-dev configure-prod configure-test

configure-dev:
	@echo "Setting up development configuration..."
	@lusush -c "config set debug.default_level 3"
	@lusush -c "config set completion.git_branches true"
	@lusush -c "config set prompt.theme dark"
	@lusush -c "config validate"

configure-prod:
	@echo "Setting up production configuration..."
	@lusush -c "config load configs/production.conf"
	@lusush -c "config set security.audit_commands true"
	@lusush -c "config validate"

configure-test:
	@echo "Setting up test configuration..."
	@lusush -c "config set debug.default_level 4"
	@lusush -c "config set debug.auto_profile true"
	@lusush -c "config validate"

# Usage:
# make configure-dev
# make configure-prod
```

### CI/CD Pipeline Integration

```bash
# file: scripts/configure-ci.sh
#!/usr/bin/env lusush

function configure_ci_environment() {
    local stage=$1
    
    echo "Configuring Lusush for CI stage: $stage"
    
    # Base CI configuration
    config set history.save_on_exit false      # Don't persist history in CI
    config set completion.timeout 1000         # Faster timeouts
    config set prompt.theme minimal            # Minimal output
    
    # Stage-specific configuration
    case "$stage" in
        build)
            config set debug.default_level 1
            config set debug.auto_profile false
            ;;
        test)
            config set debug.default_level 3
            config set debug.auto_profile true
            config set debug.output_file "test-debug.log"
            ;;
        deploy)
            config set debug.default_level 2
            config set security.audit_commands true
            config set logging.audit_log "deploy-audit.log"
            ;;
    esac
    
    # Validate configuration
    config validate || {
        echo "CI configuration validation failed" >&2
        exit 1
    }
    
    echo "CI configuration applied successfully"
}

# Usage in CI pipeline:
# ./scripts/configure-ci.sh build
# ./scripts/configure-ci.sh test
# ./scripts/configure-ci.sh deploy
```

### Docker Integration

```bash
# Dockerfile for containerized development
FROM ubuntu:22.04

# Install Lusush
RUN apt-get update && apt-get install -y \
    build-essential \
    meson \
    libreadline-dev \
    git

COPY . /lusush
WORKDIR /lusush
RUN meson setup builddir && ninja -C builddir

# Copy enterprise configuration
COPY configs/docker.conf /etc/lusush/lusush.conf

# Configure for containerized environment
RUN ./builddir/lusush -c "config set prompt.theme minimal" && \
    ./builddir/lusush -c "config set history.save_on_exit false" && \
    ./builddir/lusush -c "config set completion.timeout 500" && \
    ./builddir/lusush -c "config validate"

ENTRYPOINT ["/lusush/builddir/lusush"]
```

## 📊 Configuration Monitoring and Management

### Configuration Drift Detection

```bash
function monitor_config_drift() {
    local baseline_file=$1
    local current_config="/tmp/current_config.conf"
    
    # Export current configuration
    config export > "$current_config"
    
    # Compare with baseline
    if ! diff -q "$baseline_file" "$current_config" >/dev/null; then
        echo "⚠️  Configuration drift detected!"
        echo "Changes:"
        diff "$baseline_file" "$current_config" || true
        
        # Optionally restore baseline
        printf "Restore baseline configuration? (y/N) "
        read -r reply
        case "$reply" in
            [Yy]*) 
            config load "$baseline_file"
                echo "✅ Baseline configuration restored"
                ;;
            *)
                echo "Keeping current configuration"
                ;;
        esac
        fi
    else
        echo "✅ Configuration matches baseline"
    fi
    
    rm -f "$current_config"
}

# Usage
monitor_config_drift "configs/production-baseline.conf"
```

### Configuration Backup and Restore

```bash
function backup_configuration() {
    local backup_dir=${1:-"backups"}
    local timestamp=$(date '+%Y%m%d_%H%M%S')
    local backup_file="$backup_dir/lusush_config_$timestamp.conf"
    
    mkdir -p "$backup_dir"
    
    # Export current configuration
    config export > "$backup_file"
    
    # Create metadata
    cat > "$backup_file.meta" << EOF
timestamp: $timestamp
hostname: $(hostname)
user: $(whoami)
lusush_version: $(lusush --version)
config_hash: $(sha256sum "$backup_file" | cut -d' ' -f1)
EOF
    
    echo "Configuration backed up to: $backup_file"
}

function restore_configuration() {
    local backup_file=$1
    
    if [ ! -f "$backup_file" ]; then
        echo "Backup file not found: $backup_file" >&2
        return 1
    fi
    
    # Validate backup before restore
    if config validate "$backup_file"; then
        config load "$backup_file"
        echo "✅ Configuration restored from: $backup_file"
    else
        echo "❌ Backup file validation failed" >&2
        return 1
    fi
}
```

## 📋 Configuration Best Practices

### 1. Version Control Integration

```bash
# Keep configuration in version control
mkdir -p ~/.config/lusush/profiles
cd ~/.config/lusush

# Initialize git repository for configurations
git init
cat > .gitignore << 'EOF'
# Ignore sensitive information
*password*
*secret*
*token*
private/

# Ignore temporary files
*.tmp
*.bak
EOF

# Commit baseline configurations
git add profiles/ templates/
git commit -m "Initial configuration templates"
```

### 2. Environment-Specific Overrides

```bash
# Use environment variables for shell settings
export LUSUSH_THEME="corporate"
export LUSUSH_HISTORY_SIZE="5000"

# Configure shell behavior from environment
if [ -n "$LUSUSH_THEME" ]; then
    config set theme_name "$LUSUSH_THEME"
fi

if [ -n "$LUSUSH_HISTORY_SIZE" ]; then
    config set history_size "$LUSUSH_HISTORY_SIZE"
fi
```

### 3. Configuration Documentation

```bash
# Document your configuration choices
# file: configs/README.md

## Configuration Guide

### Development Environment
- Debug level: 3 (detailed debugging)
- Git integration: Full (branches, status, remotes)
- Theme: Dark (better for development)
- Completion: Comprehensive (git, docker, kubernetes)

### Production Environment  
- Debug level: 1 (minimal debugging)
- Security: Enhanced (audit, confirmation)
- Theme: Corporate (professional appearance)
- Completion: Essential only (performance)

### Testing Environment
- Debug level: 4 (maximum debugging)
- Profiling: Enabled (performance analysis)
- Logging: Verbose (detailed test logs)
```

## 🎓 Configuration Mastery Checklist

### Basic Configuration Mastery
- [ ] Use `config set/get` for runtime changes
- [ ] Understand configuration sections and hierarchy
- [ ] Create and use configuration profiles
- [ ] Validate configuration changes

### Intermediate Configuration Mastery
- [ ] Implement environment-specific configurations
- [ ] Create configuration templates
- [ ] Monitor and detect configuration drift
- [ ] Integrate with build systems

### Advanced Configuration Mastery
- [ ] Design enterprise configuration management
- [ ] Implement centralized configuration deployment
- [ ] Create automated configuration validation
- [ ] Build configuration monitoring systems

### Expert Configuration Mastery
- [ ] Architect multi-environment configuration strategies
- [ ] Implement configuration as code practices
- [ ] Design configuration security and compliance
- [ ] Mentor others in configuration management

## 🌟 Real-World Examples

### Startup Configuration
```bash
# Rapid development environment
config set theme_name colorful
config set completion_enabled true
config set fuzzy_completion true
config set auto_cd true
config set git_prompt_enabled true
```

### Enterprise Configuration
```bash
# Corporate compliance requirements
config set theme_name corporate
config set theme_corporate_company "ACME Corp"
config set theme_show_company true
config set autocorrect_interactive true
config set confirm_exit true
```

### DevOps Configuration
```bash
# Infrastructure automation focus
config set theme_name minimal
config set ssh_completion_enabled true
config set show_remote_context true
config set cache_ssh_hosts true
config set git_prompt_enabled true
```

## 📖 Additional Resources

- **[Advanced Scripting Guide](ADVANCED_SCRIPTING_GUIDE.md)** - Integrate configuration with advanced scripting
- **[Shell Script Configuration](docs/SHELL_SCRIPT_CONFIGURATION.md)** - Traditional shell configuration methods
- **[Configuration Examples](examples/)** - Real-world configuration examples
- **Traditional Configuration Files**:
  - `~/.profile` - POSIX login script
  - `~/.lusush_login` - Lusush login script  
  - `~/.lusushrc.sh` - Interactive shell script configuration
  - `~/.lusush_logout` - Lusush logout script
  - `~/.lusushrc` or `~/.config/lusush/lusushrc` - INI-style configuration

## ⚠️ Development Status & Future Changes

Lusush is actively developed with frequent improvements and new features. Configuration options may evolve as the shell matures:

- **Current Options**: Use `config show` to see all available configuration keys in your version
- **Future Changes**: Configuration syntax and option names may change in future releases
- **Integration Status**: Most documented options are functional, but some advanced features may be partially implemented
- **Testing Recommended**: Always verify configuration changes work as expected in your environment
- **Version Compatibility**: Configuration files may need updates when upgrading Lusush versions

**Stay Updated**: Check the CHANGELOG.md and release notes for configuration changes when upgrading.

---

**Master Lusush configuration and transform your shell into a personalized, powerful development environment.**

*Configure once, benefit everywhere! ⚡*