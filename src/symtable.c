#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "errors.h"
#include "node.h"
#include "symtable.h"
#include "strings.h"

symtable_stack_s symtable_stack;
size_t symtable_level;

void init_symtable(void)
{
    symtable_s *global_symtable = NULL;

    symtable_stack.symtable_count = 1;
    symtable_level = 0;

    global_symtable = calloc(1, sizeof(symtable_s));
    if (!global_symtable)
        error_abort("init_symtable");

    symtable_stack.global_symtable = global_symtable;
    symtable_stack.local_symtable = global_symtable;
    symtable_stack.symtable_list[0] = global_symtable;
    global_symtable->level = 0;
}

symtable_s *new_symtable(size_t level)
{
    symtable_s *symtable = NULL;
    symtable = calloc(1, sizeof(symtable_s));
    if (!symtable)
        error_abort("new_symtable");
    symtable->level = level;
    return symtable;
}

void free_symtable(symtable_s *symtable)
{
    symtable_entry_s *entry = NULL, *next = NULL;

    if (!symtable)
        return;

    entry = symtable->head;

    while (entry) {
        if (entry->name)
            free_str(entry->name);

        if (entry->val)
            free_str(entry->val);

        if (entry->func_body)
            free_node_tree(entry->func_body);

        next = entry->next;
        free(entry);
        entry = next;
    }

    free(symtable);
}

void free_global_symtable(void)
{
    free_symtable(get_global_symtable());
}

void dump_local_symtable(void)
{
    symtable_s *symtable = symtable_stack.local_symtable;
    int i = 0;
    int indent = symtable->level * 4;
    fprintf(stderr, "%*sSymbol table [Level %zu]:\r\n", indent, " ",
            symtable->level);
    fprintf(stderr, "%*s===========================\r\n",
            indent, " ");
    fprintf(stderr, "%*s  No               Symbol                    Val\r\n",
            indent, " ");
    fprintf(stderr, "%*s------ -------------------------------- ------------\r\n",
            indent, " ");

    symtable_entry_s *entry = symtable->head;

    while (entry) {
        fprintf(stderr, "%*s[%04d] %-32s '%s'\r\n",
                indent, " ", i++, entry->name, entry->val);
        entry = entry->next;
    }

    fprintf(stderr, "%*s------ -------------------------------- ------------\r\n",
            indent, " ");
}

symtable_entry_s *add_to_symtable(char *symbol)
{
    symtable_s *st = symtable_stack.local_symtable;
    symtable_entry_s *entry = NULL;

    if (!symbol || *symbol == '\0')
        return NULL;

    if ((entry = lookup_symbol(st, symbol)))
        return entry;

    entry = calloc(1, sizeof(symtable_entry_s));
    if (!entry)
        error_abort("add_to_symtable");

    entry->name = alloc_str(strlen(symbol) + 1, false);
    if (!entry->name)
        error_abort("add_to_symtable");

    strncpy(entry->name, symbol, strlen(symbol) + 1);

    if (!st->head) {
        st->head = entry;
        st->tail = entry;
    } else {
        st->tail->next = entry;
        st->tail = entry;
    }

    return entry;
}

int remove_from_symtable(symtable_s *symtable, symtable_entry_s *entry)
{
    int res = 0;
    symtable_entry_s *e = NULL, *p = NULL;

    if (!symtable)
        return 1;

    if (entry->name)
        free_str(entry->name);

    if (entry->val)
        free_str(entry->val);

    if (entry->func_body)
        free_node_tree(entry->func_body);

    if (symtable->head == entry) {
        symtable->head = symtable->head->next;
        if (symtable->tail == entry)
            symtable->tail = NULL;
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

symtable_entry_s *lookup_symbol(symtable_s *symtable, const char *str)
{
    if (!str || !symtable)
        return NULL;

    symtable_entry_s *entry = symtable->head;

    while (entry) {
        if (strcmp(entry->name, str) == 0)
            return entry;
        entry = entry->next;
    }

    return NULL;
}

symtable_entry_s *get_symtable_entry(const char *str)
{
    ssize_t i = symtable_stack.symtable_count - 1;

    do {
        symtable_s *symtable = symtable_stack.symtable_list[i];
        symtable_entry_s *entry = lookup_symbol(symtable, str);

        if(entry)
            return entry;
    } while(--i >= 0);

    return NULL;
}

void symtable_entry_setval(symtable_entry_s *entry, char *val)
{
    if (entry->val)
        free(entry->val);

    if (!val) {
        entry->val = NULL;
    } else {
        char *val2 = NULL;

        val2 = calloc(strlen(val) + 1, sizeof(char));
        if (!val2)
            error_syscall("symtable_entry_setval");

        strcpy(val2, val);
        entry->val = val2;
    }
}

void symtable_stack_add(symtable_s *symtable)
{
    symtable_stack.symtable_list[symtable_stack.symtable_count++] = symtable;
    symtable_stack.local_symtable = symtable;
}

symtable_s *symtable_stack_push(void)
{
    symtable_s *st = new_symtable(++symtable_level);
    symtable_stack_add(st);
    return st;
}

symtable_s *symtable_stack_pop(void)
{
    if (symtable_stack.symtable_count == 0)
        return NULL;

    symtable_s *st = symtable_stack.symtable_list[symtable_stack.symtable_count - 1];

    symtable_stack.symtable_list[--symtable_stack.symtable_count] = NULL;
    symtable_level--;

    if (symtable_stack.symtable_count == 0) {
        symtable_stack.local_symtable  = NULL;
        symtable_stack.global_symtable = NULL;
    } else {
        symtable_stack.local_symtable = symtable_stack.symtable_list[symtable_stack.symtable_count - 1];
    }

    return st;
}

symtable_s *get_local_symtable(void)
{
    return symtable_stack.local_symtable;
}

symtable_s *get_global_symtable(void)
{
    return symtable_stack.global_symtable;
}

symtable_stack_s *get_symtable_stack(void)
{
    return &symtable_stack;
}
