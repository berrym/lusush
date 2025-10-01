# Lusush User Guide

**Version**: v1.3.0-dev  
**Last Updated**: January 17, 2025  
**Status**: Production-Ready Core Features with Active Development

The complete guide to using Lusush - the world's first Shell Development Environment with integrated interactive debugging capabilities.

---

## 🎯 **What is Lusush?**

Lusush is a POSIX-compliant shell (85% compliance) that provides unique script development capabilities through integrated debugging:

- **Interactive Debugging**: Set breakpoints, inspect variables, step through code *(unique among shells)*
- **Modern Interface**: Syntax highlighting, autosuggestions, git integration *(in development)*
- **POSIX Foundation**: Strong POSIX compliance for standard shell scripting
- **Development Focus**: Built specifically for POSIX script development and debugging workflows

**Important**: Lusush focuses on POSIX shell scripting with debugging capabilities. For advanced shell features, Bash and Zsh remain more mature and feature-complete options.

> **Development Status**: Lusush is under active development with 85% POSIX compliance. Core debugging functionality works reliably. Advanced shell features found in Bash/Zsh (arrays, `[[ ]]`, brace expansion) are not currently supported.

---

## 🚀 **Getting Started**

### **Starting Lusush**
```bash
# Basic usage
lusush

# With enhanced display features
lusush --enhanced-display

# Execute a script
lusush script.sh

# Execute commands from stdin
echo 'echo "Hello World"' | lusush

# Interactive with debugging enabled
lusush -c "debug on"
```

### **Your First Commands**
```bash
# Basic shell operations work exactly like other shells
ls -la
cd /home/user
pwd
echo "Hello from Lusush!"

# But now you can debug your scripts interactively
debug help
```

### **Your First Debugging Session**
```bash
# Create a test script
cat > test.sh << 'EOF'
#!/usr/bin/env lusush
for i in 1 2 3; do
    echo "Processing item: $i"
    result=$((i * 2))
    echo "Result: $result"
done
echo "Processing complete"
EOF

# Enable debugging and set a breakpoint
debug on
debug break add test.sh 3

# Run the script - it will pause at the breakpoint
source test.sh
```

---

## 🔍 **Interactive Debugging System**

### **Core Debugging Concepts**

Lusush's debugging system is unique in shell design - no other shell provides interactive debugging capabilities for shell scripts.

**Key Features:**
- **Breakpoints**: Pause execution at specific lines
- **Variable Inspection**: View all variables and their values
- **Step Execution**: Execute code line by line
- **Context Display**: See source code around breakpoints
- **Interactive Commands**: Full command system within debugger

### **Basic Debugging Workflow**

#### **1. Enable Debugging**
```bash
debug on                    # Enable debugging system
debug help                  # Show all available commands
```

#### **2. Set Breakpoints**
```bash
debug break add script.sh 10    # Set breakpoint at line 10
debug break add script.sh 25    # Set breakpoint at line 25
debug break list                # List all breakpoints
```

#### **3. Run Your Script**
```bash
source script.sh               # Script will pause at breakpoints
# or
./script.sh                    # If script has #!/usr/bin/env lusush
```

#### **4. Debug Interactive Session**
When a breakpoint hits, you'll see:
```
>>> BREAKPOINT HIT <<<
Breakpoint 1 at script.sh:10 (hit count: 1)

Context at script.sh:10:
    8: for file in *.txt; do
    9:     if [ -f "$file" ]; then
>  10:         process_file "$file"
   11:     fi
   12: done

(lusush-debug) 
```

### **Debug Commands Reference**

#### **Breakpoint Management**
```bash
debug break add <file> <line>   # Set breakpoint
debug break list               # List all breakpoints
debug break remove <id>        # Remove specific breakpoint
debug break clear              # Remove all breakpoints
```

#### **Execution Control**
```bash
c                             # Continue execution
continue                      # Continue execution (same as 'c')
s                             # Step to next line
step                          # Step to next line (same as 's')
n                             # Step over function calls
next                          # Step over function calls (same as 'n')
```

