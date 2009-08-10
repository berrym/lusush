#include "ldefs.h"
#include "ltypes.h"

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

enum {  BUILTIN_CMD_EXIT=0,
        BUILTIN_CMD_CD,
        BUILTIN_CMD_PWD,
        BUILTIN_CMD_HISTORY,
        BUILTIN_CMD_CNT
};

/**
 * Builtin commands
 *      exit    exit program
 *      cd      change directory
 *      pwd     print working directory
 *      history print command history
 */
extern const char *builtin_cmds[BUILTIN_CMD_CNT];

extern void cd(const char *);
extern void pwd(void);

#if defined( USING_READLINE )
extern void history(void);
#else
extern void history(CMD *);
#endif

#endif
