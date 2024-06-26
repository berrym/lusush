#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "node.h"

#include <stddef.h>

// Type of a symbol table entry's value
typedef enum {
    SYM_STR,
    SYM_FUNC,
} symbol_type_t;

// the symbol table entry structure
typedef struct symtable_entry {
    char *name;
    symbol_type_t val_type;
    char *val;
    unsigned int flags;
    struct symtable_entry *next;
    node_t *func_body;
} symtable_entry_t;

// The symbol table structure
typedef struct {
    size_t level;
    symtable_entry_t *head, *tail;
} symtable_t;

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

typedef struct {
    size_t symtable_count;
    symtable_t *symtable_list[MAX_SYMTAB];
    symtable_t *global_symtable, *local_symtable;
} symtable_stack_t;

// Function prototypes
symtable_t *new_symtable(size_t);
symtable_t *symtable_stack_push(void);
symtable_t *symtable_stack_pop(void);
int remove_from_symtable(symtable_t *, symtable_entry_t *);
symtable_entry_t *add_to_symtable(char *);
symtable_entry_t *lookup_symbol(symtable_t *, const char *);
symtable_entry_t *get_symtable_entry(const char *);
symtable_t *get_local_symtable(void);
symtable_t *get_global_symtable(void);
symtable_stack_t *get_symtable_stack(void);
void init_symtable(void);
void dump_local_symtable(void);
void free_symtable(symtable_t *);
void free_global_symtable(void);
void symtable_entry_setval(symtable_entry_t *, char *);

#endif