#### **Variable Inspection**
```bash
vars                          # Show all variables
vars <pattern>                # Show variables matching pattern
eval <expression>             # Evaluate expression in current context
print <variable>              # Print specific variable value
```

#### **Context and Navigation**
```bash
where                         # Show current location
stack                         # Show call stack
up                           # Move up in call stack
down                         # Move down in call stack
list                         # Show source around current line
```

#### **Debug Control**
```bash
help                         # Show all debug commands
quit                         # Exit debugger and continue
q                            # Exit debugger (same as quit)
```

### **Advanced Debugging Examples**

#### **Debugging Loops** *(Production Ready)*
```bash
# Create loop script
cat > loop_debug.sh << 'EOF'
#!/usr/bin/env lusush
total=0
for i in 1 2 3 4 5; do
    echo "Processing: $i"
    total=$((total + i))
    echo "Running total: $total"
done
echo "Final total: $total"
EOF

# Debug the loop
debug on
debug break add loop_debug.sh 4  # Break inside loop
debug break add loop_debug.sh 5  # Break at calculation
source loop_debug.sh

# When breakpoint hits, inspect variables:
# (lusush-debug) vars i total
# (lusush-debug) eval echo "i=$i, total=$total"
# (lusush-debug) continue
```

#### **Debugging Functions**
```bash
# Create function script
cat > function_debug.sh << 'EOF'
#!/usr/bin/env lusush
calculate() {
    local input=$1
    local multiplier=${2:-2}
    local result=$((input * multiplier))
    echo "Calculated: $input * $multiplier = $result"
    return $result
}

for num in 3 7 12; do
    calculate $num 5
    echo "Exit code: $?"
done
EOF

# Debug functions
debug on
debug break add function_debug.sh 4  # Inside function
debug break add function_debug.sh 9  # After function call
source function_debug.sh

# Inspect function variables and parameters
# (lusush-debug) vars input multiplier result
# (lusush-debug) stack
```

#### **Debugging Conditionals**
```bash
# Create conditional script
cat > conditional_debug.sh << 'EOF'
#!/usr/bin/env lusush
check_file() {
    local file=$1
    if [ -f "$file" ]; then
        echo "File exists: $file"
        size=$(wc -l < "$file")
        echo "Lines: $size"
        return 0
    else
        echo "File not found: $file"
        return 1
    fi
}

for file in "test.txt" "missing.txt" "script.sh"; do
    check_file "$file"
    echo "Check result: $?"
done
EOF

# Debug conditionals
debug on
debug break add conditional_debug.sh 4  # At if condition
debug break add conditional_debug.sh 6  # Inside if block
debug break add conditional_debug.sh 9  # Inside else block
source conditional_debug.sh
```

---

## 🎨 **Enhanced Display Features**

### **Enabling Enhanced Display**
```bash
# Start with enhanced features
lusush --enhanced-display

# Or enable in running session
display enhanced on

# Check status
display status
```

### **Syntax Highlighting** *(In Development)*

Real-time syntax highlighting with professional color schemes:
- **Commands**: Green highlighting for valid commands
- **Strings**: Yellow highlighting for quoted strings
- **Keywords**: Blue highlighting for shell keywords (`if`, `for`, `while`)
- **Variables**: Magenta highlighting for variable references
- **Comments**: Gray highlighting for comments

```bash
# These commands will show with syntax highlighting:
for i in 1 2 3; do echo "Item: $i"; done
if [ -f "test.txt" ]; then cat "test.txt"; fi
```

### **Git Integration** *(Production Ready)*

Real-time git status in shell prompt:
```bash
user@host:/project (main ✓) $ git add .
user@host:/project (main +1) $ git commit -m "feature"  
user@host:/project (main ↑1) $ git push
user@host:/project (main ✓) $ 
```

