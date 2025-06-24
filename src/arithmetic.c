/*
 * Modern Arithmetic Expansion Module for Lusush Shell
 *
 * This module provides POSIX-compliant arithmetic expansion using the shunting yard algorithm.
 * It replaces the legacy shunt.c implementation with a modern, clean interface that integrates
 * with the modern shell architecture.
 *
 * Based on the shunting yard algorithm implementation from:
 * - Original: http://en.literateprograms.org/Shunting_yard_algorithm_(C)
 * - Modified by Mohammed Isam for Layla shell
 * - Further modernized for Lusush shell
 *
 * Copyright (c) 2024 Michael Berry <trismegustis@gmail.com>
 */

#include "../include/arithmetic.h"

#include "../include/lusush.h"
#include "../include/symtable.h"


#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Get global symbol table manager (defined in symtable.c)
extern symtable_manager_t *symtable_get_global_manager(void);

#define MAXOPSTACK 64
#define MAXNUMSTACK 64
#define MAXBASE 36

// Stack item for arithmetic evaluation
typedef struct {
    enum {
        ITEM_LONG_INT = 1,
        ITEM_VAR_PTR = 2
    } type;

    union {
        ssize_t val;
        char *var_name;  // Store variable name instead of pointer
    };
} stack_item_t;

// Operator associativity
enum {
    ASSOC_NONE = 0,
    ASSOC_LEFT = 1,
    ASSOC_RIGHT = 2
};

// Operator structure
typedef struct {
    char op;
    int prec;
    int assoc;
    char unary;
    char chars;
    ssize_t (*eval)(stack_item_t *a1, stack_item_t *a2);
} op_t;

// Arithmetic evaluation context
typedef struct {
    op_t *opstack[MAXOPSTACK];
    int nopstack;
    stack_item_t numstack[MAXNUMSTACK];
    int nnumstack;
    bool errflag;
    char *error_message;
} arithm_context_t;

// Cleanup function for arithmetic context
static void arithm_context_cleanup(arithm_context_t *ctx) {
    if (!ctx) return;

    // Free any variable names stored in the number stack
    for (int i = 0; i < ctx->nnumstack; i++) {
        if (ctx->numstack[i].type == ITEM_VAR_PTR && ctx->numstack[i].var_name) {
            free(ctx->numstack[i].var_name);
            ctx->numstack[i].var_name = NULL;
        }
    }

    if (ctx->error_message) {
        free(ctx->error_message);
        ctx->error_message = NULL;
    }
}

// Global error state
bool arithm_error_flag = false;
char *arithm_error_message = NULL;

// Forward declarations
static ssize_t long_value(stack_item_t *item);
static void push_opstack(arithm_context_t *ctx, op_t *op);
static op_t *pop_opstack(arithm_context_t *ctx);
static void push_numstackl(arithm_context_t *ctx, ssize_t val);
static void push_numstackv(arithm_context_t *ctx, const char *var_name);
static stack_item_t pop_numstack(arithm_context_t *ctx);
static void shunt_op(arithm_context_t *ctx, op_t *op);
static op_t *get_op(const char *expr);
static ssize_t get_num(const char *expr, int *nchars);
static char *get_var_name(const char *expr, int *nchars);
static bool valid_name_char(char c);

// Operator evaluation functions
static ssize_t eval_uminus(stack_item_t *a1, stack_item_t *a2) {
    (void)a2;
    return -long_value(a1);
}

static ssize_t eval_uplus(stack_item_t *a1, stack_item_t *a2) {
    (void)a2;
    return long_value(a1);
}

static ssize_t eval_lognot(stack_item_t *a1, stack_item_t *a2) {
    (void)a2;
    return !long_value(a1);
}

static ssize_t eval_bitnot(stack_item_t *a1, stack_item_t *a2) {
    (void)a2;
    return ~long_value(a1);
}

static ssize_t eval_mul(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) * long_value(a2);
}

static ssize_t eval_add(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) + long_value(a2);
}

static ssize_t eval_sub(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) - long_value(a2);
}

