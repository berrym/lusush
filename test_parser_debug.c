/**
 * Test program for the new parser to debug integration issues
 */

#include "../include/parser_new_simple.h"
#include "../include/node.h"
#include "../include/scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Create a simple source for testing
    source_t src;
    src.buf = strdup("echo hello");
    src.bufsize = strlen(src.buf);
    src.pos = INIT_SRC_POS;
    src.curchar = 0;
    src.curline = 1;
    src.curlinestart = 0;
    src.wstart = 0;
    
    printf("Testing new parser with input: '%s'\n", src.buf);
    
    // Create parser
    parser_t *parser = parser_create(&src, NULL);
    if (!parser) {
        printf("ERROR: Failed to create parser\n");
        free(src.buf);
        return 1;
    }
    
    printf("Parser created successfully\n");
    
    // Try to parse
    node_t *cmd = parser_parse(parser);
    if (!cmd) {
        printf("ERROR: Failed to parse command\n");
    } else {
        printf("SUCCESS: Command parsed successfully\n");
        printf("Node type: %d\n", cmd->type);
        free_node_tree(cmd);
    }
    
    // Clean up
    parser_destroy(parser);
    free(src.buf);
    
    return 0;
}