**Git Status Indicators:**
- `✓` - Clean working directory
- `+N` - N staged files
- `!N` - N modified files  
- `?N` - N untracked files
- `↑N` - N commits ahead of remote
- `↓N` - N commits behind remote

### **Professional Themes** *(Production Ready)*

```bash
# List available themes
theme list

# Available themes:
# - classic: Traditional shell appearance
# - modern: Clean, modern design
# - dark: Dark theme for extended use
# - light: High contrast light theme
# - minimal: Distraction-free interface
# - corporate: Professional enterprise appearance

# Set theme
theme set dark
theme set corporate

# Show current theme
theme show
```

### **Tab Completion Enhancement** *(Partial Implementation)*

Enhanced context-aware completion:
```bash
# Git command completion
git che<TAB>        # Expands to 'checkout'
git checkout <TAB>  # Shows available branches

# File path completion
ls /usr/lo<TAB>     # Expands to '/usr/local/'
cd ~/Doc<TAB>       # Expands to '~/Documents/'

# Command completion
sys<TAB>           # Shows system commands: systemctl, systemd, etc.
```

---

## ⚙️ **Configuration System**

### **Basic Configuration**

Lusush features a modern configuration system with **dual interfaces** for maximum flexibility:

```bash
# Show all configuration sections
config show                           # All sections
config show shell                     # All 24 POSIX shell options
config show completion                # Tab completion settings
config show prompt                    # Prompt and theme settings

# Modern shell options interface (NEW in v1.3.0)
config set shell.errexit true        # Exit on command failure (set -e)
config set shell.xtrace on           # Trace execution (set -x) 
config set shell.posix true          # Strict POSIX compliance
config set shell.privileged true     # Security restrictions

# Traditional POSIX syntax still works perfectly
set -e                               # Same as shell.errexit true
set -o xtrace                        # Same as shell.xtrace true

# Other configuration areas
config set completion.enabled true   # Enable tab completion
config set prompt.theme dark         # Set theme
config set behavior.spell_correction true  # Smart corrections

# Get specific configuration values
config get shell.errexit            # Check shell option state
config get prompt.theme             # Get current theme
config get completion.enabled       # Check completion status
```

### **Configuration Files**

Lusush uses a hierarchical configuration system:
```bash
# System-wide configuration
/etc/lusush/lusush.conf

# User configuration
~/.config/lusush/lusush.conf

# Local project configuration
.lusush/config

# Session configuration (temporary)
# Set via config command in session
```

### **Initialization Scripts**
```bash
# User initialization script
~/.config/lusush/init.lsh

# Example init.lsh content:
# Modern shell configuration
config set shell.errexit true       # Safe scripting
config set shell.hashall true       # Performance
config set shell.emacs true         # Editing preference

# Enhanced features  
config set completion.enabled true  # Tab completion
config set prompt.theme dark        # Preferred theme
config set behavior.spell_correction true # Smart corrections

# Traditional POSIX also works
set -o pipefail                     # Pipeline failure detection

# Enable debugging by default
debug on

# Configure git integration
config set prompt.git_prompt_enabled true
```

### **Debug Configuration**
```bash
# Debug system configuration
debug level 0          # Basic debugging
debug level 1          # Verbose debugging
debug level 2          # Trace execution
debug level 3          # Advanced tracing

# Debug output configuration
debug output file      # Write debug output to file
debug output terminal  # Write debug output to terminal
debug output both      # Write to both file and terminal

# Breakpoint configuration
debug break persistent on    # Save breakpoints between sessions
debug break conditions on    # Enable conditional breakpoints
```

---

## 🏗️ **Advanced Features**

### **POSIX Compliance** *(Production Ready)*

Lusush maintains strong POSIX compliance (85%) while adding modern features:

