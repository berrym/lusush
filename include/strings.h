#include <stdbool.h>

#ifndef STRINGS_H
#define STRINGS_H

char *alloc_string(size_t, bool);
size_t strip_leading_whspc(char *);
size_t strip_trailing_whspc(char *);
void null_replace_newline(char *);

#endif
