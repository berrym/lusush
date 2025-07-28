/**
 * @file test_lle_043_configuration_integration.c
 * @brief Test suite for LLE-043 Configuration Integration
 *
 * Tests the complete integration between LLE and the Lusush configuration system,
 * including configuration loading, validation, application to line editor instances,
 * dynamic updates, callback notifications, and individual setting management.
 *
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "../../src/line_editor/config.h"
#include "../../src/line_editor/line_editor.h"
#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// Test Helper Functions and Data
// ============================================================================

/**
 * @brief Test callback data structure
 */
typedef struct {
    int callback_count;
    bool callback_called;
    lle_lusush_config_t last_config;
} test_config_callback_data_t;

/**
 * @brief Test callback function for configuration change notifications
 */
static void test_config_callback(const lle_lusush_config_t *config, void *user_data) {
    test_config_callback_data_t *data = (test_config_callback_data_t *)user_data;
    if (data && config) {
        data->callback_count++;
        data->callback_called = true;
        lle_config_copy(&data->last_config, config);
    }
}

/**
 * @brief Create a test configuration with known values
 */
static void create_test_config(lle_lusush_config_t *config) {
    lle_config_init_defaults(config);
    
    // Override some defaults for testing
    config->multiline_mode = false;
    config->syntax_highlighting = false;
    config->history_size = 500;
    config->undo_levels = 50;
    config->theme_name = malloc(16);
    strcpy(config->theme_name, "test_theme");
}

// ============================================================================
// Configuration Initialization and Defaults Tests
// ============================================================================