```bash
# All standard POSIX features work exactly as expected
for i in *.txt; do
    if [ -f "$i" ]; then
        wc -l "$i"
    fi
done

# Parameter expansion
filename="/path/to/file.txt"
echo "Directory: ${filename%/*}"
echo "Basename: ${filename##*/}"
echo "Extension: ${filename##*.}"

# Command substitution
current_date=$(date '+%Y-%m-%d')
file_count=$(ls -1 | wc -l)

# Arithmetic expansion
result=$((5 + 3 * 2))
counter=$((counter + 1))
```

### **Advanced Function System** *(Production Ready)*

Enhanced function capabilities with parameter validation:

```bash
# Function with parameter validation
process_file() {
    local file=$1
    local options=${2:-""}
    
    if [ -z "$file" ]; then
        echo "Error: File parameter required"
        return 1
    fi
    
    if [ ! -f "$file" ]; then
        echo "Error: File not found: $file"
        return 2
    fi
    
    echo "Processing $file with options: $options"
    # Process the file...
    return 0
}

# Advanced return value handling
get_file_info() {
    local file=$1
    if [ -f "$file" ]; then
        local size=$(stat -c%s "$file")
        local lines=$(wc -l < "$file")
        echo "$size:$lines"  # Return structured data
        return 0
    else
        return 1
    fi
}

# Use function with structured return
if info=$(get_file_info "test.txt"); then
    size=${info%:*}
    lines=${info#*:}
    echo "File size: $size bytes, Lines: $lines"
fi
```

### **Multiline Input Support** *(Production Ready)*

Intelligent handling of multiline constructs:

```bash
# Complex multiline functions
deploy_service() {
    local service=$1
    local environment=$2
    local version=${3:-"latest"}
    
    case "$environment" in
        "production"|"prod")
            echo "Deploying $service v$version to production"
            if [ "$version" = "latest" ]; then
                echo "Warning: Using 'latest' in production"
                read -p "Continue? (y/N): " confirm
                [ "$confirm" = "y" ] || return 1
            fi
            ;;
        "staging"|"stage")
            echo "Deploying $service v$version to staging"
            ;;
        "development"|"dev")
            echo "Deploying $service v$version to development"
            ;;
        *)
            echo "Error: Unknown environment: $environment"
            return 1
            ;;
    esac
    
    # Deployment logic here...
    return 0
}

# Complex case statements
handle_signal() {
    case "$1" in
        "TERM"|"QUIT")
            echo "Graceful shutdown requested"
            cleanup_resources
            exit 0
            ;;
        "HUP")
            echo "Reloading configuration"
            reload_config
            ;;
        "USR1")
            echo "User signal 1 received"
            toggle_debug_mode
            ;;
        *)
            echo "Unknown signal: $1"
            ;;
    esac
}
```

### **Here Documents** *(Production Ready)*

Full support for here documents with variable expansion:

```bash
# Basic here document
cat << EOF
This is a here document.
Variables like $USER and $HOME are expanded.
The current date is $(date).
EOF

# Here document with SQL
mysql -u root -p << SQL
USE database_name;
SELECT COUNT(*) FROM users WHERE active = 1;
SELECT * FROM recent_logins WHERE date > '$(date -d '1 week ago' '+%Y-%m-%d')';
SQL

# Here document without expansion (quoted delimiter)
cat << 'SCRIPT'
#!/bin/bash
# This script contains literal $variables
echo "The variable $PATH will not be expanded"
SCRIPT
```

---

## 📊 **Performance and Optimization**

### **Performance Characteristics**
- **Startup Time**: < 100ms (typical)
- **Command Response**: < 1ms (sub-millisecond)
- **Memory Usage**: ~5MB base usage
- **Debug Overhead**: ~2-5% when debugging enabled
- **Theme Switching**: < 5ms

### **Performance Optimization**
```bash
# Disable features for maximum performance
lusush --minimal                # Minimal features
lusush --no-debug              # Disable debugging system
lusush --no-enhanced-display   # Disable enhanced display

# Profile script performance
debug profile on
source script.sh
debug profile report
debug profile off
```

