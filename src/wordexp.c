#include "../include/lusush.h"
#include "../include/errors.h"
#include "../include/symtable.h"
#include "../include/strings.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

// String builder implementation for efficient string manipulation
str_builder_t *sb_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 256;
    }
    
    str_builder_t *sb = malloc(sizeof(str_builder_t));
    if (!sb) {
        return NULL;
    }
    
    sb->data = malloc(initial_capacity);
    if (!sb->data) {
        free(sb);
        return NULL;
    }
    
    sb->data[0] = '\0';
    sb->len = 0;
    sb->capacity = initial_capacity;
    
    return sb;
}

void sb_free(str_builder_t *sb) {
    if (sb) {
        free(sb->data);
        free(sb);
    }
}

static bool sb_ensure_capacity(str_builder_t *sb, size_t needed) {
    if (sb->len + needed + 1 <= sb->capacity) {
        return true;
    }
    
    size_t new_capacity = sb->capacity;
    while (new_capacity < sb->len + needed + 1) {
        new_capacity *= 2;
    }
    
    char *new_data = realloc(sb->data, new_capacity);
    if (!new_data) {
        return false;
    }
    
    sb->data = new_data;
    sb->capacity = new_capacity;
    return true;
}

bool sb_append(str_builder_t *sb, const char *str) {
    if (!sb || !str) {
        return false;
    }
    
    size_t str_len = strlen(str);
    return sb_append_len(sb, str, str_len);
}

bool sb_append_char(str_builder_t *sb, char c) {
    if (!sb || !sb_ensure_capacity(sb, 1)) {
        return false;
    }
    
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
    return true;
}

