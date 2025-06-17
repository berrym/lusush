#ifndef CMD_SUBST_H
#define CMD_SUBST_H

#include "../include/expand.h"

/**
 * command_substitute_ctx:
 *     Perform command substitution (both `...` and $(...) forms)
 *     with expansion context support.
 * 
 * @param orig_cmd  The command string to substitute
 * @param ctx       The expansion context
 * @return          Newly allocated string with command output, or NULL on error
 */
char *command_substitute_ctx(char *orig_cmd, expand_ctx_t *ctx);

/**
 * command_substitute:
 *     Backward compatibility wrapper for command substitution.
 * 
 * @param orig_cmd  The command string to substitute
 * @return          Newly allocated string with command output, or NULL on error
 */
char *command_substitute(char *orig_cmd);

#endif /* CMD_SUBST_H */
