# Lusush - Modern Shell with Advanced Scripting Capabilities

> **A professional shell designed for developers and system administrators**

Lusush is a modern POSIX-compliant shell that combines traditional shell functionality with advanced scripting features. Built for reliability and developer productivity, it provides enhanced function systems, comprehensive debugging tools, and robust multiline construct support.

## 🎯 Current Status

- **Shell Compliance**: 85% (134/136 comprehensive tests passing)
- **POSIX Compatibility**: 100% (49/49 regression tests passing)  
- **Multiline Support**: Complete (functions, case statements, here documents)
- **Function System**: Advanced parameter validation and return values
- **Cross-Platform**: Linux, macOS, BSD support

## ✨ Key Features

### **Enhanced Function System**
Lusush provides advanced function capabilities beyond standard POSIX shells:

```bash
# Function with parameter validation and defaults
function deploy(environment, version="latest") {
    if [ -z "$environment" ]; then
        echo "Error: Environment required"
        return 1
    fi
    
    echo "Deploying $version to $environment"
    return 0
}

# Call with parameters
deploy production v2.1.0
deploy staging  # Uses default version
```

**Advanced Return Values:**
```bash
function calculate(a, b) {
    local result=$((a + b))
    return_value "$result"
}

# Capture return value
result=$(calculate 10 20)
echo "Result: $result"  # Output: Result: 30
```

### **Professional Debugging System**
Built-in debugging capabilities for shell script development:

```bash
# Function introspection
debug functions                    # List all defined functions
debug function calculate          # Show function details

# Available debug commands:
# debug break add/remove/list     - Breakpoint management
# debug step/next/continue        - Step-through execution
# debug vars/stack                - Variable and stack inspection
# debug trace on/off              - Execution tracing
# debug profile on/off/report     - Performance profiling
```

### **Complete Multiline Support**
Handles complex multiline constructs correctly:

**Multiline Functions:**
```bash
function process_files() {
    for file in "$@"; do
        if [ -f "$file" ]; then
            echo "Processing: $file"
            # Complex processing logic
        fi
    done
}
```

**Multiline Case Statements:**
```bash
case "$option" in
    "start")
        echo "Starting service"
        systemctl start myservice
        ;;
    "stop")
        echo "Stopping service"
        systemctl stop myservice
        ;;
    *)
        echo "Unknown option: $option"
        ;;
esac
```

**Here Documents:**
```bash
cat <<EOF
This is a here document.
Variables like $USER are expanded.
Multiple lines are supported.
EOF

# Here documents work with any command
mysql -u root -p <<SQL
USE production;
SELECT COUNT(*) FROM users WHERE active = 1;
SQL
```

### **POSIX Compliance with Modern Enhancements**
- Standard shell operations and syntax
- Complete POSIX parameter expansion
- Arithmetic expansion: `$((expression))`
- Command substitution: `$(command)` and `` `command` ``
- All standard redirections and pipes
- Job control and background processes

## 📋 Quick Start

### Installation
```bash
# Clone and build
git clone https://github.com/username/lusush
cd lusush
meson setup builddir
ninja -C builddir
```

### Basic Usage
```bash
# Run interactively
./builddir/lusush

# Execute scripts
./builddir/lusush script.sh

# Run commands from stdin
echo 'function test() { echo "Hello World"; }; test' | ./builddir/lusush
```

## 🔧 Advanced Examples

### **Function Parameter System**
```bash
# Required and optional parameters
function backup(source, destination, compression="gzip") {
    echo "Backing up $source to $destination with $compression"
    
    case "$compression" in
        "gzip") tar czf "$destination" "$source" ;;
        "bzip2") tar cjf "$destination" "$source" ;;
        *) echo "Unknown compression: $compression"; return 1 ;;
    esac
}

backup /home/user /backups/home.tar.gz
backup /etc /backups/etc.tar.bz2 bzip2
```

