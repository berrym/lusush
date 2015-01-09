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

static ALIAS *head = NULL, *tail = NULL;
static bool initialized = false;

int init_alias_list(void)
{
    if (head)
        return 0;

    if ((head = alloc_alias()) == NULL) {
        return -1;
    }

    set_alias("h", "help");
    vprint("%sinit_alias_list: successful init_alias_list call\n", DBGSTR);

    return 0;
}

ALIAS *alloc_alias()
{
    tail = find_end();

    if ((tail = calloc(1, sizeof(ALIAS))) == NULL) {
        perror("lusush: alloc_alias: calloc");
        return NULL;
    }

    return tail;
}

ALIAS *find_end(void)
{
    if (!head)
        return NULL;

    ALIAS *curr = head;

    if (curr->next) {
        while (curr->next) {
            curr = curr->next;
        }
    }

    return curr;
}

ALIAS *lookup_alias(char *key)
{
    ALIAS *curr = NULL, *prev = NULL;

    for (curr = head; curr != NULL; prev = curr, curr = curr->next) {
        if (strncmp(curr->key, key, BUFSIZE) == 0) {
            return curr;
        }
    }

    return NULL;
}

char *expand_alias(char *key)
{
    ALIAS *curr;

    if ((curr = lookup_alias(key)) == NULL) {
        return NULL;
    }

    return curr->val;
}

int set_alias(char *key, char *val)
{
    ALIAS *curr = NULL;

    if (head && !initialized) {
        vprint("%sset_alias: setting root alias node\n", DBGSTR);
        strncpy(head->key, key, BUFSIZE);
        strncpy(head->val, val, BUFSIZE);
        initialized = true;
        return 0;
    }

    if ((curr = lookup_alias(key))) {
        vprint("%sset_alias: re-setting alias\n", DBGSTR);
        strncpy(curr->key, key, BUFSIZE);
        strncpy(curr->val, val, BUFSIZE);
        return 0;
    }

    curr = find_end();
    if ((curr->next = alloc_alias()) == NULL) {
        fprintf(stderr, "Unable to allocate newalias!\n");
        return -1;
    }
    curr = curr->next;
    strncpy(curr->key, key, BUFSIZE);
    strncpy(curr->val, val, BUFSIZE);
    vprint("%sset_alias: new alias set!\n", DBGSTR);

    return 0;
}

void unset_alias(char *key)
{
    ALIAS *curr = NULL, *prev = NULL;

    for (curr = head; curr != NULL; prev = curr, curr = curr->next) {
        if (strncmp(curr->key, key, BUFSIZE) == 0) {
            if (prev == NULL) {
                head = curr->next;
            }
            else {
                prev->next = curr->next;
            }

            free(curr);

            return;
        }
    }
}

void print_alias_list()
{
    ALIAS *curr = head;
    printf("aliases:\n");
    do {
        printf("%s->%16s\n", curr->key, curr->val);
        curr = curr->next;
    } while (curr->next);
}