### **Memory Management**
```bash
# Check memory usage
debug memory status

# Force garbage collection (if needed)
debug memory gc

# Monitor memory during script execution
debug memory monitor on
source large_script.sh
debug memory monitor off
```

---

## 🔧 **Troubleshooting**

### **Common Issues**

#### **Debugging Not Working**
```bash
# Check if debugging is enabled
debug status

# Enable debugging
debug on

# Check for debug output
debug level 1
echo 'echo "test"' | lusush

# Verify breakpoints
debug break list
```

#### **Enhanced Display Issues**
```bash
# Check display status
display status

# Check terminal capabilities
echo $TERM
tput colors    # Should be >= 8 for color support

# Force enable enhanced display
LUSUSH_ENHANCED_DISPLAY=1 lusush
```

#### **Git Integration Not Showing**
```bash
# Check if in git repository
git status

# Enable git integration
display git on

# Check theme supports git
theme list
theme set modern    # Themes with git support
```

#### **Performance Issues**
```bash
# Check what features are enabled
config show | grep enabled

# Disable expensive features
config set behavior.enhanced_display_mode false
config set completion.hints false
config set behavior.debug_mode false

# Profile startup time
time lusush -c exit
```

### **Debug Information**
```bash
# Show version and build information
lusush --version

# Show configuration paths
lusush --show-paths

# Show enabled features
lusush --show-features

# Generate debug report
lusush --debug-report > debug_info.txt
```

---

## 📚 **Examples and Use Cases**

### **Development Workflows**

#### **Script Development**
```bash
# Create development script
cat > deploy.sh << 'EOF'
#!/usr/bin/env lusush
set -e  # Exit on error

deploy_app() {
    local app=$1
    local env=$2
    
    echo "Deploying $app to $env"
    
    # Validation
    if [ ! -f "config/$env.conf" ]; then
        echo "Error: Configuration not found for $env"
        return 1
    fi
    
    # Deployment steps
    echo "Building application..."
    make build
    
    echo "Running tests..."
    make test
    
    echo "Deploying to $env..."
    make deploy ENV=$env
    
    echo "Deployment complete"
    return 0
}

# Debug this function
deploy_app "myapp" "staging"
EOF

# Debug the deployment script
debug on
debug break add deploy.sh 8   # Break at validation
debug break add deploy.sh 15  # Break before deployment
source deploy.sh
```

#### **System Administration**
```bash
# System monitoring script
cat > monitor.sh << 'EOF'
#!/usr/bin/env lusush

check_service() {
    local service=$1
    local status=$(systemctl is-active "$service")
    
    case "$status" in
        "active")
            echo "✓ $service is running"
            return 0
            ;;
        "inactive")
            echo "✗ $service is stopped"
            return 1
            ;;
        "failed")
            echo "✗ $service has failed"
            systemctl status "$service" | head -5
            return 2
            ;;
        *)
            echo "? $service status unknown: $status"
            return 3
            ;;
    esac
}

# Check critical services
for service in nginx postgresql redis; do
    check_service "$service"
    if [ $? -ne 0 ]; then
        echo "Issue detected with $service"
        # Debug what went wrong
        debug break add monitor.sh $LINENO
    fi
done
EOF

# Debug service monitoring
debug on
source monitor.sh
```

### **Educational Examples**

#### **Learning Shell Concepts**
```bash
# Demonstrate variable scoping
cat > scope_demo.sh << 'EOF'
#!/usr/bin/env lusush

global_var="I'm global"

test_scope() {
    local local_var="I'm local"
    global_var="Modified in function"
    
    echo "Inside function:"
    echo "  local_var: $local_var"
    echo "  global_var: $global_var"
}

echo "Before function:"
echo "  global_var: $global_var"

test_scope

echo "After function:"
echo "  global_var: $global_var"
echo "  local_var: $local_var"  # This should be empty
EOF

# Debug to see variable scoping in action
debug on
debug break add scope_demo.sh 9   # Inside function
debug break add scope_demo.sh 16  # After function
source scope_demo.sh

# Use 'vars' command at each breakpoint to see variable scoping
```

