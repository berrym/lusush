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
    
    pty_wait_for_prompt(session, 3000);
    
    printf("=== OUTPUT (%zu bytes) ===\n", session->output_length);
    printf("%s\n", session->output_buffer);
    printf("=== END OUTPUT ===\n");
    
    printf("Contains '$ ': %s\n", strstr(session->output_buffer, "$ ") ? "YES" : "NO");
    
    pty_session_close(session);
    return 0;
}
