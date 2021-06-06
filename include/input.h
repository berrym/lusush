#include <stdio.h>

#ifndef INPUT_H
#define INPUT_H

void free_input_buffers(void);

#if !defined(USING_READLINE)

char *get_input(FILE *);

#endif

#if defined(USING_EDITLINE)

#include <editline/readline.h>

char *get_input(FILE *);
char * rl_gets(const char *);

#endif

#if defined(USING_READLINE)

#include <readline/readline.h>
#include <readline/history.h>

char *get_input(FILE *);
char * rl_gets(const char *);

#endif

#endif
