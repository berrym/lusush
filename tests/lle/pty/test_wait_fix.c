#include "pty_test_harness.h"
#include <stdio.h>

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
    
    printf("Calling pty_wait_for_prompt...\n");
    if (pty_wait_for_prompt(session, 3000)) {
        printf("SUCCESS: Found prompt!\n");
        printf("Captured %zu bytes\n", session->output_length);
        printf("Output: %.*s\n", (int)(session->output_length < 100 ? session->output_length : 100), session->output_buffer);
    } else {
        printf("FAILED: Timeout waiting for prompt\n");
        printf("Captured %zu bytes\n", session->output_length);
    }
    
    pty_session_close(session);
    return 0;
}