LLE_TEST(config_init_defaults) {
    printf("Testing configuration initialization with defaults... ");
    
    lle_lusush_config_t config;
    memset(&config, 0xFF, sizeof(config));  // Fill with garbage
    
    // Test successful initialization
    LLE_ASSERT_EQ(lle_config_init_defaults(&config), LLE_CONFIG_SUCCESS);
    
    // Verify default boolean values (all features enabled)
    LLE_ASSERT(config.multiline_mode);
    LLE_ASSERT(config.syntax_highlighting);
    LLE_ASSERT(config.show_completions);
    LLE_ASSERT(config.history_enabled);
    LLE_ASSERT(config.undo_enabled);
    LLE_ASSERT(config.colors_enabled);
    LLE_ASSERT(config.theme_auto_detect);
    LLE_ASSERT(config.fuzzy_completion);
    LLE_ASSERT(!config.completion_case_sensitive);
    LLE_ASSERT(config.hints_enabled);
    LLE_ASSERT(!config.history_no_dups);
    LLE_ASSERT(!config.history_timestamps);
    
    // Verify default numeric values
    LLE_ASSERT_EQ(config.history_size, 1000);
    LLE_ASSERT_EQ(config.undo_levels, 100);
    LLE_ASSERT_EQ(config.max_completion_items, 50);
    LLE_ASSERT_EQ(config.buffer_initial_size, 256);
    LLE_ASSERT_EQ(config.display_cache_size, 1024);
    LLE_ASSERT_EQ(config.refresh_rate_ms, 16);
    
    // Verify string values are NULL
    LLE_ASSERT_NULL(config.theme_name);
    
    // Test NULL parameter
    LLE_ASSERT_EQ(lle_config_init_defaults(NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    printf("PASSED\n");
}

LLE_TEST(config_cleanup) {
    printf("Testing configuration cleanup... ");
    
    lle_lusush_config_t config;
    create_test_config(&config);
    
    // Verify string is allocated
    LLE_ASSERT_NOT_NULL(config.theme_name);
    LLE_ASSERT_STR_EQ(config.theme_name, "test_theme");
    
    // Clean up
    lle_config_cleanup(&config);
    
    // Verify string is freed and structure is zeroed
    LLE_ASSERT_NULL(config.theme_name);
    LLE_ASSERT_EQ(config.multiline_mode, false);
    LLE_ASSERT_EQ(config.history_size, 0);
    
    // Test NULL parameter (should not crash)
    lle_config_cleanup(NULL);
    
    printf("PASSED\n");
}

// ============================================================================
// Configuration Loading Tests
// ============================================================================

LLE_TEST(config_load_from_lusush) {
    printf("Testing configuration loading from Lusush system... ");
    
    lle_lusush_config_t config;
    lle_config_init_defaults(&config);
    
    // Test configuration loading
    lle_config_result_t result = lle_config_load(&config);
    
    // Should succeed even if Lusush config is not fully initialized
    // The function should use defaults for missing values
    LLE_ASSERT(result == LLE_CONFIG_SUCCESS);
    
    // Verify numeric values are within expected ranges
    LLE_ASSERT(config.history_size >= 10);
    LLE_ASSERT(config.history_size <= 50000);
    LLE_ASSERT(config.undo_levels >= 5);
    LLE_ASSERT(config.undo_levels <= 1000);
    
    // Test NULL parameter
    LLE_ASSERT_EQ(lle_config_load(NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_config_cleanup(&config);
    
    printf("PASSED\n");
}

// ============================================================================
// Configuration Validation Tests
// ============================================================================

LLE_TEST(config_validation) {
    printf("Testing configuration validation and correction... ");
    
    lle_lusush_config_t config;
    lle_config_init_defaults(&config);
    
    // Set invalid values
    config.history_size = 5;          // Too small
    config.undo_levels = 2000;        // Too large
    config.max_completion_items = 1;  // Too small
    config.refresh_rate_ms = 200;     // Too large
    
    // Validate and correct
    LLE_ASSERT_EQ(lle_config_validate(&config), LLE_CONFIG_SUCCESS);
    
    // Verify corrections
    LLE_ASSERT_EQ(config.history_size, 10);      // Corrected to minimum
    LLE_ASSERT_EQ(config.undo_levels, 1000);     // Corrected to maximum
    LLE_ASSERT_EQ(config.max_completion_items, 5); // Corrected to minimum
    LLE_ASSERT_EQ(config.refresh_rate_ms, 100);  // Corrected to maximum
    
    // Test empty string theme name handling
    config.theme_name = malloc(1);
    config.theme_name[0] = '\0';
    
    LLE_ASSERT_EQ(lle_config_validate(&config), LLE_CONFIG_SUCCESS);
    LLE_ASSERT_NULL(config.theme_name);  // Should be freed and set to NULL
    
    // Test NULL parameter
    LLE_ASSERT_EQ(lle_config_validate(NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    printf("PASSED\n");
}

// ============================================================================
// Configuration Copy and Comparison Tests
// ============================================================================

LLE_TEST(config_copy) {
    printf("Testing configuration copying... ");
    
    lle_lusush_config_t source, dest;
    create_test_config(&source);
    memset(&dest, 0, sizeof(dest));
    
    // Test successful copy
    LLE_ASSERT_EQ(lle_config_copy(&dest, &source), LLE_CONFIG_SUCCESS);
    
    // Verify all fields are copied
    LLE_ASSERT_EQ(dest.multiline_mode, source.multiline_mode);
    LLE_ASSERT_EQ(dest.history_size, source.history_size);
    LLE_ASSERT_NOT_NULL(dest.theme_name);
    LLE_ASSERT_STR_EQ(dest.theme_name, "test_theme");
    
    // Verify string is independently allocated
    LLE_ASSERT(dest.theme_name != source.theme_name);
    
    // Test NULL parameters
    LLE_ASSERT_EQ(lle_config_copy(NULL, &source), LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT_EQ(lle_config_copy(&dest, NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_config_cleanup(&source);
    lle_config_cleanup(&dest);
    
    printf("PASSED\n");
}

LLE_TEST(config_equals) {
    printf("Testing configuration comparison... ");
    
    lle_lusush_config_t config1, config2;
    create_test_config(&config1);
    
    // Test copying and equality
    LLE_ASSERT_EQ(lle_config_copy(&config2, &config1), LLE_CONFIG_SUCCESS);
    LLE_ASSERT(lle_config_equals(&config1, &config2));
    
    // Test difference in boolean field
    config2.multiline_mode = !config1.multiline_mode;
    LLE_ASSERT(!lle_config_equals(&config1, &config2));
    config2.multiline_mode = config1.multiline_mode;
    
    // Test difference in numeric field
    config2.history_size = config1.history_size + 1;
    LLE_ASSERT(!lle_config_equals(&config1, &config2));
    config2.history_size = config1.history_size;
    
    // Test difference in string field
    free(config2.theme_name);
    config2.theme_name = malloc(16);
    strcpy(config2.theme_name, "different");
    LLE_ASSERT(!lle_config_equals(&config1, &config2));
    
    // Test NULL string handling
    free(config2.theme_name);
    config2.theme_name = NULL;
    LLE_ASSERT(!lle_config_equals(&config1, &config2));
    
    // Test both NULL strings
    free(config1.theme_name);
    config1.theme_name = NULL;
    LLE_ASSERT(lle_config_equals(&config1, &config2));
    
    // Test NULL parameters
    LLE_ASSERT(!lle_config_equals(NULL, &config2));
    LLE_ASSERT(!lle_config_equals(&config1, NULL));
    
    // Clean up
    lle_config_cleanup(&config1);
    lle_config_cleanup(&config2);
    
    printf("PASSED\n");
}

// ============================================================================
// Configuration Callback System Tests
// ============================================================================

LLE_TEST(config_callback_registration) {
    printf("Testing configuration callback registration... ");
    
    test_config_callback_data_t callback_data = {0};
    
    // Test successful registration
    LLE_ASSERT_EQ(lle_config_register_callback(test_config_callback, &callback_data), LLE_CONFIG_SUCCESS);
    
    // Test NULL callback rejection
    LLE_ASSERT_EQ(lle_config_register_callback(NULL, &callback_data), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Test unregistration
    LLE_ASSERT_EQ(lle_config_unregister_callback(test_config_callback, &callback_data), LLE_CONFIG_SUCCESS);
    
    // Test unregistering non-existent callback
    LLE_ASSERT_EQ(lle_config_unregister_callback(test_config_callback, &callback_data), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    printf("PASSED\n");
}

LLE_TEST(config_callback_multiple_registration) {
    printf("Testing multiple configuration callback registration... ");
    
    test_config_callback_data_t data1 = {0}, data2 = {0};
    
    // Register multiple callbacks
    LLE_ASSERT_EQ(lle_config_register_callback(test_config_callback, &data1), LLE_CONFIG_SUCCESS);
    LLE_ASSERT_EQ(lle_config_register_callback(test_config_callback, &data2), LLE_CONFIG_SUCCESS);
    
    // Unregister specific callback
    LLE_ASSERT_EQ(lle_config_unregister_callback(test_config_callback, &data1), LLE_CONFIG_SUCCESS);
    
    // Other callback should still be registered
    LLE_ASSERT_EQ(lle_config_unregister_callback(test_config_callback, &data2), LLE_CONFIG_SUCCESS);
    
    printf("PASSED\n");
}

// ============================================================================
// Line Editor Integration Tests
// ============================================================================

LLE_TEST(config_apply_to_editor) {
    printf("Testing configuration application to line editor... ");
    
    // Create line editor with default configuration
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Create test configuration
    lle_lusush_config_t config;
    create_test_config(&config);
    
    // Apply configuration
    lle_config_result_t result = lle_config_apply((struct lle_line_editor *)editor, &config);
    
    // Should succeed in test environment
    LLE_ASSERT_EQ(result, LLE_CONFIG_SUCCESS);
    
    // Test NULL parameters
    LLE_ASSERT_EQ(lle_config_apply((struct lle_line_editor *)editor, NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT_EQ(lle_config_apply(NULL, &config), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_config_cleanup(&config);
    lle_destroy(editor);
    
    printf("PASSED\n");
}

LLE_TEST(config_reload) {
    printf("Testing configuration reload... ");
    
    // Create line editor
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test configuration reload
    lle_config_result_t result = lle_config_reload((struct lle_line_editor *)editor);
    
    // Should succeed even if Lusush config is not fully initialized
    LLE_ASSERT_EQ(result, LLE_CONFIG_SUCCESS);
    
    // Test NULL parameter
    LLE_ASSERT_EQ(lle_config_reload(NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_destroy(editor);
    
    printf("PASSED\n");
}

LLE_TEST(config_get_current) {
    printf("Testing current configuration retrieval... ");
    
    // Create line editor
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    lle_lusush_config_t config;
    lle_config_result_t result = lle_config_get_current((struct lle_line_editor *)editor, &config);
    
    // Should succeed
    LLE_ASSERT_EQ(result, LLE_CONFIG_SUCCESS);
    
    // Verify configuration has reasonable values
    LLE_ASSERT(config.history_size > 0);
    LLE_ASSERT(config.undo_levels > 0);
    
    // Test NULL parameters
    LLE_ASSERT_EQ(lle_config_get_current(NULL, &config), LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT_EQ(lle_config_get_current((struct lle_line_editor *)editor, NULL), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_config_cleanup(&config);
    lle_destroy(editor);
    
    printf("PASSED\n");
}

// ============================================================================
// Individual Setting Management Tests
// ============================================================================

LLE_TEST(config_individual_bool_settings) {
    printf("Testing individual boolean setting updates... ");
    
    // Create line editor
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test valid boolean settings
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, "multiline_mode", false), LLE_CONFIG_SUCCESS);
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, "syntax_highlighting", false), LLE_CONFIG_SUCCESS);
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, "show_completions", false), LLE_CONFIG_SUCCESS);
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, "history_enabled", false), LLE_CONFIG_SUCCESS);
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, "undo_enabled", false), LLE_CONFIG_SUCCESS);
    
    // Test invalid setting name
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, "invalid_setting", true), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Test NULL parameters
    LLE_ASSERT_EQ(lle_config_set_bool(NULL, "multiline_mode", true), LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT_EQ(lle_config_set_bool((struct lle_line_editor *)editor, NULL, true), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_destroy(editor);
    
    printf("PASSED\n");
}

LLE_TEST(config_individual_size_settings) {
    printf("Testing individual size setting updates... ");
    
    // Create line editor
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test size setting (not currently supported)
    LLE_ASSERT_EQ(lle_config_set_size((struct lle_line_editor *)editor, "history_size", 2000), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Test NULL parameters
    LLE_ASSERT_EQ(lle_config_set_size(NULL, "history_size", 2000), LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT_EQ(lle_config_set_size((struct lle_line_editor *)editor, NULL, 2000), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_destroy(editor);
    
    printf("PASSED\n");
}

LLE_TEST(config_individual_string_settings) {
    printf("Testing individual string setting updates... ");
    
    // Create line editor
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    // Test string setting (not currently supported)
    LLE_ASSERT_EQ(lle_config_set_string((struct lle_line_editor *)editor, "theme_name", "new_theme"), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Test NULL parameters
    LLE_ASSERT_EQ(lle_config_set_string(NULL, "theme_name", "test"), LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT_EQ(lle_config_set_string((struct lle_line_editor *)editor, NULL, "test"), LLE_CONFIG_ERROR_INVALID_PARAM);
    
    // Clean up
    lle_destroy(editor);
    
    printf("PASSED\n");
}

// ============================================================================
// Error Handling and Utilities Tests
// ============================================================================

LLE_TEST(config_error_messages) {
    printf("Testing configuration error messages... ");
    
    // Test all error codes have messages
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message(LLE_CONFIG_SUCCESS));
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message(LLE_CONFIG_ERROR_INVALID_PARAM));
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message(LLE_CONFIG_ERROR_MEMORY));
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message(LLE_CONFIG_ERROR_FILE_ACCESS));
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message(LLE_CONFIG_ERROR_PARSE));
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message(LLE_CONFIG_ERROR_LUSUSH_CONFIG));
    
    // Test unknown error code
    LLE_ASSERT_NOT_NULL(lle_config_get_error_message((lle_config_result_t)999));
    
    // Verify messages are different
    const char *success_msg = lle_config_get_error_message(LLE_CONFIG_SUCCESS);
    const char *error_msg = lle_config_get_error_message(LLE_CONFIG_ERROR_INVALID_PARAM);
    LLE_ASSERT(strcmp(success_msg, error_msg) != 0);
    
    printf("PASSED\n");
}

LLE_TEST(config_print_summary) {
    printf("Testing configuration summary printing... ");
    
    lle_lusush_config_t config;
    create_test_config(&config);
    
    // Test printing to stdout (should not crash)
    lle_config_print_summary(&config, stdout);
    
    // Test NULL parameters (should not crash)
    lle_config_print_summary(NULL, stdout);
    lle_config_print_summary(&config, NULL);
    
    // Clean up
    lle_config_cleanup(&config);
    
    printf("PASSED\n");
}

// ============================================================================
// Integration and Edge Case Tests
// ============================================================================

LLE_TEST(config_full_workflow) {
    printf("Testing complete configuration workflow... ");
    
    // Initialize configuration
    lle_lusush_config_t config;
    LLE_ASSERT_EQ(lle_config_init_defaults(&config), LLE_CONFIG_SUCCESS);
    
    // Load from Lusush system
    LLE_ASSERT_EQ(lle_config_load(&config), LLE_CONFIG_SUCCESS);
    
    // Validate configuration
    LLE_ASSERT_EQ(lle_config_validate(&config), LLE_CONFIG_SUCCESS);
    
    // Create line editor and apply configuration
    lle_line_editor_t *editor = lle_create();
    LLE_ASSERT_NOT_NULL(editor);
    
    LLE_ASSERT_EQ(lle_config_apply((struct lle_line_editor *)editor, &config), LLE_CONFIG_SUCCESS);
    
    // Test configuration retrieval
    lle_lusush_config_t current_config;
    LLE_ASSERT_EQ(lle_config_get_current((struct lle_line_editor *)editor, &current_config), LLE_CONFIG_SUCCESS);
    
    // Test configuration copying
    lle_lusush_config_t copied_config;
    LLE_ASSERT_EQ(lle_config_copy(&copied_config, &current_config), LLE_CONFIG_SUCCESS);
    
    // Clean up
    lle_config_cleanup(&config);
    lle_config_cleanup(&current_config);
    lle_config_cleanup(&copied_config);
    lle_destroy(editor);
    
    printf("PASSED\n");
}

LLE_TEST(config_memory_stress) {
    printf("Testing configuration memory management under stress... ");
    
    // Create and destroy many configurations
    for (int i = 0; i < 100; i++) {
        lle_lusush_config_t config;
        create_test_config(&config);
        
        // Copy configuration multiple times
        lle_lusush_config_t copy1, copy2;
        LLE_ASSERT_EQ(lle_config_copy(&copy1, &config), LLE_CONFIG_SUCCESS);
        LLE_ASSERT_EQ(lle_config_copy(&copy2, &copy1), LLE_CONFIG_SUCCESS);
        
        // Verify configurations are equal
        LLE_ASSERT(lle_config_equals(&config, &copy1));
        LLE_ASSERT(lle_config_equals(&copy1, &copy2));
        
        // Clean up
        lle_config_cleanup(&config);
        lle_config_cleanup(&copy1);
        lle_config_cleanup(&copy2);
    }
    
    printf("PASSED\n");
}

// ============================================================================
// Main Test Function
// ============================================================================

int main(void) {
    printf("Running LLE-043 Configuration Integration Tests...\n");
    printf("=================================================\n");
    
    // Configuration initialization and defaults
    test_config_init_defaults();
    test_config_cleanup();
    
    // Configuration loading
    test_config_load_from_lusush();
    
    // Configuration validation
    test_config_validation();
    
    // Configuration copy and comparison
    test_config_copy();
    test_config_equals();
    
    // Configuration callback system
    test_config_callback_registration();
    test_config_callback_multiple_registration();
    
    // Line editor integration
    test_config_apply_to_editor();
    test_config_reload();
    test_config_get_current();
    
    // Individual setting management
    test_config_individual_bool_settings();
    test_config_individual_size_settings();
    test_config_individual_string_settings();
    
    // Error handling and utilities
    test_config_error_messages();
    test_config_print_summary();
    
    // Integration and edge cases
    test_config_full_workflow();
    test_config_memory_stress();
    
    printf("=================================================\n");
    printf("All LLE-043 Configuration Integration tests passed!\n");
    
    return 0;
}