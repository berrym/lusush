#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

// Core input functions
void free_input_buffers(void);
char *get_input(FILE *in);

// Enhanced multiline input functions
char *ln_gets(void);
char *get_input_complete(FILE *in);
char *get_unified_input(FILE *in);

// Legacy compatibility (deprecated - use get_unified_input instead)
char *ln_gets_complete(void);

// Get current continuation prompt for multiline input
const char *lusush_get_current_continuation_prompt(void);

#endif
