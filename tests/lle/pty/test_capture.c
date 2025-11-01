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
    
    pty_session_read_output(session, 500);
    
    printf("Output length: %zu\n", session->output_length);
    printf("Output (hex):\n");
    for (size_t i = 0; i < session->output_length && i < 200; i++) {
        printf("%02x ", (unsigned char)session->output_buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\nOutput (text):\n%.*s\n", (int)session->output_length, session->output_buffer);
    
    pty_session_close(session);
    return 0;
}
