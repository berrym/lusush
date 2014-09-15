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
    if (initialized)
	return 0;

    if ((root_node = alloc_alias()) == NULL) {
	perror("lusuh: alias.c: alloc_alias_list");
	return -1;
    }

    set_alias("h", "help");
    print_alias();
    vprint("%s init_alias_list: successful init_alias_list call\n", DBGSTR);
    initialized = true;

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

/* int set_alias(ALIAS *a, char *key, char *val) */
/* { */
/*     strncpy(a->key, key, BUFSIZE); */
/*     strncpy(a->val, val, BUFSIZE); */

/*     a->next = calloc(1, sizeof(ALIAS)); */
/*     b = find_end(); */
/*     a->prev = b->next; */

/*     return 0; */
/* } */

ALIAS *find_end(void)
{
    ALIAS *a = root_node;

    if (!root_node)
      return NULL;

    while (a->next) {
	a = a->next;
    }

    return a;
}

ALIAS *lookup_alias(char *key)
{
    ALIAS *a = root_node;
    do {
	if (strcmp(a->key, key) == 0) {
	    return a;
	}
	/* else { */
	/*     return NULL; */
	/* } */
    } while (a->next);

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

    if ((a = lookup_alias(key)) != NULL) {
	strncpy(key, a->key, BUFSIZE);
	strncpy(val, a->val, BUFSIZE);
	return 0;
    }

    a  = find_end();
    ALIAS *newalias;;
    if ((newalias = calloc(1, sizeof(ALIAS))) == NULL) {
	perror("lusush: set_alias: calloc");
	return -1;
    }

    strncpy(newalias->key, key, BUFSIZE);
    strncpy(newalias->val, val, BUFSIZE);
    newalias->prev = a;
    a->next = newalias;
    vprint("%sset_alias: new alias set!\n", DBGSTR);

    return 0;
}

void unset_alias(char *key)
{
}

void print_alias ()
{
    ALIAS *a = root_node;
    printf("aliases:\n");
    while (a->next) {	printf("%s->%16s\n", a->key, a->val);
	a = a->next;
    }
}
