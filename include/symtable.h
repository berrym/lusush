#include "node.h"

#ifndef SYMTABLE_H
#define SYMTABLE_H

// Type of a symbol table entry's value
enum symbol_type {
    SYM_STR,
    SYM_FUNC,
};

// the symbol table entry structure
struct symtable_entry {
    char *name;
    enum symbol_type val_type;
    char *val;
    unsigned int flags;
    struct symtable_entry *next;
    struct node *func_body;
};

// The symbol table structure
struct symtable {
    int level;
    struct symtable_entry *head, *tail;
};

// Values for the flags field of struct symtable_entry
#define FLAG_EXPORT (1 << 0)      // export entry to forked commands
#define FLAG_READONLY (1 << 1)    // entry is read only
#define FLAG_CMD_EXPORT (1 << 2)  // used temporarily between cmd fork and exec
#define FLAG_LOCAL (1 << 3)       // entry is local (to script or function)
#define FLAG_ALLCAPS (1 << 4)     // convert value to capital letters when assigned
#define FLAG_ALLSMALL (1 << 5)    // convert value to small letters when assigned
#define FLAG_FUNCTRACE (1 << 6)   // enable function tracing (bash, ksh)
#define FLAG_INTVAL (1 << 7)      // assign only integer values (bash)
#define FLAG_SPECIAL_VAR (1 << 8) // special shell variable, e.g. $RANDOM 
#define FLAG_TEMP_VAR (1 << 9)    // temp var (used during arithmetic expansion)

// The symbol table stack structure
#define MAX_SYMTAB 256

struct symtablestack {
    size_t symtable_count;
    struct symtable *symtable_list[MAX_SYMTAB];
    struct symtable *global_symtable, *local_symtable;
};

// Function prototypes
struct symtable *new_symtable(int);
struct symtable *symtablestack_push(void);
struct symtable *symtablestack_pop(void);
int rem_from_symtable(struct symtable_entry *, struct symtable *);
struct symtable_entry *add_to_symtable(char *);
struct symtable_entry *do_lookup(const char *, struct symtable *);
struct symtable_entry *get_symtable_entry(const char *);
struct symtable *get_local_symtable(void);
struct symtable *get_global_symtable(void);
struct symtablestack *get_symtablestack(void);
void init_symtable(void);
void dump_local_symtable(void);
void free_symtable(struct symtable *);
void free_global_symtable(void);
void symtable_entry_setval(struct symtable_entry *, char *);

#endif
