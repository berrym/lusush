#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "lusush.h"
#include "symtable.h"

char *get_shell_varp(char *name, char *def_val)
{
    symtable_entry_s *entry = get_symtable_entry(name);
    return (entry && entry->val && entry->val[0]) ? entry->val : def_val;
}

int get_shell_vari(char *name, int def_val)
{
    return (int)get_shell_varl(name, def_val);
}

long get_shell_varl(char *name, int def_val)
{
    symtable_entry_s *entry = get_symtable_entry(name);
    if (entry && entry->val && entry->val[0]) {
        char *strend = NULL;
        long i = strtol(entry->val, &strend, 10);
        if (strend == entry->val || *strend)
            return def_val;
        return i;
    }
    return def_val;
}

void set_shell_varp(char *name, char *val)
{
    // Get the entry
    symtable_entry_s *entry = get_symtable_entry(name);

    // Add to local symbol table
    if (entry == NULL)
        entry = add_to_symtable(name);

    // Set the entry's value
    symtable_entry_setval(entry, val);
}

void set_shell_vari(char *name, int val)
{
    char buf[32];
    sprintf(buf, "%d", val);
    set_shell_varp(name, buf);
}

void set_underscore_val(char *val, int set_env)
{
    symtable_entry_s *entry = add_to_symtable("_");
    symtable_entry_setval(entry, val);
    if (set_env)
        setenv("_", val, 1);
}
