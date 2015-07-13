/**
 * alias.c - alias implementation routines
 *
 * Copyright (c) 2009-2015 Michael Berry <trismegustis@gmail.com>
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
#include "alias.h"
#include "misc.h"

#define DBGSTR "DEBUG: alias.c: "

static struct alias *head = NULL;
static bool initialized = false;

/**
 * find_end:
 *      Traverse the list until the end is reached.
 */
static struct alias *find_end(void)
{
    struct alias *curr = head;

    if (!head)
        return NULL;

    while (curr->next)
        curr = curr->next;

    return curr;
}

/**
 * alloc_alias:
 *      Allocate memory for a struct alias at the end of the list.
 */
static struct alias *alloc_alias(void)
{
    struct alias *curr = find_end();

    if ((curr = calloc(1, sizeof(struct alias))) == NULL) {
        perror("lusush: alias.c: alloc_alias: calloc");
        return NULL;
    }

    return curr;
}

/**
 * lookup_alias:
 *      Find a  node in the list by key lookup and return a pointer to it.
 */
static struct alias *lookup_alias(char *key)
{
    struct alias *curr = NULL, *prev = NULL;

    for (curr = head; curr != NULL; prev = curr, curr = curr->next)
        if (strncmp(curr->key, key, MAXLINE) == 0)
            return curr;

    return NULL;
}

/**
 * init_alias_list:
 *    allocate memory for the linked list
 */
int init_alias_list(void)
{
    if (head)
        return 0;

    if ((head = alloc_alias()) == NULL)
        return -1;

    vprint("%sinit_alias_list: successful init_alias_list call\n", DBGSTR);

    return 0;
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
 *      Create a new node in the list or replace an existing one.
 */
int set_alias(char *key, char *val)
{
    struct alias *curr = NULL;

    if (!head)
        if (init_alias_list() == -1)
            return -1;

    if (head && !initialized) {
        vprint("%sset_alias: setting root alias node\n", DBGSTR);
        strncpy(head->key, key, MAXLINE);
        strncpy(head->val, val, MAXLINE);
        initialized = true;
        return 0;
    }

    if ((curr = lookup_alias(key))) {
        vprint("%sset_alias: re-setting alias\n", DBGSTR);
        strncpy(curr->key, key, MAXLINE);
        strncpy(curr->val, val, MAXLINE);
        return 0;
    }

    curr = find_end();
    if ((curr->next = alloc_alias()) == NULL)
        return -1;
    curr = curr->next;
    strncpy(curr->key, key, MAXLINE);
    strncpy(curr->val, val, MAXLINE);
    vprint("%sset_alias: new alias set!\n", DBGSTR);

    return 0;
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

    if (!head)
        initialized = false;
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
