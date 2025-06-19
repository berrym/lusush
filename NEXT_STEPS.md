# Lusush Shell - Immediate Next Steps

## Priority Queue for Development - UPDATED CRITICAL PRIORITIES

### � **WEEKS 1-3: POSIX Command-Line Options (CRITICAL PRIORITY)**

#### 1. Week 1: Essential Invocation Options
**Goal**: Implement the most critical missing POSIX options for system integration
- [ ] **`-c command_string`** - Execute command string (TOP PRIORITY)
  - Modify `parse_opts()` in src/init.c to handle `-c` flag
  - Add command string execution mode to main loop
  - Test: `lusush -c "echo hello && ls -la"`
  
- [ ] **`-s`, `-i`, `-l` flags** - Basic shell mode control
  - `-s`: Read commands from stdin explicitly
  - `-i`: Force interactive mode regardless of input
  - `-l`: Login shell behavior (profile file processing)
  - Test all invocation combinations

#### 2. Week 2: Shell Behavior Flags
**Goal**: Implement critical shell behavior options and `set` builtin
- [ ] **`-e` flag and `set -e`** - Exit immediately on command failure
  - Add global shell state management structure
  - Integrate exit-on-error checking with execution system
  - Test: Scripts exit on first failed command
  
- [ ] **`-x` flag and `set -x`** - Trace command execution
  - Add command tracing to execution pipeline
  - Format trace output properly (show commands before execution)
  - Test: Commands display before execution
  
- [ ] **`-n`, `-u` flags** - Syntax check and unset variable error
  - `-n`: Parse but don't execute (syntax check mode)
  - `-u`: Treat unset variables as errors
  - Implement basic `set` builtin for dynamic option control

#### 3. Week 3: Complete POSIX Option Compliance  
**Goal**: Implement remaining POSIX options and named option syntax
- [ ] **Additional flags** - `-v`, `-f`, `-h`, `-m`
  - `-v`: Verbose mode (print input lines as read)
  - `-f`: Disable pathname expansion/globbing
  - `-h`: Command hashing, `-m`: Job control
  
- [ ] **Named option syntax** - `-o`/`+o` support
  - `set -o errexit` (equivalent to `set -e`)
  - `set -o xtrace` (equivalent to `set -x`)  
  - `set +o option` (unset option)
  - Map all flag names to corresponding options

**IMPACT**: Eliminates the most critical POSIX compliance gap affecting system integration

### 🎯 **WEEKS 4-5: Advanced Parameter Expansion Completion**

#### 1. Pattern Substitution Implementation
**Goal**: Complete the parameter expansion system with pattern operations
- [ ] **Basic Pattern Substitution**
  ```bash
  ${var/pattern/replacement}     # Replace first match
  ${var//pattern/replacement}    # Replace all matches
  ```
  
- [ ] **Anchored Pattern Substitution**
  ```bash
  ${var/#pattern/replacement}    # Replace if matches beginning  
  ${var/%pattern/replacement}    # Replace if matches end
  ```

#### 2. Substring Operations
**Goal**: Advanced string manipulation within parameter expansion
- [ ] **Substring Extraction**
  ```bash
  ${var:offset}                  # Substring from offset
  ${var:offset:length}           # Substring with length
  ```
  
- [ ] **Advanced Substring Handling**
  - Negative offsets and edge cases
  - Integration with existing parameter expansion system
  - Comprehensive testing with various input patterns

### 🚀 **WEEKS 6-7: Advanced File Operations**

#### 1. Advanced Redirection Support
**Goal**: Complete I/O redirection using enhanced token analysis
- [ ] **Complex Redirection Patterns**
  ```bash
  # Implement these patterns using token sequence matching
  command 2>&1 >file.log
  command &>file.log  
  command >>file.log 2>&1
  command 3>&1 1>&2 2>&3
  ```
  
- [ ] **Here Documents Implementation**
  ```bash
  # Perfect showcase for multi-token pushback
  cat <<EOF
  This is a here document
  with multiple lines
  EOF
  ```
  
- [ ] **File Descriptor Management**
  ```bash
  exec 3< input.txt         # Open file descriptor
  exec 3>&-                 # Close file descriptor
  command <&3               # Read from opened descriptor
  ```

#### 2. Process Substitution (Advanced)
**Goal**: Advanced process piping and substitution
- [ ] **Process Substitution Implementation**
  ```bash
  # Future enhancement
  diff <(command1) <(command2)
  command >(filter)
  ```

### 🎯 **WEEKS 8-10: Background Process Management**

#### 1. Job Control Implementation
**Goal**: Basic background process management
- [ ] **Background Execution**
  ```bash
  command &           # Background execution
  jobs               # List active jobs
  fg %1              # Foreground job 1
  bg %1              # Background job 1
  kill %1            # Kill job 1
  ```

- [ ] **Process Management**
  - Implement process group handling
  - Add signal propagation to background jobs
  - Create job status tracking

#### 2. Advanced Job Control Features
**Goal**: Professional job control system
- [ ] **Signal Handling**
  - `SIGTSTP`, `SIGCONT`, `SIGINT`, `SIGTERM` management
  - Proper signal propagation to process groups
  - Terminal control and job suspension
  
