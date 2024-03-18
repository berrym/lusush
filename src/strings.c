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

char *get_alloced_str_direct(char *s)
{
    char *s2 = NULL;
    s2 = alloc_str(strlen(s) + 1, false);
    strcpy(s2, s);
    return s2;
}

char *get_alloced_str(char *s)
{
    if (s == NULL)
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

    return get_alloced_str_direct(s);
}

void free_alloced_str(char *s)
{
    if (s == NULL || s == empty_str || s == newline_str)
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
    if (s == NULL) {
        if (exitflag) {
            error_syscall("error: lusush internal `alloc_str`");
        } else {
            error_return("error: lusush internal `alloc_str`");
            return NULL;
        }
    }

    return s;
}

void free_str(char *s)
{
    if (s == NULL)
        return;

    free(s);
    s = NULL;
}

bool strupper(char *s)
{
    if (s == NULL)
        return false;

    while (*s) {
        *s = toupper(*s);
        s++;
    }

    return true;
}

bool strlower(char *s)
{
    if (s == NULL)
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
    if (s == NULL || !*s)
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

// search string for any one of the passed characters.
// returns a char pointer to the first occurence of any of the characters,
// NULL if none found.
char *strchr_any(char *string, char *chars)
{
    if (string == NULL || chars == NULL) {
        return NULL;
    }

    char *s = string;
    while (*s) {
        char *c = chars;
        while (*c) {
            if (*s == *c) {
                return s;
            }
            c++;
        }
        s++;
    }

    return NULL;
}


// return the passed string value, quoted in a format that can
// be used for reinput to the shell.
char *quote_val(char *val, bool add_quotes)
{
    char *res = NULL;
    size_t len;
    // empty string
    if (val == NULL || !*val) {
        len = add_quotes ? 3 : 1;
        res = calloc(len, sizeof(char));
        if (res == NULL) {
            return NULL;
        }
        strcpy(res, add_quotes ? "\"\"" : "");
        return res;
    }

    // count the number of quotes needed
    len = 0;
    char *v = val, *p;
    while (*v) {
        switch (*v) {
        case '\\':
        case  '`':
        case  '$':
        case  '"':
            len++;
            break;
        default:
            break;
        }
        v++;
    }

    len += strlen(val);

    // add two for the opening and closing quotes
    if (add_quotes) {
        len += 2;
    }
    // alloc memory for quoted string
    res = calloc(len + 1, sizeof(char));
    if (res == NULL) {
        return NULL;
    }

    p = res;

    // add opening quote (optional)
    if (add_quotes) {
        *p++ = '"';
    }

    // copy quoted val
    v = val;
    while (*v) {
        switch (*v) {
        case '\\':
        case  '`':
        case  '$':
        case  '"':
            // add '\' for quoting
            *p++ = '\\';
            // copy char
            *p++ = *v++;
            break;
        default:
            // copy next char
            *p++ = *v++;
            break;
        }
    }

    // add closing quote (optional)
    if (add_quotes) {
        *p++ = '"';
    }
    *p = '\0';
    return res;
}


// alloc memory for, or extend the host (or user) names buffer if needed..
// in the first call, the buffer is initialized to 32 entries.. subsequent
// calls result in the buffer size doubling, so that it becomes 64, 128, ...
// count is the number of used entries in the buffer, while len is the number
// of alloc'd entries (size of buffer divided by sizeof(char **)).
// returns true if the buffer is alloc'd/extended, false otherwise.
bool check_buffer_bounds(const size_t *count, size_t *len, char ***buf)
{
    if (*count >= *len) {
        if ((*buf) == NULL) {
            // first call. alloc memory for the buffer
            *buf = calloc(32, sizeof(char **));
            if ((*buf) == NULL) {
                return false;
            }
            *len = 32;
        } else {
            // subsequent calls. extend the buffer
            const size_t newlen = (*len) * 2;
            char **hn2 = realloc(*buf, newlen * sizeof(char **));
            if (hn2 == NULL) {
                return false;
            }
            *buf = hn2;
            *len = newlen;
        }
    }
    return true;
}


// free the memory used to store the strings list pointed to by buf.
void free_buffer(size_t len, char **buf)
{
    if (!len) {
        return;
    }
    
    while (len--) {
        free(buf[len]);
        buf[len] = NULL;
    }

    free(buf);
    buf = NULL;
}
