#include "../include/symtable.h"

#include "../include/errors.h"
#include "../include/node.h"
#include "../include/strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

symtable_stack_t symtable_stack;
size_t symtable_level;

void init_symtable(void) {
    symtable_t *global_symtable = NULL;

    symtable_stack.symtable_count = 1;
    symtable_level = 0;

    global_symtable = calloc(1, sizeof(symtable_t));
    if (global_symtable == NULL) {
        error_syscall("error: `init_symtable`");
    }

    symtable_stack.global_symtable = global_symtable;
    symtable_stack.local_symtable = global_symtable;
    symtable_stack.symtable_list[0] = global_symtable;
    global_symtable->level = 0;
}

symtable_t *new_symtable(size_t level) {
    symtable_t *symtable = NULL;
    symtable = calloc(1, sizeof(symtable_t));
    if (symtable == NULL) {
        error_syscall("new_symtable");
    }
    symtable->level = level;
    return symtable;
}

void free_symtable(symtable_t *symtable) {
    symtable_entry_t *entry = NULL, *next = NULL;

    if (!symtable) {
        return;
    }

    entry = symtable->head;

    while (entry) {
        if (entry->name) {
            free_str(entry->name);
        }

        if (entry->val) {
            free_str(entry->val);
        }

        if (entry->func_body) {
            free_node_tree(entry->func_body);
        }

        next = entry->next;
        free(entry);
        entry = next;
    }

    free(symtable);
}

void free_global_symtable(void) { free_symtable(get_global_symtable()); }

void dump_local_symtable(void) {
    symtable_t *symtable = symtable_stack.local_symtable;
    int i = 0;
    int indent = symtable->level * 4;

    fprintf(stderr, "%*sSymbol table [Level %zu]:\n", indent, " ",
            symtable->level);

    fprintf(stderr, "%*s===========================\n", indent, " ");

    fprintf(stderr, "%*s  No               Symbol                    Val\n",
            indent, " ");

    fprintf(stderr, "%*s------ -------------------------------- ------------\n",
            indent, " ");

    symtable_entry_t *entry = symtable->head;

    while (entry) {
        fprintf(stderr, "%*s[%04d] %-32s '%s'\n", indent, " ", i++, entry->name,
                entry->val);
        entry = entry->next;
    }

    fprintf(stderr, "%*s------ -------------------------------- ------------\n",
            indent, " ");
}

symtable_entry_t *add_to_symtable(char *symbol) {
    symtable_t *st = symtable_stack.local_symtable;
    symtable_entry_t *entry = NULL;

    if (!symbol || *symbol == '\0') {
        return NULL;
    }

    if ((entry = lookup_symbol(st, symbol))) {
        return entry;
    }

    entry = calloc(1, sizeof(symtable_entry_t));
    if (!entry) {
        error_abort("add_to_symtable");
    }

    entry->name = alloc_str(strlen(symbol) + 1, false);
    if (!entry->name) {
        error_abort("add_to_symtable");
    }

    strcpy(entry->name, symbol);

    if (!st->head) {
        st->head = entry;
        st->tail = entry;
    } else {
        st->tail->next = entry;
        st->tail = entry;
    }

    return entry;
}

int remove_from_symtable(symtable_t *symtable, symtable_entry_t *entry) {
    int res = 0;
    symtable_entry_t *e = NULL, *p = NULL;

    if (!symtable) {
        return 1;
    }

    if (entry->name) {
        free_str(entry->name);
    }

    if (entry->val) {
        free_str(entry->val);
    }

    if (entry->func_body) {
        free_node_tree(entry->func_body);
    }

    if (symtable->head == entry) {
        symtable->head = symtable->head->next;
        if (symtable->tail == entry) {
            symtable->tail = NULL;
        }
        res = 1;
    } else {
        e = symtable->head;

        while (e && e != entry) {
            p = e;
            e = e->next;
        }

        if (e == entry) {
            p->next = entry->next;
            res = 1;
        }
    }

    free(entry);
    entry = NULL;

    return res;
}

symtable_entry_t *lookup_symbol(symtable_t *symtable, const char *str) {
    if (!str || !symtable) {
        return NULL;
    }

    symtable_entry_t *entry = symtable->head;

    while (entry) {
        if (strcmp(entry->name, str) == 0) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

symtable_entry_t *get_symtable_entry(const char *str) {
    ssize_t i = symtable_stack.symtable_count - 1;

    do {
        symtable_t *symtable = symtable_stack.symtable_list[i];
        symtable_entry_t *entry = lookup_symbol(symtable, str);

        if (entry) {
            return entry;
        }
    } while (--i >= 0);

    return NULL;
}

void symtable_entry_setval(symtable_entry_t *entry, char *val) {
    if (entry->val) {
        free(entry->val);
    }

    if (!val) {
        entry->val = NULL;
    } else {
        char *val2 = NULL;

        val2 = calloc(strlen(val) + 1, sizeof(char));
        if (!val2) {
            error_syscall("symtable_entry_setval");
        }

        strcpy(val2, val);
        entry->val = val2;
    }
}

void symtable_stack_add(symtable_t *symtable) {
    symtable_stack.symtable_list[symtable_stack.symtable_count++] = symtable;
    symtable_stack.local_symtable = symtable;
}

symtable_t *symtable_stack_push(void) {
    symtable_t *st = new_symtable(++symtable_level);
    symtable_stack_add(st);
    return st;
}

symtable_t *symtable_stack_pop(void) {
    if (symtable_stack.symtable_count == 0) {
        return NULL;
    }

    symtable_t *st =
        symtable_stack.symtable_list[symtable_stack.symtable_count - 1];

    symtable_stack.symtable_list[--symtable_stack.symtable_count] = NULL;
    symtable_level--;

    if (symtable_stack.symtable_count == 0) {
        symtable_stack.local_symtable = NULL;
        symtable_stack.global_symtable = NULL;
    } else {
        symtable_stack.local_symtable =
            symtable_stack.symtable_list[symtable_stack.symtable_count - 1];
    }

    return st;
}

symtable_t *get_local_symtable(void) { return symtable_stack.local_symtable; }

symtable_t *get_global_symtable(void) { return symtable_stack.global_symtable; }

symtable_stack_t *get_symtable_stack(void) { return &symtable_stack; }
