/*
 * Comprehensive Keybinding Test for Lusush Line Editor
 * 
 * This program tests the readline-compatible keybinding functionality:
 * - Ctrl+A (move to beginning of line)
 * - Ctrl+E (move to end of line) 
 * - Ctrl+R (reverse history search)
 * - Other standard readline keybindings
 * 
 * Usage: ./test_keybindings
 * 
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "src/line_editor/line_editor.h"

void print_help() {
    printf("Lusush Line Editor Keybinding Test\n");
    printf("==================================\n\n");
    
    printf("This program tests readline-compatible keybindings:\n\n");
    
    printf("Navigation:\n");
    printf("  Ctrl+A        Move cursor to beginning of line\n");
    printf("  Ctrl+E        Move cursor to end of line\n");
    printf("  Ctrl+B        Move cursor left (same as Left Arrow)\n");
    printf("  Ctrl+F        Move cursor right (same as Right Arrow)\n");
    printf("  Alt+B         Move cursor backward one word\n");
    printf("  Alt+F         Move cursor forward one word\n\n");
    
    printf("History:\n");
    printf("  Ctrl+P        Previous history entry (same as Up Arrow)\n");
    printf("  Ctrl+N        Next history entry (same as Down Arrow)\n");
    printf("  Ctrl+R        Reverse incremental search\n\n");
    
    printf("Editing:\n");
    printf("  Ctrl+K        Kill text from cursor to end of line\n");
    printf("  Ctrl+U        Kill entire line\n");
    printf("  Ctrl+W        Kill word backward\n");
    printf("  Alt+D         Delete word forward\n");
    printf("  Ctrl+H        Backspace (same as Backspace key)\n");
    printf("  Ctrl+D        Delete character (or EOF if line empty)\n\n");
    
    printf("Control:\n");
    printf("  Ctrl+L        Clear screen and redraw\n");
    printf("  Ctrl+G        Cancel current operation\n");
    printf("  Ctrl+C        Send SIGINT (handled by shell)\n");
    printf("  Enter         Accept line\n\n");
    
    printf("Test Commands:\n");
    printf("  help          Show this help\n");
    printf("  test-nav      Test navigation keybindings\n");
    printf("  test-edit     Test editing keybindings\n");
    printf("  test-history  Test history keybindings\n");
    printf("  exit          Exit the test program\n\n");
}

void add_test_history(lle_line_editor_t *editor) {
    if (!editor->history) return;
    
    // Add comprehensive test history for Ctrl+R testing
    lle_history_add(editor->history, "echo hello world", false);
    lle_history_add(editor->history, "ls -la /tmp", false);
    lle_history_add(editor->history, "grep pattern file.txt", false);
    lle_history_add(editor->history, "find . -name '*.c'", false);
    lle_history_add(editor->history, "make clean && make", false);
    lle_history_add(editor->history, "git status", false);
    lle_history_add(editor->history, "cat /proc/cpuinfo", false);
    lle_history_add(editor->history, "echo test message", false);
    lle_history_add(editor->history, "ps aux | grep lusush", false);
    lle_history_add(editor->history, "tail -f /var/log/messages", false);
    
    printf("Added 10 test history entries for Ctrl+R testing\n");
}

void test_navigation_help() {
    printf("\nNavigation Keybinding Test:\n");
    printf("---------------------------\n");
    printf("1. Type some text: 'Hello World Test'\n");
    printf("2. Press Ctrl+A - cursor should move to beginning\n");
    printf("3. Press Ctrl+E - cursor should move to end\n");
    printf("4. Press Ctrl+A then Right Arrow a few times\n");
    printf("5. Press Alt+F - cursor should jump forward one word\n");
    printf("6. Press Alt+B - cursor should jump backward one word\n");
    printf("7. Press Enter when done testing\n\n");
}

void test_editing_help() {
    printf("\nEditing Keybinding Test:\n");
    printf("------------------------\n");
    printf("1. Type: 'This is a test line for editing'\n");
    printf("2. Press Ctrl+A to go to beginning\n");
    printf("3. Press Alt+F to move forward one word\n");
    printf("4. Press Ctrl+K - should kill from cursor to end\n");
    printf("5. Type new text to replace\n");
    printf("6. Press Ctrl+U - should clear entire line\n");
    printf("7. Type: 'word1 word2 word3'\n");
    printf("8. Press Ctrl+W - should delete last word\n");
    printf("9. Press Enter when done testing\n\n");
}

void test_history_help() {
    printf("\nHistory Keybinding Test:\n");
    printf("------------------------\n");
    printf("1. Press Ctrl+P or Up Arrow - navigate through history\n");
    printf("2. Press Ctrl+N or Down Arrow - navigate forward\n");
    printf("3. Press Ctrl+R and type 'echo' - should search history\n");
    printf("4. Press Ctrl+R again to find next match\n");
    printf("5. Press Enter to accept, or Ctrl+G to cancel\n");
    printf("6. Try searching for 'git', 'grep', 'find', etc.\n");
    printf("7. Type 'done' and press Enter when finished\n\n");
}

int main() {
    printf("Lusush Line Editor Comprehensive Keybinding Test\n");
    printf("===============================================\n\n");
    
    // Create line editor with default config
    lle_line_editor_t *editor = lle_create();
    if (!editor) {
        fprintf(stderr, "Error: Failed to create line editor\n");
        return 1;
    }
    
    // Add test history entries
    add_test_history(editor);
    
    printf("Type 'help' for keybinding reference, or start testing immediately.\n");
    printf("Type 'exit' to quit the test program.\n\n");
    
    char *result;
    int command_count = 0;
    
    while ((result = lle_readline(editor, "keybind-test> ")) != NULL) {
        command_count++;
        
        // Handle special test commands
        if (strcmp(result, "help") == 0) {
            print_help();
        }
        else if (strcmp(result, "test-nav") == 0) {
            test_navigation_help();
        }
        else if (strcmp(result, "test-edit") == 0) {
            test_editing_help();
        }
        else if (strcmp(result, "test-history") == 0) {
            test_history_help();
        }
        else if (strcmp(result, "exit") == 0) {
            free(result);
            break;
        }
        else if (strlen(result) == 0) {
            printf("(empty line - try typing some text and using keybindings)\n");
        }
        else {
            printf("Command %d: '%s'\n", command_count, result);
            printf("Length: %zu characters\n", strlen(result));
            
            // Provide feedback on common test phrases
            if (strstr(result, "Hello World") != NULL) {
                printf("Good! Try Ctrl+A and Ctrl+E to move cursor to beginning/end.\n");
            }
            else if (strstr(result, "test") != NULL) {
                printf("Nice! Try Ctrl+K to kill to end of line, or Ctrl+U to clear all.\n");
            }
        }
        
        // Add command to history (except test commands)
        if (editor->history && strlen(result) > 0 && 
            strcmp(result, "help") != 0 && 
            strcmp(result, "test-nav") != 0 &&
            strcmp(result, "test-edit") != 0 &&
            strcmp(result, "test-history") != 0) {
            lle_history_add(editor->history, result, false);
        }
        
        free(result);
        printf("\n");
    }
    
    printf("\nKeybinding test complete. Commands entered: %d\n", command_count);
    
    if (command_count > 0) {
        printf("Summary:\n");
        printf("- If Ctrl+A/Ctrl+E moved the cursor correctly, navigation is working!\n");
        printf("- If Ctrl+R opened history search, reverse search is working!\n");
        printf("- If Ctrl+K/Ctrl+U deleted text correctly, editing is working!\n");
        printf("- If any keybindings didn't work, please report the issue.\n");
    }
    
    // Cleanup
    lle_destroy(editor);
    
    printf("\nThank you for testing the Lusush Line Editor keybindings!\n");
    return 0;
}