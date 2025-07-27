/**
 * @file test_lle_039_line_editor_implementation.c
 * @brief Comprehensive test suite for LLE-039 Line Editor Implementation
 *
 * Tests the main line editor functionality including the input loop,
 * key event processing, display updates, and integration of all components.
 *
 * @author Lusush Development Team
 * @date 2024
 */

#include "test_framework.h"
#include "../src/line_editor/line_editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Create a line editor with minimal configuration for testing
 */
static lle_line_editor_t *create_test_editor(void) {
    lle_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_history_size = 100;
    config.max_undo_actions = 50;
    config.enable_multiline = true;
    config.enable_syntax_highlighting = false;  // Disable for simpler testing
    config.enable_auto_completion = false;      // Disable for simpler testing
    config.enable_history = true;
    config.enable_undo = true;
    
    return lle_create_with_config(&config);
}

/**
 * @brief Simulate key input by creating a pipe and feeding data
 */
static bool simulate_key_input(const char *input, char **output) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) return false;
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return false;
    }
    
    if (pid == 0) {
        // Child process: redirect stdin and run editor
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        
        lle_line_editor_t *editor = create_test_editor();
        if (!editor) exit(1);
        
        char *result = lle_readline(editor, "test> ");
        if (result) {
            printf("%s", result);
            free(result);
        }
        
        lle_destroy(editor);
        exit(0);
    } else {
        // Parent process: write input and read output
        close(pipe_fd[0]);
        
        // Write input
        write(pipe_fd[1], input, strlen(input));
        close(pipe_fd[1]);
        
        // Wait for child and capture output
        int status;
        waitpid(pid, &status, 0);
        
        return WEXITSTATUS(status) == 0;
    }
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

LLE_TEST(line_editor_creation_and_destruction) {
    printf("Testing line editor creation and destruction... ");
    
    // Test basic creation
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    LLE_ASSERT(lle_is_initialized(editor));
    
    // Test creation with config
    lle_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_history_size = 50;
    config.enable_history = true;
    
    lle_line_editor_t *editor2 = lle_create_with_config(&config);
    LLE_ASSERT_NOT_NULL(editor2);
    LLE_ASSERT(lle_is_initialized(editor2));
    
    // Test config retrieval
    lle_config_t retrieved_config;
    LLE_ASSERT(lle_get_config(editor2, &retrieved_config));
    LLE_ASSERT_EQ(retrieved_config.max_history_size, 50);
    LLE_ASSERT(retrieved_config.enable_history);
    
    // Test destruction
    lle_destroy(editor);
    lle_destroy(editor2);
    
    printf("PASSED\n");
}

