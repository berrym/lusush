#include "../include/lusush.h"
#include <sys/types.h>
#include <unistd.h>

// Global variables for special shell variables
int last_exit_status = 0;
pid_t shell_pid = 0;
char **shell_argv = NULL;
int shell_argc = 0;
pid_t last_background_pid = 0;
