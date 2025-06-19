# LUSUSH SHELL ARCHITECTURE REFERENCE
*Technical Architecture Diagrams and Development Guide*
*Created: June 19, 2025*

## Table of Contents
1. [Overall Architecture Overview](#overall-architecture-overview)
2. [Input Processing Flow](#input-processing-flow)
3. [Parser Architecture](#parser-architecture)
4. [AST Node Structure](#ast-node-structure)
5. [Execution Engine](#execution-engine)
6. [Memory Management](#memory-management)
7. [Control Structure Processing](#control-structure-processing)
8. [Development Hotspots](#development-hotspots)

---

## Overall Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         LUSUSH SHELL                           │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │   INPUT     │  │   SCANNER   │  │   PARSER    │  │  EXEC   │ │
│  │  HANDLER    │─▶│  TOKENIZER  │─▶│   ENGINE    │─▶│ ENGINE  │ │
│  │             │  │             │  │             │  │         │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘ │
│         │                 │                 │             │     │
│         ▼                 ▼                 ▼             ▼     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │  LINENOISE  │  │   TOKEN     │  │    AST      │  │  PROC   │ │
│  │  INTERFACE  │  │  BUFFER     │  │   NODES     │  │ MGMT    │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘ │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │  BUILTINS   │  │  SYMBOL     │  │   ERROR     │             │
│  │  MODULE     │  │   TABLE     │  │  HANDLING   │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

### Key Components
- **Input Handler**: Command line processing, file reading (`src/input.c`)
- **Scanner/Tokenizer**: Lexical analysis, token generation (`src/scanner.c`) 
- **Parser Engine**: Syntax analysis, AST generation (`src/parser.c`)
- **Execution Engine**: Command execution, process management (`src/exec.c`)
- **Symbol Table**: Variable storage and scope management (`src/symtable.c`)
- **Builtins**: Built-in command implementations (`src/builtins/`)

---

## Input Processing Flow

```
User Input/Script File
         │
         ▼
┌─────────────────┐
│   input.c       │  ◄─── Handles: stdin, file input, command line
│   read_input()  │       Features: Line buffering, EOF handling
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   scanner.c     │  ◄─── Handles: Character-by-character scanning
│   tokenize()    │       Features: Multi-char operators, quote handling
└─────────────────┘       Token types: WORD, OPERATOR, KEYWORD, etc.
         │
         ▼
┌─────────────────┐
│   Token Buffer  │  ◄─── Features: Pushback support (unget_token)
│   token_t       │       Critical for: Control structure parsing
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   parser.c      │  ◄─── Handles: Syntax analysis, AST construction
│   parse_*()     │       Key functions: parse_command(), parse_if_statement()
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   AST Nodes     │  ◄─── Structure: node_t with type and children
│   node_t        │       Types: NODE_COMMAND, NODE_IF, NODE_FOR, etc.
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   exec.c        │  ◄─── Handles: Command execution, process management
│   execute_node()│       Key functions: do_if_clause(), do_for_loop()
└─────────────────┘
```

---

## Parser Architecture

### Current Parser Functions Map

```
parser.c Structure:
├── parse_command()           ◄─── Main entry point
│   ├── parse_if_statement()  ◄─── Control structures
│   ├── parse_for_statement()
│   ├── parse_while_statement()
│   └── parse_basic_command() ◄─── Simple commands
│
├── Command List Parsing:
│   ├── parse_command_list()           ◄─── Single terminator
│   └── parse_command_list_multi_term() ◄─── Multiple terminators
│
├── Helper Functions:
│   ├── parse_condition_then_pair()    ◄─── IF statement bodies
│   ├── parse_redirection()            ◄─── I/O redirection  
│   └── tokenize_skip_comments()       ◄─── Token filtering
│
└── New Architecture (under development):
    ├── parse_complete_command()       ◄─── Unified command parsing
    ├── parse_simple_command()         ◄─── Single command handling
    └── is_command_delimiter()          ◄─── Delimiter detection
```

### Parser Flow for Control Structures

```
Control Structure Parsing Flow:

Input: "if condition; then cmd1; cmd2; fi"
         │
         ▼
┌─────────────────┐
│ parse_command() │ ─── Recognizes TOKEN_KEYWORD_IF
└─────────────────┘
         │
         ▼
┌─────────────────┐
│parse_if_statement()│ ─── Calls parse_condition_then_pair()
└─────────────────┘
         │
         ▼
┌─────────────────┐
│parse_condition_ │ ─── Uses parse_command_list_multi_term()
│then_pair()      │     Terminators: [ELIF, ELSE, FI]
└─────────────────┘
         │
         ▼
┌─────────────────┐
│parse_command_   │ ─── Handles multiple commands until terminator
│list_multi_term()│     Supports: newlines, semicolons
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   AST NODE      │ ─── NODE_IF with command list children
│   NODE_IF       │     Structure: condition, then_body, [else_body]
└─────────────────┘
```

---

## AST Node Structure

### Node Type Hierarchy

```
node_t Structure:
├── type (node_type_t)
├── val (node_val_t)        ◄─── Union: string, number, etc.
├── first_child (node_t*)   ◄─── Linked list of children
├── next_sibling (node_t*)  ◄─── Sibling nodes
├── prev_sibling (node_t*)
└── parent (node_t*)

Node Types:
├── NODE_COMMAND    ◄─── Basic commands and command lists
├── NODE_IF         ◄─── if/then/else structures  
├── NODE_FOR        ◄─── for loops
├── NODE_WHILE      ◄─── while loops
├── NODE_UNTIL      ◄─── until loops (parser only)
├── NODE_CASE       ◄─── case statements (parser only)
├── NODE_VAR        ◄─── Variables and arguments
├── NODE_PIPE       ◄─── Pipeline operators
└── NODE_REDIR_*    ◄─── Redirection operators
```

### AST Example for "if true; then var=A; echo done; fi"

```
NODE_IF
├── NODE_COMMAND (condition: "true")
│   └── NODE_VAR ("true")
│
└── NODE_COMMAND (then body: command list)
    ├── NODE_COMMAND (first command: "var=A")
    │   └── NODE_VAR ("var=A")
    │
    └── NODE_COMMAND (second command: "echo done")  
        ├── NODE_VAR ("echo")
        └── NODE_VAR ("done")
```

---

## Execution Engine

### Execution Flow Diagram

```
execute_node() Entry Point
         │
         ▼
┌─────────────────┐
│  Switch on      │
│  node->type     │
└─────────────────┘
         │
    ┌────┴────┬────────┬────────┬─────────┐
    ▼         ▼        ▼        ▼         ▼
┌────────┐ ┌──────┐ ┌───────┐ ┌───────┐ ┌────────┐
│NODE_IF │ │NODE_ │ │NODE_  │ │NODE_  │ │NODE_   │
│        │ │FOR   │ │WHILE  │ │COMMAND│ │PIPE    │
└────────┘ └──────┘ └───────┘ └───────┘ └────────┘
    │         │        │        │         │
    ▼         ▼        ▼        ▼         ▼
┌────────┐ ┌──────┐ ┌───────┐ ┌───────┐ ┌────────┐
│do_if_  │ │do_for│ │do_    │ │execute│ │execute_│
│clause()│ │_loop()│ │while_ │ │_command│ │pipeline│
│        │ │      │ │loop() │ │()     │ │_from_  │
│        │ │      │ │       │ │       │ │node()  │
└────────┘ └──────┘ └───────┘ └───────┘ └────────┘
```

### Multi-Command Execution Logic

```
Multi-Command Body Execution:

┌─────────────────┐
│ Control         │ ─── Examples: do_if_clause(), do_for_loop()
│ Structure       │
│ Executor        │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Check if body   │ ─── if (body->first_child->type == NODE_COMMAND)
│ is command list │     Then: iterate children
│ or single cmd   │     Else: execute single command
└─────────────────┘
         │
    ┌────┴─────┐
    ▼          ▼
┌────────┐ ┌─────────────────┐
│Single  │ │  Command List   │
│Command │ │  Iteration:     │
│        │ │  for each child │ ─── while (cmd = cmd->next_sibling)
│        │ │  execute_node() │     execute_node(cmd)
└────────┘ └─────────────────┘
```

---

## Control Structure Processing

### Multi-Command Parsing Achievement

```
BEFORE FIX:                           AFTER FIX:
if condition; then                    if condition; then
    cmd1; cmd2; cmd3                      cmd1; cmd2; cmd3  
fi                                    fi
    │                                     │
    ▼                                     ▼
parse_basic_command()                 parse_command_list_multi_term()
    │                                     │
    ▼                                     ▼
Single NODE_COMMAND                   NODE_COMMAND (list)
└── NODE_VAR("cmd1;cmd2;cmd3")           ├── NODE_COMMAND("cmd1")
                                         ├── NODE_COMMAND("cmd2") 
                                         └── NODE_COMMAND("cmd3")
    │                                     │
    ▼                                     ▼
Commands concatenated                 Commands executed separately
❌ BROKEN                            ✅ WORKING
```

### Control Structure State Machine

```
Control Structure Parsing States:

START
  │
  ▼ (if/for/while keyword)
EXPECT_CONDITION
  │
  ▼ (condition parsed)
EXPECT_THEN_OR_DO
  │
  ▼ (then/do keyword)  
PARSE_BODY
  │ ┌─────────────┐
  ├─│  NEWLINE    │── Continue parsing body
  │ └─────────────┘
  │ ┌─────────────┐  
  ├─│ SEMICOLON   │── Continue parsing body
  │ └─────────────┘
  │ ┌─────────────┐
  └─│TERMINATOR   │── End body parsing
    │(fi/done/etc)│
    └─────────────┘
          │
          ▼
    STRUCTURE_COMPLETE
```

---

## Memory Management

### Token Lifecycle

```
Token Memory Management:

┌─────────────────┐
│ tokenize()      │ ─── malloc() new token_t
│ Creates token   │     Sets: type, text, src
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Token used in   │ ─── Parser examines token
│ parser          │     May call unget_token()
└─────────────────┘
         │
    ┌────┴─────┐
    ▼          ▼
┌────────┐ ┌─────────────────┐
│Token   │ │ unget_token()   │ ─── Push back for re-parsing
│consumed│ │ Saves token     │     Critical for control structures
└────────┘ └─────────────────┘
    │              │
    ▼              ▼
┌────────┐ ┌─────────────────┐
│free_   │ │ Token reused    │ ─── Next tokenize() call
│token() │ │ in next cycle   │     Returns saved token
└────────┘ └─────────────────┘
```

### AST Memory Management

```
AST Node Lifecycle:

┌─────────────────┐
│ new_node()      │ ─── malloc() node_t, initialize
│ Creates node    │     Sets: type, val, pointers = NULL
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ add_child_node()│ ─── Links parent/child relationships
│ Links nodes     │     Updates: first_child, next_sibling
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ execute_node()  │ ─── Traverses AST for execution
│ Uses AST        │     Recursive processing
└─────────────────┘
         │
         ▼
┌─────────────────┐
│free_node_tree() │ ─── Recursive cleanup
│ Cleanup         │     Frees: all children, then parent
└─────────────────┘
```

---

## Development Hotspots

### 🔥 **CRITICAL AREAS NEEDING ATTENTION**

#### 1. Nested Control Structure Parsing ❌ **BROKEN**
```
Location: src/parser.c - parse_command_list_multi_term()
Issue: Infinite loops when parsing nested if statements

Current Flow:
if outer_condition; then
    if inner_condition; then  ◄─── HANGS HERE
        commands
    fi
fi

Root Cause: Parser cannot handle nested terminators properly
Fix Required: Enhanced terminator stack or recursive parsing logic
```

#### 2. Case Statement Pattern Matching ⚠️ **INCOMPLETE**
```
Location: src/parser.c - parse_case_statement() (DONE)
         src/exec.c - do_case_clause() (NEEDS IMPLEMENTATION)
         
Current Status: 
- Parser creates AST ✅
- Execution not implemented ❌

Required Implementation:
- Pattern matching logic (*, ?, [ranges])
- Multiple pattern support (pat1|pat2|pat3)
- Default case handling
```

#### 3. Until Loop Execution ⚠️ **INCOMPLETE**
```
Location: src/parser.c - parse_until_statement() (DONE)
         src/exec.c - do_until_loop() (NEEDS IMPLEMENTATION)

Current Status:
- Parser creates AST ✅ 
- Execution stub exists but not implemented ❌

Required Implementation:
- Condition evaluation with negated logic (opposite of while)
- Loop body execution until condition becomes true
```

### 🟡 **ENHANCEMENT OPPORTUNITIES**

#### 1. Parser Architecture Unification
```
Current State: Multiple parsing approaches
- parse_basic_command() - Legacy single command
- parse_command_list() - Multi-command with single terminator  
- parse_command_list_multi_term() - Multi-command with multiple terminators
- parse_complete_command() - New architecture (partial)

Opportunity: Unify into single coherent parsing architecture
Benefits: Easier maintenance, consistent behavior, better nesting support
```

#### 2. Error Recovery Enhancement
```
Current State: Basic error reporting
Location: src/errors.c, parser error handling

Opportunities:
- Better syntax error recovery
- More descriptive error messages
- Error position tracking
- Suggestion system for common mistakes
```

#### 3. Performance Optimization
```
Areas for improvement:
- Token buffer efficiency (src/scanner.c)
- AST node pooling (src/node.c)
- Symbol table lookup optimization (src/symtable.c)
- Reduce malloc/free overhead
```

### 🟢 **STABLE AREAS** (Low priority for changes)

- **Input handling** (`src/input.c`) - Stable, well-tested
- **Built-in commands** (`src/builtins/`) - Complete, working well
- **Word expansion** (`src/wordexp.c`) - Comprehensive implementation
- **Variable management** (`src/vars.c`, `src/symtable.c`) - Solid foundation
- **Process execution** (`src/exec.c` - basic commands) - Reliable

---

## Future Architecture Considerations

### Potential Refactoring Areas

1. **Parser Redesign**: Move to recursive descent parser for better nesting support
2. **AST Optimization**: Consider flatter structures for better performance  
3. **Memory Pooling**: Implement object pools for frequent allocations
4. **Incremental Parsing**: Support for interactive editing and completion
5. **Plugin Architecture**: Modular builtin system for extensibility

### Testing Strategy Recommendations

1. **Unit Tests**: Each parser function should have isolated tests
2. **Integration Tests**: Full command-to-execution pipelines  
3. **Stress Tests**: Deep nesting, large inputs, memory pressure
4. **Regression Tests**: Ensure fixes don't break existing functionality
5. **POSIX Compliance**: Automated testing against POSIX specification

---

*This architecture reference should be updated as the codebase evolves. Key areas requiring immediate attention are marked with ❌ and ⚠️ symbols.*
