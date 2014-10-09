/**
 * alias.c - alias implementation routines
 *
 * Copyright (c) 2009-2014 Michael Berry <trismegustis@gmail.com>
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

static ALIAS *root_node = NULL;
bool initialized = false;

int init_alias_list(void)
{
    if (root_node)
        return 0;

    if ((root_node = alloc_alias()) == NULL) {
        perror("lusush: alias.c: alloc_alias_list");
        return -1;
    }

    set_alias("h", "help");
    vprint("%sinit_alias_list: successful init_alias_list call\n", DBGSTR);

    return 0;
}

ALIAS *alloc_alias()
{
    ALIAS *a, *last;

    if ((a = calloc(1, sizeof(ALIAS))) == NULL) {
        perror("lusush: alloc_alias: calloc");
        return NULL;
    }

    last = find_end();

    if (a != last)
        a->prev = last;
    else
        a->prev = NULL;

    a->next = NULL;

    return a;
}

ALIAS *find_end(void)
{
    if (!root_node)
        return NULL;

    ALIAS *a = root_node;

    if (a->next) {
        while (a->next) {
            a = a->next;
        }
    }

    return a;
}

ALIAS *lookup_alias(char *key)
{
    if (!root_node)
        return NULL;

    ALIAS *a = root_node;
    if (a && (a == root_node)) {
        vprint("looking up aliases\n");
        do {
            if (strcmp(a->key, key) == 0) {
                return a;
            }
            if (a->next)
                a = a->next;
        } while (a->next);
    }
    else {
        printf("%s: lookup_alias: not at root node\n", DBGSTR);
    }

    return NULL;
}

char *expand_alias(char *key)
{
    ALIAS *a;
    if ((a = lookup_alias(key)) == NULL) {
        return NULL;
    }

    return a->val;
}

int set_alias(char *key, char *val)
{
    ALIAS *a;

    if (root_node && !initialized) {
        vprint("%sset_alias: setting root alias node\n", DBGSTR);
        strncpy(root_node->key, key, BUFSIZE);
        strncpy(root_node->val, val, BUFSIZE);
        initialized = true;
        return 0;
    }

    if ((a = lookup_alias(key))) {
        vprint("%sset_alias: re-setting alias\n", DBGSTR);
        strncpy(a->key, key, BUFSIZE);
        strncpy(a->val, val, BUFSIZE);
        return 0;
    }

    ALIAS *newalias = find_end();
    if ((newalias->next = calloc(1, sizeof(ALIAS))) == NULL) {
        perror("lusush: set_alias: calloc");
        return -1;
    }

    newalias = newalias->next;
    strncpy(newalias->key, key, BUFSIZE);
    strncpy(newalias->val, val, BUFSIZE);

    a  = find_end();
    newalias->prev = a;
    a->next = newalias;
    vprint("%sset_alias: new alias set!\n", DBGSTR);

    return 0;
}

void unset_alias(char *key)
{
}

void print_alias_list()
{
    ALIAS *a = root_node;
    printf("aliases:\n");
    do {
        printf("%s->%16s\n", a->key, a->val);
        a = a->next;
    } while (a->next);
}