static ssize_t eval_div(stack_item_t *a1, stack_item_t *a2) {
    ssize_t divisor = long_value(a2);
    if (divisor == 0) {
        arithm_set_error("division by zero");
        return 0;
    }
    return long_value(a1) / divisor;
}

static ssize_t eval_mod(stack_item_t *a1, stack_item_t *a2) {
    ssize_t divisor = long_value(a2);
    if (divisor == 0) {
        arithm_set_error("division by zero in modulo operation");
        return 0;
    }
    return long_value(a1) % divisor;
}

static ssize_t eval_lsh(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) << long_value(a2);
}

static ssize_t eval_rsh(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) >> long_value(a2);
}

static ssize_t eval_lt(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) < long_value(a2);
}

static ssize_t eval_le(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) <= long_value(a2);
}

static ssize_t eval_gt(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) > long_value(a2);
}

static ssize_t eval_ge(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) >= long_value(a2);
}

static ssize_t eval_eq(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) == long_value(a2);
}

static ssize_t eval_ne(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) != long_value(a2);
}

static ssize_t eval_bitand(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) & long_value(a2);
}

static ssize_t eval_bitxor(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) ^ long_value(a2);
}

static ssize_t eval_bitor(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) | long_value(a2);
}

static ssize_t eval_logand(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) && long_value(a2);
}

static ssize_t eval_logor(stack_item_t *a1, stack_item_t *a2) {
    return long_value(a1) || long_value(a2);
}

