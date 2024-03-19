#include "../include/errors.h"
#include "../include/exec.h"
#include "../include/lusush.h"
#include "../include/strings.h"
#include "../include/symtable.h"
#include <ctype.h>
#include <errno.h>
#include <errors.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// special value to represent an invalid variable
#define INVALID_VAR ((char *)-1)

// convert the string *word to a cmd_token struct, so it can be passed to
// functions such as word_expand().
// returns the malloc'd cmd_token struct, or NULL if insufficient memory.
struct word_s *make_word(char *str) {
    // alloc struct memory
    struct word_s *word = calloc(1, sizeof(struct word_s));
    if (word == NULL) {
        return NULL;
    }

    // alloc string memory
    const size_t len = strlen(str);
    char *data = calloc(len + 1, sizeof(char));
    if (data == NULL) {
        error_return("error: `make_word`");
        free(word);
        return NULL;
    }

    // copy string
    strcpy(data, str);
    word->data = data;
    word->len = len;
    word->next = NULL;

    // return struct
    return word;
}

// free the memory used by a list of words.
void free_all_words(struct word_s *first) {
    while (first) {
        struct word_s *del = first;
        first = first->next;

        if (del->data) {
            // free the word text
            free(del->data);
        }
        // free the word
        free(del);
    }
}

// convert a tree of tokens into a command string (i.e. re-create the original
// command line from the token tree.
// returns the malloc'd command string, or NULL if there is an error.
char *wordlist_to_str(struct word_s *word) {
    if (word == NULL) {
        return NULL;
    }
    size_t len = 0;
    const struct word_s *w = word;
    while (w) {
        len += w->len + 1;
        w = w->next;
    }
    char *str = calloc(len + 1, sizeof(char));
    if (str == NULL) {
        error_return("error: `wordlist_to_str`");
        return NULL;
    }
    char *str2 = str;
    w = word;
    while (w) {
        sprintf(str2, "%s ", w->data);
        str2 += w->len + 1;
        w = w->next;
    }
    // remove the last separator
    str2[-1] = '\0';
    return str;
}

// delete the character at the given index in the given str.
void delete_char_at(char *str, size_t index) {
    char *p1 = str + index;
    char *p2 = p1 + 1;
    while ((*p1++ = *p2++)) {
        ;
    }
}

// check if the given str is a valid name.. POSIX says a names can consist of
// alphanumeric chars and underscores, and start with an alphabetic char or
// underscore. returns true if str is a valid name, false otherwise.
bool is_name(const char *str) {
    // names start with alpha char or an underscore...
    if (!isalpha(*str) && *str != '_') {
        return false;
    }
    // ...and contain alphanumeric chars and/or underscores
    while (*++str) {
        if (!isalnum(*str) && *str != '_') {
            return false;
        }
    }
    return true;
}

// find the closing quote that matches the opening quote, which is the first
// char of the data string.
// sq_nesting is a flag telling us if we should allow single quote nesting
// (prohibited by POSIX, but allowed in ANSI-C strings).
// returns the zero-based index of the closing quote.. a return value of 0
// means we didn't find the closing quote.
size_t find_closing_quote(char *data) {
    // check the type of quote we have
    char quote = data[0];
    if (quote != '\'' && quote != '"' && quote != '`') {
        return 0;
    }
    // find the matching closing quote
    size_t i = 0, len = strlen(data);
    while (++i < len) {
        if (data[i] == quote) {
            if (data[i - 1] == '\\') {
                if (quote != '\'') {
                    continue;
                }
            }
            return i;
        }
    }
    return 0;
}

