/**
 * @file builtins.h
 * @brief Shell builtin command declarations
 *
 * Declares all shell builtin commands and the builtin dispatch table
 * for command lookup and execution.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>
#include <stddef.h>

#include "libhashtable/ht.h"

/** Builtin command entry */
typedef struct builtin_s {
    const char *name;  /**< Command name */
    const char *doc;   /**< Help documentation */
    int (*func)(int argc, char **argv);  /**< Handler function */
} builtin;

/** Array of all builtin commands */
extern builtin builtins[];

/** Number of builtin commands */
extern const size_t builtins_count;

/* ============================================================================
 * Builtin Command Handlers
 * ============================================================================ */

int bin_exit(int argc, char **argv);      /**< @brief Exit the shell */
int bin_help(int argc, char **argv);      /**< @brief Display help */
int bin_cd(int argc, char **argv);        /**< @brief Change directory */
int bin_pwd(int argc, char **argv);       /**< @brief Print working directory */
int bin_history(int argc, char **argv);   /**< @brief Command history */
int bin_alias(int argc, char **argv);     /**< @brief Define aliases */
int bin_unalias(int argc, char **argv);   /**< @brief Remove aliases */
int bin_clear(int argc, char **argv);     /**< @brief Clear screen */
int bin_terminal(int argc, char **argv);  /**< @brief Terminal info */
int bin_type(int argc, char **argv);      /**< @brief Show command type */
int bin_unset(int argc, char **argv);     /**< @brief Unset variables */
int bin_echo(int argc, char **argv);      /**< @brief Echo arguments */
int bin_printf(int argc, char **argv);    /**< @brief Formatted output */
int bin_export(int argc, char **argv);    /**< @brief Export variables */
int bin_source(int argc, char **argv);    /**< @brief Source a script */
int bin_test(int argc, char **argv);      /**< @brief Test conditions */
int bin_read(int argc, char **argv);      /**< @brief Read input */
int bin_eval(int argc, char **argv);      /**< @brief Evaluate arguments */
int bin_true(int argc, char **argv);      /**< @brief Return success */
int bin_false(int argc, char **argv);     /**< @brief Return failure */
int bin_set(int argc, char **argv);       /**< @brief Set options/variables */
int bin_shift(int argc, char **argv);     /**< @brief Shift arguments */
int bin_break(int argc, char **argv);     /**< @brief Break from loop */
int bin_continue(int argc, char **argv);  /**< @brief Continue loop */
int bin_return(int argc, char **argv);    /**< @brief Return from function */
int bin_return_value(int argc, char **argv); /**< @brief Return with value */
int bin_trap(int argc, char **argv);      /**< @brief Set signal traps */
int bin_exec(int argc, char **argv);      /**< @brief Execute command */
int bin_wait(int argc, char **argv);      /**< @brief Wait for jobs */
int bin_umask(int argc, char **argv);     /**< @brief Set file mode mask */
int bin_ulimit(int argc, char **argv);    /**< @brief Resource limits */
int bin_times(int argc, char **argv);     /**< @brief Process times */
int bin_getopts(int argc, char **argv);   /**< @brief Parse options */
int bin_local(int argc, char **argv);     /**< @brief Local variables */
int bin_hash(int argc, char **argv);      /**< @brief Command hash table */
int bin_fc(int argc, char **argv);        /**< @brief Fix command (history) */
int bin_debug(int argc, char **argv);     /**< @brief Debug commands */

/* ============================================================================
 * Command Hash Table
 * ============================================================================ */

/** @brief Initialize the command hash table */
void init_command_hash(void);

/** @brief Free the command hash table */
void free_command_hash(void);

/**
 * @brief Check if a command is a builtin
 *
 * Searches the builtins table for a matching command name.
 *
 * @param name Command name to check
 * @return true if name is a builtin, false otherwise
 */
bool is_builtin(const char *name);

/**
 * @brief Find a command in PATH
 *
 * @param command Command name to search for
 * @return Full path or NULL (caller must free)
 */
char *find_command_in_path(const char *command);

/** Command hash table for PATH caching */
extern ht_strstr_t *command_hash;

#endif