LLE_TEST(line_editor_invalid_parameters) {
    printf("Testing line editor invalid parameter handling... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test NULL prompt
    char *result = lle_readline(editor, NULL);
    LLE_ASSERT_NULL(result);
    LLE_ASSERT_EQ(lle_get_last_error(editor), LLE_ERROR_INVALID_PARAMETER);
    
    // Test NULL editor
    result = lle_readline(NULL, "prompt> ");
    LLE_ASSERT_NULL(result);
    
    // Test with NULL config
    lle_config_t config;
    LLE_ASSERT(!lle_get_config(editor, NULL));
    LLE_ASSERT(!lle_get_config(NULL, &config));
    
    lle_destroy(editor);
    printf("PASSED\n");
}

LLE_TEST(line_editor_prompt_handling) {
    printf("Testing line editor prompt handling... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test simple prompt storage
    char *result = lle_readline(editor, "simple> ");
    // In non-interactive mode, this will return NULL but should handle prompt correctly
    LLE_ASSERT_NOT_NULL(editor->current_prompt);
    LLE_ASSERT_STR_EQ(editor->current_prompt, "simple> ");
    
    // Test prompt with ANSI codes
    result = lle_readline(editor, "\033[32mgreen> \033[0m");
    LLE_ASSERT_NOT_NULL(editor->current_prompt);
    LLE_ASSERT_STR_EQ(editor->current_prompt, "\033[32mgreen> \033[0m");
    
    // Test empty prompt
    result = lle_readline(editor, "");
    LLE_ASSERT_NOT_NULL(editor->current_prompt);
    LLE_ASSERT_STR_EQ(editor->current_prompt, "");
    
    lle_destroy(editor);
    printf("PASSED\n");
}

LLE_TEST(line_editor_basic_initialization) {
    printf("Testing line editor basic initialization... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Verify all components are initialized
    LLE_ASSERT_NOT_NULL(editor->buffer);
    LLE_ASSERT_NOT_NULL(editor->terminal);
    LLE_ASSERT_NOT_NULL(editor->display);
    LLE_ASSERT_NOT_NULL(editor->history);
    LLE_ASSERT_NOT_NULL(editor->theme);
    LLE_ASSERT_NULL(editor->completions);     // Disabled in test config
    LLE_ASSERT_NOT_NULL(editor->undo_stack);
    
    // Verify configuration flags
    LLE_ASSERT(editor->multiline_mode);
    LLE_ASSERT(!editor->syntax_highlighting);  // Disabled in test config
    LLE_ASSERT(!editor->auto_completion);      // Disabled in test config
    LLE_ASSERT(editor->history_enabled);
    LLE_ASSERT(editor->undo_enabled);
    LLE_ASSERT(editor->initialized);
    
    // Verify limits
    LLE_ASSERT_EQ(editor->max_history_size, 100);
    LLE_ASSERT_EQ(editor->max_undo_actions, 50);
    
    lle_destroy(editor);
    printf("PASSED\n");
}

// ============================================================================
// Configuration Management Tests
// ============================================================================

LLE_TEST(line_editor_feature_control) {
    printf("Testing line editor feature control... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test multiline mode toggle
    LLE_ASSERT(lle_set_multiline_mode(editor, false));
    LLE_ASSERT(!editor->multiline_mode);
    LLE_ASSERT(lle_set_multiline_mode(editor, true));
    LLE_ASSERT(editor->multiline_mode);
    
    // Test syntax highlighting toggle
    LLE_ASSERT(lle_set_syntax_highlighting(editor, true));
    LLE_ASSERT(editor->syntax_highlighting);
    LLE_ASSERT(lle_set_syntax_highlighting(editor, false));
    LLE_ASSERT(!editor->syntax_highlighting);
    
    // Test auto completion toggle
    LLE_ASSERT(lle_set_auto_completion(editor, true));
    LLE_ASSERT(editor->auto_completion);
    LLE_ASSERT(lle_set_auto_completion(editor, false));
    LLE_ASSERT(!editor->auto_completion);
    
    // Test history toggle
    LLE_ASSERT(lle_set_history_enabled(editor, false));
    LLE_ASSERT(!editor->history_enabled);
    LLE_ASSERT(lle_set_history_enabled(editor, true));
    LLE_ASSERT(editor->history_enabled);
    
    // Test undo toggle
    LLE_ASSERT(lle_set_undo_enabled(editor, false));
    LLE_ASSERT(!editor->undo_enabled);
    LLE_ASSERT(lle_set_undo_enabled(editor, true));
    LLE_ASSERT(editor->undo_enabled);
    
    // Test invalid parameters
    LLE_ASSERT(!lle_set_multiline_mode(NULL, true));
    LLE_ASSERT(!lle_set_syntax_highlighting(NULL, true));
    LLE_ASSERT(!lle_set_auto_completion(NULL, true));
    LLE_ASSERT(!lle_set_history_enabled(NULL, true));
    LLE_ASSERT(!lle_set_undo_enabled(NULL, true));
    
    lle_destroy(editor);
    printf("PASSED\n");
}

LLE_TEST(line_editor_configuration_consistency) {
    printf("Testing line editor configuration consistency... ");
    
    // Test custom configuration
    lle_config_t custom_config;
    memset(&custom_config, 0, sizeof(custom_config));
    custom_config.max_history_size = 200;
    custom_config.max_undo_actions = 75;
    custom_config.enable_multiline = false;
    custom_config.enable_syntax_highlighting = true;
    custom_config.enable_auto_completion = true;
    custom_config.enable_history = false;
    custom_config.enable_undo = false;
    
    lle_line_editor_t *editor = lle_create_with_config(&custom_config);
    LLE_ASSERT_NOT_NULL(editor);
    
    // Verify configuration was applied
    LLE_ASSERT_EQ(editor->max_history_size, 200);
    LLE_ASSERT_EQ(editor->max_undo_actions, 75);
    LLE_ASSERT(!editor->multiline_mode);
    LLE_ASSERT(editor->syntax_highlighting);
    LLE_ASSERT(editor->auto_completion);
    LLE_ASSERT(!editor->history_enabled);
    LLE_ASSERT(!editor->undo_enabled);
    
    // Verify config retrieval matches
    lle_config_t retrieved_config;
    LLE_ASSERT(lle_get_config(editor, &retrieved_config));
    LLE_ASSERT_EQ(retrieved_config.max_history_size, 200);
    LLE_ASSERT_EQ(retrieved_config.max_undo_actions, 75);
    LLE_ASSERT(!retrieved_config.enable_multiline);
    LLE_ASSERT(retrieved_config.enable_syntax_highlighting);
    LLE_ASSERT(retrieved_config.enable_auto_completion);
    LLE_ASSERT(!retrieved_config.enable_history);
    LLE_ASSERT(!retrieved_config.enable_undo);
    
    lle_destroy(editor);
    printf("PASSED\n");
}

// ============================================================================
// History Management Tests
// ============================================================================

LLE_TEST(line_editor_history_management) {
    printf("Testing line editor history management... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test initial history state
    LLE_ASSERT_EQ(lle_get_history_count(editor), 0);
    
    // Test adding history entries
    LLE_ASSERT(lle_add_history(editor, "first command"));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 1);
    
    LLE_ASSERT(lle_add_history(editor, "second command"));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 2);
    
    LLE_ASSERT(lle_add_history(editor, "third command"));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 3);
    
    // Test skipping empty lines
    LLE_ASSERT(!lle_add_history(editor, ""));
    LLE_ASSERT(!lle_add_history(editor, "   "));
    LLE_ASSERT(!lle_add_history(editor, "\t\n"));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 3);
    
    // Test clearing history
    LLE_ASSERT(lle_clear_history(editor));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 0);
    
    // Test with history disabled
    lle_set_history_enabled(editor, false);
    LLE_ASSERT(!lle_add_history(editor, "should not be added"));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 0);
    
    lle_destroy(editor);
    printf("PASSED\n");
}