static ssize_t eval_exp(stack_item_t *a1, stack_item_t *a2) {
    ssize_t base = long_value(a1);
    ssize_t exp = long_value(a2);
    if (exp < 0) {
        arithm_set_error("negative exponent not supported");
        return 0;
    }

    ssize_t result = 1;
    for (ssize_t i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

// Operator character constants
#define CH_GT '>'
#define CH_LT '<'
#define CH_GE 0x01
#define CH_LE 0x02
#define CH_RSH 0x03
#define CH_LSH 0x04
#define CH_NE 0x05
#define CH_EQ 0x06
#define CH_AND 0x07
#define CH_OR 0x08
#define CH_EXP 0x09

// Operator definitions (only binary operators in main table)
static op_t operators[] = {
    {'(', 0, ASSOC_NONE, 0, 1, NULL},
    {')', 0, ASSOC_NONE, 0, 1, NULL},
    {'!', 2, ASSOC_RIGHT, 1, 1, eval_lognot},
    {'~', 2, ASSOC_RIGHT, 1, 1, eval_bitnot},
    {CH_EXP, 3, ASSOC_RIGHT, 0, 2, eval_exp},
    {'*', 4, ASSOC_LEFT, 0, 1, eval_mul},
    {'/', 4, ASSOC_LEFT, 0, 1, eval_div},
    {'%', 4, ASSOC_LEFT, 0, 1, eval_mod},
    {'+', 5, ASSOC_LEFT, 0, 1, eval_add},
    {'-', 5, ASSOC_LEFT, 0, 1, eval_sub},
    {CH_LSH, 6, ASSOC_LEFT, 0, 2, eval_lsh},
    {CH_RSH, 6, ASSOC_LEFT, 0, 2, eval_rsh},
    {CH_LT, 7, ASSOC_LEFT, 0, 1, eval_lt},
    {CH_LE, 7, ASSOC_LEFT, 0, 2, eval_le},
    {CH_GT, 7, ASSOC_LEFT, 0, 1, eval_gt},
    {CH_GE, 7, ASSOC_LEFT, 0, 2, eval_ge},
    {CH_EQ, 8, ASSOC_LEFT, 0, 2, eval_eq},
    {CH_NE, 8, ASSOC_LEFT, 0, 2, eval_ne},
    {'&', 9, ASSOC_LEFT, 0, 1, eval_bitand},
    {'^', 10, ASSOC_LEFT, 0, 1, eval_bitxor},
    {'|', 11, ASSOC_LEFT, 0, 1, eval_bitor},
    {CH_AND, 12, ASSOC_LEFT, 0, 2, eval_logand},
    {CH_OR, 13, ASSOC_LEFT, 0, 2, eval_logor},
    {0, 0, 0, 0, 0, NULL}
};

// Unary operator definitions (separate from main table)
static op_t op_uminus = {'-', 2, ASSOC_RIGHT, 1, 1, eval_uminus};
static op_t op_uplus = {'+', 2, ASSOC_RIGHT, 1, 1, eval_uplus};

// Operator shortcuts
#define OP_UMINUS (&op_uminus)
#define OP_UPLUS (&op_uplus)

// Get long value from stack item
static ssize_t long_value(stack_item_t *item) {
    switch (item->type) {
        case ITEM_LONG_INT:
            return item->val;
        case ITEM_VAR_PTR: {
            if (item->var_name) {
                symtable_manager_t *manager = symtable_get_global_manager();
                if (manager) {
                    char *value = symtable_get_var(manager, item->var_name);
                    if (value) {
                        return atol(value);
                    }
                }
            }
            return 0;
        }
        default:
            return 0;
    }
}

// Stack management functions
static void push_opstack(arithm_context_t *ctx, op_t *op) {
    if (ctx->nopstack >= MAXOPSTACK) {
        ctx->errflag = true;
        arithm_set_error("operator stack overflow");
        return;
    }
    ctx->opstack[ctx->nopstack++] = op;
}

static op_t *pop_opstack(arithm_context_t *ctx) {
    if (ctx->nopstack <= 0) {
        ctx->errflag = true;
        arithm_set_error("operator stack underflow");
        return NULL;
    }
    return ctx->opstack[--ctx->nopstack];
}

static void push_numstackl(arithm_context_t *ctx, ssize_t val) {
    if (ctx->nnumstack >= MAXNUMSTACK) {
        ctx->errflag = true;
        arithm_set_error("number stack overflow");
        return;
    }
    ctx->numstack[ctx->nnumstack].type = ITEM_LONG_INT;
    ctx->numstack[ctx->nnumstack].val = val;
    ctx->nnumstack++;
}

static void push_numstackv(arithm_context_t *ctx, const char *var_name) {
    if (ctx->nnumstack >= MAXNUMSTACK) {
        ctx->errflag = true;
        arithm_set_error("number stack overflow");
        return;
    }
    ctx->numstack[ctx->nnumstack].type = ITEM_VAR_PTR;
    ctx->numstack[ctx->nnumstack].var_name = strdup(var_name);
    ctx->nnumstack++;
}

static stack_item_t pop_numstack(arithm_context_t *ctx) {
    stack_item_t empty = {ITEM_LONG_INT, {0}};
    if (ctx->nnumstack <= 0) {
        ctx->errflag = true;
        arithm_set_error("number stack underflow");
        return empty;
    }
    return ctx->numstack[--ctx->nnumstack];
}

// Check if character is valid for variable names
static bool valid_name_char(char c) {
    return isalnum(c) || c == '_';
}

// Get operator from expression
static op_t *get_op(const char *expr) {
    for (op_t *op = operators; op->op; op++) {
        if (op->chars == 1 && *expr == op->op) {
            return op;
        } else if (op->chars == 2) {
            if (op->op == CH_EQ && expr[0] == '=' && expr[1] == '=') {
                return op;
            } else if (op->op == CH_NE && expr[0] == '!' && expr[1] == '=') {
                return op;
            } else if (op->op == CH_LE && expr[0] == '<' && expr[1] == '=') {
                return op;
            } else if (op->op == CH_GE && expr[0] == '>' && expr[1] == '=') {
                return op;
            } else if (op->op == CH_LSH && expr[0] == '<' && expr[1] == '<') {
                return op;
            } else if (op->op == CH_RSH && expr[0] == '>' && expr[1] == '>') {
                return op;
            } else if (op->op == CH_AND && expr[0] == '&' && expr[1] == '&') {
                return op;
            } else if (op->op == CH_OR && expr[0] == '|' && expr[1] == '|') {
                return op;
            } else if (op->op == CH_EXP && expr[0] == '*' && expr[1] == '*') {
                return op;
            }
        }
    }
    return NULL;
}

// Parse number from expression
static ssize_t get_num(const char *expr, int *nchars) {
    char *endptr;
    ssize_t result;

    if (expr[0] == '0' && (expr[1] == 'x' || expr[1] == 'X')) {
        // Hexadecimal
        result = strtol(expr, &endptr, 16);
    } else if (expr[0] == '0' && isdigit(expr[1])) {
        // Octal
        result = strtol(expr, &endptr, 8);
    } else {
        // Decimal
        result = strtol(expr, &endptr, 10);
    }

    *nchars = endptr - expr;
    return result;
}

// Get variable name from expression
static char *get_var_name(const char *expr, int *nchars) {
    const char *start = expr;
    *nchars = 0;

    // Variable names start with letter or underscore
    if (!isalpha(*expr) && *expr != '_') {
        return NULL;
    }

    // Count valid name characters
    while (valid_name_char(*expr)) {
        expr++;
        (*nchars)++;
    }

    // Extract variable name
    char *name = malloc(*nchars + 1);
    if (!name) {
        arithm_set_error("memory allocation failed");
        return NULL;
    }

    strncpy(name, start, *nchars);
    name[*nchars] = '\0';

    // Ensure variable exists in symbol table with default value "0"
    symtable_manager_t *manager = symtable_get_global_manager();
    if (manager && !symtable_var_exists(manager, name)) {
        symtable_set_var(manager, name, "0", SYMVAR_NONE);
    }

    return name;
}

// Shunting yard algorithm implementation
static void shunt_op(arithm_context_t *ctx, op_t *op) {
    if (op->op == '(') {
        push_opstack(ctx, op);
    } else if (op->op == ')') {
        while (ctx->nopstack > 0 && ctx->opstack[ctx->nopstack - 1]->op != '(') {
            op_t *pop_op = pop_opstack(ctx);
            if (ctx->errflag) return;

            stack_item_t a1 = pop_numstack(ctx);
            if (ctx->errflag) return;

            if (pop_op->unary) {
                push_numstackl(ctx, pop_op->eval(&a1, NULL));
            } else {
                stack_item_t a2 = pop_numstack(ctx);
                if (ctx->errflag) return;
                push_numstackl(ctx, pop_op->eval(&a2, &a1));
            }
            if (ctx->errflag) return;
        }

        if (ctx->nopstack > 0 && ctx->opstack[ctx->nopstack - 1]->op == '(') {
            pop_opstack(ctx); // Remove the '('
        } else {
            ctx->errflag = true;
            arithm_set_error("mismatched parentheses");
        }
    } else {
        while (ctx->nopstack > 0 && ctx->opstack[ctx->nopstack - 1]->op != '(' &&
               ((op->assoc == ASSOC_LEFT && op->prec >= ctx->opstack[ctx->nopstack - 1]->prec) ||
                (op->assoc == ASSOC_RIGHT && op->prec > ctx->opstack[ctx->nopstack - 1]->prec))) {

            op_t *pop_op = pop_opstack(ctx);
            if (ctx->errflag) return;

            stack_item_t a1 = pop_numstack(ctx);
            if (ctx->errflag) return;

            if (pop_op->unary) {
                push_numstackl(ctx, pop_op->eval(&a1, NULL));
            } else {
                stack_item_t a2 = pop_numstack(ctx);
                if (ctx->errflag) return;
                push_numstackl(ctx, pop_op->eval(&a2, &a1));
            }
            if (ctx->errflag) return;
        }
        push_opstack(ctx, op);
    }
}

// Error handling functions
void arithm_init(void) {
    arithm_clear_error();
}

void arithm_cleanup(void) {
    arithm_clear_error();
}

const char *arithm_get_last_error(void) {
    return arithm_error_message;
}

void arithm_set_error(const char *message) {
    arithm_clear_error();
    arithm_error_flag = true;
    if (message) {
        arithm_error_message = strdup(message);
    }
}

void arithm_clear_error(void) {
    arithm_error_flag = false;
    if (arithm_error_message) {
        free(arithm_error_message);
        arithm_error_message = NULL;
    }
}

// Main arithmetic expansion function
char *arithm_expand(const char *orig_expr) {
    if (!orig_expr) {
        return strdup("0");
    }

    // Initialize context
    arithm_context_t ctx = {0};
    arithm_clear_error();

    // Parse expression and remove $(( )) wrapper if present
    const char *expr;
    char *cleaned_expr = NULL;

    if (strncmp(orig_expr, "$((", 3) == 0) {
        size_t len = strlen(orig_expr);
        if (len >= 5 && orig_expr[len-2] == ')' && orig_expr[len-1] == ')') {
            // Extract expression between $(( and ))
            size_t expr_len = len - 5;
            cleaned_expr = malloc(expr_len + 1);
            if (!cleaned_expr) {
                arithm_set_error("memory allocation failed");
                return NULL;
            }
            strncpy(cleaned_expr, orig_expr + 3, expr_len);
            cleaned_expr[expr_len] = '\0';
            expr = cleaned_expr;
        } else {
            arithm_set_error("malformed arithmetic expression: missing closing ))");
            return NULL;
        }
    } else {
        expr = orig_expr;
    }

    // Tokenize and evaluate expression using clearer state management
    const char *current = expr;
    op_t start_op = {'X', 0, ASSOC_NONE, 0, 0, NULL};
    op_t *last_op = &start_op;

    while (*current && !ctx.errflag) {
        // Skip whitespace
        while (*current && isspace(*current)) {
            current++;
        }

        if (!*current) break;

        // Try to parse an operator first
        op_t *op = get_op(current);
        if (op) {
            // Handle unary operators
            if (last_op && (last_op == &start_op || last_op->op != ')')) {
                if (op->op == '-') {
                    op = OP_UMINUS;
                } else if (op->op == '+') {
                    op = OP_UPLUS;
                } else if (op->op != '(' && !op->unary) {
                    arithm_set_error("illegal use of binary operator");
                    break;
                }
            }

            shunt_op(&ctx, op);
            if (ctx.errflag) break;
            last_op = op;
            current += op->chars;
        } else if (isdigit(*current)) {
            // Parse number
            int nchars;
            ssize_t num = get_num(current, &nchars);
            push_numstackl(&ctx, num);
            if (ctx.errflag) break;
            last_op = NULL;
            current += nchars;
        } else if (valid_name_char(*current)) {
            // Parse variable name
            int nchars;
            char *var_name = get_var_name(current, &nchars);
            if (var_name) {
                push_numstackv(&ctx, var_name);
                free(var_name);
            } else {
                push_numstackl(&ctx, 0); // Undefined variable = 0
            }
            if (ctx.errflag) break;
            last_op = NULL;
            current += nchars;
        } else {
            arithm_set_error("syntax error in arithmetic expression");
            break;
        }
    }

    // Process remaining operators
    while (ctx.nopstack > 0 && !ctx.errflag) {
        op_t *op = pop_opstack(&ctx);
        if (ctx.errflag) break;

        stack_item_t a1 = pop_numstack(&ctx);
        if (ctx.errflag) break;

        if (op->unary) {
            push_numstackl(&ctx, op->eval(&a1, NULL));
        } else {
            stack_item_t a2 = pop_numstack(&ctx);
            if (ctx.errflag) break;
            push_numstackl(&ctx, op->eval(&a2, &a1));
        }
        if (ctx.errflag) break;
    }

    // Clean up
    if (cleaned_expr) {
        free(cleaned_expr);
    }

    // Check for errors
    if (ctx.errflag || arithm_error_flag) {
        arithm_context_cleanup(&ctx);
        return NULL;
    }

    // Should have exactly one result
    if (ctx.nnumstack != 1) {
        arithm_set_error("invalid arithmetic expression");
        arithm_context_cleanup(&ctx);
        return NULL;
    }

    // Format result
    ssize_t result = long_value(&ctx.numstack[0]);
    char *result_str = malloc(32);
    if (!result_str) {
        arithm_set_error("memory allocation failed");
        arithm_context_cleanup(&ctx);
        return NULL;
    }

    snprintf(result_str, 32, "%ld", result);
    arithm_context_cleanup(&ctx);
    return result_str;
}
