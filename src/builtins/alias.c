#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "errors.h"
#include "lusush.h"
#include "strings.h"

struct alias_list *alias_list = NULL;

void init_aliases(void)
{
    if ((alias_list = calloc(1, sizeof(struct alias_list))) == NULL) {
        error_syscall("init_aliases");
        return;
    }

    alias_list->head = alias_list->tail = NULL;
    alias_list->len = 0;

    set_alias("ll", "ls -alF");
    set_alias("..", "cd ..");
    set_alias("...", "cd ../../../");
}

struct alias_entry *alloc_alias(void)
{
    struct alias_entry *new = NULL;

    if ((new = calloc(1, sizeof(struct alias_entry))) == NULL) {
        error_syscall("alloc_alias");
        return NULL;
    }

    if ((new->key = calloc(MAX_ALIAS_LEN + 1, sizeof(char))) == NULL) {
        error_syscall("alloc_alias");
        return NULL;
    }

    if ((new->val = calloc(MAX_ALIAS_LEN + 1, sizeof(char))) == NULL) {
        error_syscall("alloc_alias");
        return NULL;
    }

    new->next = NULL;

    return new;
}

struct alias_entry *lookup_alias(const char *key)
{
    struct alias_entry *curr = NULL;

    for (curr = alias_list->head; curr != NULL; curr = curr->next)
        if (strncmp(curr->key, key, MAX_ALIAS_LEN) == 0)
            return curr;

    return NULL;
}

void free_alias_list(void)
{
    if (!alias_list || !alias_list->head)
        return;

    struct alias_entry *curr, *next;

    curr = alias_list->head;
    next = curr->next;

    while (curr) {
        if (curr->key)
            free(curr->key);

        if (curr->val)
            free(curr->val);

        next = curr->next;

        if (curr)
            free(curr);

        curr = next;
    }

    if (alias_list)
        free(alias_list);
}

void print_alias_list(void)
{
    if (!alias_list || !alias_list->head)
        return;

    struct alias_entry *curr = alias_list->head;

    printf("aliases:\n");
    while (curr) {
        printf("%s=%s\n", curr->key, curr->val);
        curr = curr->next;
    }
}

char *expand_alias(char *key)
{
    struct alias_entry *curr = NULL;

    if ((curr = lookup_alias(key)) == NULL)
        return NULL;

    return curr->val;
}

bool set_alias(const char *key, const char *val)
{
    struct alias_entry *curr = NULL, *new_alias = NULL;

    if (!valid_alias_name(key)) {
        error_message("error: invalid alias name");
        return false;
    }

    // Replace an existing alias
    if ((curr = lookup_alias(key))) { 
        strncpy(curr->val, val, MAX_ALIAS_LEN);
        return true;
    }

    // Allocate a new alias node
    if ((new_alias = alloc_alias()) == NULL)
        return false;

    strncpy(new_alias->key, key, strnlen(key, MAX_ALIAS_LEN));
    strncpy(new_alias->val, val, strnlen(val, MAX_ALIAS_LEN));

    // Special case for dealing with the head node
    if (!alias_list->head) {
        alias_list->head = new_alias;
        alias_list->tail = new_alias;
    }
    else {
        alias_list->tail->next = new_alias;
        alias_list->tail = new_alias;
    }

    alias_list->len++;

    return true;
}

void unset_alias(const char *key)
{
    struct alias_entry *curr = NULL, *prev = NULL;

    for (curr = alias_list->head; curr != NULL; prev = curr, curr = curr->next) {
        if (strncmp(curr->key, key, MAX_ALIAS_LEN) == 0) {
            if (prev == NULL)
                alias_list->head = curr->next;
            else
                prev->next = curr->next;
            free(curr);
            curr = NULL;
            break;
        }
    }
}

bool valid_alias_name(const char *key)
{
    const char *p = key;

    if (!*p)
        return false;

    while (*p) {
        if (isalnum((int)*p)
            || *p == '.'
            || *p == '_'
            || *p == '!'
            || *p == '%'
            || *p == ','
            || *p == '@') {
            p++;
        } else {
            return false;
        }
    }

    return true;
}
