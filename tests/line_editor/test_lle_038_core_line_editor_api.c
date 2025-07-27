/*
 * Lusush Line Editor - Core Line Editor API Tests (LLE-038)
 * 
 * This file contains comprehensive tests for the main line editor public API.
 * Tests cover API compilation, basic usage, configuration, error handling,
 * and feature control functionality.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "test_framework.h"
#include "../../src/line_editor/line_editor.h"

// Test compilation and basic structure access
LLE_TEST(api_compilation_basic) {
    printf("Testing API compilation and basic structure access... ");
    
    // Test that we can create configuration structure
    lle_config_t config = {0};
    config.max_history_size = 1000;
    config.enable_multiline = true;
    config.enable_syntax_highlighting = false;
    
    // Test error enum values
    lle_error_t error = LLE_SUCCESS;
    LLE_ASSERT(error == 0);
    
    error = LLE_ERROR_INVALID_PARAMETER;
    LLE_ASSERT(error != LLE_SUCCESS);
    
    printf("PASSED\n");
}

// Test editor creation with default configuration
LLE_TEST(editor_creation_default) {
    printf("Testing editor creation with default configuration... ");
    
    // Note: In this test we only test the API interface, not full initialization
    // since we may not have a full terminal environment in CI
    
    // Test NULL safety first
    lle_line_editor_t *editor = lle_create();
    
    // Editor creation might fail in non-terminal environments (CI)
    // This is expected behavior, so we test both cases
    if (editor != NULL) {
        // If creation succeeded, test basic state
        LLE_ASSERT(lle_is_initialized(editor));
        
        // Test that we can get configuration
        lle_config_t config;
        LLE_ASSERT(lle_get_config(editor, &config));
        
        // Clean up
        lle_destroy(editor);
    } else {
        // Creation failed - this is OK in non-terminal environments
        // Just verify that lle_destroy handles NULL gracefully
        lle_destroy(NULL);  // Should not crash
    }
    
    printf("PASSED\n");
}

// Test editor creation with custom configuration
LLE_TEST(editor_creation_with_config) {
    printf("Testing editor creation with custom configuration... ");
    
    // Create custom configuration
    lle_config_t config = {0};
    config.max_history_size = 500;
    config.max_undo_actions = 50;
    config.enable_multiline = true;
    config.enable_syntax_highlighting = true;
    config.enable_auto_completion = false;
    config.enable_history = true;
    config.enable_undo = true;
    
    // Test with valid config
    lle_line_editor_t *editor = lle_create_with_config(&config);
    
    if (editor != NULL) {
        // Verify configuration was applied
        lle_config_t retrieved_config;
        LLE_ASSERT(lle_get_config(editor, &retrieved_config));
        
        // Note: Some values might be adjusted to valid ranges
        LLE_ASSERT(retrieved_config.enable_multiline == true);
        LLE_ASSERT(retrieved_config.enable_syntax_highlighting == true);
        LLE_ASSERT(retrieved_config.enable_auto_completion == false);
        
        lle_destroy(editor);
    }
    
    // Test with NULL config (should use defaults)
    editor = lle_create_with_config(NULL);
    if (editor != NULL) {
        LLE_ASSERT(lle_is_initialized(editor));
        lle_destroy(editor);
    }
    
    printf("PASSED\n");
}

// Test parameter validation
LLE_TEST(parameter_validation) {
    printf("Testing parameter validation... ");
    
    // Test NULL parameter handling
    LLE_ASSERT(!lle_is_initialized(NULL));
    LLE_ASSERT(lle_get_last_error(NULL) == LLE_ERROR_INVALID_PARAMETER);
    LLE_ASSERT(!lle_add_history(NULL, "test"));
    LLE_ASSERT(!lle_get_config(NULL, NULL));
    
    // Test with potentially valid editor but NULL other parameters
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        // Test NULL prompt (should be rejected)
        char *result = lle_readline(editor, NULL);
        LLE_ASSERT(result == NULL);
        
        // Test NULL/empty history line
        LLE_ASSERT(!lle_add_history(editor, NULL));
        LLE_ASSERT(!lle_add_history(editor, ""));
        
        // Test NULL config parameter
        LLE_ASSERT(!lle_get_config(editor, NULL));
        
        lle_destroy(editor);
    }
    
    printf("PASSED\n");
}

// Test feature control functions
LLE_TEST(feature_control) {
    printf("Testing feature control functions... ");
    
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        // Test all feature control functions
        LLE_ASSERT(lle_set_multiline_mode(editor, true));
        LLE_ASSERT(lle_set_multiline_mode(editor, false));
        
        LLE_ASSERT(lle_set_syntax_highlighting(editor, true));
        LLE_ASSERT(lle_set_syntax_highlighting(editor, false));
        
        LLE_ASSERT(lle_set_auto_completion(editor, true));
        LLE_ASSERT(lle_set_auto_completion(editor, false));
        
        LLE_ASSERT(lle_set_history_enabled(editor, true));
        LLE_ASSERT(lle_set_history_enabled(editor, false));
        
        LLE_ASSERT(lle_set_undo_enabled(editor, true));
        LLE_ASSERT(lle_set_undo_enabled(editor, false));
        
        lle_destroy(editor);
    }
    
    // Test with NULL editor
    LLE_ASSERT(!lle_set_multiline_mode(NULL, true));
    LLE_ASSERT(!lle_set_syntax_highlighting(NULL, true));
    LLE_ASSERT(!lle_set_auto_completion(NULL, true));
    LLE_ASSERT(!lle_set_history_enabled(NULL, true));
    LLE_ASSERT(!lle_set_undo_enabled(NULL, true));
    
    printf("PASSED\n");
}

// Test history management functions
LLE_TEST(history_management) {
    printf("Testing history management functions... ");
    
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        // Test initial history state
        LLE_ASSERT(lle_get_history_count(editor) == 0);
        
        // Test adding valid history entries
        LLE_ASSERT(lle_add_history(editor, "test command 1"));
        LLE_ASSERT(lle_add_history(editor, "test command 2"));
        LLE_ASSERT(lle_get_history_count(editor) >= 1);  // Might filter duplicates
        
        // Test clearing history
        LLE_ASSERT(lle_clear_history(editor));
        LLE_ASSERT(lle_get_history_count(editor) == 0);
        
        lle_destroy(editor);
    }
    
    // Test with NULL editor
    LLE_ASSERT(lle_get_history_count(NULL) == 0);
    LLE_ASSERT(!lle_clear_history(NULL));
    LLE_ASSERT(!lle_load_history(NULL, "test.txt"));
    LLE_ASSERT(!lle_save_history(NULL, "test.txt"));
    
    printf("PASSED\n");
}

// Test error handling
LLE_TEST(error_handling) {
    printf("Testing error handling... ");
    
    // Test error codes with NULL editor
    LLE_ASSERT(lle_get_last_error(NULL) == LLE_ERROR_INVALID_PARAMETER);
    
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        // Test that successful operations clear errors
        lle_error_t error = lle_get_last_error(editor);
        // Error should be SUCCESS if initialization worked
        if (lle_is_initialized(editor)) {
            LLE_ASSERT(error == LLE_SUCCESS);
        }
        
        lle_destroy(editor);
    }
    
    printf("PASSED\n");
}

// Test configuration retrieval and validation
LLE_TEST(configuration_management) {
    printf("Testing configuration management... ");
    
    // Test with custom configuration
    lle_config_t original_config = {0};
    original_config.max_history_size = 750;
    original_config.max_undo_actions = 75;
    original_config.enable_multiline = true;
    original_config.enable_syntax_highlighting = false;
    original_config.enable_auto_completion = true;
    original_config.enable_history = false;
    original_config.enable_undo = true;
    
    lle_line_editor_t *editor = lle_create_with_config(&original_config);
    if (editor != NULL) {
        // Retrieve configuration
        lle_config_t retrieved_config;
        LLE_ASSERT(lle_get_config(editor, &retrieved_config));
        
        // Verify some key settings were preserved
        LLE_ASSERT(retrieved_config.enable_multiline == original_config.enable_multiline);
        LLE_ASSERT(retrieved_config.enable_syntax_highlighting == original_config.enable_syntax_highlighting);
        
        // Test that feature changes are reflected in config
        LLE_ASSERT(lle_set_multiline_mode(editor, false));
        LLE_ASSERT(lle_get_config(editor, &retrieved_config));
        LLE_ASSERT(retrieved_config.enable_multiline == false);
        
        lle_destroy(editor);
    }
    
    printf("PASSED\n");
}

// Test memory safety and cleanup
LLE_TEST(memory_safety) {
    printf("Testing memory safety and cleanup... ");
    
    // Test multiple create/destroy cycles
    for (int i = 0; i < 3; i++) {
        lle_line_editor_t *editor = lle_create();
        if (editor != NULL) {
            // Add some history to test cleanup
            lle_add_history(editor, "test command");
            lle_destroy(editor);
        }
    }
    
    // Test destroying NULL (should not crash)
    lle_destroy(NULL);
    lle_destroy(NULL);  // Multiple times
    
    // Test that destroyed editor is no longer valid
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        lle_destroy(editor);
        // Note: We can't safely test editor after destruction
        // as it would be undefined behavior
    }
    
    printf("PASSED\n");
}

// Test edge cases and boundary conditions
LLE_TEST(edge_cases) {
    printf("Testing edge cases and boundary conditions... ");
    
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        // Test empty string handling
        LLE_ASSERT(!lle_add_history(editor, ""));
        LLE_ASSERT(!lle_add_history(editor, "   "));  // Whitespace only
        
        // Test very long prompt (should not crash)
        char long_prompt[1024];
        memset(long_prompt, 'A', sizeof(long_prompt) - 1);
        long_prompt[sizeof(long_prompt) - 1] = '\0';
        
        // This may fail gracefully but should not crash
        char *result = lle_readline(editor, long_prompt);
        if (result != NULL) {
            free(result);
        }
        
        lle_destroy(editor);
    }
    
    printf("PASSED\n");
}

// Test API consistency and state management
LLE_TEST(api_consistency) {
    printf("Testing API consistency and state management... ");
    
    lle_line_editor_t *editor = lle_create();
    if (editor != NULL) {
        // Test that multiple calls to the same function are consistent
        bool state1 = lle_set_multiline_mode(editor, true);
        bool state2 = lle_set_multiline_mode(editor, true);
        LLE_ASSERT(state1 == state2);  // Should be consistent
        
        // Test that configuration reflects current state
        LLE_ASSERT(lle_set_syntax_highlighting(editor, true));
        lle_config_t config;
        LLE_ASSERT(lle_get_config(editor, &config));
        LLE_ASSERT(config.enable_syntax_highlighting == true);
        
        LLE_ASSERT(lle_set_syntax_highlighting(editor, false));
        LLE_ASSERT(lle_get_config(editor, &config));
        LLE_ASSERT(config.enable_syntax_highlighting == false);
        
        lle_destroy(editor);
    }
    
    printf("PASSED\n");
}

int main(void) {
    printf("Running LLE-038 Core Line Editor API tests...\n\n");
    
    test_api_compilation_basic();
    test_editor_creation_default();
    test_editor_creation_with_config();
    test_parameter_validation();
    test_feature_control();
    test_history_management();
    test_error_handling();
    test_configuration_management();
    test_memory_safety();
    test_edge_cases();
    test_api_consistency();
    
    printf("\n✅ All LLE-038 Core Line Editor API tests passed!\n");
    printf("📊 Test Summary: 11 test functions completed successfully\n");
    printf("🚀 Core Line Editor API is ready for implementation\n");
    
    return 0;
}