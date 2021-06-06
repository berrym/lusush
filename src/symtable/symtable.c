#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lusush.h"
#include "errors.h"
#include "node.h"
#include "parser.h"
#include "symtable.h"

struct symtablestack symtable_stack;
int symtable_level;

void init_symtable(void)
{
    struct symtable *global_symtable = NULL;

    symtable_stack.symtable_count = 1;
    symtable_level = 0;

    if ((global_symtable = calloc(1, sizeof(struct symtable))) == NULL)
        error_abort("init_symtable: calloc");

    symtable_stack.global_symtable = global_symtable;
    symtable_stack.local_symtable = global_symtable;
    symtable_stack.symtable_list[0] = global_symtable;
    global_symtable->level = 0;
}

struct symtable *new_symtable(int level)
{
    struct symtable *symtable = NULL;
    if ((symtable = calloc(1, sizeof(struct symtable))) == NULL)
        error_abort("new_symtable: calloc");
    symtable->level = level;
    return symtable;
}

void free_symtable(struct symtable *symtable)
{
    if (!symtable)
        return;

    struct symtable_entry *entry = symtable->head;
    struct symtable_entry *next = NULL;

    while (entry) {
        if (entry->name)
            free(entry->name);

        if (entry->val)
            free(entry->val);

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
    struct symtable *symtable = symtable_stack.local_symtable;
    int i = 0;
    int indent = symtable->level * 4;
    fprintf(stderr, "%*sSymbol table [Level %d]:\r\n", indent, " ",
            symtable->level);
    fprintf(stderr, "%*s===========================\r\n",
            indent, " ");
    fprintf(stderr, "%*s  No               Symbol                    Val\r\n",
            indent, " ");
    fprintf(stderr, "%*s------ -------------------------------- ------------\r\n",
            indent, " ");

    struct symtable_entry *entry = symtable->head;

    while (entry) {
        fprintf(stderr, "%*s[%04d] %-32s '%s'\r\n",
                indent, " ", i++, entry->name, entry->val);
        entry = entry->next;
    }

    fprintf(stderr, "%*s------ -------------------------------- ------------\r\n",
            indent, " ");
}

struct symtable_entry *add_to_symtable(char *symbol)
{
    if (!symbol || *symbol == '\0')
        return NULL;

    struct symtable *st = symtable_stack.local_symtable;
    struct symtable_entry *entry = NULL;

    if ((entry = do_lookup(symbol, st)))
        return entry;

    if ((entry = calloc(1, sizeof(struct symtable_entry))) == NULL)
        error_abort("add_to_symtable: calloc");

    if ((entry->name = calloc(strnlen(symbol, MAXLINE) + 1, sizeof(char))) == NULL)
        error_abort("add_to_symtable: calloc");

    strncpy(entry->name, symbol, strnlen(symbol, MAXLINE) + 1);

    if (!st->head) {
        st->head = entry;
        st->tail = entry;
    } else {
        st->tail->next = entry;
        st->tail = entry;
    }

    return entry;
}

int rem_from_symtable(struct symtable_entry *entry, struct symtable *symtable)
{
    int res = 0;

    if (entry->val)
        free(entry->val);

    if (entry->func_body)
        free_node_tree(entry->func_body);

    free(entry->name);

    if (symtable->head == entry) {
        symtable->head = symtable->head->next;
        if(symtable->tail == entry)
            symtable->tail = NULL;
        res = 1;
    } else {
        struct symtable_entry *e = symtable->head;
        struct symtable_entry *p = NULL;

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
    return res;
}

struct symtable_entry *do_lookup(const char *str, struct symtable *symtable)
{
    if (!str || !symtable)
        return NULL;

    struct symtable_entry *entry = symtable->head;

    while (entry) {
        if (strncmp(entry->name, str, strnlen(str, MAXLINE)) == 0)
            return entry;
        entry = entry->next;
    }

    return NULL;
}

struct symtable_entry *get_symtable_entry(const char *str)
{
    int i = symtable_stack.symtable_count-1;

    do
    {
        struct symtable *symtable = symtable_stack.symtable_list[i];
        struct symtable_entry *entry = do_lookup(str, symtable);

        if(entry)
            return entry;
    } while(--i >= 0);

    return NULL;
}

void symtable_entry_setval(struct symtable_entry *entry, char *val)
{
    if (entry->val)
        free(entry->val);

    if (!val) {
        entry->val = NULL;
    }
    else
    {
        char *val2 = NULL;

        if ((val2 = calloc(strnlen(val, MAXLINE) + 1, sizeof(char))) == NULL)
            error_syscall("symtable_entry_setval: calloc");

        if (val2)
            strncpy(val2, val, strnlen(val, MAXLINE));

        entry->val = val2;
    }
}

void symtable_stack_add(struct symtable *symtable)
{
    symtable_stack.symtable_list[symtable_stack.symtable_count++] = symtable;
    symtable_stack.local_symtable = symtable;
}

struct symtable *symtable_stack_push(void)
{
    struct symtable *st = new_symtable(++symtable_level);
    symtable_stack_add(st);
    return st;
}

struct symtable *symtable_stack_pop(void)
{
    if (symtable_stack.symtable_count == 0)
        return NULL;

    struct symtable *st = \
        symtable_stack.symtable_list[symtable_stack.symtable_count - 1];

    symtable_stack.symtable_list[--symtable_stack.symtable_count] = NULL;
    symtable_level--;

    if (symtable_stack.symtable_count == 0) {
        symtable_stack.local_symtable  = NULL;
        symtable_stack.global_symtable = NULL;
    } else {
        symtable_stack.local_symtable = \
            symtable_stack.symtable_list[symtable_stack.symtable_count - 1];
    }

    return st;
}

struct symtable *get_local_symtable(void)
{
    return symtable_stack.local_symtable;
}

struct symtable *get_global_symtable(void)
{
    return symtable_stack.global_symtable;
}

struct symtablestack *get_symtable_stack(void)
{
    return &symtable_stack;
}
