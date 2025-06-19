# Lusush Shell Development Roadmap

## Current State Assessment
- ✅ Enhanced multi-token pushback system
- ✅ Basic POSIX control structures (if/then/else, for, while, until, case)
- ✅ Core builtins (echo, export, source, test, read, eval, etc.)
- ✅ Input buffering and completion detection
- ✅ Command line completion with linenoise
- ✅ Word expansion and pathname expansion
- ✅ Professional build system and versioning
- ✅ **Variable assignment and expansion system** - COMPLETED (all patterns work)
- ✅ **Scanner token typing fix** - CRITICAL BUG RESOLVED
- ✅ **Semicolon command separation** - WORKING (echo a; echo b)
- ✅ **Logical operators (&&, ||)** - COMPLETELY WORKING with proper short-circuit semantics
- ✅ **Multi-character operator parsing** - Scanner properly handles `&&`, `||`, `>>`, etc.
- ✅ **Pipeline vs logical operator separation** - Proper routing to correct execution systems

## 🎯 MAJOR BREAKTHROUGH: Mixed Operators Completely Fixed ✅

**CRITICAL ACHIEVEMENT**: Mixed operator parsing now works perfectly - `cmd | pipe && logical` expressions functional

**Technical Solution Implemented**:
- **Modified `is_command_delimiter()`**: Excluded TOKEN_PIPE to prevent premature command splitting
- **Enhanced `parse_simple_command()`**: Added explicit NODE_PIPE creation for pipe tokens
- **Improved `execute_command()`**: Added pipeline detection and routing to execute_pipeline_from_node
- **Robust architecture**: Proper separation between pipeline and logical operator execution

**Real-World Impact**: Complex expressions like `echo test | grep test && echo found` now work correctly

**Previous Limitations RESOLVED**:
- ✅ **Mixed operators** - `echo test | grep test || echo not_found` - NOW WORKING
- ✅ **Pipeline + logical combinations** - Commands like `cmd1 | cmd2 && cmd3` - NOW WORKING  
- ✅ **Proper operator precedence** - Pipes execute before logical operators - NOW WORKING

## Priority 1: CRITICAL POSIX Command-Line Options (URGENT - TOP PRIORITY)

### 1.1 Fundamental Command-Line Interface (CRITICAL MISSING)
**Goal**: Implement essential POSIX command-line options for basic shell usage
- [ ] **`-c command_string`** - Execute command string (CRITICAL for automation and scripts)
- [ ] **`-s`** - Read commands from standard input explicitly  
- [ ] **`-i`** - Force interactive mode regardless of input source
- [ ] **`-l`** - Make shell act as login shell (read profile files)

### 1.2 Shell Behavior Control Options (HIGH PRIORITY)
**Goal**: Implement POSIX shell behavior flags and `set` builtin
- [ ] **`-e`** - Exit immediately on command failure (`set -e`)
- [ ] **`-x`** - Print commands and arguments as executed - trace mode (`set -x`)
- [ ] **`-n`** - Read commands but don't execute - syntax check mode (`set -n`)
- [ ] **`-u`** - Treat unset variables as error (`set -u`)
- [ ] **`-v`** - Print shell input lines as read - verbose mode (`set -v`)
- [ ] **`-f`** - Disable pathname expansion/globbing (`set -f`)
- [ ] **`-h`** - Remember command locations in hash table (`set -h`)
- [ ] **`-m`** - Enable job control (`set -m`)

### 1.3 Named Option Control (COMPLETE POSIX COMPLIANCE)
**Goal**: Implement full POSIX option syntax
- [ ] **`-o option`** - Set named option (`set -o errexit`, `set -o xtrace`, etc.)
- [ ] **`+o option`** - Unset named option
- [ ] **Option inheritance** - Subshell option propagation
- [ ] **Global shell state management** - Track all option flags consistently

**IMPACT**: This represents the most critical gap in POSIX compliance, affecting:
- System integration (cron, systemd, automation tools)
- Developer workflow (CI/CD pipelines, build systems)
- Shell testing and validation
- Basic shell replacement capability

## Priority 2: Advanced Parameter Expansion Completion (WEEKS 4-5)

### 2.1 Pattern Substitution (BUILD ON EXISTING FOUNDATION)
**Goal**: Complete the parameter expansion system with pattern operations
- [ ] **`${var/pattern/replacement}`** - Replace first match
- [ ] **`${var//pattern/replacement}`** - Replace all matches  
- [ ] **`${var/#pattern/replacement}`** - Replace if matches beginning
- [ ] **`${var/%pattern/replacement}`** - Replace if matches end

