# POSIX Command-Line Options Implementation Plan

## Critical Gap Analysis

**CURRENT STATE**: lusush v0.2.1 has comprehensive core shell functionality but lacks essential POSIX command-line options, representing the most critical compliance gap.

**IMPACT**: This gap prevents lusush from being used as:
- A system shell replacement
- An automation shell for scripts and CI/CD
- A compatible shell for existing POSIX-compliant workflows

## Missing POSIX Command-Line Options

### Critical Invocation Options (Week 1 Priority)

**`-c command_string`** - Execute command string and exit
- **Usage**: `lusush -c "echo hello; ls -la"`
- **Critical for**: Automation, scripting, system integration
- **Used by**: cron, systemd, make, CI/CD pipelines, other programs

**`-s`** - Read commands from standard input
- **Usage**: `echo "ls -la" | lusush -s`
- **Critical for**: Explicit stdin processing
- **Used by**: Script processors, command pipelines

**`-i`** - Force interactive mode
- **Usage**: `lusush -i < script.sh` (interactive with script input)
- **Critical for**: Debugging, testing, development workflows

**`-l`** - Login shell behavior
- **Usage**: `lusush -l` (read profile files, login shell setup)
- **Critical for**: System login shells, profile processing

### Essential Behavior Flags (Week 2 Priority)

**`-e`** - Exit immediately if a command exits with non-zero status
- **Usage**: `lusush -e script.sh` or `set -e`
- **Critical for**: Robust script execution, error handling
- **POSIX requirement**: Essential for shell script reliability

**`-x`** - Print commands and their arguments as they are executed
- **Usage**: `lusush -x script.sh` or `set -x`
- **Critical for**: Debugging, tracing, development
- **POSIX requirement**: Essential debugging tool

**`-n`** - Read commands but do not execute them (syntax check)
- **Usage**: `lusush -n script.sh` or `set -n`
- **Critical for**: Syntax validation, CI/CD validation pipelines
- **POSIX requirement**: Essential for script validation

**`-u`** - Treat unset variables as an error
- **Usage**: `lusush -u script.sh` or `set -u`
- **Critical for**: Script safety, preventing silent failures
- **POSIX requirement**: Important for robust scripting

### Additional POSIX Flags (Week 3 Priority)

**`-v`** - Print shell input lines as they are read (verbose)
- **Usage**: `lusush -v script.sh` or `set -v`
- **Used for**: Input tracing, debugging input processing

**`-f`** - Disable pathname expansion (globbing)
- **Usage**: `lusush -f` or `set -f`
- **Used for**: Preventing unwanted glob expansion

**`-h`** - Remember command locations (hash table)
- **Usage**: `lusush -h` or `set -h`
- **Used for**: Performance optimization, command caching

**`-m`** - Enable job control
- **Usage**: `lusush -m` or `set -m`
- **Used for**: Background job management, process control

### Named Option Control (Week 3 Priority)

**`-o option`** - Set named option
- **Usage**: `lusush -o errexit` (equivalent to `-e`)
- **Named options**: `errexit`, `xtrace`, `nounset`, `verbose`, etc.
- **POSIX requirement**: Complete option name support

**`+o option`** - Unset named option  
- **Usage**: `lusush +o errexit` (disable exit on error)
- **Used for**: Dynamic option control, option toggling

## Implementation Architecture

### Current Option Parsing (src/init.c)

```c
// Current implementation only supports:
// -h, --help: Show help message
// -v, --version: Show version information
```

### Required Implementation

**1. Expand option parsing in `parse_opts()`**
```c
typedef struct shell_options {
    bool command_mode;          // -c flag
    char *command_string;       // -c argument
    bool stdin_mode;            // -s flag  
    bool interactive;           // -i flag
    bool login_shell;           // -l flag
    bool exit_on_error;         // -e flag
    bool trace_execution;       // -x flag
    bool syntax_check;          // -n flag
    bool unset_error;           // -u flag
    bool verbose;               // -v flag
    bool no_globbing;           // -f flag
    bool hash_commands;         // -h flag
    bool job_control;           // -m flag
} shell_options_t;
```

**2. Add global shell state management**
```c
// Global shell options accessible throughout the program
extern shell_options_t shell_opts;

// Option checking functions
bool should_exit_on_error(void);
bool should_trace_execution(void);
bool is_syntax_check_mode(void);
```

