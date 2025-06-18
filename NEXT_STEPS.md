# Lusush Shell - Immediate Next Steps

## Priority Queue for Development

### 🔥 **WEEK 1: Parser Robustness (High Priority)**

#### 1. Fix Remaining Parser Edge Cases
**Goal**: Leverage the new multi-token pushback system to resolve parsing issues
- [ ] **Control Structure Fixes**
  - Use `match_token_sequence()` to detect for/while/until patterns reliably
  - Fix double-consumption bugs in parse_command_list()
  - Test nested control structures thoroughly
  
- [ ] **Enhanced Error Recovery**
  - Implement parser checkpoints using token position tracking
  - Add multi-token backtracking for better error recovery
  - Use `peek_token_ahead()` for context-aware error messages

- [ ] **Validation Testing**
  - Create test cases for all control structure combinations
  - Test multiline and nested constructs
  - Verify token pushback works correctly in all scenarios

#### 2. Advanced Redirection Support
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
  
- [ ] **Process Substitution** (Advanced)
  ```bash
  # Future enhancement
  diff <(command1) <(command2)
  command >(filter)
  ```

### 🚀 **WEEK 2: Input System Integration**

#### 3. Perfect Input Buffering Integration
**Goal**: Ensure input system works flawlessly with enhanced token system
- [ ] **Input Completion Detection**
  - Verify `is_command_complete()` works with new token pushback
  - Test multiline command detection accuracy
  - Handle edge cases in input buffering

- [ ] **Interactive Experience**
  - Test command line editing with complex constructs
  - Ensure proper cursor positioning in multiline mode
  - Validate linenoise integration

### 🎯 **WEEK 3-4: Core Shell Features**

#### 4. Job Control Implementation
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

#### 5. Parameter Expansion Phase 1
**Goal**: Essential parameter expansion features
- [ ] **Basic Expansions**
  ```bash
  ${var:-default}    # Default value if unset
  ${var:+alternate}  # Alternate value if set
  ${#var}           # Variable length
  ```

- [ ] **Pattern Matching**
  ```bash
  ${var#pattern}     # Remove shortest prefix
  ${var##pattern}    # Remove longest prefix
  ${var%pattern}     # Remove shortest suffix
  ${var%%pattern}    # Remove longest suffix
  ```

## 🔧 **Development Strategy**

### Immediate Actions (This Week)
1. **Focus on parser fixes** - use the new token pushback capabilities
2. **Implement here documents** - perfect demonstration of enhanced system
3. **Create comprehensive tests** - validate all improvements
4. **Document progress** - maintain clear development log

### Success Metrics
- [ ] All control structures parse correctly in nested scenarios
- [ ] Here documents work for both simple and complex cases
- [ ] Error messages are helpful and context-aware
- [ ] No regression in existing functionality
- [ ] Memory usage remains efficient

### Testing Approach
```bash
# Test cases to implement
echo 'for i in 1 2 3; do for j in a b; do echo "$i$j"; done; done' | ./lusush
echo 'cat <<EOF
line 1
line 2
EOF' | ./lusush
echo 'if true; then echo "nested"; for x in 1; do echo $x; done; fi' | ./lusush
```

## 📋 **Implementation Checklist**

### Parser Improvements
- [ ] Update parse_for_statement() to use match_token_sequence()
- [ ] Update parse_while_statement() to use enhanced error recovery
- [ ] Update parse_until_statement() with proper token handling
- [ ] Fix parse_command_list() terminator consumption issues
- [ ] Add parser checkpoint/restore functionality

### Redirection Support
- [ ] Implement parse_complex_redirection() function
- [ ] Add here_document parsing and execution
- [ ] Create redirection test suite
- [ ] Update execution engine for new redirection types

### Testing Infrastructure
- [ ] Create parser_test.c with comprehensive test cases
- [ ] Add memory leak detection to tests
- [ ] Implement automated regression testing
- [ ] Create performance benchmarks

### Documentation Updates
- [ ] Update README.md with new features
- [ ] Document the enhanced parsing capabilities
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
