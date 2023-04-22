#include <sys/types.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "errors.h"
#include "lusush.h"
#include "strings.h"
#include "symtable.h"

// Symbol table for strings
symtable_s *str_list = NULL;

// Dummy values for an empty string and a newline string
char *empty_str = "";
char *newline_str = "\n";

void init_str_symtable(void)
{
    str_list = new_symtable(0);
}

char *__get_alloced_str(char *str)
{
    char *str2 = NULL;
    str2 = alloc_str(strlen(str) + 1, false);
    strcpy(str2, str);
    return str2;
}

char *get_alloced_str(char *s)
{
    if (!s)
        return NULL;

    if (!*s)
        return empty_str;

    if (*s == '\n' && s[1] == '\0')
        return newline_str;

    if (str_list) {
        symtable_entry_s *entry = get_symtable_entry(s);
        if (entry) {
            return entry->name;
        } else {
            entry = add_to_symtable(s);
            if (entry)
                return entry->name;
        }
    }

    return __get_alloced_str(s);
}

void free_alloced_str(char *s)
{
    if (!s || s == empty_str || s == newline_str)
        return;

    if (str_list) {
        symtable_entry_s *entry = get_symtable_entry(s);
        if (entry)
            remove_from_symtable(str_list, entry);

        return;
    }

    free_str(s);
}

char *alloc_str(size_t len, bool exitflag)
{
    char *s = NULL;

    s = calloc(len, sizeof(char));
    if (!s) {
        if (exitflag) {
            error_syscall("alloc_str");
        } else {
            error_return("alloc_str");
            return NULL;
        }
    }

    return s;
}

void free_str(char *s)
{
    if (!s)
        return;

    free(s);
    s = NULL;
}

bool strupper(char *s)
{
    if (!s)
        return false;

    while (*s) {
        *s = toupper(*s);
        s++;
    }

    return true;
}

bool strlower(char *s)
{
    if (!s)
        return false;

    while (*s) {
        *s = tolower(*s);
        s++;
    }

    return true;
}

char *str_strip_whitespace(char *s)
{
    char *p = NULL, *t = NULL;

    for (p = s; isspace((int)*p); p++);

    if (*p == '\0')
        return p;

    t = p + strlen(p) - 1;
    while (t > p && isspace((int)*t))
        t--;

    *++t = '\0';

    return p;
}

size_t str_skip_whitespace(char *s)
{
    size_t offset = 0;
    char c;

    while (((c = *s) != EOF) && isspace((int)c)) {
        s++;
        offset++;
    }

    return offset;
}

size_t str_strip_leading_whitespace(char *s)
{
    char buf[MAXLINE + 1] = { '\0' }; // buffer to store modified string
    size_t offset = 0;                // loop counter

    // Iterate over leading whitespace ignoring it
    for (offset = 0; offset <= strlen(s) && isspace((int)s[offset]); offset++);

    if (!offset)
        return 0;

    // Copy the rest of the string into buf
    for (size_t i = 0; s[offset]; offset++, i++)
        buf[i] = s[offset];

    if (strcmp(buf, s) == 0)
        return 0;

    // Overwrite s with buf
    memset(s, '\0', strlen(s));
    strcpy(s, buf);

    return offset;
}

ssize_t str_strip_trailing_whitespace(char *s)
{
    ssize_t offset = 0;

    while (strlen(s) && isspace((int)s[strlen(s) - 1])) {
        s[strlen(s) - 1] = '\0';
        offset--;
    }

    return offset;
}

void null_replace_newline(char *s)
{
    if (!*s)
        return;

    if (s[strlen(s) - 1] == '\n')
        s[strlen(s) - 1] = '\0';
}

void null_terminate_str(char *s)
{
    if (!*s)
        return;

    strncat(s, "\0", 1);
}