---

## 📖 **Best Practices**

### **Debugging Best Practices**

1. **Start Simple**: Begin with basic breakpoints, add complexity gradually
2. **Use Variable Inspection**: Leverage `vars` command to understand state
3. **Strategic Breakpoints**: Place breakpoints at decision points and state changes
4. **Clean Sessions**: Use `debug break clear` to start fresh debugging sessions
5. **Document Issues**: Use debug output to understand and document complex bugs

### **Performance Best Practices**

1. **Selective Features**: Only enable features you need
2. **Theme Selection**: Choose appropriate themes for your workflow
3. **Debug Overhead**: Disable debugging in production scripts
4. **Memory Awareness**: Monitor memory usage for large scripts
5. **Configuration Management**: Use configuration files for consistent setups

### **Code Organization**

1. **Function Design**: Use functions with clear parameters and return values
2. **Error Handling**: Implement proper error checking and handling
3. **Documentation**: Comment complex logic and debugging points
4. **Modular Scripts**: Break large scripts into smaller, debuggable modules
5. **Testing**: Use debugging capabilities to test edge cases

---

## 🎓 **Learning Resources**

### **Next Steps**
1. **[Debugging Guide](DEBUGGING_GUIDE.md)**: Deep dive into debugging capabilities
2. **[Configuration Guide](CONFIGURATION_GUIDE.md)**: Advanced configuration options
3. **[Examples Directory](../examples/)**: Working examples and tutorials
4. **[Feature Comparison](FEATURE_COMPARISON.md)**: Compare with other shells

### **Community Resources**
- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Community Q&A and help
- **Wiki**: Community-contributed documentation and tips

---

## ⚠️ **Development Status Reminder**

Lusush is under active development with regular updates and new features. 

### **What Works Reliably**
- Interactive debugging system (production ready)
- POSIX shell compliance (85% compatible)
- Standard POSIX shell features (variables, functions, loops, pipes, redirections)
- Configuration and theme systems
- Cross-platform compatibility

### **In Active Development**
- Advanced syntax highlighting improvements
- Enhanced autosuggestion algorithms
- Extended tab completion contexts
- Additional debugging features

### **Not Currently Supported (Use Bash/Zsh for these)**
- **Bash Extensions**: `[[ ]]` tests, associative arrays, brace expansion `{1..10}`
- **Advanced Arrays**: `array[index]` syntax, `${array[@]}` expansions
- **Process Substitution**: `<()` and `>()` constructs
- **Zsh Extensions**: Advanced glob patterns, parameter expansion modifiers
- **Advanced Parameter Expansion**: Most non-POSIX parameter expansions

### **⚠️ Critical Limitation: Variable Scope in For Loops**
Variables modified inside `for` loops do not persist outside the loop due to subshell execution:

```bash
# ❌ This will NOT work as expected
result=0
for i in 1 2 3; do
    result=$((result + i))  # Updates inside loop only
done
echo $result  # Will still be 0!

# ✅ Use while loops for variable persistence
result=0
i=1
while [ $i -le 3 ]; do
    result=$((result + i))  # Updates persist
    i=$((i + 1))
done
echo $result  # Will be 6 as expected
```

**Recommendation**: Use `while` loops when you need variables to persist across iterations.

### **When to Choose Lusush vs Others**
- **Choose Lusush**: When you need to debug POSIX shell scripts or develop new POSIX-compliant scripts
- **Choose Bash**: For system administration, existing Bash scripts, or need Bash-specific features
- **Choose Zsh**: For advanced interactive use, complex customization, or Zsh-specific features
- **Choose Fish**: For modern interactive experience without POSIX script compatibility needs

Always check the latest documentation and release notes for current feature status. Most features documented in this guide work as described, but advanced features may be subject to ongoing enhancement.

---

**Happy shell development with Lusush - the world's first Shell Development Environment!**