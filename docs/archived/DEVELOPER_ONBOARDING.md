# DEVELOPER ONBOARDING - LUSUSH SHELL PROJECT

## Project Status Overview

**Current Version**: 0.4.0  
**Major Achievement**: POSIX Parser Migration Phase 1 Complete  
**Architecture**: Dual parser system with incremental POSIX migration  

## Quick Start for Developers

### Building and Testing
```bash
git clone <repository>
cd lusush
meson setup builddir
meson compile -C builddir
./test_parser_migration_progress.sh
```

### Key Files to Understand

#### Core Architecture
- **src/lusush.c**: Main shell loop, parser routing, complexity analysis
- **src/exec.c**: Execution engine with new parser adapter  
- **src/parser_new.c**: New POSIX-compliant parser
- **src/parser.c**: Legacy parser (still used for complex commands)

#### Testing and Validation
- **test_parser_migration_progress.sh**: Comprehensive migration test
- **MIGRATION_PHASE_1_COMPLETE.md**: Detailed achievement documentation
- **TECHNICAL_STATUS.md**: Complete technical overview

#### Documentation
- **README.md**: User-facing documentation
- **PARSER_REWRITE_DESIGN.md**: Parser architecture design
- **PROGRESS_TRACKER.md**: Development milestone tracking

## Current Implementation State

### What Works (Production Ready)
- **Simple Commands**: All handled by new POSIX parser
- **Pipelines**: Existing proven pipeline system
- **Control Structures**: for/while/if loops via legacy parser
- **Logical Operators**: && and || via legacy parser
- **Built-in Commands**: All 12+ POSIX built-ins functional
- **POSIX Options**: All 12 essential command-line options

### Parser Routing Logic
```c
// In src/lusush.c
command_complexity_t complexity = analyze_command_complexity(input);
switch (complexity) {
    case CMD_SIMPLE:    // -> New POSIX parser
    case CMD_PIPELINE:  // -> Existing pipeline system  
    case CMD_COMPLEX:   // -> Legacy parser
}
```

### Execution Flow
```
Input -> Complexity Analysis -> Parser Selection -> Execution
Simple Commands: New Parser -> execute_new_parser_command()
Pipelines: Pipeline System -> execute_pipeline_simple()
Complex: Legacy Parser -> execute_node()
```

## Development Guidelines

### For Feature Development
1. **Determine Parser Target**: Which parser should handle the new feature?
2. **Simple Commands**: Extend `src/parser_new.c` and update complexity analysis
3. **Complex Features**: May require legacy parser until migration reaches that phase
4. **Always Test**: Use migration test suite to validate changes

### For Bug Fixes
1. **Identify Parser Path**: Which execution path contains the bug?
2. **Simple Commands**: Check `execute_new_parser_command()` and new parser
3. **Complex Commands**: Check legacy parser and `execute_node()`
4. **Cross-Parser Issues**: May be in complexity analysis logic

### For Migration Work (Next Phases)
1. **Phase 2 Target**: Pipeline migration to new parser
2. **Approach**: Extend new parser grammar for pipeline syntax
3. **Strategy**: Update complexity analysis to route pipelines to new parser
4. **Validation**: Ensure existing pipeline functionality preserved

## Code Organization Principles

### Parser Architecture
- **Dual System**: New and legacy parsers coexist during migration
- **Complexity Analysis**: Intelligent routing based on command structure
- **Execution Adapters**: Bridge different AST formats to unified execution

### Memory Management
- **Critical**: All parser allocations must be properly freed
- **AST Cleanup**: Use `free_node_tree()` after execution
- **Parser Cleanup**: Use `parser_destroy()` for new parser instances

### Error Handling
- **Graceful Degradation**: Complex commands fall back to legacy parser
- **Consistent Interface**: All parsers return compatible AST structures
- **User Experience**: Error messages should be clear and actionable

## Testing Strategy

### Comprehensive Validation
- **Migration Test Suite**: Validates all parser routes work correctly
- **Regression Testing**: Ensures no existing functionality breaks
- **Incremental Testing**: Each migration phase thoroughly validated

### Test Commands
```bash
./test_parser_migration_progress.sh    # Full migration validation
./test_migration.sh                    # Basic migration test
NEW_PARSER_DEBUG=1 ./builddir/lusush   # Debug new parser routing
```

## Next Development Priorities

### Immediate (Phase 2)
1. **Pipeline Grammar**: Extend new parser to handle `cmd1 | cmd2` syntax
2. **Pipeline Execution**: Migrate pipeline execution to new parser
3. **Complexity Update**: Route pipelines to new parser

### Medium Term (Phase 3)
1. **Control Structure Grammar**: Implement for/while/if in new parser
2. **Control Execution**: Migrate control structure execution
3. **Complex Command Migration**: Reduce dependency on legacy parser

### Long Term (Phases 4-5)
1. **Advanced Features**: Logical operators, parameter expansion
2. **Performance Optimization**: Single parser performance tuning
3. **Code Cleanup**: Remove legacy parser and simplify architecture

## Common Development Patterns

### Adding New Grammar to New Parser
```c
// In src/parser_new.c
static node_t *parse_new_construct(parser_t *parser) {
    // Implementation following POSIX grammar
    // Return AST node compatible with execution engine
}
```

### Updating Complexity Analysis
```c
// In src/lusush.c  
command_complexity_t analyze_command_complexity(const char *input) {
    // Add detection logic for new constructs
    // Return appropriate complexity level
}
```

### Adding Execution Support
```c
// In src/exec.c
// Extend execute_new_parser_command() or add new adapter
// Ensure compatibility with existing execution patterns
```

## Architecture Benefits

### For Developers
- **Incremental Development**: Add features without breaking existing functionality
- **Clear Separation**: Easy to understand which parser handles what
- **Flexible Migration**: Can adjust migration pace based on complexity

### For Users
- **Zero Downtime**: Full shell functionality throughout migration
- **Backward Compatibility**: Existing scripts continue working
- **Progressive Enhancement**: POSIX compliance improves incrementally

## Support and Resources

### Documentation
- **Technical Deep Dive**: See TECHNICAL_STATUS.md
- **Migration Details**: See MIGRATION_PHASE_1_COMPLETE.md  
- **Parser Design**: See PARSER_REWRITE_DESIGN.md

### Development Workflow
- **Version Control**: Professional git commit messages and tagged releases
- **Testing**: Comprehensive test coverage for all changes
- **Documentation**: Keep docs updated with implementation changes

This architecture represents a mature approach to complex system migration, 
balancing innovation with stability to achieve POSIX compliance while 
maintaining production reliability.
