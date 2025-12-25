#include "lusush.h"
#include "posix_history.h"

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

// Global shell state variables
bool exit_flag = false;
int last_exit_status = 0;
pid_t shell_pid = 0;
char **shell_argv = NULL;
int shell_argc = 0;
pid_t last_background_pid = 0;
bool shell_argv_is_dynamic =
    false; // Track if shell_argv is dynamically allocated

// Global POSIX history manager
posix_history_manager_t *global_posix_history = NULL;
