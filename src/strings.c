#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "errors.h"
#include "lusush.h"
#include "strings.h"
#include "symtable.h"

char *alloc_string(size_t len, bool exitflag)
{
    char *s = NULL;

    if ((s = calloc(len + 1, sizeof(char))) == NULL) {
        if (exitflag)
            error_syscall("alloc_string");
        else
            error_return("alloc_string");
    }

    return s;
}

size_t strip_leading_whspc(char *s)
{
    char buf[MAXLINE] = { '\0' }; // buffer to store modified string
    size_t k, l;                  // loop counters

    // Iterate over leading whitespace ignoring it
    for (k = 0; k <= MAXLINE && isspace((int)s[k]); k++);

    if (!k)
        return 0;

    // Copy the rest of the string into buf
    for (l = 0; s[k]; k++, l++)
        buf[l] = s[k];

    // If buf differs from s overwrite s with buf
    if (strncmp(buf, s, MAXLINE) == 0)
        return 0;

    memset(s, '\0', strnlen(s, MAXLINE));
    strncpy(s, buf, strnlen(buf, MAXLINE));

    return k;
}

size_t strip_trailing_whspc(char *s)
{
    size_t i = 0;
    while (strnlen(s, MAXLINE) && isspace((int)s[strnlen(s, MAXLINE) - 1])) {
        s[strnlen(s, MAXLINE) - 1] = '\0';
        i++;
    }
    return i;
}

void null_replace_newline(char *s)
{
    if (s[strnlen(s, MAXLINE) - 1] == '\n')
        s[strnlen(s, MAXLINE) - 1] = '\0';
}

struct symtable *str_list = NULL;

// dummy value for an empty string and a newline string
char *empty_str = "";
char *newline_str = "\n";

void init_str_symtable(void)
{
    str_list = new_symtable(0);
}

char *__get_malloced_str(char *str)
{
    char *str2 = NULL;
    str2 = alloc_string(strlen(str) + 1, false);
    strncpy(str2, str, strnlen(str, MAXLINE));
    return str2;
}

char *get_malloced_str(char *str)
{
    if (!str)
        return NULL;

    if (!*str)
        return empty_str;

    if (*str == '\n' && str[1] == '\0')
        return newline_str;
    
    if (str_list) {
        struct symtable_entry *entry = get_symtable_entry(str);
        if (entry) {
            return entry->name;
        } else {
            entry = add_to_symtable(str);
            if (entry)
                return entry->name;
        }
    }

    return __get_malloced_str(str);
}
