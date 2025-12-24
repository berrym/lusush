#include "strings.h"

#include "errors.h"
#include "lusush.h"
#include "symtable.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Symbol table for strings
symtable_t *str_list = NULL;

// Dummy values for an empty string and a newline string
char *empty_str = "";
char *newline_str = "\n";

void init_str_symtable(void) { str_list = new_symtable(0); }

char *alloc_str(size_t len, bool exitflag) {
    char *s = NULL;

    s = calloc(len, sizeof(char));
    if (s == NULL) {
        if (exitflag) {
            error_syscall("error: `alloc_str`");
        } else {
            error_return("error: `alloc_str`");
            return NULL;
        }
    }

    return s;
}

void free_str(char *s) {
    if (s == NULL) {
        return;
    }

    free(s);
}

char *get_alloced_str_direct(char *s) {
    char *s2 = NULL;
    s2 = alloc_str(strlen(s) + 1, false);
    strcpy(s2, s);
    return s2;
}

char *get_alloced_str(char *s) {
    if (s == NULL) {
        return NULL;
    }

    if (!*s) {
        return empty_str;
    }

    if (*s == '\n' && s[1] == '\0') {
        return newline_str;
    }

    if (str_list) {
        symtable_entry_t *entry = get_symtable_entry(s);
        if (entry) {
            return entry->name;
        } else {
            entry = add_to_symtable(s);
            if (entry) {
                return entry->name;
            }
        }
    }

    return get_alloced_str_direct(s);
}

void free_alloced_str(char *s) {
    if (s == NULL || s == empty_str || s == newline_str) {
        return;
    }

    if (str_list) {
        symtable_entry_t *entry = get_symtable_entry(s);
        if (entry) {
            remove_from_symtable(str_list, entry);
        }

        return;
    }

    free_str(s);
}

bool strupper(char *s) {
    if (s == NULL) {
        return false;
    }

    while (*s) {
        *s = toupper(*s);
        s++;
    }

    return true;
}

bool strlower(char *s) {
    if (s == NULL) {
        return false;
    }

    while (*s) {
        *s = tolower(*s);
        s++;
    }

    return true;
}

char *str_strip_whitespace(char *s) {
    char *p = NULL, *t = NULL;

    for (p = s; isspace((int)*p); p++)
        ;

    if (*p == '\0') {
        return p;
    }

    t = p + strlen(p) - 1;
    while (t > p && isspace((int)*t)) {
        t--;
    }

    *++t = '\0';

    return p;
}

size_t str_skip_whitespace(char *s) {
    size_t offset = 0;
    char c;

    while (((c = *s) != EOF) && isspace((int)c)) {
        s++;
        offset++;
    }

    return offset;
}

size_t str_strip_leading_whitespace(char *s) {
    char buf[MAXLINE + 1] = {'\0'}; // buffer to store modified string
    size_t offset = 0;              // loop counter

    // Iterate over leading whitespace ignoring it
    for (offset = 0; offset <= strlen(s) && isspace((int)s[offset]); offset++)
        ;

    if (!offset) {
        return 0;
    }

    // Copy the rest of the string into buf
    for (size_t i = 0; s[offset]; offset++, i++) {
        buf[i] = s[offset];
    }

    if (strcmp(buf, s) == 0) {
        return 0;
    }

    // Overwrite s with buf
    memset(s, '\0', strlen(s));
    strcpy(s, buf);

    return offset;
}

ssize_t str_strip_trailing_whitespace(char *s) {
    ssize_t offset = 0;

    while (strlen(s) && isspace((int)s[strlen(s) - 1])) {
        s[strlen(s) - 1] = '\0';
        offset--;
    }

    return offset;
}

void null_replace_newline(char *s) {
    if (s == NULL || !*s) {
        return;
    }

    if (s[strlen(s) - 1] == '\n') {
        s[strlen(s) - 1] = '\0';
    }
}

void null_terminate_str(char *s) {
    if (!*s) {
        return;
    }

    strncat(s, "\0", 1);
}

