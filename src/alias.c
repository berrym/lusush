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

typedef struct _alias {
    char key[BUFSIZ];
    char val[BUFSIZ];
} ALIAS;

static size_t MAX_ALIAS = 50;
static size_t alias_cnt = 0;
static bool  initialized = false;
static ALIAS **alias_list = NULL;

/**
 * alloc_alias_list:
 *      allocate room on heap for alias_list
 */
static int alloc_alias_list(void)
{
    if (alias_list) {
        vprint("%salloc_alias_list: alias_list ALREADY allocd\n", DBGSTR);
        return -1;
    }

    if ((alias_list = calloc(MAX_ALIAS, sizeof(ALIAS *))) == NULL) {
        perror("lusush: alias.c: alloc_alias_list");
        return -1;
    }
    vprint("%salloc_alias_list: alias_list allocd\n", DBGSTR);

    return 0;
}

/**
 * alloc_alias:
 *      allocate room on heap for aliases, using first availiable location
 */
static ALIAS *alloc_alias(void)
{
    size_t i = 0;

    // check that memory has been allocated for list of aliases
    if (!initialized) {
        if (!alias_list) {
            if (alloc_alias_list() < 0) {
                return NULL;
            }
        }

        *alias_list = calloc(1, sizeof(ALIAS));
        if (!*alias_list) {
            perror("lusush: alias.c: alloc_alias");
            return NULL;
        }
        vprint("%salloc_alias: *alias_list allocd\n", DBGSTR);
        initialized = true;
    }
    else {
        for (i = 0; i < MAX_ALIAS; i++) {
            if (!*(alias_list + i))
                break;
        }

        // allocate memory for new alias
        if ((*(alias_list + i) = calloc(1, sizeof(ALIAS))) == NULL) {
            perror("lusush: alias.c");
            return NULL;
        }

        vprint("%salloc_alias: *(alias_list + i) allocd\n", DBGSTR);
    }

    ++alias_cnt;

    return *(alias_list + i);
}

/**
 * free_alias:
 *      free memory allocated by an alias
 */
static void free_alias(ALIAS *alias)
{
    if (!alias) {
        vprint("%sunset_alias: bad pointer\n", DBGSTR);
        return;
    }

    memset(alias->key, 0, BUFSIZ);
    memset(alias->key, 0, BUFSIZ);
    free(alias);
    alias = NULL;
}

/**
 * lookup_alias:
 *      find an alias by it's key and return a pointer to it
 */
static ALIAS *lookup_alias(char *key)
{
    size_t i = 0;

    if (!initialized) {
        vprint("%slookup_alias: aliases not initialized\n", DBGSTR);
        return NULL;
    }

    for (i = 0; i < MAX_ALIAS; i++) {
        if (*(alias_list + i)) {
            if (strncmp(alias_list[i]->key, key, BUFSIZ) == 0) {
                vprint("%slookup_alias: found alias for %s\n",
                        DBGSTR, key);
                return *(alias_list + i);
            }
        }
    }

    vprint("%slookup_alias: did not find alias for %s\n", DBGSTR, key);

    return NULL;
}

/**
 * expand_alias:
 *      return the expanded alias string value associated with key
 */
char *expand_alias(char *key)
{
    ALIAS *alias = NULL;

    alias = lookup_alias(key);

    if (!alias) {
        vprint("%sexpand_alias: unable to expand key=%s\n",
                DBGSTR, key);
        return NULL;
    }

    vprint("%sexpand_alias: expanded %s to %s\n",
            DBGSTR, alias->key, alias->val);

    return alias->val;
}

/**
 * set_alias:
 *    define a new alias
 */
int set_alias(char *key, char *val)
{
    ALIAS *alias = NULL;

    if ((!key || !*key) || (!val || !*val)) {
        fprintf(stderr, "lusush: set_alias: invalid paramter(s).\n");
        return -1;
    }

    if (!(alias = lookup_alias(key)))
        alias = alloc_alias();

    if (!alias) {
        vprint("%sset_alias: alias creation failed!\n", DBGSTR);
        return -1;
    }

    // set the new alias
    strncpy(alias->key, key, strlen(key));
    strncpy(alias->val, val, strlen(val));

    vprint("%sset_alias: key=%s val=%s\n",
            DBGSTR, alias->key, alias->val);

    return 0;
}

/**
 * unset_alias:
 *    remove an existing alias
 */
void unset_alias(char *key)
{
    ALIAS *alias = NULL;

    if (!key || !*key)
        return;

    if (!(alias = lookup_alias(key)))
        return;

    free_alias(alias);
}

/**
 * print_alias:
 *    display all existing alias
 */
void print_alias(void)
{
    int i = 0;

    for (i = 0; i < MAX_ALIAS; i++) {
        if (alias_list[i]) {
            printf("\t%-16s\t%s\n", alias_list[i]->key, alias_list[i]->val);
        }
    }
}

/**
 * set_max_alias:
 *    set the maximum number of definable aliases for runtime
 */
void set_max_alias(size_t new_max)
{
    MAX_ALIAS = new_max;
}