bool sb_append_len(str_builder_t *sb, const char *str, size_t len) {
    if (!sb || !str) {
        return false;
    }
    
    // Allow appending empty strings (len == 0 is valid)
    if (len == 0) {
        return true;
    }
    
    if (!sb_ensure_capacity(sb, len)) {
        return false;
    }
    
    memcpy(sb->data + sb->len, str, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
    return true;
}

char *sb_finalize(str_builder_t *sb) {
    if (!sb) {
        return NULL;
    }
    
    char *result = malloc(sb->len + 1);
    if (!result) {
        return NULL;
    }
    
    strcpy(result, sb->data);
    return result;
}

// Context management
exp_ctx_t *create_expansion_context(void) {
    exp_ctx_t *ctx = calloc(1, sizeof(exp_ctx_t));
    if (!ctx) {
        return NULL;
    }
    
    // Initialize with defaults
    reset_expansion_context(ctx);
    return ctx;
}

void free_expansion_context(exp_ctx_t *ctx) {
    free(ctx);
}

void reset_expansion_context(exp_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    
    ctx->in_double_quotes = false;
    ctx->in_single_quotes = false;
    ctx->in_var_assign = false;
    ctx->var_assign_eq_count = 0;
    ctx->no_field_split = false;
    ctx->no_pathname_expand = false;
    ctx->no_tilde_expand = false;
}

// Utility functions
bool is_expansion_disabled(void) {
    return get_shell_vari("NO_WORD_EXPAND", false);
}

bool should_expand_tilde(const char *str, size_t pos, const exp_ctx_t *ctx) {
    if (!str || !ctx) {
        return false;
    }
    
    // Don't expand inside quotes
    if (ctx->in_double_quotes || ctx->in_single_quotes) {
        return false;
    }
    
    // Don't expand if tilde expansion is disabled
    if (ctx->no_tilde_expand) {
        return false;
    }
    
    // Expand if:
    // 1. It's at the beginning of the word
    // 2. It's in a variable assignment after = or :
    if (pos == 0) {
        return true;
    }
    
    if (ctx->in_var_assign && pos > 0) {
        char prev = str[pos - 1];
        return (prev == '=' && ctx->var_assign_eq_count == 1) || prev == ':';
    }
    
    return false;
}

size_t find_expansion_end(const char *str, size_t start, char delimiter) {
    if (!str) {
        return 0;
    }
    
    size_t pos = start;
    int depth = 0;
    bool in_quotes = false;
    char quote_char = 0;
    
    while (str[pos]) {
        if (!in_quotes) {
            if (str[pos] == '\'' || str[pos] == '"') {
                in_quotes = true;
                quote_char = str[pos];
            } else if (str[pos] == delimiter) {
                if (depth == 0) {
                    return pos;
                }
                depth--;
            } else if ((delimiter == '}' && str[pos] == '{') ||
                      (delimiter == ')' && str[pos] == '(')) {
                depth++;
            }
        } else {
            if (str[pos] == quote_char && (pos == 0 || str[pos-1] != '\\')) {
                in_quotes = false;
                quote_char = 0;
            }
        }
        pos++;
    }
    
    return (delimiter && depth == 0) ? pos : 0;
}

// Check if a character is a valid variable name character
static bool is_var_char(char c) {
    return isalnum(c) || c == '_';
}

// Check if a string is a valid variable name
static bool is_valid_var_name(const char *str) {
    if (!str || !*str) {
        return false;
    }
    
    // Must start with letter or underscore
    if (!isalpha(*str) && *str != '_') {
        return false;
    }
    
    // Rest must be alphanumeric or underscore
    for (const char *p = str + 1; *p; p++) {
        if (!is_var_char(*p)) {
            return false;
        }
    }
    
    return true;
}

// Get special variable value
static const char *get_special_var(const char *name) {
    static char buffer[32];
    
    if (!name || !*name) return NULL;
    
    switch (*name) {
        case '?':
            if (name[1] == '\0') {
                snprintf(buffer, sizeof(buffer), "%d", last_exit_status);
                return buffer;
            }
            break;
        case '$':
            if (name[1] == '\0') {
                snprintf(buffer, sizeof(buffer), "%d", (int)shell_pid);
                return buffer;
            }
            break;
        case '0':
            if (name[1] == '\0') {
                return (shell_argc > 0 && shell_argv[0]) ? shell_argv[0] : "lusush";
            }
            break;
        case '#':
            if (name[1] == '\0') {
                snprintf(buffer, sizeof(buffer), "%d", shell_argc > 1 ? shell_argc - 1 : 0);
                return buffer;
            }
            break;
        default:
            // Check for positional parameters $1, $2, etc.
            if (isdigit(*name) && name[1] == '\0') {
                int pos = *name - '0';
                if (pos > 0 && pos < shell_argc && shell_argv[pos]) {
                    return shell_argv[pos];
                }
                return "";
            }
            break;
    }
    
    return NULL;
}

// Find the end of a variable name starting at position start
static size_t find_var_name_end(const char *str, size_t start) {
    if (!str || !is_var_char(str[start])) {
        return start;
    }
    
    size_t pos = start;
    while (str[pos] && is_var_char(str[pos])) {
        pos++;
    }
    
    return pos;
}

// Update expansion context based on current character
static void update_context(exp_ctx_t *ctx, const char *str, size_t pos) {
    if (!ctx || !str) {
        return;
    }
    
    char c = str[pos];
    
    switch (c) {
        case '"':
            if (!ctx->in_single_quotes) {
                ctx->in_double_quotes = !ctx->in_double_quotes;
            }
            break;
            
        case '\'':
            if (!ctx->in_double_quotes) {
                ctx->in_single_quotes = !ctx->in_single_quotes;
            }
            break;
            
        case '=':
            if (!ctx->in_double_quotes && !ctx->in_single_quotes) {
                // Check if this is a variable assignment
                // Extract the part before '=' and check if it's a valid variable name
                char *var_name = malloc(pos + 1);
                if (var_name) {
                    strncpy(var_name, str, pos);
                    var_name[pos] = '\0';
                    
                    if (is_valid_var_name(var_name)) {
                        ctx->in_var_assign = true;
                        ctx->var_assign_eq_count++;
                    }
                    free(var_name);
                }
            }
            break;
    }
}

// Tilde expansion implementation
expansion_t tilde_expand(const char *str, const exp_ctx_t *ctx) {
    (void)ctx;  // Suppress unused parameter warning
    expansion_t result = {EXP_NO_EXPANSION, NULL, 0};
    
    if (!str || *str != '~') {
        return result;
    }
    
    if (is_expansion_disabled()) {
        return result;
    }
    
    // Find the end of the tilde prefix
    size_t end = 1;
    while (str[end] && str[end] != '/' && str[end] != ':') {
        end++;
    }
    
    char *home = NULL;
    
    // Null tilde prefix (~) - use $HOME or getpwuid
    if (end == 1) {
        const symtable_entry_t *entry = get_symtable_entry("HOME");
        if (entry && entry->val) {
            home = entry->val;
        } else {
            // Fall back to password database
            struct passwd *pass = getpwuid(getuid());
            if (pass) {
                home = pass->pw_dir;
            }
        }
    } else {
        // Extract username and look up in password database
        char *username = malloc(end);
        if (username) {
            strncpy(username, str + 1, end - 1);
            username[end - 1] = '\0';
            
            struct passwd *pass = getpwnam(username);
            if (pass) {
                home = pass->pw_dir;
            }
            free(username);
        }
    }
    
    if (!home) {
        error_message("tilde expansion: unable to determine home directory");
        result.result = EXP_ERROR;
        return result;
    }
    
    // Create expanded string
    result.expanded = malloc(strlen(home) + 1);
    if (!result.expanded) {
        error_message("tilde expansion: memory allocation failed");
        result.result = EXP_ERROR;
        return result;
    }
    
    strcpy(result.expanded, home);
    result.len = strlen(home);
    result.result = EXP_OK;
    
    return result;
}

// Variable expansion implementation
expansion_t var_expand(const char *str, const exp_ctx_t *ctx) {
    (void)ctx;  // Suppress unused parameter warning
    expansion_t result = {EXP_NO_EXPANSION, NULL, 0};
    
    if (!str || *str != '$') {
        return result;
    }
    
    if (is_expansion_disabled()) {
        return result;
    }
    
    const char *var_start = str + 1;
    size_t var_name_len = 0;
    const char *var_name_start = var_start;
    
    // Handle ${var} vs $var format
    if (*var_start == '{') {
        var_name_start = var_start + 1;
        
        // Find closing brace
        size_t brace_end = find_expansion_end(var_name_start, 0, '}');
        if (brace_end == 0) {
            result.result = EXP_ERROR;
            return result;
        }
        var_name_len = brace_end;
    } else {
        // Find end of variable name
        // Check for special single-character variables first
        if (*var_start == '?' || *var_start == '$' || *var_start == '#' || 
            (*var_start >= '0' && *var_start <= '9')) {
            var_name_len = 1;
            var_name_start = var_start;
        } else if (!isalpha(*var_start) && *var_start != '_') {
            result.result = EXP_NO_EXPANSION;
            return result;
        } else {
            size_t end = find_var_name_end(var_start, 0);
            var_name_len = end;
            var_name_start = var_start;
        }
    }
    
    if (var_name_len == 0) {
        result.result = EXP_NO_EXPANSION;
        return result;
    }
    
    // Extract variable name
    char *var_name = malloc(var_name_len + 1);
    if (!var_name) {
        error_message("parameter expansion: memory allocation failed");
        result.result = EXP_ERROR;
        return result;
    }
    
    strncpy(var_name, var_name_start, var_name_len);
    var_name[var_name_len] = '\0';
    
    // Handle special variable expansion patterns
    bool get_length = false;
    char *actual_var_name = var_name;
    char *param_expansion_op = NULL;
    bool allocated_actual_var_name = false;
    
    // Check for parameter expansion patterns like ${var=value}, ${var:-default}, etc.
    char *op_pos = strpbrk(var_name, "=:-+");
    if (op_pos) {
        // Extract the variable name (before the operator)
        size_t actual_name_len = op_pos - var_name;
        actual_var_name = malloc(actual_name_len + 1);
        if (!actual_var_name) {
            error_message("parameter expansion: memory allocation failed");
            result.result = EXP_ERROR;
            free(var_name);
            return result;
        }
        strncpy(actual_var_name, var_name, actual_name_len);
        actual_var_name[actual_name_len] = '\0';
        allocated_actual_var_name = true;
        
        // Store the operator and value for later processing
        param_expansion_op = strdup(op_pos);
    } else if (*var_name == '#' && var_name[1] != '\0') {
        // ${#var} - get length of var
        get_length = true;
        actual_var_name = var_name + 1;  // This is NOT a malloc'd pointer
        allocated_actual_var_name = false;
    } else {
        actual_var_name = var_name;
        allocated_actual_var_name = false;
    }
    // If it's just '#', treat it as the special variable (argument count)
    
    // Look up variable value - check special variables first
    const char *var_value = get_special_var(actual_var_name);
    bool var_exists = (var_value != NULL);
    if (!var_value) {
        const symtable_entry_t *entry = get_symtable_entry(actual_var_name);
        var_exists = (entry && entry->val);
        var_value = var_exists ? entry->val : "";
    }        // Handle parameter expansion operations
        if (param_expansion_op) {
            char *op = param_expansion_op;
            char *expansion_default = NULL;
            
            if (strncmp(op, "=", 1) == 0) {
            // ${var=value} - assign value if var is unset
            expansion_default = op + 1;
            if (!var_exists || strlen(var_value) == 0) {
                // Variable is unset or empty, assign the default value
                set_shell_varp(actual_var_name, expansion_default);
                var_value = expansion_default;
            }
        } else if (strncmp(op, ":-", 2) == 0) {
            // ${var:-default} - use default if var is unset or empty
            expansion_default = op + 2;
            if (!var_exists || strlen(var_value) == 0) {
                var_value = expansion_default;
            }
        } else if (strncmp(op, ":=", 2) == 0) {
            // ${var:=default} - assign default if var is unset or empty
            expansion_default = op + 2;
            if (!var_exists || strlen(var_value) == 0) {
                set_shell_varp(actual_var_name, expansion_default);
                var_value = expansion_default;
            }
        } else if (strncmp(op, ":+", 2) == 0) {
            // ${var:+alternate} - use alternate if var is set and non-empty
            expansion_default = op + 2;
            if (var_exists && strlen(var_value) > 0) {
                var_value = expansion_default;
            } else {
                var_value = "";
            }
        } else if (strncmp(op, "-", 1) == 0) {
            // ${var-default} - use default if var is unset (but not if empty)
            expansion_default = op + 1;
            if (!var_exists) {
                var_value = expansion_default;
            }
        } else if (strncmp(op, "+", 1) == 0) {
            // ${var+alternate} - use alternate if var is set (even if empty)
            expansion_default = op + 1;
            if (var_exists) {
                var_value = expansion_default;
            } else {
                var_value = "";
            }
        }
    } else {
        // Check for unset variable error (-u option) only when no expansion operator is present
        if (!var_exists && is_posix_option_set('u')) {
            error_message("unset variable: %s", actual_var_name);
            result.result = EXP_ERROR;
            // Clean up allocated memory before returning
            if (param_expansion_op) {
                free(param_expansion_op);
            }
            if (allocated_actual_var_name) {
                free(actual_var_name);
            }
            free(var_name);
            return result;
        }
    }
    
    if (get_length) {
        // Return string length
        char length_str[32];
        snprintf(length_str, sizeof(length_str), "%zu", strlen(var_value));
        
        result.expanded = malloc(strlen(length_str) + 1);
        if (result.expanded) {
            strcpy(result.expanded, length_str);
            result.len = strlen(length_str);
            result.result = EXP_OK;
        } else {
            result.result = EXP_ERROR;
        }
    } else {
        // Return variable value (including empty string)
        result.expanded = malloc(strlen(var_value) + 1);
        if (result.expanded) {
            strcpy(result.expanded, var_value);
            result.len = strlen(var_value);
            result.result = EXP_OK;
        } else {
            result.result = EXP_ERROR;
        }
    }
    
    // Clean up allocated memory
    if (param_expansion_op) {
        free(param_expansion_op);
    }
    if (allocated_actual_var_name) {
        free(actual_var_name);
    }
    free(var_name);
    return result;
}

// Command substitution implementation
expansion_t command_substitute_exp(const char *str, const exp_ctx_t *ctx) {
    (void)ctx;  // Suppress unused parameter warning
    expansion_t result = {EXP_NO_EXPANSION, NULL, 0};
    
    if (!str) {
        return result;
    }
    
    if (is_expansion_disabled()) {
        return result;
    }
    
    bool backquoted = (*str == '`');
    const char *cmd_start;
    size_t cmd_len;
    
    if (backquoted) {
        // `command` format
        cmd_start = str + 1;
        const char *end = strchr(cmd_start, '`');
        if (!end) {
            result.result = EXP_ERROR;
            return result;
        }
        cmd_len = end - cmd_start;
    } else if (*str == '$' && str[1] == '(') {
        // $(command) format
        cmd_start = str + 2;
        size_t end = find_expansion_end(cmd_start, 0, ')');
        if (end == 0) {
            result.result = EXP_ERROR;
            return result;
        }
        cmd_len = end;
    } else {
        return result;
    }
    
    // Extract command
    char *cmd = malloc(cmd_len + 1);
    if (!cmd) {
        result.result = EXP_ERROR;
        return result;
    }
    
    strncpy(cmd, cmd_start, cmd_len);
    cmd[cmd_len] = '\0';
    
    // Execute command and capture output
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        error_message("command substitution: failed to execute command");
        free(cmd);
        result.result = EXP_ERROR;
        return result;
    }
    
    str_builder_t *sb = sb_create(1024);
    if (!sb) {
        error_message("command substitution: memory allocation failed");
        pclose(fp);
        free(cmd);
        result.result = EXP_ERROR;
        return result;
    }
    
    char buffer[1024];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (!sb_append_len(sb, buffer, bytes_read)) {
            sb_free(sb);
            pclose(fp);
            free(cmd);
            result.result = EXP_ERROR;
            return result;
        }
    }
    
    pclose(fp);
    free(cmd);
    
    // Remove trailing newlines
    while (sb->len > 0 && (sb->data[sb->len - 1] == '\n' || sb->data[sb->len - 1] == '\r')) {
        sb->len--;
        sb->data[sb->len] = '\0';
    }
    
    result.expanded = sb_finalize(sb);
    if (result.expanded) {
        result.len = strlen(result.expanded);
        result.result = EXP_OK;
    } else {
        result.result = EXP_ERROR;
    }
    
    sb_free(sb);
    return result;
}