- [ ] **Job Status Tracking**
  - Job state management (running, stopped, done)
  - Exit status tracking and reporting
  - Job completion notification

### 🎯 **WEEKS 11-12: Testing and Polish**

#### 1. Comprehensive Testing Framework
**Goal**: Ensure reliability and POSIX compliance
- [ ] **POSIX Compliance Testing**
  - Test all implemented command-line options
  - Validate shell behavior flags work correctly
  - Run standard POSIX shell test suites
  
- [ ] **Integration Testing**  
  - Test complex option combinations
  - Validate interaction between features
  - Performance testing with real-world scripts

#### 2. Documentation and Examples
**Goal**: Professional documentation and examples
- [ ] **User Documentation**
  - Complete command-line option reference
  - Shell scripting examples and best practices
  - Migration guide from other shells
  
- [ ] **Developer Documentation**
  - Architecture documentation updates
  - API reference for shell state management
  - Contribution guidelines and coding standards
  ${var%%pattern}    # Remove longest suffix
  ```

## 🔧 **Development Strategy - UPDATED PRIORITIES**

### Immediate Actions (Weeks 1-3) - CRITICAL
1. **Implement POSIX command-line options** - Address the most critical compliance gap
2. **Focus on `-c` option first** - Enable automation and system integration
3. **Add shell behavior flags** - Implement `-e`, `-x`, `-n`, `-u` for robust scripting
4. **Complete POSIX option syntax** - Named options and full compliance

### Short-term Goals (Weeks 4-7)
1. **Complete parameter expansion** - Pattern substitution and substring operations
2. **Advanced file operations** - Here documents, complex redirection, file descriptors
3. **Enhanced I/O handling** - Process substitution and advanced piping
4. **Comprehensive testing** - Validate all new features

### Medium-term Goals (Weeks 8-12)
1. **Background process management** - Job control and process tracking
2. **Advanced job control** - Signal handling and terminal management
3. **Testing and polish** - POSIX compliance validation and documentation
4. **Performance optimization** - Real-world usage optimization

### Success Metrics - UPDATED
- [ ] `lusush -c "command"` works for automation and scripting
- [ ] Shell behavior flags (`-e`, `-x`, `-n`, `-u`) provide robust script execution
- [ ] All POSIX command-line options implemented and compliant
- [ ] Advanced parameter expansion patterns functional
- [ ] Background job control and process management working
- [ ] No regression in existing functionality
- [ ] Performance suitable for daily shell usage

### Testing Approach - POSIX Focused
```bash
# Critical option testing
lusush -c "echo hello && echo world"
lusush -e -c "false; echo should_not_execute" 
lusush -x -c "echo traced_command"
lusush -n script.sh  # syntax check only

# Advanced feature testing  
echo 'for i in 1 2 3; do for j in a b; do echo "$i$j"; done; done' | ./lusush
echo '${var/pattern/replacement}' | ./lusush
echo 'command &; jobs; fg %1' | ./lusush
```

## 📋 **Implementation Checklist - UPDATED PRIORITIES**

### CRITICAL: POSIX Command-Line Options (Weeks 1-3)
- [ ] Expand `parse_opts()` in src/init.c to handle all POSIX options
- [ ] Implement shell_options_t structure for global state management
- [ ] Add command string execution mode for `-c` option
- [ ] Integrate behavior flags with execution system in src/exec.c
- [ ] Implement `set` builtin in src/builtins/builtins.c
- [ ] Add named option syntax support (`-o`/`+o`)
- [ ] Create comprehensive option testing suite

### Advanced Parameter Expansion (Weeks 4-5)
- [ ] Implement pattern substitution in src/wordexp.c
- [ ] Add substring operations (`${var:offset:length}`)
- [ ] Handle edge cases and negative offsets
- [ ] Create parameter expansion test suite

### Advanced File Operations (Weeks 6-7)
- [ ] Implement parse_complex_redirection() function
- [ ] Add here_document parsing and execution
- [ ] Create file descriptor management system
- [ ] Update execution engine for new redirection types

### Background Process Management (Weeks 8-10)
- [ ] Implement background execution framework
- [ ] Add job control builtins (`jobs`, `fg`, `bg`, `kill`)
- [ ] Create process group and signal handling
- [ ] Implement job status tracking and notification

### Testing and Documentation (Ongoing)
- [ ] Create POSIX compliance test suite
- [ ] Add automated regression testing
- [ ] Update documentation for all new features
- [ ] Create user examples and migration guides
- [ ] Create examples of complex constructs
- [ ] Update build and testing instructions

## 🎯 **Success Definition**

By the end of this development cycle, lusush should:
1. **Parse all POSIX control structures flawlessly** in any combination
2. **Handle complex I/O redirection** including here documents
3. **Provide excellent error messages** with helpful context
4. **Maintain high performance** with the enhanced token system
5. **Have comprehensive test coverage** for all new features

This focused approach leverages the powerful token pushback system we built and establishes a solid foundation for the more advanced features in the roadmap.
