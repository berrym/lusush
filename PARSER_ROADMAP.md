# Parser Enhancement Roadmap - UPDATED STATUS (June 17, 2025)

## 🔴 CRITICAL ISSUE: Control Structure Parsing Broken

### Current Status
After implementing quote state tracking and error recovery improvements, basic control 
structure parsing has been broken. **IMMEDIATE FIX REQUIRED**.

### ✅ Successfully Implemented
1. **Quote State Tracking System**
   - `is_line_complete()` function working correctly
   - Proper nested quote detection
   - POSIX-compliant escape handling
   - Command substitution detection

2. **Enhanced Error Recovery Framework**
   - Parser error context with recovery modes
   - Better error messages with suggestions
   - Error location reporting

3. **Multi-line Input Foundation** 
   - Line completion detection
   - Quote continuation support

### ❌ Critical Issues
1. **Control structures fail**: `for`, `while`, `case` statements not parsing
2. **Token lifecycle broken**: Source position management incorrect
3. **EOF tokens**: Appearing when expecting variable names

### 🎯 IMMEDIATE ACTIONS NEEDED
1. **Fix token consumption in `parse_command()`** - tokens freed incorrectly
2. **Debug source position management** - position not advancing properly  
3. **Test each control structure individually** - systematic debugging
4. **Restore basic functionality** before advancing features

### Working Features
- ✅ Basic commands: `echo "hello"`
- ✅ Simple if: `if true; then echo ok; fi` 
- ✅ Pipelines: `echo test | cat`
- ✅ Command substitution: `echo $(date)`
- ✅ Quote handling test passed

### Broken Features  
- ❌ For loops: `for i in 1 2 3; do echo $i; done`
- ❌ While loops: `while [ $x -lt 10 ]; do echo $x; done`
- ❌ Case statements: `case $var in pattern) echo match;; esac`

---

# Parser Enhancement Roadmap for lusush

## Current Parser Issues & Solutions

### **Priority 1: Multi-line Command Support**

#### **Problem**: Incomplete handling of line continuations and multi-line commands
```bash
# These currently fail or behave unexpectedly:
echo "hello \
world"

if test -f file; \
then echo "exists"; fi

for i in a b c; do \
  echo $i; \
done
```

#### **Solution**: Enhanced line continuation parser
1. **Lexer enhancement** - Detect `\` + newline sequences
2. **Line buffering** - Accumulate continuation lines before parsing
3. **Prompt management** - Show `PS2` continuation prompt in interactive mode

---

### **Priority 2: Robust Nested Structure Parsing**

#### **Problem**: Parser can get confused with deeply nested structures
```bash
# Complex nesting currently problematic:
if test -f file; then
    for i in $(ls *.txt); do
        case $i in
            *.tmp) rm "$i" ;;
            *) echo "keeping $i" ;;
        esac
    done
fi
```

#### **Solution**: Recursive descent with proper error boundaries
1. **Parser state management** - Track nesting depth and context
2. **Error recovery zones** - Define recovery points for each structure
3. **Token lookahead** - Better prediction of structure boundaries

---

### **Priority 3: Enhanced Quote and Escape Processing**

#### **Problem**: Complex quoting scenarios fail
```bash
# These can break the parser:
echo 'don'\''t break'
echo "nested $(echo "quotes")"
echo $'escape\tsequences\nhere'
```

#### **Solution**: State-machine based quote processor
1. **Quote state tracking** - Maintain proper quote context
2. **Escape sequence handling** - Process escapes in correct contexts
3. **Nested substitution** - Handle command/arithmetic substitution in quotes

---

### **Priority 4: Better Error Recovery**

#### **Problem**: Parser gives up too easily on errors
```bash
# Single error kills entire command:
if test -f; then echo "missing arg breaks everything"
```

#### **Solution**: Panic-mode error recovery
1. **Synchronization tokens** - Find safe restart points
2. **Error context preservation** - Keep parsing other parts
3. **User-friendly messages** - Suggest corrections where possible

---

## Implementation Plan

### **Phase 1: Foundation (Week 1)**
- [ ] Implement line continuation detection in lexer
- [ ] Add multi-line input buffering for interactive mode
- [ ] Fix token lifecycle management (consistent alloc/free)
- [ ] Add parser state debugging infrastructure

### **Phase 2: Structure Enhancement (Week 2)**  
- [ ] Rewrite `parse_command_list()` with proper termination
- [ ] Add nesting depth tracking and limits
- [ ] Implement proper error recovery synchronization
- [ ] Add comprehensive parser unit tests

### **Phase 3: Quote Processing (Week 3)**
- [ ] State-machine based quote handler
- [ ] Enhanced escape sequence processing
- [ ] Nested substitution support in quotes
- [ ] Quote context preservation across lines

### **Phase 4: Error Handling (Week 4)**
- [ ] Panic-mode error recovery implementation
- [ ] Context-aware error messages
- [ ] Error suggestion system
- [ ] Interactive error recovery (continue parsing)

---

## Testing Strategy

### **Test Categories**
1. **Multi-line commands** - Various continuation scenarios
2. **Nested structures** - Deep nesting, mixed structures
3. **Quote complexity** - All quote combinations and escapes
4. **Error scenarios** - Malformed commands, missing tokens
5. **Edge cases** - Empty commands, unusual whitespace, etc.

### **Test Infrastructure**
- Parser unit tests with isolated test cases
- Integration tests with full command execution
- Fuzzing tests for robustness validation
- Performance tests for complex nested structures

---

## Success Criteria

### **Functional Goals**
- [ ] All POSIX shell constructs parse correctly
- [ ] Multi-line commands work seamlessly
- [ ] Complex nested structures parse without errors
- [ ] Quote processing handles all edge cases
- [ ] Error recovery allows continued parsing

### **Quality Goals**
- [ ] Zero memory leaks in parser
- [ ] Consistent token lifecycle management
- [ ] Comprehensive error messages
- [ ] Performance acceptable for interactive use
- [ ] Full test coverage of parser components

---

## Benefits for Future Features

Once the parser is robust:
- **Function definitions** become straightforward to add
- **Heredoc support** can be implemented cleanly
- **Advanced redirection** won't break existing parsing
- **Job control** syntax will parse correctly
- **Any new language features** will have solid foundation

---

*This enhanced parser will make lusush significantly more reliable and ready for production use.*