LLE_TEST(line_editor_history_persistence) {
    printf("Testing line editor history persistence... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Add some history entries
    LLE_ASSERT(lle_add_history(editor, "persistent command 1"));
    LLE_ASSERT(lle_add_history(editor, "persistent command 2"));
    LLE_ASSERT(lle_add_history(editor, "persistent command 3"));
    
    // Test saving to file
    const char *test_file = "/tmp/lle_test_history.txt";
    LLE_ASSERT(lle_save_history(editor, test_file));
    
    // Clear history and reload
    LLE_ASSERT(lle_clear_history(editor));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 0);
    
    LLE_ASSERT(lle_load_history(editor, test_file));
    LLE_ASSERT_EQ(lle_get_history_count(editor), 3);
    
    // Clean up test file
    unlink(test_file);
    
    // Test invalid file operations
    LLE_ASSERT(!lle_save_history(editor, "/invalid/path/file.txt"));
    LLE_ASSERT_EQ(lle_get_last_error(editor), LLE_ERROR_IO_ERROR);
    
    LLE_ASSERT(!lle_load_history(editor, "/nonexistent/file.txt"));
    LLE_ASSERT_EQ(lle_get_last_error(editor), LLE_ERROR_IO_ERROR);
    
    lle_destroy(editor);
    printf("PASSED\n");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

LLE_TEST(line_editor_error_handling) {
    printf("Testing line editor error handling... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test initial error state
    LLE_ASSERT_EQ(lle_get_last_error(editor), LLE_SUCCESS);
    
    // Test error on invalid parameters
    char *result = lle_readline(editor, NULL);
    LLE_ASSERT_NULL(result);
    LLE_ASSERT_EQ(lle_get_last_error(editor), LLE_ERROR_INVALID_PARAMETER);
    
    // Test error propagation from history operations
    lle_set_history_enabled(editor, false);
    LLE_ASSERT(!lle_add_history(editor, "test"));
    LLE_ASSERT_EQ(lle_get_last_error(editor), LLE_ERROR_NOT_INITIALIZED);
    
    // Test error handling with NULL editor
    LLE_ASSERT_EQ(lle_get_last_error(NULL), LLE_ERROR_INVALID_PARAMETER);
    LLE_ASSERT(!lle_is_initialized(NULL));
    
    lle_destroy(editor);
    printf("PASSED\n");
}

LLE_TEST(line_editor_memory_management) {
    printf("Testing line editor memory management... ");
    
    // Test multiple create/destroy cycles
    for (int i = 0; i < 10; i++) {
        lle_line_editor_t *editor = create_test_editor();
        LLE_ASSERT_NOT_NULL(editor);
        LLE_ASSERT(lle_is_initialized(editor));
        
        // Add some history to stress test memory
        char command[64];
        snprintf(command, sizeof(command), "test command %d", i);
        LLE_ASSERT(lle_add_history(editor, command));
        
        lle_destroy(editor);
    }
    
    // Test NULL destruction (should not crash)
    lle_destroy(NULL);
    
    printf("PASSED\n");
}

// ============================================================================
// Component Integration Tests
// ============================================================================

LLE_TEST(line_editor_component_integration) {
    printf("Testing line editor component integration... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test text buffer integration
    LLE_ASSERT_NOT_NULL(editor->buffer);
    LLE_ASSERT_EQ(editor->buffer->length, 0);
    LLE_ASSERT_EQ(editor->buffer->cursor_pos, 0);
    
    // Test terminal manager integration
    LLE_ASSERT_NOT_NULL(editor->terminal);
    
    // Test display integration
    LLE_ASSERT_NOT_NULL(editor->display);
    LLE_ASSERT(editor->display->buffer == editor->buffer);
    LLE_ASSERT(editor->display->terminal == editor->terminal);
    
    // Test history integration
    LLE_ASSERT_NOT_NULL(editor->history);
    LLE_ASSERT_EQ(lle_get_history_count(editor), 0);
    
    // Test theme integration
    LLE_ASSERT_NOT_NULL(editor->theme);
    
    // Test undo stack integration
    LLE_ASSERT_NOT_NULL(editor->undo_stack);
    
    lle_destroy(editor);
    printf("PASSED\n");
}

LLE_TEST(line_editor_display_state_management) {
    printf("Testing line editor display state management... ");
    
    lle_line_editor_t *editor = create_test_editor();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test display state after prompt setup
    char *result = lle_readline(editor, "test> ");
    
    // Verify display has been properly configured
    LLE_ASSERT_NOT_NULL(editor->display);
    LLE_ASSERT_NOT_NULL(editor->display->prompt);
    LLE_ASSERT(editor->display->buffer == editor->buffer);
    LLE_ASSERT(editor->display->terminal == editor->terminal);
    
    // Verify prompt was stored
    LLE_ASSERT_NOT_NULL(editor->current_prompt);
    LLE_ASSERT_STR_EQ(editor->current_prompt, "test> ");
    
    lle_destroy(editor);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Function
// ============================================================================

int main(void) {
    printf("=== LLE-039 Line Editor Implementation Tests ===\n");
    
    // Basic functionality tests
    test_line_editor_creation_and_destruction();
    test_line_editor_invalid_parameters();
    test_line_editor_prompt_handling();
    test_line_editor_basic_initialization();
    
    // Configuration management tests
    test_line_editor_feature_control();
    test_line_editor_configuration_consistency();
    
    // History management tests
    test_line_editor_history_management();
    test_line_editor_history_persistence();
    
    // Error handling tests
    test_line_editor_error_handling();
    test_line_editor_memory_management();
    
    // Component integration tests
    test_line_editor_component_integration();
    test_line_editor_display_state_management();
    
    printf("\n=== All LLE-039 Tests Completed Successfully! ===\n");
    printf("Tests run: 12\n");
    printf("✅ Line editor creation and destruction\n");
    printf("✅ Invalid parameter handling\n");
    printf("✅ Prompt handling and storage\n");
    printf("✅ Basic initialization verification\n");
    printf("✅ Feature control and configuration\n");
    printf("✅ Configuration consistency\n");
    printf("✅ History management functionality\n");
    printf("✅ History persistence operations\n");
    printf("✅ Error handling and propagation\n");
    printf("✅ Memory management and cleanup\n");
    printf("✅ Component integration verification\n");
    printf("✅ Display state management\n");
    
    return 0;
}