**3. Implement `set` builtin (src/builtins/builtins.c)**
```c
int builtin_set(char **args);
// Handle: set -e, set +e, set -o errexit, set +o errexit, etc.
```

**4. Integrate options with execution system (src/exec.c)**
```c
// In execute_command():
if (should_exit_on_error() && exit_status != 0) {
    exit(exit_status);
}

if (should_trace_execution()) {
    print_command_trace(node);
}
```

## Implementation Schedule

### Week 1: Critical Invocation Options
- **Day 1-2**: Implement `-c command_string` option
  - Expand `parse_opts()` to handle `-c` flag
  - Add command string execution mode
  - Test: `lusush -c "echo test"`
  
- **Day 3-4**: Add `-s`, `-i`, `-l` flags
  - Implement stdin mode detection
  - Add interactive mode forcing
  - Add login shell behavior
  
- **Day 5**: Integration testing
  - Test all invocation modes
  - Verify compatibility with existing functionality

### Week 2: Shell Behavior Flags  
- **Day 1-2**: Implement `-e` (exit on error)
  - Add exit-on-error checking to execution system
  - Integrate with command execution pipeline
  - Test: Script fails on first error
  
- **Day 3**: Implement `-x` (trace execution)
  - Add command tracing to execution system
  - Format trace output properly
  - Test: Commands show before execution
  
- **Day 4**: Implement `-n` (syntax check) and `-u` (unset error)
  - Add syntax-only mode (parse but don't execute)
  - Add unset variable checking
  - Test: Syntax errors detected, unset vars cause errors
  
- **Day 5**: Implement basic `set` builtin
  - Handle `set -e`, `set +e`, `set -x`, `set +x`
  - Dynamic option toggling
  - Test: Runtime option changes work

### Week 3: Complete POSIX Compliance
- **Day 1**: Add remaining flags (`-v`, `-f`, `-h`, `-m`)
  - Implement verbose mode, globbing control
  - Add command hashing, job control flags
  
- **Day 2-3**: Implement named options (`-o`/`+o`)
  - Map flag names to options (errexit==-e, xtrace==-x)
  - Handle all POSIX named options
  - Test: `set -o errexit` equivalent to `set -e`
  
- **Day 4-5**: Comprehensive testing and validation
  - Test all option combinations
  - Verify POSIX compliance with standard test suites
  - Integration testing with existing features

## Validation Plan

### Manual Testing
```bash
# Test -c option
lusush -c "echo hello && echo world"
lusush -c "false && echo should_not_print"

# Test behavior flags
lusush -e -c "false; echo should_not_execute"
lusush -x -c "echo traced_command"
lusush -n script.sh  # syntax check only

# Test set builtin
echo "set -e; false; echo unreachable" | lusush
echo "set -x; echo traced" | lusush
```

### Integration Testing
- Verify all existing functionality works with new options
- Test option inheritance in subshells
- Validate performance impact is minimal
- Check memory management of option state

### POSIX Compliance Testing
- Run standard POSIX shell test suites
- Validate against real-world automation scripts
- Test compatibility with existing shell scripts

## Success Criteria

### Week 1 Success
- [ ] `lusush -c "command"` executes single commands correctly
- [ ] `-s`, `-i`, `-l` flags control shell behavior appropriately
- [ ] All invocation modes work without breaking existing functionality

### Week 2 Success  
- [ ] `set -e` causes shell to exit on command failures
- [ ] `set -x` traces command execution clearly
- [ ] `set -n` performs syntax checking without execution
- [ ] `set -u` detects and errors on unset variables

### Week 3 Success
- [ ] All POSIX command-line options implemented and functional
- [ ] Named option syntax (`-o`/`+o`) works correctly
- [ ] Option inheritance and state management robust
- [ ] lusush passes standard POSIX option compliance tests

## Impact Assessment

**After Implementation**:
- lusush becomes suitable for system shell replacement
- Automation and CI/CD integration becomes possible
- POSIX compliance gap eliminated
- Foundation ready for advanced shell features

**Risk Mitigation**:
- Incremental implementation prevents breaking existing features
- Comprehensive testing at each phase
- Option state properly isolated from core functionality
- Backwards compatibility maintained for current usage patterns

This implementation plan addresses the most critical compliance gap and establishes lusush as a production-ready POSIX shell.