// delete the character at the given index in the given str.
void delete_char_at(char *s, size_t index) {
    char *p1 = s + index;
    char *p2 = p1 + 1;
    while ((*p1++ = *p2++))
        ;
}

/**
 * strchr_any:
 *      Search string for any one of the passed characters.
 *      Returns a char pointer to the first occurence of any of the characters,
 *      NULL if none found.
 */
char *strchr_any(char *s, char *chars) {
    if (s == NULL || chars == NULL) {
        return NULL;
    }

    char *p = s;
    while (*p) {
        char *c = chars;
        while (*c) {
            if (*p == *c) {
                return p;
            }
            c++;
        }
        p++;
    }

    return NULL;
}

/**
 * find_opening_quote_type:
 *      Determine wether a quoted value starts with a single or double quote,
 *      return then char value found, or NUL byte.
 */
char find_opening_quote_type(char *s) {
    for (char *p = s; *p; p++) {
        if (*p == '\'' || *p == '"' || *p == '`') {
            return *p;
        }
    }

    return '\0';
}

/**
 * find_last_quote:
 *      Find the last closing quote that matches the opening quote, which is the
 *      first char of the data string.
 *      Returns the zero-based index of the closing quote. Return value of 0
 *      means we didn't find the closing quote, or the quotes were imbalanced.
 */
size_t find_last_quote(char *s) {
    // check the type of quote we have
    char quote = s[0];
    if (quote != '\'' && quote != '"' && quote != '`') {
        return 0;
    }

    // find the matching closing quote
    size_t i = 0, last = 0, count = 1, len = strlen(s);
    while (++i < len) {
        if (s[i] == quote) {
            if (s[i - 1] == '\\') {
                if (quote != '\'') {
                    continue;
                }
            }
            last = i;
            count++;
        }
    }

    // if quotes are balanced return the index of the last quote
    if ((count % 2) == 0) {
        return last;
    }

    return 0;
}

/**
 * find_closing_quote:
 *      Find the closing quote that matches the opening quote, which is the
 *      first char of the data string.
 *      Returns the zero-based index of the closing quote. Return value of 0
 *      means we didn't find the closing quote, or the quotes were imbalanced.
 */
size_t find_closing_quote(char *s) {
    // check the type of quote we have
    char quote = s[0];
    if (quote != '\'' && quote != '"' && quote != '`') {
        return 0;
    }
    // find the matching closing quote
    size_t i = 0, len = strlen(s);
    while (++i < len) {
        if (s[i] == '\\' && i + 1 < len) {
            // Skip escaped character (including escaped quotes)
            i++;
            continue;
        }
        if (s[i] == quote) {
            return i;
        }
    }
    return 0;
}

// find the closing brace that matches the opening brace, which is the first
// char of the data string.
// returns the zero-based index of the closing brace.. a return value of 0
// means we didn't find the closing brace.
size_t find_closing_brace(char *s) {
    // check the type of opening brace we have
    char opening_brace = s[0], closing_brace;
    if (opening_brace != '{' && opening_brace != '(') {
        return 0;
    }

    // determine the closing brace according to the opening brace
    if (opening_brace == '{') {
        closing_brace = '}';
    } else {
        closing_brace = ')';
    }

    // find the matching closing brace
    size_t ob_count = 1, cb_count = 0;
    size_t i = 0, len = strlen(s);
    while (++i < len) {
        if ((s[i] == '"') || (s[i] == '\'') || (s[i] == '`')) {
            // skip escaped quotes
            if (s[i - 1] == '\\') {
                continue;
            }
            // skip quoted substrings
            char quote = s[i];
            while (++i < len) {
                if (s[i] == quote && s[i - 1] != '\\') {
                    break;
                }
            }
            if (i == len) {
                return 0;
            }
            continue;
        }
        // keep the count of opening and closing braces
        if (s[i - 1] != '\\') {
            if (s[i] == opening_brace) {
                ob_count++;
            } else if (s[i] == closing_brace) {
                cb_count++;
            }
        }

        // break when we have a matching number of opening and closing braces
        if (ob_count == cb_count) {
            break;
        }
    }

    if (ob_count != cb_count) {
        return 0;
    }

    return i;
}

