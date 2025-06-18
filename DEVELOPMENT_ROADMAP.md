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

## Recent Major Breakthrough: Scanner Token Typing Fixed
**Critical Issue Resolved**: Scanner was not calling `set_token_type()`, causing all tokens to have `TOKEN_EMPTY` type instead of proper types like `TOKEN_SEMI`. This prevented semicolons and other operators from being recognized by the parser.

**Impact**: Semicolon command separation now works correctly, enabling proper POSIX-compliant command chaining.

## Priority 1: Parser Robustness & Multi-Character Operators

### 1.1 Multi-Character Operator Parsing (IMMEDIATE PRIORITY)
**Goal**: Fix scanner to handle compound operators properly
- [ ] **Fix && and || operators** - Currently parsed as separate & or | tokens
- [ ] **Fix >> append redirection** - Currently parsed as separate > tokens  
- [ ] **Fix 2>&1 and similar redirections** - File descriptor parsing needed
- [ ] **Implement proper operator lookahead** in scanner tokenization loop
- [ ] **Add operator precedence handling** in parser

### 1.2 Control Structure Robustness
**Goal**: Fix control structures to work with semicolons and mixed delimiters
- [ ] **Fix if/then/else parsing** with semicolons: `if true; then echo test; fi`
- [ ] **Fix for loop parsing** with semicolons: `for i in 1 2 3; do echo $i; done`
- [ ] **Implement proper keyword recognition** in control structure contexts
- [ ] **Add delimiter flexibility** - handle both newlines and semicolons consistently

### 1.3 Advanced Redirection & Pipes
**Goal**: Complete POSIX I/O redirection support
- [ ] Parse complex redirection: `2>&1`, `&>file`, `>>file`
- [ ] Implement here documents (`<<EOF`) and here strings (`<<<`)
- [ ] Add process substitution (`<(command)`, `>(command)`)
- [ ] Support pipe arrays and multiple redirections

## Priority 2: Advanced Shell Features

### 2.1 Job Control
**Goal**: Background processes and job management
- [ ] Implement background job execution (`command &`)
- [ ] Add job control builtins (`jobs`, `fg`, `bg`, `kill`)
- [ ] Support process groups and signal handling
- [ ] Implement job status tracking and notifications

### 2.2 Advanced Parameter Expansion
**Goal**: Complete POSIX parameter expansion
- [ ] Implement `${var:-default}`, `${var:+value}` patterns
- [ ] Add `${var#pattern}`, `${var%pattern}` (prefix/suffix removal)
- [ ] Support `${#var}` (length) and `${var/pattern/replacement}`
- [ ] Implement array variables and `${array[@]}` expansion

### 2.3 Arithmetic Expansion
**Goal**: Shell arithmetic with `$((expression))`
- [ ] Implement arithmetic expression parser
- [ ] Support variables in arithmetic contexts
- [ ] Add arithmetic assignment operators (`+=`, `-=`, etc.)
- [ ] Implement arithmetic for loops: `for ((i=0; i<10; i++))`

## Priority 3: Enhanced User Experience

### 3.1 Advanced Command Completion
**Goal**: Leverage token pushback for intelligent completion
- [ ] Context-aware completion using `match_token_sequence()`
- [ ] Complete command arguments based on command type
- [ ] File path completion with proper escaping
- [ ] Variable name completion in appropriate contexts

### 3.2 Interactive Features
**Goal**: Modern shell interaction
- [ ] History expansion (`!!`, `!n`, `!string`)
- [ ] Improved prompt with git integration and status
- [ ] Tab completion for builtins and functions
- [ ] Syntax highlighting in command line editing

### 3.3 Configuration & Customization
**Goal**: User customization capabilities
- [ ] Shell configuration file support (`~/.lusushrc`)
- [ ] Customizable prompt (PS1, PS2 variables)
- [ ] Alias persistence and loading
- [ ] Environment variable management improvements

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
Focus on **Parser Robustness** using the new token pushback system:
1. Fix remaining control structure parsing issues
2. Implement advanced error recovery
3. Add complex redirection support
4. Improve multiline handling

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
- [ ] Fix remaining parser edge cases with token pushback
- [ ] Implement basic job control (`&`, `jobs`, `fg`, `bg`)
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
