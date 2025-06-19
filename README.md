# Lusush Shell

A POSIX-compliant shell with modern features, built in C.

## Version 0.4.0 - POSIX Parser Migration Phase 1 Complete

Lusush has successfully completed **Phase 1 of the POSIX parser migration**, implementing a pragmatic dual-parser architecture that maintains full compatibility while introducing modern POSIX-compliant parsing for simple commands.

## POSIX Parser Migration Progress

### Phase 1 Complete - Simple Command Migration
- **New POSIX Parser**: Handles simple commands with full POSIX compliance
- **Intelligent Command Routing**: Complexity analysis routes commands to appropriate parser
- **Execution Adapter**: Bridge between new parser AST and existing execution engine
- **Zero Downtime Migration**: All existing functionality preserved during transition

#### Current Parser Routing
- **Simple Commands** (NEW POSIX parser): `echo hello`, `pwd`, `ls -la`
- **Pipelines** (existing system): `echo test | cat`, `ls | head -3` 
- **Control Structures** (old parser): `for i in a b; do echo $i; done`
- **Logical Operators** (old parser): `echo success && echo second`

### Migration Roadmap
- **Phase 1** (COMPLETE): Simple command migration with dual parser architecture
- **Phase 2** (NEXT): Pipeline migration to new parser
- **Phase 3** (PLANNED): Control structure migration to new parser
- **Phase 4** (PLANNED): Advanced feature migration (logical operators, parameter expansion)
- **Phase 5** (PLANNED): Complete migration, remove old parser

## Current POSIX Compliance Status

### Complete POSIX Features
- **All 12 Essential POSIX Command-Line Options** (`-c`, `-s`, `-i`, `-l`, `-e`, `-x`, `-n`, `-u`, `-v`, `-f`, `-h`, `-m`)
- **Complete Set Builtin** with option management (`set -e`, `set +x`, etc.)
- **Multi-Command Control Structures**:
  - `if/then/else/elif/fi` statements with multi-command bodies
  - `for/in/do/done` loops with multi-command bodies
  - `while/do/done` loops with multi-command bodies and infinite loop protection
  - Both newline and semicolon separators work correctly
- **Robust Word Expansion** (variables, parameters, command substitution, globbing)
- **Memory-Safe Operation** (critical bugs fixed, no more crashes)
- **Command Substitution** (both `$()` modern and `` `cmd` `` legacy syntax)
- **Pipeline Processing** with proper logical operator handling
- **Complete Parameter Expansion** (`${var:-default}`, `${var:=value}`, `${var:+alternate}`, etc.)
- **POSIX Simple Commands** (via new parser with full standards compliance)

### Known Limitations
- **Nested Control Structures**: Parsing issues cause hangs, should be avoided
- **Case Statements**: Parser implemented, pattern matching pending
- **Until Loops**: Parser implemented, execution support pending
- **Function Definitions**: Not yet implemented

### Recent Major Achievements (June 2025)

#### Parser Migration Phase 1 (CURRENT)
- **Dual Parser Architecture**: Intelligent command complexity analysis with appropriate parser routing
- **POSIX Simple Commands**: All simple commands now use new POSIX-compliant parser
- **Execution Adapter**: Seamless bridge between new parser AST and existing execution engine
- **Full Compatibility**: Zero breaking changes, all existing functionality preserved

#### Multi-Command Control Structures (PREVIOUS)
- **Multi-Command Parsing Fix**: Control structures now properly parse and execute multiple commands in their bodies
- **Semicolon Support**: Both newline and semicolon separators work correctly in control structure bodies
- **Enhanced Execution Logic**: Proper command list handling for complex control flow patterns

## Features

### Core Functionality
- **POSIX-compliant command parsing** with robust error handling
- **Multi-character operator support** (`&&`, `||`, `>>`, `<<`, etc.)
- **Logical operators with short-circuit evaluation** 
- **Multiple command separators** (semicolons, newlines, logical operators)
- **Pipeline processing** with proper distinction from logical operators
- **Variable expansion** with quote handling and field splitting
- **POSIX parameter expansion** (`${var=value}`, `${var:-default}`, `${var:+alternate}`, etc.)
- **Enhanced echo builtin** with escape sequence processing enabled by default
- **Comprehensive command substitution** (both `$()` modern and backtick legacy syntax)
- **File redirection** (input, output, append)
- **Command completion** with linenoise integration

### Interactive Features
- **Command line editing** with history
- **Tab completion** for commands and files
- **Modern terminal interface** 
- **Built-in commands** (echo, export, source, test, read, eval, set, etc.)

## Building

Lusush uses the Meson build system:

```bash
meson setup builddir
meson compile -C builddir
```

## Usage

Run the shell:
```bash
./builddir/lusush
```

### Examples

**Command separation:**
```bash
echo first && echo second || echo backup
echo a; echo b; echo c
```

**Pipeline processing:**
```bash
echo "test data" | grep "test"
```