// find the closing brace that matches the opening brace, which is the first
// char of the data string.
// returns the zero-based index of the closing brace.. a return value of 0
// means we didn't find the closing brace.
size_t find_closing_brace(char *data) {
    // check the type of opening brace we have
    char opening_brace = data[0], closing_brace;
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
    size_t i = 0, len = strlen(data);
    while (++i < len) {
        if ((data[i] == '"') || (data[i] == '\'') || (data[i] == '`')) {
            // skip escaped quotes
            if (data[i - 1] == '\\') {
                continue;
            }
            // skip quoted substrings
            char quote = data[i];
            while (++i < len) {
                if (data[i] == quote && data[i - 1] != '\\') {
                    break;
                }
            }
            if (i == len) {
                return 0;
            }
            continue;
        }
        // keep the count of opening and closing braces
        if (data[i - 1] != '\\') {
            if (data[i] == opening_brace) {
                ob_count++;
            } else if (data[i] == closing_brace) {
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

// substitute the substring of s1, from character start to character end,
// with the s2 string.
// start should point to the first char to be deleted from s1.
// end should point to the last char to be deleted from s, NOT the
// char coming after it.
// returns the malloc'd new string, or NULL on error.
char *substitute_str(char *s1, char *s2, size_t start, size_t end) {
    if (no_word_expand) {
        return s1;
    }

    // get the prefix (the part before start)
    char before[start + 1];
    strncpy(before, s1, start);
    before[start] = '\0';
    // get the postfix (the part after end)
    const size_t afterlen = strlen(s1) - end + 1;
    char after[afterlen];
    strcpy(after, s1 + end + 1);
    // alloc memory for the new string
    const size_t totallen = start + afterlen + strlen(s2);
    char *final = calloc(totallen + 1, sizeof(char));
    if (final == NULL) {
        error_return("error: `substitute_str`");
        return NULL;
    }
    if (!totallen) { // empty string
        final[0] = '\0';
    } else { // concatenate the three parts into one string
        strcpy(final, before);
        strcat(final, s2);
        strcat(final, after);
    }
    // return the new string
    return final;
}

int substitute_word(char **pstart, char **p, size_t len, char *(func)(char *),
                    bool add_quotes) {
    if (no_word_expand) {
        return 0;
    }

    // extract the word to be substituted
    char *tmp = calloc(len + 1, sizeof(char));
    if (tmp == NULL) {
        (*p) += len;
        return 0;
    }
    strncpy(tmp, *p, len);
    tmp[len--] = '\0';

    // and expand it
    char *tmp2;
    if (func) {
        tmp2 = func(tmp);
        if (tmp2 == INVALID_VAR) {
            tmp2 = NULL;
        }
        if (tmp2) {
            free(tmp);
        }
    } else {
        tmp2 = tmp;
    }

    // error expanding the string. keep the original string as-is
    if (tmp2 == NULL) {
        (*p) += len;
        free(tmp);
        return 0;
    }

    // save our current position in the word
    size_t i = (*p) - (*pstart);

    // substitute the expanded word
    tmp = quote_val(tmp2, add_quotes);
    free(tmp2);
    if (tmp) {
        // substitute the expanded word
        if ((tmp2 = substitute_str(*pstart, tmp, i, i + len))) {
            // adjust our pointer to point to the new string
            free(*pstart);
            (*pstart) = tmp2;
            len = strlen(tmp);
        }
        free(tmp);
    }

    // adjust our pointer to point to the new string
    (*p) = (*pstart) + i + len - 1;
    return 1;
}

// perform word expansion on a single word, pointed to by orig_word.
// returns the head of the linked list of the expanded fields and stores the
// last field in the tail pointer.
struct word_s *word_expand(char *orig_word) {
    if (orig_word == NULL) {
        return NULL;
    }

    if (!*orig_word) {
        return make_word(orig_word);
    }

    char *pstart = calloc(strlen(orig_word) + 1, sizeof(char));
    if (pstart == NULL) {
        return NULL;
    }
    strcpy(pstart, orig_word);

    char *p = pstart, *p2;
    char *tmp;
    char c;
    size_t i = 0;
    size_t len;
    bool in_double_quotes = false;
    bool in_var_assign = false;
    int var_assign_eq = 0;
    bool expanded = false;
    char *(*func)(char *);

    do {
        switch (*p) {
        case '~':
            if (no_word_expand) {
                break;
            }
            // don't perform tilde expansion inside double quotes
            if (in_double_quotes) {
                break;
            }
            // expand a tilde prefix only if:
            //- it is the first unquoted char in the string.
            //- it is part of a variable assignment, and is preceded by the
            // first
            //  equals sign or a colon.
            if (p == pstart ||
                (in_var_assign &&
                 (p[-1] == ':' || (p[-1] == '=' && var_assign_eq == true)))) {
                // find the end of the tilde prefix
                bool tilde_quoted = false;
                bool endme = false;
                p2 = p + 1;

                while (*p2) {
                    switch (*p2) {
                    case '\\':
                        tilde_quoted = true;
                        p2++;
                        break;
                    case '"':
                    case '\'':
                        i = find_closing_quote(p2);
                        if (i) {
                            tilde_quoted = true;
                            p2 += i;
                        }
                        break;
                    case '/':
                        endme = true;
                        break;
                    case ':':
                        if (in_var_assign) {
                            endme = true;
                        }
                        break;
                    }
                    if (endme) {
                        break;
                    }
                    p2++;
                }

                // if any part of the prefix is quoted, no expansion is done
                if (tilde_quoted) {
                    // just skip the tilde prefix
                    p = p2;
                    break;
                }

                // otherwise, extract the prefix
                len = p2 - p;
                substitute_word(&pstart, &p, len, tilde_expand,
                                !in_double_quotes);
                expanded = true;
            }
            break;
        case '"':
            if (no_word_expand) {
                break;
            }
            // toggle quote mode
            in_double_quotes = !in_double_quotes;
            break;
        case '=':
            if (no_word_expand) {
                break;
            }
            // skip it if inside double quotes
            if (in_double_quotes) {
                break;
            }
            // check the previous string is a valid var name
            len = p - pstart;
            tmp = calloc(len + 1, sizeof(char));
            if (tmp == NULL) {
                error_return("error: `word_expand`");
                break;
            }

            strncpy(tmp, pstart, len);
            tmp[len] = '\0';

            // if the string before '=' is a valid var name, we have a variable
            // assignment.. we set in_var_assign to indicate that, and we set
            // var_assign_eq which indicates this is the first equals sign (we
            // use this when performing tilde expansion -- see code above).
            if (is_name(tmp)) {
                in_var_assign = true;
                var_assign_eq++;
            }
            free_str(tmp);
            break;
        case '\\':
            if (no_word_expand) {
                break;
            }
            // skip backslash (we'll remove it later on)
            p++;
            break;
        case '\'':
            if (no_word_expand) {
                break;
            }

            // if inside double quotes, treat the single quote as a normal char
            if (in_double_quotes) {
                break;
            }
            // skip everything, up to the closing single quote
            p += find_closing_quote(p);
            break;
        case '`':
            if (no_word_expand) {
                break;
            }

            // find the closing back quote
            if ((len = find_closing_quote(p)) == 0) {
                // not found. bail out
                break;
            }
            // otherwise, extract the command and substitute its output//
            substitute_word(&pstart, &p, len + 1, command_substitute, 0);
            expanded = true;
            break;
        // the $ sign might introduce:
        // - parameter expansions: ${var} or $var
        // - command substitutions: $()
        // - arithmetic expansions: $(())
        case '$':
            if (no_word_expand) {
                break;
            }

            c = p[1];
            switch (c) {
            case '{':
                // find the closing quote
                if ((len = find_closing_brace(p + 1)) == 0) {
                    // not found. bail out
                    break;
                }
                // calling var_expand() might return an INVALID_VAR result which
                // makes the following call fail.
                if (!substitute_word(&pstart, &p, len + 2, var_expand, 0)) {
                    free(pstart);
                    return NULL;
                }
                expanded = true;
                break;
            // arithmetic expansion $(()) or command substitution $().
            case '(':
                if (no_word_expand) {
                    break;
                }

                // check if we have one or two opening braces
                i = 0;
                if (p[2] == '(') {
                    i++;
                }
                // find the closing quote
                if ((len = find_closing_brace(p + 1)) == 0) {
                    // not found. bail out
                    break;
                }
                // otherwise, extract the expression and substitute its value.
                // if we have one brace (i == 0), we'll perform command
                // substitution. otherwise, arithmetic expansion.
                func = i ? arithm_expand : command_substitute;
                substitute_word(&pstart, &p, len + 2, func, 0);
                expanded = true;
                break;
            default:
                if (no_word_expand) {
                    break;
                }

                // var names must start with an alphabetic char or _
                if (!isalpha(p[1]) && p[1] != '_') {
                    break;
                }
                p2 = p + 1;
                // get the end of the var name
                while (*p2) {
                    if (!isalnum(*p2) && *p2 != '_') {
                        break;
                    }
                    p2++;
                }
                // empty name
                if (p2 == p + 1) {
                    break;
                }
                // perform variable expansion
                substitute_word(&pstart, &p, p2 - p, var_expand, 0);
                expanded = true;
                break;
            }
            break;
        default:
            if (no_word_expand) {
                break;
            }

            if (isspace(*p) && !in_double_quotes) {
                expanded = true;
            }
            break;
        }
    } while (*(++p));

    // if we performed word expansion, do field splitting
    struct word_s *words = NULL;
    if (expanded) {
        words = field_split(pstart);
    }
    // no expansion done, or no field splitting done
    if (words == NULL) {
        words = make_word(pstart);
        // error making word struct
        if (words == NULL) {
            error_message("error: `word_expand`: insufficient memory");
            free(pstart);
            return NULL;
        }
    }
    free(pstart);

    // perform pathname expansion and quote removal
    if (!no_word_expand) {
        words = pathnames_expand(words);
        remove_quotes(words);
    }

    // return the expanded list
    return words;
}

// perform tilde expansion.
// returns the malloc'd expansion of the tilde prefix, NULL if expansion failed.
char *tilde_expand(char *s) {
    if (no_word_expand) {
        return s;
    }

    char *home = NULL;
    const size_t len = strlen(s);
    char *s2 = NULL;

    // null tilde prefix. substitute with the value of home
    if (len == 1) {
        const symtable_entry_s *entry = get_symtable_entry("HOME");
        if (entry && entry->val) {
            home = entry->val;
        } else {
            // POSIX doesn't say what to do if $HOME is null/unset.. we follow
            // what bash does, which is searching our home directory in the
            // password database.
            const struct passwd *pass = getpwuid(getuid());
            if (pass) {
                // get the value of home
                home = pass->pw_dir;
            }
        }
    } else {
        // we have a login name
        const struct passwd *pass = getpwnam(s + 1);
        if (pass) {
            home = pass->pw_dir;
        }
    }

    // we have a NULL value
    if (home == NULL) {
        return NULL;
    }

    // return the home dir we've found
    s2 = calloc(strlen(home) + 1, sizeof(char));
    if (s2 == NULL) {
        return NULL;
    }
    strcpy(s2, home);
    return s2;
}

// perform variable (parameter) expansion.
// our options are:
// syntax           POSIX description   var defined     var undefined
// ======           =================   ===========     =============
// $var             Substitute          var             nothing
// ${var}           Substitute          var             nothing
// ${var:-thing}    Use Deflt Values    var             thing (var unchanged)
// ${var:=thing}    Assgn Deflt Values  var             thing (var set to thing)
// ${var:?message}  Error if NULL/Unset var             print message and exit
// shell,
//                                                      (if message is empty,
//                                                      print "var: parameter
//                                                      not set")
// ${var:+thing}    Use Alt. Value      thing           nothing
// ${#var}          Calculate String Length
// Using the same options in the table above, but without the colon, results in
// a test for a parameter that is unset. using the colon results in a test for a
// parameter that is unset or null.

// TODO: we should test our implementation of the following string processing
//      functions (see section 2.6.2 - Parameter Expansion in POSIX):
//      ${parameter%[word]}      Remove Smallest Suffix Pattern
//      ${parameter%%[word]}     Remove Largest Suffix Pattern
//      ${parameter#[word]}      Remove Smallest Prefix Pattern
//      ${parameter##[word]}     Remove Largest Prefix Pattern

// perform variable (parameter) expansion.
// returns an malloc'd string of the expanded variable value, or NULL if the
// variable is not defined or the expansion failed.
// this function should not be called directly by any function outside of this
// module (hence the double underscores that prefix the function name).
char *var_expand(char *orig_var_name) {
    if (no_word_expand) {
        return orig_var_name;
    }

    // sanity check
    if (orig_var_name == NULL) {
        return NULL;
    }

    // if the var substitution is in the $var format, remove the $.
    // if it's in the ${var} format, remove the ${}.
    // skip the $
    orig_var_name++;
    size_t len = strlen(orig_var_name);
    if (*orig_var_name == '{') {
        // remove the }
        orig_var_name[len - 1] = '\0';
        orig_var_name++;
    }

    // check we don't have an empty varname
    if (!*orig_var_name) {
        return NULL;
    }

    bool get_length = false;
    // if varname starts with #, we need to get the string length
    if (*orig_var_name == '#') {
        // use of '#' should come with omission of ':'
        if (strchr(orig_var_name, ':')) {
            error_message("error: `var_expand`: invalid "
                          "variable substitution: %s",
                          orig_var_name);
            return INVALID_VAR;
        }
        get_length = true;
        orig_var_name++;
    }

    // check we don't have an empty varname
    if (!*orig_var_name) {
        return NULL;
    }

    // search for a colon, which we use to separate the variable name from the
    // value or substitution we are going to perform on the variable.
    char *sub = strchr(orig_var_name, ':');
    if (sub == NULL) { // we have a substitution without a colon
        // search for the char that indicates what type of substitution we need
        // to do
        sub = strchr_any(orig_var_name, "-=?+%#");
    }

    // get the length of the variable name (without the substitution part)
    len = sub ? (size_t)(sub - orig_var_name) : strlen(orig_var_name);

    // if we have a colon+substitution, skip the colon
    if (sub && *sub == ':') {
        sub++;
    }

    // copy the varname to a buffer
    char var_name[len + 1];
    strncpy(var_name, orig_var_name, len);
    var_name[len] = '\0';

    // commence variable substitution.
    char *empty_val = "";
    char *tmp = NULL;
    bool setme = false;

    symtable_entry_s *entry = get_symtable_entry(var_name);
    tmp = (entry && entry->val && entry->val[0]) ? entry->val : empty_val;

    // first case: variable is unset or empty.
    if (tmp == NULL || tmp == empty_val) {
        // do we have a substitution clause?
        if (sub && *sub) {
            // check the substitution operation we need to perform
            switch (sub[0]) {
            case '-': // use default value
                tmp = sub + 1;
                break;
            case '=': // assign the variable a value
                // NOTE: only variables, not positional or special parameters
                // can be assigned this way (we'll fix this later).
                tmp = sub + 1;
                // assign the EXPANSION OF tmp, not tmp
                // itself, to var_name (we'll set the value below).
                setme = true;
                break;
            case '?': // print error msg if variable is null/unset
                if (sub[1] == '\0') {
                    error_message("error: `var_expand`: %s: "
                                  "parameter not set",
                                  var_name);
                } else {
                    error_message("error: `var_expand` failed: %s: %s\n",
                                  var_name, sub + 1);
                }
                return INVALID_VAR;
            // use alternative value (we don't have alt. value here)//
            case '+':
                return NULL;
            // pattern matching notation. can't match anything
            // if the variable is not defined, now can we?
            case '#':
            case '%':
                break;
            default: // unknown operator
                return INVALID_VAR;
            }
        }
        // no substitution clause. return NULL as the variable is unset/null
        else {
            tmp = empty_val;
        }
    }
    // second case: variable is set/not empty.
    else {
        // do we have a substitution clause?
        if (sub && *sub) {
            // check the substitution operation we need to perform
            switch (sub[0]) {
            case '-': // use default value
                break;
            case '=': // assign the variable a value
                tmp = sub + 1;
                setme = true;
                break;
            case '?': // print error msg if variable is null/unset
                break;
            // use alternative value
            case '+':
                tmp = sub + 1;
                break;
            // for the prefix and suffix matching routines (below).
            // bash expands the pattern part, but ksh doesn't seem to do
            // the same (as far as the manpage is concerned). we follow ksh.
            case '%': // match suffix
                sub++;
                // perform word expansion on the value
                char *p = word_expand_to_str(tmp);
                // word expansion failed
                if (p == NULL) {
                    return INVALID_VAR;
                }
                bool longest = false;
                // match the longest or shortest suffix
                if (*sub == '%') {
                    longest = true, sub++;
                }
                // perform the match
                if ((len = match_suffix(sub, p, longest)) == 0) {
                    return p;
                }
                // return the match
                char *p2 = calloc(len + 1, sizeof(char));
                if (p2) {
                    strncpy(p2, p, len);
                    p2[len] = '\0';
                }
                free(p);
                return p2;
            case '#': // match prefix
                sub++;
                // perform word expansion on the value
                p = word_expand_to_str(tmp);
                // word expansion failed
                if (p == NULL) {
                    return INVALID_VAR;
                }
                longest = false;
                // match the longest or shortest suffix
                if (*sub == '#') {
                    longest = true, sub++;
                }
                // perform the match
                if ((len = match_prefix(sub, p, longest)) == 0) {
                    return p;
                }
                // return the match
                p2 = calloc(strlen(p) - len + 1, sizeof(char));
                if (p2) {
                    strcpy(p2, p + len);
                }
                free(p);
                return p2;
            default: // unknown operator
                return INVALID_VAR;
            }
        }
        // no substitution clause. return the variable's original value
    }

    // we have substituted the variable's value. now go POSIX style on it.
    bool expanded = false;
    if (tmp) {
        if ((tmp = word_expand_to_str(tmp))) {
            expanded = true;
        }
    }

    // do we need to set new value to the variable?
    if (setme) {
        // if variable not defined, add it now
        if (entry == NULL) {
            entry = add_to_symtable(var_name);
        }
        // and set its value
        if (entry) {
            symtable_entry_setval(entry, tmp);
        }
    }

    char *p = NULL;
    if (get_length) {
        char buf[32];
        if (tmp == NULL) {
            sprintf(buf, "0");
        } else {
            sprintf(buf, "%lu", strlen(tmp));
        }
        // get a copy of the buffer
        p = calloc(strlen(buf) + 1, sizeof(char));
        if (p) {
            strcpy(p, buf);
        }
    } else {
        // "normal" variable value
        p = calloc(strlen(tmp) + 1, sizeof(char));
        if (p) {
            strcpy(p, tmp);
        }
    }

    // free the expanded word list
    if (expanded) {
        free(tmp);
    }

    // return the result
    return p ? p : INVALID_VAR;
}

// perform command substitutions.
// the backquoted flag tells if we are called from a backquoted command
// substitution:
//   `command`
// or a regular one:
//   $(command)
char *command_substitute(char *orig_cmd) {
    if (no_word_expand) {
        return orig_cmd;
    }

    if (orig_cmd == NULL) {
        return NULL;
    }

    char b[1024];
    size_t bufsize = 0;
    char *buf = NULL;
    char *p = NULL;
    size_t i = 0;
    const bool backquoted = (*orig_cmd == '`') ? true : false;

    // fix cmd in the backquoted version.. we skip the first char (if using the
    // old, backquoted version), or the first two chars (if using the POSIX
    // version).
    char *cmd = calloc(strlen(orig_cmd) + 1, sizeof(char));
    if (cmd == NULL) {
        error_return("error: `command_substitute`");
        return NULL;
    }

    strcpy(cmd, orig_cmd + (backquoted ? 1 : 2));

    char *cmd2 = cmd;
    const size_t cmdlen = strlen(cmd);

    if (backquoted) {
        // remove the last back quote
        if (cmd[cmdlen - 1] == '`') {
            cmd[cmdlen - 1] = '\0';
        }

        // fix the backslash-escaped chars
        char *p1 = cmd;

        do {
            if (*p1 == '\\' &&
                (p1[1] == '$' || p1[1] == '`' || p1[1] == '\\')) {
                char *p2 = p1, *p3 = p1 + 1;
                while ((*p2++ = *p3++)) {
                    ;
                }
            }
        } while (*(++p1));
    } else {
        // remove the last closing brace
        if (cmd[cmdlen - 1] == ')') {
            cmd[cmdlen - 1] = '\0';
        }
    }

    FILE *fp = popen(cmd2, "r");
    if (fp == NULL) {
        error_message("error: `command_substitute`: failed to open pipe");
        free(cmd2);
        return NULL;
    }

    // read the command output
    while ((i = fread(b, 1, 1024, fp))) {
        // first time. alloc buffer
        if (buf == NULL) {
            // add 1 for the null terminating byte
            buf = calloc(i + 1, sizeof(char));
            if (buf == NULL) {
                goto fin;
            }
            p = buf;
        }
        // extend buffer
        else {
            char *buf2 = realloc(buf, bufsize + i + 1);
            if (buf2 == NULL) {
                free(buf);
                buf = NULL;
                goto fin;
            }
            buf = buf2;
            p = buf + bufsize;
        }

        bufsize += i;

        // copy the input and add the null terminating byte
        memcpy(p, b, i);
        p[i] = '\0';
    }

    if (!bufsize) {
        free(cmd2);
        return NULL;
    }

    // now remove any trailing newlines
    i = bufsize - 1;

    if (buf != NULL) {
        while (buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = '\0';
            i--;
        }
    }

fin:
    // close the pipe
    pclose(fp);

    // free used memory
    free(cmd2);

    if (buf == NULL) {
        error_message("error: `command_substitute`: "
                      "insufficient memory to perform command substitution");
    }

    return buf;
}

// check if char c is a valid $IFS character.
// returns true if char c is an $IFS character, false otherwise.
static bool is_IFS_char(const char c, const char *IFS) {
    if (!*IFS) {
        return false;
    }

    do {
        if (c == *IFS) {
            return true;
        }
    } while (*++IFS);

    return false;
}

// skip all whitespace characters that are part of $IFS.
void skip_IFS_whitespace(char **str, char *IFS) {
    char *IFS2 = IFS;
    char *s2 = *str;

    do {
        if (*s2 == *IFS2) {
            s2++;
            IFS2 = IFS - 1;
        }
    } while (*++IFS2);

    *str = s2;
}

// skip $IFS delimiters, which can be whitespace characters as well as other
// chars.
void skip_IFS_delim(char *str, char *IFS_space, char *IFS_delim, size_t *_i,
                    size_t len) {
    size_t i = *_i;

    while ((i < len) && is_IFS_char(str[i], IFS_space)) {
        i++;
    }

    while ((i < len) && is_IFS_char(str[i], IFS_delim)) {
        i++;
    }

    while ((i < len) && is_IFS_char(str[i], IFS_space)) {
        i++;
    }

    *_i = i;
}

// convert the words resulting from a word expansion into separate fields.
// returns a pointer to the first field, NULL if no field splitting was done.
struct word_s *field_split(char *str) {
    const symtable_entry_s *entry = get_symtable_entry("IFS");
    char *IFS = entry ? entry->val : NULL;
    char *p;

    // POSIX says no IFS means: "space/tab/NL"
    if (IFS == NULL) {
        IFS = " \t\n";
    }

    // POSIX says empty IFS means no field splitting
    if (IFS[0] == '\0') {
        return NULL;
    }

    // get the IFS spaces and delimiters separately
    char IFS_space[64];
    char IFS_delim[64];

    if (strcmp(IFS, " \t\n") == 0) { // "standard" IFS
        IFS_space[0] = ' ';
        IFS_space[1] = '\t';
        IFS_space[2] = '\n';
        IFS_space[3] = '\0';
        IFS_delim[0] = '\0';
    } else { // "custom" IFS
        p = IFS;
        char *sp = IFS_space;
        char *dp = IFS_delim;

        do {
            if (isspace(*p)) {
                *sp++ = *p++;
            } else {
                *dp++ = *p++;
            }
        } while (*p);

        *sp = '\0';
        *dp = '\0';
    }

    const size_t len = strlen(str);
    size_t i = 0, j = 0, k;
    int fields = 1;
    char quote = 0;

    // skip any leading whitespaces in the string
    skip_IFS_whitespace(&str, IFS_space);

    // estimate the needed number of fields
    do {
        switch (str[i]) {
        // skip escaped chars
        case '\\':
            // backslash has no effect inside single quotes
            if (quote != '\'') {
                i++;
            }
            break;
        // don't count whitespaces inside quotes
        case '\'':
        case '"':
        case '`':
            if (quote == str[i]) {
                quote = 0;
            } else {
                quote = str[i];
            }
            break;
        default:
            // skip normal characters if we're inside quotes
            if (quote) {
                break;
            }

            if (is_IFS_char(str[i], IFS_space) ||
                is_IFS_char(str[i], IFS_delim)) {
                skip_IFS_delim(str, IFS_space, IFS_delim, &i, len);
                if (i < len) {
                    fields++;
                }
            }
            break;
        }
    } while (++i < len);

    // we have only one field. no field splitting needed
    if (fields == 1) {
        return NULL;
    }

    struct word_s *first_field = NULL;
    struct word_s *cur = NULL;

    // create the fields
    i = 0;
    j = 0;
    quote = 0;

    do {
        switch (str[i]) {
        // skip escaped chars
        case '\\':
            // backslash has no effect inside single quotes
            if (quote != '\'') {
                i++;
            }
            break;
        // skip single quoted substrings
        case '\'':
            p = str + i + 1;
            while (*p && *p != '\'') {
                p++;
            }
            i = p - str;
            break;
        // remember if we're inside/outside double and back quotes
        case '"':
        case '`':
            if (quote == str[i]) {
                quote = 0;
            } else {
                quote = str[i];
            }
            break;
        default:
            // skip normal characters if we're inside quotes
            if (quote) {
                break;
            }

            // delimit the field if we have an IFS space or delimiter char, or
            // if we reached the end of the input string.
            if (is_IFS_char(str[i], IFS_space) ||
                is_IFS_char(str[i], IFS_delim) || (i == len)) {
                // copy the field text
                char *tmp = calloc(i - j + 1, sizeof(char));
                if (tmp == NULL) {
                    error_return("error: `field_split`");
                    return first_field;
                }

                strncpy(tmp, str + j, i - j);
                tmp[i - j] = '\0';

                // create a new struct for the field
                struct word_s *fld = calloc(1, sizeof(struct word_s));
                if (fld == NULL) {
                    free(tmp);
                    return first_field;
                }

                fld->data = tmp;
                fld->len = i - j;
                fld->next = NULL;

                if (first_field == NULL) {
                    first_field = fld;
                }

                if (cur == NULL) {
                    cur = fld;
                } else {
                    cur->next = fld;
                    cur = fld;
                }

                k = i;

                // skip trailing IFS spaces/delimiters
                skip_IFS_delim(str, IFS_space, IFS_delim, &i, len);
                j = i;

                if (i != k && i < len) {
                    i--; // go back one step so the loop will work correctly
                }
            }
            break;
        }
    } while (++i <= len);

    return first_field;
}

// perform pathname expansion.
struct word_s *pathnames_expand(struct word_s *words) {
    if (no_word_expand) {
        return words;
    }

    struct word_s *curr_word = words;
    struct word_s *prev_word = NULL;

    while (curr_word) {
        char *p = curr_word->data;

        // check if we should perform filename globbing
        if (!has_glob_chars(p, strlen(p))) {
            prev_word = curr_word;
            curr_word = curr_word->next;
            continue;
        }

        glob_t glob;
        char **matches = get_filename_matches(p, &glob);

        // no matches found
        if (matches == NULL || matches[0] == NULL) {
            globfree(&glob);
        } else {
            // save the matches
            struct word_s *head = NULL, *tail = NULL;

            for (size_t j = 0; j < glob.gl_pathc; j++) {
                // skip '..' and '.'
                if (matches[j][0] == '.' &&
                    (matches[j][1] == '.' || matches[j][1] == '\0' ||
                     matches[j][1] == '/')) {
                    continue;
                }
                // add the path to the list
                if (head == NULL) {
                    // first item in the list
                    head = make_word(matches[j]);
                    tail = head;
                } else {
                    if (tail == NULL) {
                        error_message("error: "
                                      "`pathnames_expand`: couldn't add to "
                                      "path expansion list, tail is NULL");
                        return NULL;
                    }
                    // add to the list's tail
                    tail->next = make_word(matches[j]);

                    if (tail->next) {
                        tail = tail->next;
                    }
                }
            }

            // add the new list to the existing list
            if (curr_word == words) {
                words = head;
            } else if (prev_word) {
                prev_word->next = head;
            }

            prev_word = tail;
            if (tail == NULL) {
                error_message("error: `pathnames_expand`: couldn't add "
                              "to path expansion list, tail is NULL");
                return NULL;
            }
            tail->next = curr_word->next;

            // free the word we've just globbed
            curr_word->next = NULL;
            free_all_words(curr_word);
            curr_word = tail;

            // free the matches list
            globfree(&glob);
            // finished globbing this word
        }

        prev_word = curr_word;
        curr_word = curr_word->next;
    }

    return words;
}

// perform quote removal.
void remove_quotes(struct word_s *wordlist) {
    if (no_word_expand) {
        return;
    }

    if (wordlist == NULL) {
        return;
    }

    bool in_double_quotes = false;
    struct word_s *word = wordlist;
    char *p = NULL;

    while (word) {
        p = word->data;
        while (*p) {
            switch (*p) {
            case '"':
                // toggle quote mode
                in_double_quotes = !in_double_quotes;
                delete_char_at(p, 0);
                break;
            case '\'':
                // don't delete if inside double quotes
                if (in_double_quotes) {
                    p++;
                    break;
                }
                delete_char_at(p, 0);
                // find the closing quote
                while (*p && *p != '\'') {
                    p++;
                }
                // and remove it
                if (*p == '\'') {
                    delete_char_at(p, 0);
                }
                break;
            case '`':
                delete_char_at(p, 0);
                break;
            case '\v':
            case '\f':
            case '\t':
            case '\r':
            case '\n':
                p++;
                break;
            case '\\':
                if (in_double_quotes) {
                    switch (p[1]) {
                    // in double quotes, backslash preserves its special quoting
                    // meaning only when followed by one of the following chars.
                    case '$':
                    case '`':
                    case '"':
                    case '\\':
                    case '\n':
                        delete_char_at(p, 0);
                        p++;
                        break;
                    default:
                        p++;
                        break;
                    }
                } else {
                    // parse single-character backslash quoting.
                    delete_char_at(p, 0);
                    p++;
                }
                break;
            default:
                p++;
                break;
            }
        }

        // update the word's length
        word->len = strlen(word->data);

        // move on to the next word
        word = word->next;
    }
}

// A simple shortcut to perform word-expansions on a string,
// returning the result as a string.
char *word_expand_to_str(char *word) {
    if (no_word_expand) {
        return word;
    }

    struct word_s *w = word_expand(word);

    if (w == NULL) {
        return NULL;
    }

    char *res = wordlist_to_str(w);
    free_all_words(w);

    return res;
}
