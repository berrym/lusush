# LUSUSH QUICK REFERENCE CARD
*Essential Architecture Information for Developers*

## **IMMEDIATE DEVELOPMENT TARGETS**

### 1. Implement Case Pattern Matching
```
File: src/exec.c - do_case_clause() 
Status: Parser works, execution stub exists
Need: Pattern matching logic (*, ?, [ranges])
```

### 2. Implement Until Loop Execution  
```
File: src/exec.c - do_until_loop()
Status: Parser works, execution stub exists  
Need: Negated condition logic (opposite of while)
```

### 3. Advanced Parameter Expansion Features
```
File: src/wordexp.c
Status: Basic patterns implemented
Need: Pattern substitution ${var/pattern/replacement}
```

## 🧭 **KEY FILE NAVIGATION**

### Core Processing Chain
```
src/input.c     ─▶ Input handling, file reading
src/scanner.c   ─▶ Tokenization, character processing  
src/parser.c    ─▶ Syntax analysis, AST generation
src/exec.c      ─▶ Command execution, control structures
```

### Critical Parser Functions
```
parse_command()                   ─ Main entry point
parse_if_statement()              ─ IF structure parsing
parse_command_list_multi_term()   ─ Multi-command bodies (NEW)
parse_condition_then_pair()       ─ IF body helper
```

### Critical Execution Functions  
```
execute_node()     ─ Main dispatcher
do_if_clause()     ─ IF execution (ENHANCED)
do_for_loop()      ─ FOR execution (WORKING)
do_while_loop()    ─ WHILE execution (WORKING)
```

## **CURRENT STATUS MATRIX**

| Feature | Parser | Execution | Multi-Cmd | Status |
|---------|--------|-----------|-----------|---------|
| IF/THEN/ELSE | ✅ | ✅ | ✅ | COMPLETE |
| FOR loops | ✅ | ✅ | ✅ | COMPLETE |  
| WHILE loops | ✅ | ✅ | ✅ | COMPLETE |
| Nested structures | ✅ | ⚠️ | ⚠️ | FIXED (no hangs) |
| CASE statements | ✅ | ❌ | N/A | PARTIAL |
| UNTIL loops | ✅ | ❌ | N/A | PARTIAL |

## **DEBUGGING QUICK COMMANDS**

### Test Multi-Command Control Structures
```bash
./builddir/lusush -c 'if true; then var1=A; var2=B; echo "vars: $var1 $var2"; fi'
./builddir/lusush -c 'for i in 1 2; do echo "num: $i"; val=$i; done; echo "final: $val"'
```

### Test Nested Structures (Previously Hanging - Now Fixed)
```bash
./builddir/lusush -c 'if true; then if true; then echo "nested works"; fi; fi'
```

### Test Nested Structure Issue (WILL HANG)
```bash
timeout 5s ./builddir/lusush -c 'if true; then if true; then echo "nested"; fi; fi'
```

### Memory and Token Analysis
```bash
valgrind ./builddir/lusush -c 'simple command'
gdb ./builddir/lusush
```

## **KNOWN AREAS REQUIRING ATTENTION**

1. **Case pattern matching** - Implementation stub exists, needs pattern logic
2. **Until loop execution** - Parser complete, execution stub needs implementation  
3. **Memory leaks** - AST cleanup in error paths needs validation
4. **Advanced parameter expansion** - Pattern substitution features missing

## 💡 **DEVELOPMENT TIPS**

### Adding New Control Structure
1. Add token type in `src/scanner.c`
2. Add node type in `include/node.h`  
3. Add parser function in `src/parser.c`
4. Add executor function in `src/exec.c`
5. Update `execute_node()` dispatcher
6. Add comprehensive tests

### Debugging Parser Issues
1. Add debug prints in `tokenize()`
2. Check `unget_token()` usage
3. Verify AST structure with `print_ast()` 
4. Test with minimal examples first

### Performance Profiling
1. Use `time` for basic measurements
2. Use `valgrind --tool=callgrind` for detailed analysis
3. Profile token allocation/deallocation
4. Monitor AST node creation patterns

---

*Keep this reference handy during development for quick access to critical information.*