### 2.2 Substring Operations
**Goal**: Advanced string manipulation within parameter expansion
- [ ] **`${var:offset}`** - Substring from offset
- [ ] **`${var:offset:length}`** - Substring with length
- [ ] **Advanced offset handling** - Negative offsets and edge cases

## Priority 3: Comment and Script Processing Completion

### 3.1 Comment Processing (BUILD ON EXISTING WORK)
**Goal**: Complete comment and script processing features (partially implemented)
- ✅ **Comment processing** - `#` comments now fully supported
- ✅ **Shebang processing** - `#!/path/to/shell` lines properly handled  
- ✅ **Inline comments** - Comments after commands work correctly
- [ ] **Improve comment edge cases** - Comments in strings, complex scenarios

## Priority 4: Advanced Redirection & File Descriptors  
**Goal**: Complete POSIX I/O redirection support  
- [ ] **Fix file descriptor redirection**: `ls /bad 2>/dev/null`, `cmd 2>&1`
- [ ] **Implement here documents** (`<<EOF`) and here strings (`<<<`)
- [ ] **Add process substitution** (`<(command)`, `>(command)`)
- [ ] **Support pipe arrays** and multiple redirections

## Priority 5: Background Process Management (WEEKS 8-10)

### 5.1 Job Control Framework
**Goal**: Background processes and job management
- [ ] Implement background job execution (`command &`)
- [ ] Add job control builtins (`jobs`, `fg`, `bg`, `kill`)
- [ ] Support process groups and signal handling
- [ ] Implement job status tracking and notifications

### 5.2 Advanced Process Management
**Goal**: Professional job control features
- [ ] Process group management and terminal control
- [ ] Signal handling for job control (`SIGTSTP`, `SIGCONT`, etc.)
- [ ] Child process cleanup and orphan handling
- [ ] Job notification and status reporting

## Priority 6: Advanced Shell Features (FUTURE)

### 6.1 Shell Functions and Advanced Scripting
**Goal**: Complete shell programming features
- [ ] **Shell function definitions** - `function name() { commands; }`
- [ ] **Function local variables** - `local var=value`
- [ ] **Function argument handling** - `$1`, `$2`, `$@`, `$#` in functions
- [ ] **Function recursion support** - Proper stack management

### 6.2 Advanced Command Features
**Goal**: Enhanced command processing
- [ ] **Command hashing** - Remember command locations for performance
- [ ] **Command history** - Store and recall previous commands  
- [ ] **History expansion** - `!!`, `!n`, `!string` syntax
- [ ] **Command completion** - Enhanced tab completion system

### 6.3 Error Handling and Debugging
**Goal**: Professional debugging and error reporting
- [ ] **Enhanced error messages** - Better context and suggestions
- [ ] **Stack traces** - Show call stack on errors
- [ ] **Debug mode** - Step-by-step execution
- [ ] **Profiling support** - Performance analysis tools
- [ ] Implement arithmetic for loops: `for ((i=0; i<10; i++))`

## Priority 4: Compatibility & Standards

### 4.1 POSIX Compliance Improvements
**Goal**: Better standards adherence
- [ ] Audit current POSIX compliance gaps
- [ ] Implement missing POSIX features
- [ ] Add POSIX mode vs. enhanced mode
- [ ] Comprehensive POSIX test suite integration

### 4.2 Shell Function Enhancements
**Goal**: Proper function support
- [ ] Local variable scoping in functions
- [ ] Function argument handling (`$1`, `$2`, etc.)
- [ ] Return value handling
- [ ] Function export and inheritance

### 4.3 Signal Handling
**Goal**: Robust signal management
- [ ] Proper Ctrl+C handling in different contexts
- [ ] Signal propagation to child processes
- [ ] Trap statement implementation
- [ ] Signal safety in all shell operations

## Priority 5: Performance & Scalability

### 5.1 Memory Management Optimization
**Goal**: Efficient resource usage
- [ ] Implement memory pools for frequent allocations
- [ ] Optimize token and node tree memory usage
- [ ] Add memory debugging and leak detection
- [ ] Profile and optimize hot paths

### 5.2 Large Script Handling
**Goal**: Handle complex shell scripts efficiently
- [ ] Optimize parser for large input files
- [ ] Implement lazy evaluation where possible
- [ ] Add script caching mechanisms
- [ ] Improve performance of nested function calls