**Command substitution (both syntaxes):**
```bash
MODERN=$(date '+%Y-%m-%d')     # Modern syntax
LEGACY=`date '+%H:%M'`         # Legacy backtick syntax  
echo "Today is $MODERN at $LEGACY"
```

**Enhanced echo with escape sequences:**
```bash
echo "Line1\nLine2\tTabbed"    # Escape sequences enabled by default
echo -n "No newline"           # -n flag supported
```

**Multi-command control structures:**
```bash
# IF statements with multiple commands
if test -f README.md; then 
    echo "File exists"
    var1=VALUE1
    var2=VALUE2; var3=VALUE3
fi

# IF-ELSE with multiple commands
if [ "$USER" = "admin" ]; then
    echo "Admin user detected"
    admin_flag=true
else
    echo "Regular user"
    admin_flag=false; user_type=standard
fi

# FOR loops with multiple commands
for i in 1 2 3; do
    echo "Processing number: $i"
    result=$((i * 2))
    echo "Result: $result"
done

for file in *.md; do
    echo "Processing: $file"
    size=$(wc -l < "$file"); echo "Lines: $size"
done

# WHILE loops with multiple commands (with infinite loop protection)
counter=0
while [ "$counter" -lt 3 ]; do
    echo "Iteration: $counter"
    counter=$((counter + 1))
    status="processing"
done

# Mixed separators work correctly
if true; then
    assignment1=A; assignment2=B
    echo "Both assignments completed"
    final_status=complete
fi
```

**Known control structure limitations:**
```bash
# Nested control structures currently cause hangs - avoid for now
# if condition; then
#     if nested_condition; then
#         echo "This will hang"
#     fi
# fi
```

**Mixed operators (major achievement):**
```bash
echo "test" | grep "test" && echo "found" || echo "not found"
```

### Testing Current Capabilities

**POSIX Options Test Suite** (19/20 tests passing):
```bash
./test_posix_options.sh
```

**Comprehensive Feature Demonstration**:
```bash
./builddir/lusush test-comprehensive.sh
```

**Individual Feature Testing**:
```bash
# Test POSIX command-line options
./builddir/lusush -c 'echo "Command mode works"'
./builddir/lusush -v -c 'echo "Verbose mode"'
./builddir/lusush -e -c 'false; echo "This wont print"'

# Test set builtin
./builddir/lusush -c 'set -x; echo "Trace enabled"'
./builddir/lusush -c 'set; echo "Show all options"'

# Test word expansion
./builddir/lusush -c 'VAR=hello; echo "${VAR} world"'
./builddir/lusush -c 'echo "Current date: $(date)"'
./builddir/lusush -c 'echo "Files: *.md"'

# Test parameter expansion
./builddir/lusush -c 'echo "${UNSET:-default_value}"'
./builddir/lusush -c 'VAR=test; echo "${VAR:+replacement}"'
./builddir/lusush -c 'echo "${MISSING:+wont_show}"'  # Recently fixed
```

**Known Working Features**:
- ✅ All POSIX command-line options (`-c`, `-e`, `-x`, `-v`, etc.)
- ✅ Set builtin with option management
- ✅ Multi-command control structures (`if`, `for`, `while` with both newline and semicolon separators)
- ✅ Variable expansion and parameter expansion (including `:+` operator)
- ✅ Command substitution (modern and legacy)
- ✅ Pipeline and logical operators
- ✅ Globbing and pathname expansion
- ✅ Memory-safe operation (no crashes)

**Known Limitations**:
- ❌ Nested control structures (cause parsing hangs)
- ⚠️ Case statements (parser implemented, pattern matching pending)
- ⚠️ Until loops (parser implemented, execution pending)
- ⚠️ Function definitions (not implemented)
- ⚠️ Advanced parameter expansion patterns
- ⚠️ Minor escape sequence display issues

See `POSIX_COMPLIANCE_STATUS.md` for detailed analysis.

The test capabilities demonstrate:

**Known limitations requiring fixes:**

### � CRITICAL LIMITATIONS (Require Major Development)

#### **Nested Control Structures Not Working**
- **Issue**: Simple nested if statements cause parsing hangs
- **Working**: Single-level control structures with multi-command bodies
- **Impact**: Complex scripts with nested conditionals fail
- **Fix Required**: Enhanced parser logic for nested control structure termination
- **Estimated Effort**: Several days of parser development

#### **Function Definitions Not Supported**
- **Missing**: `function_name() { commands; }` syntax
- **Impact**: Cannot define reusable functions
- **Fix Required**: Parser support for function syntax, execution environment scoping

### 🟡 MODERATE LIMITATIONS (Affecting Compatibility)

#### **Parameter Expansion Edge Cases**
- **Issue**: `${VAR:+alternate}` operator fails with unset variables (returns exit code 1)
- **Working**: `${VAR:-default}`, `${VAR:=value}`, `${VAR+alternate}` 
- **Fix Required**: Debug parameter expansion operator precedence, fix `:+` operator logic
- **Estimated Effort**: 1-2 days

