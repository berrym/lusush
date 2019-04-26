/**
 * alias.c - alias implementation routines
 *
 * Copyright (c) 2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lusush.h"
#include "errors.h"
#include "alias.h"

struct alias {
    char key[MAXLINE];
    char val[MAXLINE];
    struct alias *next;
};

static struct alias *head = NULL;

/**
 * alloc_alias:
 *      Allocate memory for a struct alias.
 */
static struct alias *alloc_alias(void)
{
    struct alias *new_alias = NULL;

    if ((new_alias = calloc(1, sizeof(struct alias))) == NULL) {
        error_return("lusush: alloc_alias error");
        return NULL;
    }

    return new_alias;
}

/**
 * lookup_alias:
 *      Find a  node in the list by key lookup and return a pointer to it.
 */
static struct alias *lookup_alias(const char *key)
{
    struct alias *curr = NULL;

    for (curr = head; curr != NULL; curr = curr->next)
        if (strncmp(curr->key, key, MAXLINE) == 0)
            return curr;

    return NULL;
}

/**
 * free_alias_list:
 *      Free each node in the alias list.
 */
void free_alias_list(void)
{
    struct alias *curr = NULL;

    if (!head)
        return;

    while ((curr = head) != NULL) {
        head = head->next;
        free(curr);
        curr = NULL;
    }
}

/**
 * print_alias_list:
 *      Display the key->val mappings of the alias list.
 */
void print_alias_list(void)
{
    struct alias *curr = head;

    printf("aliases:\n");
    while (curr) {
        printf("%s->\t%s\n", curr->key, curr->val);
        curr = curr->next;
    }
}

/**
 * expand_alias:
 *      Lookup an alias by key and return it's associated value.
 */
char *expand_alias(char *key)
{
    struct alias *curr = NULL;

    if ((curr = lookup_alias(key)) == NULL)
        return NULL;

    return curr->val;
}

/**
 * set_alias:
 *      Create and insert new node in the list or replace an existing one.
 */
void set_alias(const char *key, const char *val)
{
    struct alias *curr = NULL, *new_alias = NULL;

    // Replace an existing alias
    if ((curr = lookup_alias(key))) {
        vputs("set_alias: re-setting alias\n");
        strncpy(curr->val, val, MAXLINE);
        return;
    }

    // Allocate a new alias node
    if ((new_alias = alloc_alias()) == NULL)
        return;

    // Special case for dealing with the head node
    if (head == NULL || (strncasecmp(head->key, key, MAXLINE) > 0)) {
        vputs("set_alias: setting root alias node\n");
        strncpy(new_alias->key, key, MAXLINE);
        strncpy(new_alias->val, val, MAXLINE);
        new_alias->next = head;
        head = new_alias;
    }
    else {
        // Find the node prior to point of insertion
        curr = head;
        while (curr->next && (strncasecmp(key, curr->next->key, MAXLINE) > 0))
            curr = curr->next;

        // Insert the new node
        new_alias->next = curr->next;
        curr->next = new_alias;
        strncpy(new_alias->key, key, strnlen(key, MAXLINE));
        strncpy(new_alias->val, val, strnlen(val, MAXLINE));
        vputs("set_alias: new alias set!\n");
    }
}

/**
 * unset_alias:
 *      Remove a node in the list.
 */
void unset_alias(char *key)
{
    struct alias *curr = NULL, *prev = NULL;

    for (curr = head; curr != NULL; prev = curr, curr = curr->next) {
        if (strncmp(curr->key, key, MAXLINE) == 0) {
            if (prev == NULL)
                head = curr->next;
            else
                prev->next = curr->next;
            free(curr);
            curr = NULL;
            break;
        }
    }
}