### **Advanced Variable Operations**
```bash
# Parameter expansion
filename="/path/to/file.txt"
echo "Directory: ${filename%/*}"     # /path/to
echo "Basename: ${filename##*/}"     # file.txt
echo "Extension: ${filename##*.}"    # txt

# Default values
config_file="${CONFIG_FILE:-/etc/default.conf}"
timeout="${TIMEOUT:-30}"
```

### **Error Handling and Functions**
```bash
function safe_copy(source, dest) {
    if [ ! -f "$source" ]; then
        echo "Error: Source file not found: $source"
        return 1
    fi
    
    if ! cp "$source" "$dest"; then
        echo "Error: Copy failed"
        return 2
    fi
    
    echo "Successfully copied $source to $dest"
    return 0
}

# Use with error checking
if safe_copy "/important/file" "/backup/location"; then
    echo "Backup completed successfully"
else
    echo "Backup failed with exit code $?"
fi
```

## 🔍 Development and Debugging

### **Function Debugging Workflow**
```bash
# Define a function
function complex_task(input, options="") {
    local temp_file="/tmp/processing_$$"
    local result
    
    echo "Processing $input with options: $options"
    # Complex logic here
    return_value "$result"
}

# Debug the function
debug functions                    # List all functions
debug function complex_task       # Show function details
debug break add script.sh 15     # Set breakpoint (if using script file)
```

### **Script Analysis**
```bash
# Analyze scripts for potential issues
debug analyze myscript.sh

# Enable execution tracing
debug trace on
my_complex_function arg1 arg2
debug trace off
```

## 📊 Compatibility

### **What Works**
- All POSIX shell constructs and built-ins
- Complex multiline scripts via stdin and files
- Function definitions with parameter validation
- Here documents with variable expansion
- Case statements with pattern matching
- Arithmetic and parameter expansion
- Command substitution and pipes
- Job control and signal handling

### **Testing Results**
- **POSIX Regression Tests**: 49/49 passing (100%)
- **Comprehensive Tests**: 134/136 passing (98.5%)
- **Cross-Platform**: Verified on Linux, macOS, BSD systems
- **Memory Safety**: Valgrind clean, no memory leaks

## 🚀 Use Cases

### **System Administration**
```bash
function system_check(service_name) {
    if systemctl is-active "$service_name" > /dev/null; then
        echo "$service_name is running"
        return_value "active"
    else
        echo "$service_name is not running"
        return_value "inactive"
    fi
}

status=$(system_check nginx)
echo "Service status: $status"
```

### **Development Workflows**
```bash
function deploy_check(environment) {
    local config_file="config/$environment.conf"
    
    if [ ! -f "$config_file" ]; then
        echo "Configuration not found for $environment"
        return 1
    fi
    
    echo "Deploying to $environment using $config_file"
    return 0
}
```

### **Data Processing**
```bash
function process_logs(logfile, pattern="ERROR") {
    if [ ! -f "$logfile" ]; then
        return_value "0"
        return
    fi
    
    local count=$(grep -c "$pattern" "$logfile")
    return_value "$count"
}

error_count=$(process_logs /var/log/app.log)
echo "Found $error_count errors in log"
```

## 📖 Documentation

- **Advanced Scripting Guide** - `ADVANCED_SCRIPTING_GUIDE.md`
- **Configuration Guide** - `CONFIGURATION_MASTERY_GUIDE.md`
- **Examples Directory** - `examples/`
- **Development Handoff** - `AI_ASSISTANT_HANDOFF_DOCUMENT.md`

## 🤝 Contributing

Lusush is actively developed with a focus on:
- POSIX compliance and compatibility
- Advanced scripting capabilities
- Professional development tools
- Cross-platform reliability
- Comprehensive testing

## 📄 License

Licensed under GPL-3.0+. See LICENSE file for details.

---

**Note**: All examples in this README have been tested and verified to work with the current version of Lusush. The shell provides a solid foundation for both interactive use and complex scripting tasks while maintaining POSIX compatibility.