#### **Advanced Parameter Expansion Missing**
- **Missing**: Pattern substitution `${var/pattern/replacement}`, `${var#pattern}`, `${var%pattern}`
- **Fix Required**: Implement pattern matching in parameter expansion

#### **Array Variables Not Implemented**
- **Missing**: `array[index]=value`, `${array[@]}` syntax
- **Fix Required**: Array storage implementation, index parsing

### 🟢 MINOR ISSUES (Cosmetic or Edge Cases)

#### **Escape Sequence Display Problems**
- **Issue**: `\$` in strings displays as `$$` instead of `$`
- **Cause**: Multiple layers of escape sequence processing
- **Fix Required**: Audit escape processing pipeline, ensure single-pass handling

#### **Syntax Check Mode Error Reporting**
- **Issue**: `-n` mode returns exit code 0 instead of 2 for some syntax errors
- **Fix Required**: Enhance error propagation from parser to main shell

#### **Pipeline Builtin Warnings**  
- **Issue**: Unnecessary warnings for builtins in pipelines ("Warning: builtin 'echo' in pipeline...")
- **Fix Required**: Improve builtin pipeline detection logic

### 📋 ADVANCED FEATURES NOT YET IMPLEMENTED

#### **Here Documents and Process Substitution**
- **Missing**: `<<EOF` syntax, `<(command)`, `>(command)`
- **Fix Required**: Parser extensions, file descriptor management

#### **Job Control**
- **Missing**: Background job management, job control operators
- **Fix Required**: Process group management, signal handling enhancements

#### **Advanced Redirection**
- **Missing**: Some complex redirection patterns, file descriptor operations
- **Fix Required**: Enhanced I/O redirection parsing and handling

## Architecture

- **Mixed operator support** - Robust parsing of `cmd | pipe && logical` expressions
- **Multi-token pushback system** for sophisticated parsing
- **Unified word expansion** with proper POSIX semantics
- **Clean separation** between pipeline and logical operator processing
- **Robust scanner** with lookahead for multi-character operators
- **Professional codebase** with comprehensive error handling

## Development Status

Version 0.2.1 represents a major milestone with comprehensive POSIX parameter expansion, enhanced echo, and robust mixed operator parsing complete.

### What Works  
- All basic command separators and logical operators (&&, ||, ;)
- Variable assignment and expansion with proper quote handling
- **Multi-command control structures** - if/then/else, for, while with complete body support
- Simple and complex pipeline processing
- File redirection and basic I/O operations
- **Complete POSIX parameter expansion** - All `${var...}` patterns implemented
- **Enhanced echo builtin** - Escape sequences enabled by default
- **Command substitution** - Both `$()` and backtick syntax working
- **Mixed operator parsing** - `cmd | pipe && logical` expressions work perfectly

### Critical Development Priority

**The highest priority development tasks for enhancing lusush POSIX compliance:**

1. **Fix Nested Control Structures** 🔴 **CRITICAL**
   - Resolve parsing hangs in nested if statements
   - Enable complex control flow patterns for advanced scripting
   - This completes the control structure implementation

2. **Complete Control Structure Suite** 🟡 **HIGH**
   - Implement case statement pattern matching
   - Add until loop execution support
   - These are the remaining POSIX control structures

3. **Add Function Definitions** 🟡 **MEDIUM**
   - Support `function_name() { commands; }` syntax
   - Implement proper function scoping and parameter passing

4. **Complete Advanced Parameter Expansion** 🟡 **MEDIUM**
   - Implement pattern substitution `${var/pattern/replacement}`
   - Add pattern removal `${var#pattern}`, `${var%pattern}`

5. **Polish User Experience** 🟢 **LOW**
   - Fix escape sequence display issues (`\$` showing as `$$`)
   - Remove unnecessary pipeline warnings
   - Improve syntax error reporting

### Future Development Phases
- Advanced parameter expansion completion (pattern substitution)
- Advanced file descriptor management and here documents
- Background job control and process management
- Enhanced POSIX compliance testing and validation

## License

MIT License - see LICENSE file for details.

## Contributing

Lusush follows modern C development practices with comprehensive testing and documentation.

### Testing Current Capabilities

To test current lusush functionality, use the provided test script:

```bash
# Run current capabilities test
cat test-current-abilities.sh | ./builddir/lusush

# Note: Direct file execution may have issues, use piped input
```

The test script demonstrates:
- Variable assignment and expansion
- Command substitution with $() syntax  
- Arithmetic expansion
- Globbing and pathname expansion
- Mixed operator expressions (pipes + logical operators)

**Known limitations:**
- No shebang (`#!/bin/sh`) processing
- No comment (`#`) support in scripts
- `${var=value}` parameter expansion not implemented
- Backtick command substitution not supported
- Script file execution has issues (use piped input)
