#include "node.h"

#ifndef EXECUTOR_H
#define EXECUTOR_H

char *search_path(char *);
int do_exec_cmd(int, char **);
int do_command(struct node *);

#endif