// Arithmetic expansion implementation
expansion_t arithm_expand_exp(const char *str, const exp_ctx_t *ctx) {
    (void)ctx;  // Suppress unused parameter warning
    expansion_t result = {EXP_NO_EXPANSION, NULL, 0};
    
    if (!str || strncmp(str, "$((", 3) != 0) {
        return result;
    }
    
    if (is_expansion_disabled()) {
        return result;
    }
    
    // Find closing ))
    const char *expr_start = str + 3;
    size_t pos = 0;
    int paren_count = 0;
    bool found_end = false;
    
    while (expr_start[pos]) {
        if (expr_start[pos] == '(') {
            paren_count++;
        } else if (expr_start[pos] == ')') {
            if (paren_count > 0) {
                paren_count--;
            } else {
                // Check if next character is also ')'
                if (expr_start[pos + 1] == ')') {
                    found_end = true;
                    break;
                } else {
                    // Single ) without matching (, this is our end
                    break;
                }
            }
        }
        pos++;
    }
    
    if (!found_end) {
        error_message("arithmetic expansion: missing closing '))'");
        result.result = EXP_ERROR;
        return result;
    }
    
    size_t expr_len = pos;
    
    // Extract arithmetic expression
    char *expr = malloc(expr_len + 1);
    if (!expr) {
        error_message("arithmetic expansion: memory allocation failed");
        result.result = EXP_ERROR;
        return result;
    }
    
    strncpy(expr, expr_start, expr_len);
    expr[expr_len] = '\0';
    
    // For now, delegate to the existing arithm_expand function
    char *arith_result = arithm_expand(expr);
    
    if (arith_result) {
        result.expanded = arith_result;
        result.len = strlen(arith_result);
        result.result = EXP_OK;
    } else {
        error_message("arithmetic expansion: evaluation failed for expression '%s'", expr);
        result.result = EXP_ERROR;
    }
    
    free(expr);
    
    return result;
}

