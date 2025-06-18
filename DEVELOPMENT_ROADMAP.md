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

## Recent Major Breakthrough: Logical Operators Completely Fixed
**Critical Root Cause Resolved**: Main input loop in `lusush.c` was incorrectly routing any line containing `|` (including `||`) to pipeline execution instead of the parser, causing "Empty command in pipeline" errors.

**Solution**: Enhanced pipe detection to distinguish between single pipes (`|`) and compound operators (`||`, `|&`). Now both logical operators work perfectly with proper POSIX short-circuit behavior while preserving pipeline functionality for actual pipes.

**Impact**: All basic logical command chaining now works correctly - `&&`, `||`, semicolons, and single pipes all function as expected.

## Priority 1: Parser Robustness & Complex Operator Combinations

### 1.1 Complex Mixed Operator Parsing (IMMEDIATE PRIORITY)
**Goal**: Handle combinations of pipes and logical operators on same command line
- [ ] **Fix mixed operators** - `echo test | grep test || echo not_found`
- [ ] **Handle precedence** - Proper operator precedence in complex expressions
- [ ] **Pipeline + logical combinations** - Commands like `cmd1 | cmd2 && cmd3`
- [ ] **Implement comprehensive operator precedence** handling in parser
- [ ] **Add advanced expression parsing** for complex shell command lines

### 1.2 Advanced Redirection & File Descriptors  
**Goal**: Complete POSIX I/O redirection support  
- [ ] **Fix file descriptor redirection**: `ls /bad 2>/dev/null`, `cmd 2>&1`
- [ ] **Implement here documents** (`<<EOF`) and here strings (`<<<`)
- [ ] **Add process substitution** (`<(command)`, `>(command)`)
- [ ] **Support pipe arrays** and multiple redirections

### 1.3 Control Structure Robustness
**Goal**: Fix control structures to work with semicolons and mixed delimiters
- [ ] **Fix if/then/else parsing** with semicolons: `if true; then echo test; fi`
- [ ] **Fix for loop parsing** with semicolons: `for i in 1 2 3; do echo $i; done`
- [ ] **Implement proper keyword recognition** in control structure contexts
- [ ] **Add delimiter flexibility** - handle both newlines and semicolons consistently

## Priority 2: Missing POSIX Features (Based on test.sh Analysis)

### 2.1 Parameter Expansion and Variable Handling
**Goal**: Complete POSIX parameter expansion support
- [ ] **Implement `${var=value}` assignment syntax** - Critical missing feature
- [ ] **Add `${var:-default}` default value expansion**
- [ ] **Implement `${var:+alternate}` alternate value expansion**  
- [ ] **Add `${#var}` length expansion**
- [ ] **Implement pattern substitution** (`${var/pattern/replacement}`)

### 2.2 Command Substitution and Execution
**Goal**: Complete command substitution support
- [ ] **Add backtick command substitution** - Legacy `cmd` syntax
- [ ] **Fix script file execution** - Currently causes infinite loops
- [ ] **Implement shebang processing** - `#!/path/to/shell` support
- [ ] **Add proper comment handling** - `#` comments cause parsing errors

### 2.3 Enhanced Builtins and Shell Features
**Goal**: Improve builtin command functionality
- [ ] **Enhanced echo builtin** - Process escape sequences (\n, \t, etc.)
- [ ] **Improved printf builtin** - Full POSIX printf support
- [ ] **Add complete test builtin** - All POSIX test operators
- [ ] **Implement job control** - Background processes, job management

## Priority 3: Advanced Shell Features

### 3.1 Job Control
**Goal**: Background processes and job management
- [ ] Implement background job execution (`command &`)
- [ ] Add job control builtins (`jobs`, `fg`, `bg`, `kill`)
- [ ] Support process groups and signal handling
- [ ] Implement job status tracking and notifications

### 3.2 Advanced Parameter Expansion
**Goal**: Complete POSIX parameter expansion
- [ ] Implement `${var:-default}`, `${var:+value}` patterns
- [ ] Add `${var#pattern}`, `${var%pattern}` (prefix/suffix removal)
- [ ] Support `${#var}` (length) and `${var/pattern/replacement}`
- [ ] Implement array variables and `${array[@]}` expansion

### 3.3 Arithmetic Expansion
**Goal**: Shell arithmetic with `$((expression))`
- [ ] Implement arithmetic expression parser
- [ ] Support variables in arithmetic contexts
- [ ] Add arithmetic assignment operators (`+=`, `-=`, etc.)
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

## Implementation Strategy

### Phase 1 (Immediate - Next 2-4 weeks)
Focus on **Complex Operator Parsing** using the robust foundation we now have:
1. Fix complex mixed operator combinations (pipes + logical operators)
2. Implement advanced file descriptor redirection (2>&1, etc.)
3. Enhance control structure parsing with delimiters
4. Add comprehensive operator precedence handling

### Phase 2 (Short-term - 1-2 months)
Focus on **Core Shell Features**:
1. Job control implementation
2. Advanced parameter expansion
3. Arithmetic expansion
4. Enhanced command completion

### Phase 3 (Medium-term - 2-4 months)
Focus on **User Experience**:
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

## Technical Priorities by Impact

### High Impact, Low Effort
- [ ] Fix complex mixed operator combinations (building on working && and || foundation)
- [ ] Implement file descriptor redirection (2>&1, 2>/dev/null)
- [ ] Add here document support
- [ ] Improve error messages with context

### High Impact, Medium Effort  
- [ ] Complete parameter expansion features
- [ ] Implement arithmetic expansion
- [ ] Add advanced command completion
- [ ] Create comprehensive test suite

### High Impact, High Effort
- [ ] Full process substitution support
- [ ] Advanced signal handling
- [ ] Memory optimization and profiling
- [ ] Complete POSIX compliance audit

This roadmap leverages the enhanced token pushback system we just built and provides a clear path toward a feature-complete, robust shell implementation.
