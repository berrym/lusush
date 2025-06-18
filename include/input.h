#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

void free_input_buffers(void);
char *get_input(FILE *);
char *get_input_complete(FILE *in);

#endif
