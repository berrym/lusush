#ifndef __ENV_H__
#define __ENV_H__

/**
 * Default environment variables
 *      ENV_HOME        users' home directory
 *      ENV_PATH        users' path to programs
 *      ENV_PROMPT      users' shell prompt
 *      ENV_SHELL       users' login shell
 *      ENV_LOGNAME     users' login name
 *      ENV_MAIL        users' email address
 */
extern char *ENV_HOME;
extern char *ENV_PATH;
extern char *ENV_PROMPT;
extern char *ENV_SHELL;
extern char *ENV_LOGNAME;
extern char *ENV_MAIL;

extern int SHELL_TYPE;

extern void env_init(char **);
extern void global_cleanup(void);

#ifdef SHELL_TYPE_LOGIN
#undef SHELL_TYPE_LOGIN
#endif
#define SHELL_TYPE_LOGIN 1

#ifdef SHELL_TYPE_INTERACTIVE
#undef SHELL_TYPE_INTERACTIVE
#endif
#define SHELL_TYPE_INTERACTIVE 2

#endif