// Main word expansion function - refactored for clarity and maintainability
word_t *word_expand(const char *orig_word) {
    if (!orig_word) {
        return NULL;
    }
    
    if (!*orig_word) {
        return make_word((char*)orig_word);
    }
    
    if (is_expansion_disabled()) {
        return make_word((char*)orig_word);
    }
    
    exp_ctx_t *ctx = create_expansion_context();
    if (!ctx) {
        return NULL;
    }
    
    // Check if the entire word is quoted (should not be field-split)
    size_t word_len = strlen(orig_word);
    if (word_len >= 2) {
        if ((orig_word[0] == '"' && orig_word[word_len - 1] == '"') ||
            (orig_word[0] == '\'' && orig_word[word_len - 1] == '\'')) {
            ctx->no_field_split = true;
        }
    }
    
    str_builder_t *sb = sb_create(strlen(orig_word) * 2);
    if (!sb) {
        free_expansion_context(ctx);
        return NULL;
    }
    
    const char *p = orig_word;
    bool expanded = false;
    
    while (*p) {
        update_context(ctx, orig_word, p - orig_word);
        
        expansion_t exp_result = {EXP_NO_EXPANSION, NULL, 0};
        size_t consumed = 1;  // Default: consume one character
        
        switch (*p) {
            case '~':
                if (should_expand_tilde(orig_word, p - orig_word, ctx)) {
                    // Find end of tilde prefix
                    size_t tilde_end = 1;
                    while (p[tilde_end] && p[tilde_end] != '/' && p[tilde_end] != ':' && 
                           !isspace(p[tilde_end])) {
                        tilde_end++;
                    }
                    
                    char *tilde_prefix = malloc(tilde_end + 1);
                    if (tilde_prefix) {
                        strncpy(tilde_prefix, p, tilde_end);
                        tilde_prefix[tilde_end] = '\0';
                        
                        exp_result = tilde_expand(tilde_prefix, ctx);
                        consumed = tilde_end;
                        free(tilde_prefix);
                    }
                }
                break;
                
            case '$':
                if (!ctx->in_single_quotes) {
                    if (p[1] == '(') {
                        if (p[2] == '(') {
                            // Arithmetic expansion $((...))
                            size_t end = 3;
                            int depth = 2;
                            while (p[end] && depth > 0) {
                                if (p[end] == '(') depth++;
                                else if (p[end] == ')') depth--;
                                end++;
                            }
                            if (depth == 0) {
                                char *arith_expr = malloc(end + 1);
                                if (arith_expr) {
                                    strncpy(arith_expr, p, end);
                                    arith_expr[end] = '\0';
                                    exp_result = arithm_expand_exp(arith_expr, ctx);
                                    consumed = end;
                                    free(arith_expr);
                                }
                            }
                        } else {
                            // Command substitution $(...)
                            size_t end = find_expansion_end(p + 2, 0, ')');
                            if (end > 0) {
                                end += 3;  // Include $()
                                char *cmd_expr = malloc(end + 1);
                                if (cmd_expr) {
                                    strncpy(cmd_expr, p, end);
                                    cmd_expr[end] = '\0';
                                    exp_result = command_substitute_exp(cmd_expr, ctx);
                                    consumed = end;
                                    free(cmd_expr);
                                }
                            }
                        }
                    } else {
                        // Variable expansion $var or ${var}
                        size_t end = 1;
                        if (p[1] == '{') {
                            end = find_expansion_end(p + 2, 0, '}');
                            if (end > 0) {
                                end += 3;  // Include ${}
                            }
                        } else if (isalpha(p[1]) || p[1] == '_' || 
                                  p[1] == '?' || p[1] == '$' || p[1] == '#' ||
                                  (p[1] >= '0' && p[1] <= '9')) {
                            // Handle special variables and positional parameters
                            if (p[1] == '?' || p[1] == '$' || p[1] == '#' ||
                                (p[1] >= '0' && p[1] <= '9')) {
                                end = 2;  // Special variables are single character
                            } else {
                                end = find_var_name_end(p, 1);
                            }
                        }
                        
                        if (end > 1) {
                            char *var_expr = malloc(end + 1);
                            if (var_expr) {
                                strncpy(var_expr, p, end);
                                var_expr[end] = '\0';
                                exp_result = var_expand(var_expr, ctx);
                                consumed = end;
                                free(var_expr);
                            }
                        }
                    }
                }
                break;
                
            case '`':
                if (!ctx->in_single_quotes) {
                    // Command substitution `...` (works in double quotes and unquoted context)
                    const char *end = strchr(p + 1, '`');
                    if (end) {
                        size_t cmd_len = end - p + 1;
                        char *cmd_expr = malloc(cmd_len + 1);
                        if (cmd_expr) {
                            strncpy(cmd_expr, p, cmd_len);
                            cmd_expr[cmd_len] = '\0';
                            exp_result = command_substitute_exp(cmd_expr, ctx);
                            consumed = cmd_len;
                            free(cmd_expr);
                        }
                    }
                }
                break;
                
            case '\\':
                if (!ctx->in_single_quotes) {
                    // Handle escape sequences in double quotes or unquoted context
                    if (p[1]) {
                        char next = p[1];
                        switch (next) {
                            case 'n':
                                if (!sb_append_char(sb, '\n')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case 't':
                                if (!sb_append_char(sb, '\t')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case 'r':
                                if (!sb_append_char(sb, '\r')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case 'b':
                                if (!sb_append_char(sb, '\b')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case 'f':
                                if (!sb_append_char(sb, '\f')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case 'a':
                                if (!sb_append_char(sb, '\a')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case 'v':
                                if (!sb_append_char(sb, '\v')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case '\\':
                                if (!sb_append_char(sb, '\\')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case '"':
                                if (!sb_append_char(sb, '"')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case '\'':
                                if (!sb_append_char(sb, '\'')) goto error_cleanup;
                                consumed = 2;
                                break;
                            case '$':
                                if (!sb_append_char(sb, '$')) goto error_cleanup;
                                consumed = 2;
                                break;
                            default:
                                // Other escape sequences or just literal backslash
                                if (!sb_append_char(sb, next)) goto error_cleanup;
                                consumed = 2;
                                break;
                        }
                    } else {
                        // Backslash at end of string
                        if (!sb_append_char(sb, '\\')) goto error_cleanup;
                        consumed = 1;
                    }
                } else {
                    // In single quotes - backslash is literal
                    if (!sb_append_char(sb, *p)) {
                        goto error_cleanup;
                    }
                }
                break;
                
            default:
                // Regular character - check for whitespace expansion
                if (isspace(*p) && !ctx->in_double_quotes && !ctx->in_single_quotes) {
                    expanded = true;
                }
                
                // CRITICAL FIX: Always append regular characters (including spaces in quotes)
                // This was the bug causing "$x $y" to become "xy" instead of "x y"
                exp_result.result = EXP_NO_EXPANSION;
                break;
        }
        
        // Handle expansion results
        if (exp_result.result == EXP_OK && exp_result.expanded) {
            if (!sb_append(sb, exp_result.expanded)) {
                free(exp_result.expanded);
                goto error_cleanup;
            }
            expanded = true;
            free(exp_result.expanded);
        } else if (exp_result.result == EXP_ERROR) {
            if (exp_result.expanded) {
                free(exp_result.expanded);
            }
            goto error_cleanup;
        } else if (exp_result.result == EXP_NO_EXPANSION) {
            // No expansion, append original characters
            for (size_t i = 0; i < consumed; i++) {
                if (!sb_append_char(sb, p[i])) {
                    goto error_cleanup;
                }
            }
        }
        
        p += consumed;
    }
    
    char *expanded_str = sb_finalize(sb);
    if (!expanded_str) {
        goto error_cleanup;
    }
    
    sb_free(sb);
    
    // If we performed expansion, handle field splitting appropriately
    word_t *words = NULL;
    
    if (expanded) {
        // Field split if not in quoted context
        if (!ctx->no_field_split) {
            words = field_split(expanded_str);
        } else {
            // In quoted context, create a single word
            words = make_word(expanded_str);
        }
    } else {
        // No expansion occurred, create single word and handle quotes
        words = make_word(expanded_str);
    }
    
    free_expansion_context(ctx);
    free(expanded_str);
    
    // Perform pathname expansion and quote removal
    if (words && !is_expansion_disabled()) {
        words = pathnames_expand(words);
        remove_quotes(words);
    }
    
    return words;

error_cleanup:
    sb_free(sb);
    free_expansion_context(ctx);
    return NULL;
}

char *word_expand_to_str(const char *word) {
    if (!word || is_expansion_disabled()) {
        if (!word) return NULL;
        char *result = malloc(strlen(word) + 1);
        if (result) {
            strcpy(result, word);
        }
        return result;
    }
    
    word_t *expanded = word_expand(word);
    if (!expanded) {
        return NULL;
    }
    
    char *result = wordlist_to_str(expanded);
    free_all_words(expanded);
    
    return result;
}

// Improved field splitting with better IFS handling
word_t *field_split(const char *str) {
    if (!str || !*str) {
        return NULL;
    }
    
    const symtable_entry_t *entry = get_symtable_entry("IFS");
    const char *IFS = entry ? entry->val : " \t\n";
    
    // Empty IFS means no field splitting
    if (!*IFS) {
        return NULL;
    }
    
    // Classify IFS characters
    char IFS_space[64] = {0};
    char IFS_delim[64] = {0};
    
    const char *p = IFS;
    char *sp = IFS_space;
    char *dp = IFS_delim;
    
    while (*p) {
        if (isspace(*p)) {
            *sp++ = *p;
        } else {
            *dp++ = *p;
        }
        p++;
    }
    
    // Skip leading IFS whitespace
    while (*str && strchr(IFS_space, *str)) {
        str++;
    }
    
    if (!*str) {
        return NULL;
    }
    
    word_t *first_word = NULL;
    word_t *current_word = NULL;
    str_builder_t *field_sb = sb_create(256);
    
    if (!field_sb) {
        return NULL;
    }
    
    bool in_single_quotes = false;
    bool in_double_quotes = false;
    
    while (*str) {
        char c = *str;
        
        // Handle quotes
        if (c == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
            if (!sb_append_char(field_sb, c)) {
                goto field_error;
            }
        } else if (c == '"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
            if (!sb_append_char(field_sb, c)) {
                goto field_error;
            }
        } else if ((strchr(IFS_space, c) || strchr(IFS_delim, c)) &&
                   !in_single_quotes && !in_double_quotes) {
            // Field delimiter found
            if (field_sb->len > 0) {
                // Create new word
                char *field_data = sb_finalize(field_sb);
                if (!field_data) {
                    goto field_error;
                }
                
                word_t *new_word = make_word(field_data);
                free(field_data);
                
                if (!new_word) {
                    goto field_error;
                }
                
                if (!first_word) {
                    first_word = current_word = new_word;
                } else {
                    current_word->next = new_word;
                    current_word = new_word;
                }
                
                // Reset field builder
                sb_free(field_sb);
                field_sb = sb_create(256);
                if (!field_sb) {
                    return first_word;
                }
            }
            
            // Skip additional IFS characters
            while (*str && (strchr(IFS_space, *str) || strchr(IFS_delim, *str))) {
                str++;
            }
            continue;
        } else {
            // Regular character
            if (!sb_append_char(field_sb, c)) {
                goto field_error;
            }
        }
        
        str++;
    }
    
    // Handle final field
    if (field_sb->len > 0) {
        char *field_data = sb_finalize(field_sb);
        if (field_data) {
            word_t *new_word = make_word(field_data);
            free(field_data);
            
            if (new_word) {
                if (!first_word) {
                    first_word = new_word;
                } else {
                    current_word->next = new_word;
                }
            }
        }
    }
    
    sb_free(field_sb);
    return first_word;

field_error:
    sb_free(field_sb);
    return first_word;
}

// Improved pathname expansion with better error handling
word_t *pathnames_expand(word_t *words) {
    if (!words || is_expansion_disabled()) {
        return words;
    }
    
    word_t *current = words;
    word_t *prev = NULL;
    
    while (current) {
        char *pattern = current->data;
        
        // Check if the word contains glob characters
        if (!has_glob_chars(pattern, strlen(pattern))) {
            prev = current;
            current = current->next;
            continue;
        }
        
        // Perform pathname expansion
        glob_t glob_result;
        char **matches = get_filename_matches(pattern, &glob_result);
        
        if (!matches || !matches[0]) {
            // No matches found - keep original pattern
            globfree(&glob_result);
            prev = current;
            current = current->next;
            continue;
        }
        
        // Create new word list from matches
        word_t *match_head = NULL;
        word_t *match_tail = NULL;
        
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            // Skip hidden files starting with . unless explicitly requested
            if (matches[i][0] == '.' && pattern[0] != '.') {
                continue;
            }
            
            word_t *match_word = make_word(matches[i]);
            if (!match_word) {
                // Error creating word - cleanup and continue
                free_all_words(match_head);
                match_head = NULL;
                break;
            }
            
            if (!match_head) {
                match_head = match_tail = match_word;
            } else {
                match_tail->next = match_word;
                match_tail = match_word;
            }
        }
        
        globfree(&glob_result);
        
        if (match_head) {
            // Replace current word with matches
            if (prev) {
                prev->next = match_head;
            } else {
                words = match_head;
            }
            
            match_tail->next = current->next;
            
            // Free the original word
            current->next = NULL;
            free_all_words(current);
            
            prev = match_tail;
            current = match_tail->next;
        } else {
            // Failed to create matches - keep original
            prev = current;
            current = current->next;
        }
    }
    
    return words;
}

// Improved quote removal with better state tracking
void remove_quotes(word_t *wordlist) {
    if (!wordlist || is_expansion_disabled()) {
        return;
    }
    
    word_t *word = wordlist;
    
    while (word) {
        if (!word->data) {
            word = word->next;
            continue;
        }
        
        str_builder_t *sb = sb_create(word->len);
        if (!sb) {
            word = word->next;
            continue;
        }
        
        const char *p = word->data;
        bool in_double_quotes = false;
        
        while (*p) {
            switch (*p) {
                case '"':
                    // Toggle double quote mode (remove the quote)
                    in_double_quotes = !in_double_quotes;
                    break;
                    
                case '\'':
                    if (!in_double_quotes) {
                        // Single quotes - skip to closing quote
                        p++; // Skip opening quote
                        while (*p && *p != '\'') {
                            if (!sb_append_char(sb, *p)) {
                                goto quote_error;
                            }
                            p++;
                        }
                        // Skip closing quote (will be skipped by loop increment)
                    } else {
                        // Inside double quotes - treat as literal
                        if (!sb_append_char(sb, *p)) {
                            goto quote_error;
                        }
                    }
                    break;
                    
                case '\\':
                    if (in_double_quotes) {
                        // In double quotes - only escape special characters
                        if (p[1] && strchr("$`\"\\", p[1])) {
                            p++; // Skip backslash
                            if (!sb_append_char(sb, *p)) {
                                goto quote_error;
                            }
                        } else {
                            // Keep backslash as literal
                            if (!sb_append_char(sb, *p)) {
                                goto quote_error;
                            }
                        }
                    } else {
                        // Outside quotes - escape next character
                        if (p[1]) {
                            p++; // Skip backslash
                            if (!sb_append_char(sb, *p)) {
                                goto quote_error;
                            }
                        }
                    }
                    break;
                    
                default:
                    if (!sb_append_char(sb, *p)) {
                        goto quote_error;
                    }
                    break;
            }
            p++;
        }
        
        // Replace word data with unquoted version
        char *unquoted = sb_finalize(sb);
        if (unquoted) {
            free(word->data);
            word->data = unquoted;
            word->len = strlen(unquoted);
        }
        
        sb_free(sb);
        word = word->next;
        continue;

quote_error:
        sb_free(sb);
        word = word->next;
    }
}

// Legacy compatibility functions for old API
char *tilde_expand_legacy(char *s) {
    if (!s) return NULL;
    
    expansion_t result = tilde_expand(s, NULL);
    if (result.result == EXP_OK && result.expanded) {
        return result.expanded;
    }
    
    // Return copy of original string if no expansion
    char *copy = malloc(strlen(s) + 1);
    if (copy) {
        strcpy(copy, s);
    }
    return copy;
}

char *var_expand_legacy(char *orig_var_name) {
    if (!orig_var_name) return NULL;
    
    expansion_t result = var_expand(orig_var_name, NULL);
    if (result.result == EXP_OK && result.expanded) {
        return result.expanded;
    } else if (result.result == EXP_INVALID_VAR) {
        // Return special INVALID_VAR marker for compatibility
        return (char*)-1;
    }
    
    return NULL;
}

// Migration helper functions to integrate with existing code
char *word_expand_to_str_compat(char *word) {
    return word_expand_to_str(word);
}

word_t *word_expand_compat(char *orig_word) {
    return word_expand(orig_word);
}

// Utility functions for word manipulation

// convert the string *word to a cmd_token struct, so it can be passed to
// functions such as word_expand().
// returns the malloc'd cmd_token struct, or NULL if insufficient memory.
word_t *make_word(char *str) {
    // alloc struct memory
    word_t *word = calloc(1, sizeof(word_t));
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
void free_all_words(word_t *first) {
    while (first) {
        word_t *del = first;
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
char *wordlist_to_str(word_t *word) {
    if (word == NULL) {
        return NULL;
    }
    size_t len = 0;
    const word_t *w = word;
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
