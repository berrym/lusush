# Critical Next Steps for Lusush Development

## Current State Assessment (June 18, 2025)
✅ **Enhanced token pushback system** - Complete  
✅ **Unified interactive input system** - Complete  
✅ **Basic control structures** - Functional (for/while/until/case work!)  
✅ **Core infrastructure** - Solid foundation established  
✅ **Memory corruption fix** - Variable expansion no longer shows garbage!
✅ **Variable assignment logic** - Partially fixed (standalone assignments work)

## 🔥 **CRITICAL PRIORITY 1: Fix Tokenization Issue (URGENT)**

### **ROOT CAUSE IDENTIFIED**: Scanner Tokenization Problem
**The Issue**: Input like `y=test echo $y` is being treated as **ONE token** instead of **THREE tokens** (`["y=test", "echo", "$y"]`)

**Evidence**:
```bash
# Expected behavior:
echo 'y=test echo $y' | ./lusush
# Should set y=test temporarily and execute: echo $y → "test"

# Current (broken) behavior:
DEBUG: child nodes: [0] type=1, str='y=test echo $y'
# Entire input is ONE token instead of three separate tokens
```

**Impact**: This breaks the fundamental POSIX pattern `VAR=value command args`

### **EXACT TECHNICAL PROBLEM**
- **Scanner/Parser**: Not properly splitting input on whitespace boundaries
- **Current**: `"y=test echo $y"` → 1 node with entire string
- **Required**: `"y=test echo $y"` → 3 nodes: `"y=test"`, `"echo"`, `"$y"`

### **Immediate Actions Required**
1. **Fix Scanner Tokenization** (Day 1)
   - Investigate why whitespace isn't splitting tokens properly
   - Check input processing pipeline (stdin/pipe vs interactive)
   - Ensure proper word boundary detection

2. **Implement POSIX Variable Assignment Pattern** (Day 1-2)
   ```bash
   VAR=value command args  # Set VAR only for command execution
   VAR=value              # Set VAR permanently in shell
   ```

## ✅ **RECENT MAJOR WINS**
- **Memory corruption fixed**: Variable expansion no longer shows garbage like `hello �S�=`
- **Control structures work**: for/while/until loops execute properly with variable expansion
- **Environment variables work**: `$HOME` expands correctly
- **Basic assignments work**: `myvar=hello` followed by `echo $myvar` works

## 🚀 **PRIORITY 2: Complete Variable System (THIS WEEK)**

### Why This Matters
- Showcases the power of our enhanced token system
- Essential for shell usability and POSIX compliance
- Relatively contained feature with high impact

### Specific Implementation
1. **Here Documents** (Perfect token pushback showcase)
   ```bash
   cat <<EOF
   This is a here document
   with multiple lines
   EOF
   ```

2. **Complex Redirection Patterns**
   ```bash
   command 2>&1 >file.log
   command &>file.log  
   command >>file.log 2>&1
   ```

3. **Process Substitution** (Advanced)
   ```bash
   diff <(command1) <(command2)
   ```

## 🎯 **CRITICAL PRIORITY 3: Job Control Foundation (WEEKS 2-3)**

### Why This Is Essential
- Background processes are fundamental shell functionality
- Users expect `command &`, `jobs`, `fg`, `bg` to work
- Foundation for all advanced process management

### Implementation Steps
1. **Background Execution**
   ```c
   // Implement in exec.c
   if (command_ends_with_ampersand()) {
       execute_background(node);
   }
   ```

2. **Job Management Builtins**
   - `jobs` - list active background jobs
   - `fg %n` - bring job to foreground  
   - `bg %n` - resume job in background
   - `kill %n` - terminate job

3. **Process Group Handling**
   - Signal propagation to child processes
   - Proper cleanup of terminated jobs

## 🔧 **CRITICAL PRIORITY 4: Testing Infrastructure (PARALLEL)**

### Why This Can't Wait
- Need to validate fixes as we make them
- Prevent regressions as we add features
- Essential for confidence in the codebase

### Implementation
1. **Parser Test Suite**
   ```c
   // Create comprehensive tests for all control structures
   test_for_loops();
   test_while_loops(); 
   test_nested_structures();
   test_variable_expansion();
   ```

2. **Integration Tests**
   - Test unified input system edge cases
   - Validate token pushback in complex scenarios
   - Memory leak detection

3. **Performance Benchmarks**
   - Ensure enhanced systems don't degrade performance
   - Profile token pushback overhead
   - Memory usage validation

## 📋 **IMPLEMENTATION SCHEDULE**

### Week 1 (Critical Foundation)
- **Mon-Tue**: Fix variable expansion issues in control structures
- **Wed-Thu**: Perfect parser edge cases with token pushback system
- **Fri**: Comprehensive testing and validation

### Week 2 (Core Features)  
- **Mon-Tue**: Implement here documents and basic redirection
- **Wed-Thu**: Add background job execution (`command &`)
- **Fri**: Basic job control builtins (`jobs`, `fg`, `bg`)

### Week 3 (Polish & Robustness)
- **Mon-Tue**: Advanced redirection patterns and error handling
- **Wed-Thu**: Job control signal handling and cleanup
- **Fri**: Performance optimization and memory management

## 🎯 **SUCCESS METRICS**

### Week 1 Targets
- [ ] All control structures work with proper variable expansion
- [ ] No "No such file or directory" errors from parsing issues
- [ ] Nested structures parse and execute correctly
- [ ] Enhanced error messages provide helpful context

### Week 2 Targets  
- [ ] Here documents work perfectly
- [ ] Background jobs execute and can be managed
- [ ] Basic job control commands functional
- [ ] Complex redirection patterns supported

### Week 3 Targets
- [ ] Professional-grade error handling and recovery
- [ ] Robust job management with proper cleanup
- [ ] Performance metrics within acceptable bounds
- [ ] Comprehensive test coverage for new features

## 🚨 **CRITICAL SUCCESS FACTORS**

1. **Fix Variable Expansion First** - This is blocking everything else
2. **Leverage Our Enhanced Systems** - Use token pushback and unified input
3. **Test As We Go** - Don't accumulate technical debt
4. **Focus on Core Stability** - Perfect the foundation before adding features
5. **Document Progress** - Maintain clear development momentum

## 🎪 **VALIDATION APPROACH**

### Daily Testing
```bash
# These must work perfectly by end of Week 1
echo 'for i in 1 2 3; do echo "Number: $i"; done' | ./lusush
echo 'if true; then VAR="test"; echo "Value: $VAR"; fi' | ./lusush
echo 'while [ true ]; do echo "loop"; break; done' | ./lusush

# These must work perfectly by end of Week 2  
echo 'ls &' | ./lusush  # Background job
echo 'jobs' | ./lusush  # List jobs
printf 'cat <<EOF\nHere doc test\nEOF\n' | ./lusush
```

## 🏆 **STRATEGIC IMPACT**

After these 3 weeks, we'll have:
- **Rock-solid parsing foundation** that can handle anything
- **Essential shell features** that users expect
- **Robust job control** for real-world usage
- **Advanced I/O redirection** showcasing our enhanced systems
- **Comprehensive testing** to prevent regressions

This creates a **professional-grade shell** ready for advanced features like arithmetic expansion, parameter expansion, and enhanced completion.

The key is to **perfect the core** before adding complexity - our enhanced token and input systems give us the tools, now we need to **execute flawlessly** on the fundamentals.
