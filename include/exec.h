#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "node.h"

char *search_path(char *);
int do_exec_cmd(int, char **);
int do_command(struct node *);

#endif
