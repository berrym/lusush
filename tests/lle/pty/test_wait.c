#include "pty_test_harness.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    pty_session_t *session = NULL;
    if (pty_session_create(&session, 80, 24) != 0) {
        printf("Failed to create PTY\n");
        return 1;
    }
    
    if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
        printf("Failed to spawn lusush\n");
        pty_session_close(session);
        return 1;
    }
    
    printf("Child PID: %d\n", session->child_pid);
    
    // Try multiple reads with delays
    for (int i = 0; i < 5; i++) {
        sleep(1);
        pty_session_read_output(session, 500);
        printf("Read attempt %d: captured %zu bytes\n", i+1, session->output_length);
        if (session->output_length > 0) {
            printf("Output: %.*s\n", (int)session->output_length, session->output_buffer);
            break;
        }
    }
    
    // Check if child is still running
    printf("Child exited: %s\n", session->child_exited ? "yes" : "no");
    if (session->child_exited) {
        printf("Exit code: %d\n", session->child_exit_code);
    }
    
    pty_session_close(session);
    return 0;
}