### 5.3 Concurrent Execution
**Goal**: Better parallel processing
- [ ] Optimize pipeline execution
- [ ] Implement proper process management
- [ ] Add support for parallel command execution
- [ ] Improve signal handling in concurrent contexts

## Priority 6: Developer Experience

### 6.1 Testing Infrastructure
**Goal**: Comprehensive test coverage
- [ ] Unit tests for all major components
- [ ] Integration tests for shell features
- [ ] Performance benchmarks
- [ ] Automated regression testing

### 6.2 Debugging & Diagnostics
**Goal**: Better development tools
- [ ] Add debug mode with verbose token/parsing output
- [ ] Implement shell execution tracing (`set -x`)
- [ ] Add performance profiling capabilities
- [ ] Create parser visualization tools

### 6.3 Documentation & Examples
**Goal**: Better user and developer documentation
- [ ] Complete user manual with examples
- [ ] Developer documentation for the codebase
- [ ] Shell scripting guide specific to lusush
- [ ] API documentation for the parsing system

## Implementation Strategy - UPDATED PRIORITIES

### Phase 1 (CRITICAL - Weeks 1-3)
Focus on **POSIX Command-Line Options** - The most critical compliance gap:
1. Implement `-c command_string` for automation and script execution
2. Add shell behavior flags (`-e`, `-x`, `-n`, `-u`, `-v`, `-f`, `-h`, `-m`)
3. Implement `set` builtin for dynamic option control
4. Add named option syntax (`-o`/`+o`) for full POSIX compliance

### Phase 2 (Short-term - Weeks 4-7)
Focus on **Advanced Parameter Expansion and I/O**:
1. Complete parameter expansion with pattern substitution
2. Add substring operations and advanced expansions
3. Implement advanced redirection patterns and here documents
4. Add file descriptor management and process substitution

### Phase 3 (Medium-term - Weeks 8-12)
Focus on **Background Process Management**:
1. Background job execution and tracking
2. Job control builtins (`jobs`, `fg`, `bg`)
3. Process group management and signal handling
4. Professional job status reporting

### Phase 4 (Long-term - 3-6 months)
Focus on **Advanced Shell Features**:
1. Interactive features and history
2. Configuration system
3. Advanced prompt customization
4. POSIX compliance improvements

### Phase 4 (Long-term - 4+ months)
Focus on **Performance & Polish**:
1. Memory optimization
2. Performance improvements
3. Comprehensive testing
4. Documentation completion

## Technical Priorities by Impact - UPDATED

### High Impact, Low Effort (CRITICAL)
- [ ] **Implement `-c command_string`** - Single most critical missing feature
- [ ] **Add basic shell flags** (`-s`, `-i`, `-l`) - Essential for system integration
- [ ] **Implement `set -e`, `set -x`** - Core debugging and error handling
- [ ] **Add simple pattern substitution** - `${var/pattern/replacement}`

### High Impact, Medium Effort  
- [ ] **Complete `set` builtin** - Full POSIX option control system
- [ ] **Add named options** (`-o`/`+o`) - Complete POSIX option syntax
- [ ] **Implement all behavior flags** (`-n`, `-u`, `-v`, `-f`, `-h`, `-m`)
- [ ] **Advanced parameter expansion** - Substring operations and edge cases

### High Impact, High Effort
- [ ] **Background job control** - Process management, signals, job tracking
- [ ] **Advanced redirection** - File descriptors, here documents, process substitution
- [ ] **Shell function system** - Function definitions, local variables, recursion
- [ ] **Complete POSIX test suite compliance** - Comprehensive standards validation

## Success Metrics - UPDATED

### Phase 1 Success (Command-Line Options) - Weeks 1-3
- [ ] `lusush -c "echo test"` works for script execution
- [ ] `set -e` causes shell to exit on command failures
- [ ] `set -x` traces command execution for debugging
- [ ] All POSIX shell invocation modes function correctly

### Phase 2 Success (Advanced Features) - Weeks 4-7
- [ ] Complete parameter expansion patterns work
- [ ] Advanced redirection (`2>&1`, here documents) functional
- [ ] File descriptor management integrated
- [ ] Substring operations and pattern substitution work

### Phase 3 Success (Job Control) - Weeks 8-12
- [ ] Background jobs (`command &`) execute and are tracked
- [ ] Job control commands (`jobs`, `fg`, `bg`) work correctly
- [ ] Signal handling for job management functional
- [ ] Process cleanup and status reporting work properly

This roadmap prioritizes the critical POSIX command-line options gap as the top priority, ensuring lusush can function as a proper system shell replacement before adding advanced features.
