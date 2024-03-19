#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "node.h"
#include <stddef.h>

// Type of a symbol table entry's value
typedef enum symbol_type_e {
    SYM_STR,
    SYM_FUNC,
} symbol_type_e;

// the symbol table entry structure
typedef struct symtable_entry_s {
    char *name;
    symbol_type_e val_type;
    char *val;
    unsigned int flags;
    struct symtable_entry_s *next;
    node_s *func_body;
} symtable_entry_s;

// The symbol table structure
typedef struct symtable_s {
    size_t level;
    symtable_entry_s *head, *tail;
} symtable_s;

// Values for the flags field of struct symtable_entry
#define FLAG_EXPORT (1 << 0)     // export entry to forked commands
#define FLAG_READONLY (1 << 1)   // entry is read only
#define FLAG_CMD_EXPORT (1 << 2) // used temporarily between cmd fork and exec
#define FLAG_LOCAL (1 << 3)      // entry is local (to script or function)
#define FLAG_ALLCAPS (1 << 4)  // convert value to capital letters when assigned
#define FLAG_ALLSMALL (1 << 5) // convert value to small letters when assigned
#define FLAG_FUNCTRACE (1 << 6)   // enable function tracing (bash, ksh)
#define FLAG_INTVAL (1 << 7)      // assign only integer values (bash)
#define FLAG_SPECIAL_VAR (1 << 8) // special shell variable, e.g. $RANDOM
#define FLAG_TEMP_VAR (1 << 9)    // temp var (used during arithmetic expansion)

// The symbol table stack structure
#define MAX_SYMTAB 256

typedef struct symtable_stack_s {
    size_t symtable_count;
    symtable_s *symtable_list[MAX_SYMTAB];
    symtable_s *global_symtable, *local_symtable;
} symtable_stack_s;

// Function prototypes
symtable_s *new_symtable(size_t);
symtable_s *symtable_stack_push(void);
symtable_s *symtable_stack_pop(void);
int remove_from_symtable(symtable_s *, symtable_entry_s *);
symtable_entry_s *add_to_symtable(char *);
symtable_entry_s *lookup_symbol(symtable_s *, const char *);
symtable_entry_s *get_symtable_entry(const char *);
symtable_s *get_local_symtable(void);
symtable_s *get_global_symtable(void);
symtable_stack_s *get_symtable_stack(void);
void init_symtable(void);
void dump_local_symtable(void);
void free_symtable(symtable_s *);
void free_global_symtable(void);
void symtable_entry_setval(symtable_entry_s *, char *);

#endif
