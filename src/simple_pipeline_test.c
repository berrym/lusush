#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Simple implementation to test the pipeline concept directly

int execute_simple_pipeline(const char* cmd1, const char* cmd2) {
    printf("Executing pipeline: %s | %s\n", cmd1, cmd2);
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }
    
    // First command
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        return 1;
    } 
    else if (pid1 == 0) {
        // Child 1
        close(pipefd[0]);  // Close read end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        // Execute command
        execlp("/bin/sh", "sh", "-c", cmd1, NULL);
        perror("execlp");
        _exit(EXIT_FAILURE);
    }
    
    // Second command
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        return 1;
    }
    else if (pid2 == 0) {
        // Child 2
        close(pipefd[1]);  // Close write end
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        // Execute command
        execlp("/bin/sh", "sh", "-c", cmd2, NULL);
        perror("execlp");
        _exit(EXIT_FAILURE);
    }
    
    // Parent
    close(pipefd[0]);
    close(pipefd[1]);
    
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    printf("Commands completed: %s: %d, %s: %d\n", 
           cmd1, WEXITSTATUS(status1), 
           cmd2, WEXITSTATUS(status2));
    
    return WEXITSTATUS(status2);
}

int main() {
    printf("Testing pipeline execution\n");
    
    // Test a simple pipeline
    int result = execute_simple_pipeline("echo 'Hello World'", "grep Hello");
    printf("Pipeline execution result: %d\n", result);
    
    // Test another pipeline
    result = execute_simple_pipeline("ls -la", "grep '.c'");
    printf("Pipeline execution result: %d\n", result);
    
    printf("Test completed successfully\n");
    return 0;
}
