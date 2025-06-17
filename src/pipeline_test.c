#include "../include/node.h"
#include "../include/exec.h"
#include "../include/strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock required global variables
bool parsing_alias = false;
bool exit_flag = false;
bool exec_compound_command = false;

// Function stub for unset_alias
int unset_alias(const char *name) {
    printf("Mock: unset_alias(%s)\n", name);
    return 0;
}

// Simple test driver for the execute_pipeline function
int main() {
    printf("Testing new pipeline execution implementation\n");
    
    // Create a simple command: echo "Hello World"
    node_t *command = new_node(NODE_COMMAND);
    if (!command) {
        fprintf(stderr, "Failed to create command node\n");
        return 1;
    }
    
    node_t *cmd_name = new_node(NODE_VAR);
    if (!cmd_name) {
        fprintf(stderr, "Failed to create cmd name node\n");
        free_node_tree(command);
        return 1;
    }
    set_node_val_str(cmd_name, "echo");
    
    node_t *cmd_arg = new_node(NODE_VAR);
    if (!cmd_arg) {
        fprintf(stderr, "Failed to create cmd arg node\n");
        free_node_tree(cmd_name);
        free_node_tree(command);
        return 1;
    }
    set_node_val_str(cmd_arg, "Hello World");
    
    add_child_node(command, cmd_name);
    add_child_node(command, cmd_arg);
    
    // Execute the command
    printf("Executing simple command\n");
    int status = do_basic_command(command);
    printf("Command execution completed with status: %d\n", status);
    
    // Clean up
    free_node_tree(command);
    
    printf("\n\nTest complete\n");
    
    return 0;
}
