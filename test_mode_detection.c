#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

int main() {
    printf("=== Mode Detection Test ===\n");
    
    bool stdin_tty = isatty(STDIN_FILENO);
    bool stdout_tty = isatty(STDOUT_FILENO);
    bool stderr_tty = isatty(STDERR_FILENO);
    
    printf("STDIN isatty:  %s\n", stdin_tty ? "TRUE" : "FALSE");
    printf("STDOUT isatty: %s\n", stdout_tty ? "TRUE" : "FALSE");
    printf("STDERR isatty: %s\n", stderr_tty ? "TRUE" : "FALSE");
    
    bool interactive = stdin_tty && stdout_tty;
    printf("Interactive mode: %s\n", interactive ? "TRUE" : "FALSE");
    
    printf("\nTest scenarios:\n");
    printf("1. Run directly: ./test_mode_detection\n");
    printf("   Should show: Interactive mode: TRUE\n");
    printf("\n");
    printf("2. Run with pipe: echo '' | ./test_mode_detection\n");
    printf("   Should show: Interactive mode: FALSE\n");
    printf("\n");
    printf("3. Run with redirect: ./test_mode_detection > output.txt\n");
    printf("   Should show: Interactive mode: FALSE (in output.txt)\n");
    
    return 0;
}