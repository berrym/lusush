#include "../include/lusush.h"

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

// Global variables for special shell variables
int last_exit_status = 0;
pid_t shell_pid = 0;
char **shell_argv = NULL;
int shell_argc = 0;
pid_t last_background_pid = 0;
bool shell_argv_is_dynamic =
    false; // Track if shell_argv is dynamically allocated