// return the passed string value, quoted in a format that can
// be used for reinput to the shell.
char *quote_val(char *val, bool add_quotes) {
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
        case '`':
        case '$':
        case '"':
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
    res = alloc_str(len + 1, false);
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
        case '`':
        case '$':
        case '"':
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
bool check_buffer_bounds(const size_t *count, size_t *len, char ***buf) {
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
            char **tmp = realloc(*buf, newlen * sizeof(char **));
            if (tmp == NULL) {
                return false;
            }
            *buf = tmp;
            *len = newlen;
        }
    }
    return true;
}

void free_argv(size_t argc, char **argv) {
    if (!argc) {
        return;
    }

    while (argc--) {
        free(argv[argc]);
    }

    free(argv);
}

/**
 * process_token_escapes:
 *      Process escape sequences in a token string (for double-quoted strings)
 */
char *process_token_escapes(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    size_t src_idx = 0, dst_idx = 0;

    // Skip opening quote if present
    if (str[0] == '"') {
        src_idx = 1;
        len--; // Adjust for skipped quote
    }

    while (src_idx < len && str[src_idx] != '"') {
        if (str[src_idx] == '\\' && src_idx + 1 < len) {
            char next = str[src_idx + 1];
            switch (next) {
            case 'n':
                result[dst_idx++] = '\n';
                src_idx += 2;
                break;
            case 't':
                result[dst_idx++] = '\t';
                src_idx += 2;
                break;
            case 'r':
                result[dst_idx++] = '\r';
                src_idx += 2;
                break;
            case 'b':
                result[dst_idx++] = '\b';
                src_idx += 2;
                break;
            case 'f':
                result[dst_idx++] = '\f';
                src_idx += 2;
                break;
            case 'a':
                result[dst_idx++] = '\a';
                src_idx += 2;
                break;
            case 'v':
                result[dst_idx++] = '\v';
                src_idx += 2;
                break;
            case '\\':
                result[dst_idx++] = '\\';
                src_idx += 2;
                break;
            case '"':
                result[dst_idx++] = '"';
                src_idx += 2;
                break;
            case '\'':
                result[dst_idx++] = '\'';
                src_idx += 2;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                // Octal escape sequence
                {
                    int octal_val = 0;
                    int octal_digits = 0;
                    src_idx++; // Skip backslash
                    while (src_idx < len && octal_digits < 3 &&
                           str[src_idx] >= '0' && str[src_idx] <= '7') {
                        octal_val = octal_val * 8 + (str[src_idx] - '0');
                        src_idx++;
                        octal_digits++;
                    }
                    result[dst_idx++] = (char)octal_val;
                }
                break;
            case 'x':
                // Hexadecimal escape sequence
                if (src_idx + 3 < len && isxdigit(str[src_idx + 2]) &&
                    isxdigit(str[src_idx + 3])) {
                    int hex_val = 0;
                    src_idx += 2; // Skip \x
                    for (int i = 0;
                         i < 2 && src_idx < len && isxdigit(str[src_idx]);
                         i++) {
                        char c = str[src_idx];
                        if (c >= '0' && c <= '9') {
                            hex_val = hex_val * 16 + (c - '0');
                        } else if (c >= 'a' && c <= 'f') {
                            hex_val = hex_val * 16 + (c - 'a' + 10);
                        } else if (c >= 'A' && c <= 'F') {
                            hex_val = hex_val * 16 + (c - 'A' + 10);
                        }
                        src_idx++;
                    }
                    result[dst_idx++] = (char)hex_val;
                } else {
                    // Invalid hex escape, keep literal
                    result[dst_idx++] = '\\';
                    result[dst_idx++] = 'x';
                    src_idx += 2;
                }
                break;
            default:
                // Unknown escape, keep literal
                result[dst_idx++] = '\\';
                result[dst_idx++] = next;
                src_idx += 2;
                break;
            }
        } else {
            result[dst_idx++] = str[src_idx++];
        }
    }

    result[dst_idx] = '\0';
    return result;
}
