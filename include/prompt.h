#ifndef PROMPT_H
#define PROMPT_H

#include <stddef.h>

void build_prompt(void);
void rebuild_prompt(void);
void update_git_info(void);
void format_git_prompt(char *git_prompt, size_t size);

#endif
