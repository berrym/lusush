# Lusush Integrated Debugger Guide

**The only shell with built-in interactive debugging capabilities**

**Version**: 1.4.0

---

## Table of Contents

1. [Introduction](#introduction)
2. [Quick Start](#quick-start)
3. [Debug Commands Reference](#debug-commands-reference)
4. [Debugging Workflows](#debugging-workflows)
5. [Variable Inspection](#variable-inspection)
6. [Execution Control](#execution-control)
7. [Performance Profiling](#performance-profiling)
8. [Advanced Features](#advanced-features)
9. [Best Practices](#best-practices)
10. [Troubleshooting](#troubleshooting)

---

## Introduction

Lusush is the first and only shell to include a complete integrated debugger. This unique capability allows you to debug shell scripts interactively without any external tools, making script development, troubleshooting, and learning significantly more effective.

### Why Integrated Debugging Matters

**Traditional shell debugging limitations:**
- Limited to `set -x` tracing with cluttered output
- No variable inspection during execution
- No breakpoints or step-by-step execution
- Difficult to understand complex script flow

**Lusush integrated debugger advantages:**
- Interactive debugging with professional commands
- Real-time variable inspection with scope awareness
- Step-by-step execution control
- Performance profiling capabilities
- Educational value for learning shell scripting

### Who Benefits from Integrated Debugging

- **Developers**: Debug complex automation scripts
- **DevOps Engineers**: Troubleshoot deployment and CI/CD scripts
- **System Administrators**: Analyze and fix system scripts
- **Students/Educators**: Learn shell scripting interactively
- **Anyone**: Who writes shell scripts and wants to understand their behavior

### ⚠️ Important Limitation: Variable Scope in For Loops

**Critical**: Variables modified inside `for` loops do not persist outside the loop due to subshell execution:

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

---

## Quick Start

### Enable Debugging

The debugger is accessed through the built-in `debug` command:

```bash
# Show debugger help
debug help

# Enable debugging
debug on

# Enable with specific verbosity level (0-4)
debug on 2

# Check current debug status
debug

# Disable debugging
debug off
```

### Your First Debugging Session

```bash
# Create a simple test script
cat > first_debug.sh << 'EOF'
#!/usr/bin/env lusush
counter=1
echo "Starting loop"
while [ $counter -le 3 ]; do
    echo "Iteration: $counter"
    counter=$((counter + 1))
done
echo "Loop complete"
EOF

# Debug it interactively
debug on
debug vars                    # Show all variables
source first_debug.sh
debug off
```

### Debug Output Example

```bash
$ debug on
[DEBUG] Debug mode enabled (level: 0)
[DEBUG] Debug session started at: 23592.704952486
Debug mode enabled

$ VAR="hello"; echo $VAR
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: VAR=hello
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: echo
hello

$ debug vars
[DEBUG] ------------------------------------------------------------
[DEBUG] Variable Inspection
[DEBUG] ------------------------------------------------------------
[DEBUG] Current scope: global
[DEBUG]
[DEBUG] Shell Variables:
[DEBUG]   VAR      = 'hello'
[DEBUG]   PWD      = '/home/user/project'
[DEBUG]   ?        = '0'
[DEBUG]   $        = '12345'
[DEBUG]
Use 'debug print <varname>' to inspect specific variables
```

---

## Debug Commands Reference

### Core Commands

| Command | Description | Example |
|---------|-------------|---------|
| `debug help` | Show complete command reference | `debug help` |
| `debug` | Show current debug status | `debug` |
| `debug on [level]` | Enable debugging (optional level 0-4) | `debug on 2` |
| `debug off` | Disable debugging | `debug off` |

### Debug Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | None | Debugging disabled |
| 1 | Basic | Basic debugging information |
| 2 | Verbose | Verbose debugging output |
| 3 | Trace | Trace execution with detailed logging |
| 4 | Profile | Full profiling with performance data |

### Variable Inspection

| Command | Description | Example |
|---------|-------------|---------|
| `debug vars` | Show all variables in current scope | `debug vars` |
| `debug print <var>` | Print specific variable value | `debug print PATH` |

### Execution Control

| Command | Description | Example |
|---------|-------------|---------|
| `debug step` | Step into next statement | `debug step` |
| `debug next` | Step over next statement | `debug next` |
| `debug continue` | Continue execution | `debug continue` |

### Function Analysis

| Command | Description | Example |
|---------|-------------|---------|
| `debug functions` | List all defined functions | `debug functions` |
| `debug function <name>` | Show specific function definition | `debug function myFunc` |

### Execution Tracing

| Command | Description | Example |
|---------|-------------|---------|
| `debug trace on` | Enable execution tracing | `debug trace on` |
| `debug trace off` | Disable execution tracing | `debug trace off` |

### Performance Profiling

| Command | Description | Example |
|---------|-------------|---------|
| `debug profile on` | Enable performance profiling | `debug profile on` |
| `debug profile off` | Disable profiling | `debug profile off` |
| `debug profile report` | Show performance report | `debug profile report` |
| `debug profile reset` | Reset profiling data | `debug profile reset` |

### Advanced Features

| Command | Description | Example |
|---------|-------------|---------|
| `debug break add <file> <line>` | Add breakpoint | `debug break add script.sh 15` |
| `debug break remove <id>` | Remove breakpoint | `debug break remove 1` |
| `debug break list` | List all breakpoints | `debug break list` |
| `debug break clear` | Clear all breakpoints | `debug break clear` |
| `debug stack` | Show call stack | `debug stack` |
| `debug analyze <script>` | Analyze script for issues | `debug analyze myscript.sh` |

---

## Debugging Workflows

### Script Development Workflow

```bash
# 1. Enable debugging during development
debug on 2

# 2. Test your script with verbose output
./myscript.sh

# 3. Inspect variables when needed
debug vars

# 4. Check specific variable values
debug print myvar

# 5. Disable when satisfied
debug off
```

### Troubleshooting Existing Scripts

```bash
# 1. Enable tracing to see execution flow
debug trace on

# 2. Run the problematic script
./problematic_script.sh

# 3. Examine variables at failure point
debug vars

# 4. Analyze specific values
debug print error_var

# 5. Clean up
debug trace off
```

### Learning and Education Workflow

```bash
# 1. Enable verbose debugging
debug on 3

# 2. Step through educational examples
for i in 1 2 3; do
    echo "Processing: $i"
    result=$((i * 2))
    debug print i
    debug print result
done

# 3. Understand variable changes
debug vars
```

---

## Variable Inspection

### Basic Variable Display

```bash
# Show all variables in current scope
debug vars

# Example output:
[DEBUG] Shell Variables:
[DEBUG]   VAR      = 'hello'
[DEBUG]   COUNTER  = '3'
[DEBUG]   PWD      = '/home/user/project'
[DEBUG]   HOME     = '/home/user'
[DEBUG]   PATH     = '/usr/local/bin:/usr/bin:/bin'
[DEBUG]   USER     = 'user'
[DEBUG]   ?        = '0'
[DEBUG]   $        = '12345'
```

### Specific Variable Inspection

```bash
# Inspect a specific variable
debug print HOME
debug print PATH
debug print my_custom_var

# Check special variables
debug print ?     # Last exit status
debug print $     # Current process ID
```

### Variable Scoping

The debugger shows variables in the current scope:
- **Global variables**: Available throughout the session
- **Local variables**: Function-specific variables
- **Environment variables**: System environment
- **Special variables**: Shell-specific variables ($?, $$, etc.)

### Variable Monitoring

```bash
# Enable debugging and watch variable changes
debug on 2

counter=1
debug print counter    # Shows: counter = '1'

counter=$((counter + 1))
debug print counter    # Shows: counter = '2'
```

---

## Execution Control

### Step-by-Step Debugging

```bash
# Enable stepping mode
debug step

# Your script will pause at each command
# Use debug continue to proceed
# Use debug next to step over function calls
```

### Tracing Execution

```bash
# Enable detailed execution tracing
debug trace on

# Run commands to see detailed trace
for file in *.txt; do
    echo "Processing: $file"
done

# Example trace output:
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: for
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: echo
Processing: file1.txt
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: echo
Processing: file2.txt
```

### Conditional Debugging

```bash
# Debug only when specific conditions are met
if [ "$DEBUG_MODE" = "true" ]; then
    debug on 3
fi

# Your script logic here

if [ "$DEBUG_MODE" = "true" ]; then
    debug vars
    debug off
fi
```

---

## Performance Profiling

### Basic Profiling

```bash
# Enable performance profiling
debug profile on

# Run your script
./performance_test.sh

# View performance report
debug profile report

# Clean up
debug profile off
```

### Function-Level Profiling

```bash
# Profile specific functions
debug profile on

slow_function() {
    sleep 1
    echo "Slow operation complete"
}

fast_function() {
    echo "Quick operation"
}

# Call functions
slow_function
fast_function

# Check performance report
debug profile report
```

### Performance Analysis

The profiler provides insights into:
- **Function execution times**: How long each function takes
- **Call counts**: How many times functions are called
- **Resource usage**: Memory and CPU utilization patterns
- **Bottleneck identification**: Which parts of your script are slow

---

## Advanced Features

### Breakpoint Management

```bash
# Add breakpoints (when breakpoint support is fully implemented)
debug break add myscript.sh 25
debug break add another_script.sh 10

# List active breakpoints
debug break list

# Remove specific breakpoint
debug break remove 1

# Clear all breakpoints
debug break clear
```

### Stack Trace Analysis

```bash
# View call stack
debug stack

# Example stack output shows:
# - Current function
# - Calling function
# - Script file and line numbers
# - Variable context at each level
```

### Script Analysis

```bash
# Analyze script for common issues
debug analyze myscript.sh

# Analysis covers:
# - Syntax validation
# - Variable usage patterns
# - Potential performance issues
# - Best practice recommendations
```

---

## Best Practices

### Development Best Practices

1. **Start with basic debugging**
   ```bash
   debug on 1    # Basic level first
   ```

2. **Use appropriate verbosity levels**
   ```bash
   debug on 1    # Development
   debug on 2    # Troubleshooting
   debug on 3    # Deep debugging
   ```

3. **Inspect variables strategically**
   ```bash
   debug vars           # Overview
   debug print key_var  # Specific inspection
   ```

4. **Clean up debugging code**
   ```bash
   debug off    # Always disable when done
   ```

### Production Debugging

1. **Use conditional debugging**
   ```bash
   if [ "$DEBUG" = "1" ]; then
       debug on 1
   fi
   ```

2. **Avoid high verbosity levels in production**
   ```bash
   # Avoid debug on 3 or 4 in production
   debug on 1    # Maximum for production
   ```

3. **Profile performance carefully**
   ```bash
   debug profile on
   # ... critical section ...
   debug profile report
   debug profile off
   ```

### Educational Use

1. **Start with simple examples**
   ```bash
   debug on 2
   echo "Hello, debugging!"
   debug vars
   ```

2. **Demonstrate variable changes**
   ```bash
   debug on 2
   var=1
   debug print var
   var=$((var + 1))
   debug print var
   ```

3. **Show execution flow**
   ```bash
   debug trace on
   if [ -f "file.txt" ]; then
       echo "File exists"
   else
       echo "File not found"
   fi
   debug trace off
   ```

---

## Troubleshooting

### Common Issues

**Debug commands not working:**
```bash
# Ensure you're using Lusush
./builddir/lusush
debug help    # Should show debug commands
```

**No debug output visible:**
```bash
# Check if debugging is enabled
debug           # Shows current status
debug on        # Enable if needed
```

**Too much debug output:**
```bash
# Reduce verbosity level
debug level 1   # Less verbose
debug off       # Disable completely
```

### Debug Output Interpretation

**Understanding trace output:**
```bash
[DEBUG] TRACE: ../src/executor.c:427 - COMMAND: echo
```
- **File location**: Where in Lusush source the trace originates
- **Line number**: Specific line in source
- **Command**: The shell command being executed

**Variable inspection format:**
```bash
[DEBUG]   VAR      = 'value'
```
- **Variable name**: Left side
- **Value**: Right side in quotes
- **Scope**: Indicated in section headers

### Performance Considerations

**Debug overhead:**
- Level 1-2: Minimal performance impact
- Level 3: Moderate overhead from detailed tracing
- Level 4: Significant overhead from full profiling

**Memory usage:**
- Debug information is stored in memory
- Large scripts may use additional memory for debugging
- Use `debug off` to free debug resources

---

## Integration with Shell Features

### POSIX Options Compatibility

Debugging works seamlessly with all POSIX shell options:

```bash
# Strict error handling with debugging
set -eu
debug on 2
# Your script here
```

### Security Features

```bash
# Debugging in privileged mode
set -o privileged
debug on 1     # Limited debugging in secure mode
```

### Configuration Integration

```bash
# Use config system with debugging
config set autocorrect.enabled true
debug on
# Debugging shows autocorrection in action
```

---

## Example Scenarios

### Debugging a Deployment Script

```bash
#!/usr/bin/env lusush

# Enable debugging for troubleshooting
if [ "$DEPLOY_DEBUG" = "1" ]; then
    debug on 2
fi

# Deployment logic
check_prerequisites() {
    debug print "Prerequisites check starting"
    # Check logic here
}

deploy_application() {
    debug vars    # Show all deployment variables
    # Deployment logic here
}

# Main deployment
debug trace on    # Trace critical operations
check_prerequisites
deploy_application
debug trace off

if [ "$DEPLOY_DEBUG" = "1" ]; then
    debug profile report
    debug off
fi
```

### Educational Loop Debugging

```bash
#!/usr/bin/env lusush

# Teaching example: Understanding loop behavior
debug on 2
echo "Learning about loops with debugging"

counter=1
max=3

debug print counter
debug print max

while [ $counter -le $max ]; do
    echo "Loop iteration: $counter"
    debug print counter
    counter=$((counter + 1))
    debug print counter
done

debug vars
echo "Loop complete - final state above"
debug off
```

### Performance Analysis Example

```bash
#!/usr/bin/env lusush

# Performance testing with profiling
debug profile on

efficient_function() {
    echo "Fast operation"
}

slow_function() {
    # Simulate slow operation
    sleep 0.1
    echo "Slower operation"
}

# Test performance
for i in $(seq 1 5); do
    efficient_function
    slow_function
done

debug profile report
debug profile off
```

---

## Conclusion

The Lusush integrated debugger is a powerful, unique tool that transforms shell script development. By providing interactive debugging capabilities directly within the shell, it enables more effective script development, troubleshooting, and learning.

### Key Advantages

- **Unique capability**: No other shell offers integrated debugging
- **Professional tools**: Breakpoints, variable inspection, profiling
- **Educational value**: Perfect for learning shell scripting
- **Production ready**: Suitable for troubleshooting real-world scripts
- **Zero external dependencies**: Everything built into the shell

### Getting Started

1. **Try the basic commands**: `debug help`, `debug on`, `debug vars`
2. **Experiment with simple scripts**: Use the examples in this guide
3. **Apply to your scripts**: Start debugging your own shell scripts
4. **Explore advanced features**: Breakpoints, profiling, analysis

The integrated debugger makes Lusush not just another shell, but a complete shell development environment. Whether you're developing new scripts, troubleshooting existing ones, or learning shell scripting, the debugger provides capabilities that simply aren't available anywhere else.

**Experience the future of shell scripting with integrated debugging.**