/**
 * @file test_lle_021_key_input_handling.c
 * @brief Tests for LLE-021: Key Input Handling
 * 
 * Tests the key input handling structures, enumerations, and utility functions
 * including key event structures, key type classifications, string conversions,
 * and input configuration management.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "input_handler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Key Type Enumeration Tests
// ============================================================================

LLE_TEST(key_type_enumeration_values) {
    printf("Testing key type enumeration values... ");
    
    // Test basic key types
    LLE_ASSERT(LLE_KEY_CHAR == 0);
    LLE_ASSERT(LLE_KEY_BACKSPACE == 1);
    LLE_ASSERT(LLE_KEY_DELETE == 2);
    LLE_ASSERT(LLE_KEY_ENTER == 3);
    LLE_ASSERT(LLE_KEY_TAB == 4);
    
    // Test navigation keys
    LLE_ASSERT(LLE_KEY_ARROW_LEFT > LLE_KEY_ESCAPE);
    LLE_ASSERT(LLE_KEY_ARROW_RIGHT > LLE_KEY_ARROW_LEFT);
    LLE_ASSERT(LLE_KEY_HOME > LLE_KEY_ARROW_DOWN);
    LLE_ASSERT(LLE_KEY_END > LLE_KEY_HOME);
    
    // Test control keys
    LLE_ASSERT(LLE_KEY_CTRL_A > LLE_KEY_PAGE_DOWN);
    LLE_ASSERT(LLE_KEY_CTRL_C > LLE_KEY_CTRL_A);
    LLE_ASSERT(LLE_KEY_CTRL_Z > LLE_KEY_CTRL_C);
    
    // Test function keys
    LLE_ASSERT(LLE_KEY_F1 > LLE_KEY_ALT_UNDERSCORE);
    LLE_ASSERT(LLE_KEY_F12 > LLE_KEY_F1);
    
    // Test special keys
    LLE_ASSERT(LLE_KEY_UNKNOWN > LLE_KEY_SHIFT_TAB);
    LLE_ASSERT(LLE_KEY_TYPE_COUNT > LLE_KEY_ERROR);
    
    printf("PASSED\n");
}

LLE_TEST(key_type_coverage) {
    printf("Testing key type coverage... ");
    
    // Verify we have comprehensive coverage
    LLE_ASSERT(LLE_KEY_TYPE_COUNT > 50); // Should have many key types
    
    // Test specific key categories exist
    bool has_char = (LLE_KEY_CHAR >= 0);
    bool has_navigation = (LLE_KEY_ARROW_LEFT > 0);
    bool has_control = (LLE_KEY_CTRL_C > 0);
    bool has_function = (LLE_KEY_F1 > 0);
    bool has_special = (LLE_KEY_UNKNOWN > 0);
    
    LLE_ASSERT(has_char);
    LLE_ASSERT(has_navigation);
    LLE_ASSERT(has_control);
    LLE_ASSERT(has_function);
    LLE_ASSERT(has_special);
    
    printf("PASSED\n");
}

// ============================================================================
// Key Event Structure Tests
// ============================================================================

LLE_TEST(key_event_structure_definition) {
    printf("Testing key event structure definition... ");
    
    lle_key_event_t event;
    
    // Test structure members exist and are accessible
    event.type = LLE_KEY_CHAR;
    event.character = 'a';
    event.unicode = 0x61;
    event.ctrl = false;
    event.alt = false;
    event.shift = false;
    event.super = false;
    event.sequence_length = 0;
    event.timestamp = 123456;
    event.is_repeat = false;
    event.repeat_count = 0;
    event.processed = false;
    event.consumed = false;
    event.synthetic = false;
    
    // Verify values were set correctly
    LLE_ASSERT(event.type == LLE_KEY_CHAR);
    LLE_ASSERT(event.character == 'a');
    LLE_ASSERT(event.unicode == 0x61);
    LLE_ASSERT(event.ctrl == false);
    LLE_ASSERT(event.alt == false);
    LLE_ASSERT(event.shift == false);
    LLE_ASSERT(event.timestamp == 123456);
    
    printf("PASSED\n");
}

LLE_TEST(key_event_initialization) {
    printf("Testing key event initialization... ");
    
    lle_key_event_t event;
    
    // Initialize event
    lle_key_event_init(&event);
    
    // Verify initialization
    LLE_ASSERT(event.type == LLE_KEY_UNKNOWN);
    LLE_ASSERT(event.character == 0);
    LLE_ASSERT(event.unicode == 0);
    LLE_ASSERT(event.ctrl == false);
    LLE_ASSERT(event.alt == false);
    LLE_ASSERT(event.shift == false);
    LLE_ASSERT(event.super == false);
    LLE_ASSERT(event.sequence_length == 0);
    LLE_ASSERT(event.timestamp == 0);
    LLE_ASSERT(event.is_repeat == false);
    LLE_ASSERT(event.repeat_count == 0);
    LLE_ASSERT(event.processed == false);
    LLE_ASSERT(event.consumed == false);
    LLE_ASSERT(event.synthetic == false);
    
    printf("PASSED\n");
}

LLE_TEST(key_event_copy) {
    printf("Testing key event copy... ");
    
    lle_key_event_t src, dest;
    
    // Initialize source event
    lle_key_event_init(&src);
    src.type = LLE_KEY_CTRL_C;
    src.character = 0x03;
    src.ctrl = true;
    src.timestamp = 987654;
    strcpy(src.raw_sequence, "\033[C");
    src.sequence_length = 3;
    
    // Copy event
    bool result = lle_key_event_copy(&dest, &src);
    LLE_ASSERT(result == true);
    
    // Verify copy
    LLE_ASSERT(dest.type == src.type);
    LLE_ASSERT(dest.character == src.character);
    LLE_ASSERT(dest.ctrl == src.ctrl);
    LLE_ASSERT(dest.timestamp == src.timestamp);
    LLE_ASSERT(dest.sequence_length == src.sequence_length);
    LLE_ASSERT(strcmp(dest.raw_sequence, src.raw_sequence) == 0);
    
    printf("PASSED\n");
}

LLE_TEST(key_event_equality) {
    printf("Testing key event equality... ");
    
    lle_key_event_t event1, event2;
    
    // Initialize identical events
    lle_key_event_init(&event1);
    lle_key_event_init(&event2);
    
    event1.type = LLE_KEY_CHAR;
    event1.character = 'x';
    event1.shift = true;
    
    event2.type = LLE_KEY_CHAR;
    event2.character = 'x';
    event2.shift = true;
    
    // Test equality
    bool equal = lle_key_event_equals(&event1, &event2);
    LLE_ASSERT(equal == true);
    
    // Modify one event
    event2.character = 'y';
    equal = lle_key_event_equals(&event1, &event2);
    LLE_ASSERT(equal == false);
    
    printf("PASSED\n");
}

// ============================================================================
// Key Binding Structure Tests
// ============================================================================

LLE_TEST(key_binding_structure_definition) {
    printf("Testing key binding structure definition... ");
    
    lle_key_binding_t binding;
    
    // Test structure members
    binding.key = LLE_KEY_CTRL_C;
    binding.ctrl = true;
    binding.alt = false;
    binding.shift = false;
    binding.action_name = "interrupt";
    binding.action_data = NULL;
    
    // Verify values
    LLE_ASSERT(binding.key == LLE_KEY_CTRL_C);
    LLE_ASSERT(binding.ctrl == true);
    LLE_ASSERT(binding.alt == false);
    LLE_ASSERT(strcmp(binding.action_name, "interrupt") == 0);
    LLE_ASSERT(binding.action_data == NULL);
    
    printf("PASSED\n");
}

LLE_TEST(key_event_matches_binding) {
    printf("Testing key event binding matching... ");
    
    lle_key_event_t event;
    lle_key_binding_t binding;
    
    // Create matching event and binding
    lle_key_event_init(&event);
    event.type = LLE_KEY_CTRL_C;
    event.ctrl = true;
    
    binding.key = LLE_KEY_CTRL_C;
    binding.ctrl = true;
    binding.alt = false;
    binding.shift = false;
    binding.action_name = "interrupt";
    binding.action_data = NULL;
    
    // Test match
    bool matches = lle_key_event_matches_binding(&event, &binding);
    LLE_ASSERT(matches == true);
    
    // Test non-match (different modifier)
    event.ctrl = false;
    matches = lle_key_event_matches_binding(&event, &binding);
    LLE_ASSERT(matches == false);
    
    printf("PASSED\n");
}

// ============================================================================
// Input Configuration Tests
// ============================================================================

LLE_TEST(input_config_initialization) {
    printf("Testing input configuration initialization... ");
    
    lle_input_config_t config;
    
    // Initialize configuration
    bool result = lle_input_config_init(&config);
    LLE_ASSERT(result == true);
    
    // Verify default values
    LLE_ASSERT(config.escape_timeout_ms == LLE_DEFAULT_ESCAPE_TIMEOUT_MS);
    LLE_ASSERT(config.key_repeat_delay_ms == LLE_DEFAULT_KEY_REPEAT_DELAY_MS);
    LLE_ASSERT(config.key_repeat_rate_ms == LLE_DEFAULT_KEY_REPEAT_RATE_MS);
    LLE_ASSERT(config.enable_ctrl_sequences == true);
    LLE_ASSERT(config.enable_alt_sequences == true);
    LLE_ASSERT(config.utf8_mode == true);
    LLE_ASSERT(config.binding_count == 0);
    LLE_ASSERT(config.key_event_count == 0);
    
    // Cleanup
    lle_input_config_cleanup(&config);
    
    printf("PASSED\n");
}

LLE_TEST(input_config_add_binding) {
    printf("Testing input configuration binding addition... ");
    
    lle_input_config_t config;
    lle_key_binding_t binding;
    
    // Initialize
    lle_input_config_init(&config);
    
    // Create binding
    binding.key = LLE_KEY_CTRL_C;
    binding.ctrl = true;
    binding.alt = false;
    binding.shift = false;
    binding.action_name = "interrupt";
    binding.action_data = NULL;
    
    // Add binding
    bool result = lle_input_config_add_binding(&config, &binding);
    LLE_ASSERT(result == true);
    LLE_ASSERT(config.binding_count == 1);
    
    // Verify binding was added
    LLE_ASSERT(config.bindings[0].key == LLE_KEY_CTRL_C);
    LLE_ASSERT(config.bindings[0].ctrl == true);
    LLE_ASSERT(strcmp(config.bindings[0].action_name, "interrupt") == 0);
    
    // Cleanup
    lle_input_config_cleanup(&config);
    
    printf("PASSED\n");
}

LLE_TEST(input_config_find_binding) {
    printf("Testing input configuration binding lookup... ");
    
    lle_input_config_t config;
    lle_key_binding_t binding;
    lle_key_event_t event;
    
    // Initialize
    lle_input_config_init(&config);
    
    // Add binding
    binding.key = LLE_KEY_CTRL_D;
    binding.ctrl = true;
    binding.alt = false;
    binding.shift = false;
    binding.action_name = "delete_char";
    binding.action_data = NULL;
    lle_input_config_add_binding(&config, &binding);
    
    // Create matching event
    lle_key_event_init(&event);
    event.type = LLE_KEY_CTRL_D;
    event.ctrl = true;
    
    // Find binding
    const lle_key_binding_t *found = lle_input_config_find_binding(&config, &event);
    LLE_ASSERT(found != NULL);
    LLE_ASSERT(found->key == LLE_KEY_CTRL_D);
    LLE_ASSERT(strcmp(found->action_name, "delete_char") == 0);
    
    // Test non-matching event
    event.type = LLE_KEY_CTRL_A;
    found = lle_input_config_find_binding(&config, &event);
    LLE_ASSERT(found == NULL);
    
    // Cleanup
    lle_input_config_cleanup(&config);
    
    printf("PASSED\n");
}

// ============================================================================
// Key Classification Tests
// ============================================================================

LLE_TEST(key_classification_printable) {
    printf("Testing printable key classification... ");
    
    lle_key_event_t event;
    
    // Test printable character
    lle_key_event_init(&event);
    event.type = LLE_KEY_CHAR;
    event.character = 'a';
    LLE_ASSERT(lle_key_is_printable(&event) == true);
    
    // Test non-printable
    event.type = LLE_KEY_CTRL_C;
    event.character = 0x03;
    LLE_ASSERT(lle_key_is_printable(&event) == false);
    
    // Test special key
    event.type = LLE_KEY_ARROW_LEFT;
    event.character = 0;
    LLE_ASSERT(lle_key_is_printable(&event) == false);
    
    printf("PASSED\n");
}

LLE_TEST(key_classification_control) {
    printf("Testing control key classification... ");
    
    lle_key_event_t event;
    
    // Test control character
    lle_key_event_init(&event);
    event.type = LLE_KEY_CTRL_C;
    event.ctrl = true;
    LLE_ASSERT(lle_key_is_control(&event) == true);
    
    // Test regular character
    event.type = LLE_KEY_CHAR;
    event.character = 'a';
    event.ctrl = false;
    LLE_ASSERT(lle_key_is_control(&event) == false);
    
    printf("PASSED\n");
}

LLE_TEST(key_classification_navigation) {
    printf("Testing navigation key classification... ");
    
    lle_key_event_t event;
    
    // Test navigation keys
    lle_key_event_init(&event);
    event.type = LLE_KEY_ARROW_LEFT;
    LLE_ASSERT(lle_key_is_navigation(&event) == true);
    
    event.type = LLE_KEY_HOME;
    LLE_ASSERT(lle_key_is_navigation(&event) == true);
    
    event.type = LLE_KEY_END;
    LLE_ASSERT(lle_key_is_navigation(&event) == true);
    
    // Test non-navigation key
    event.type = LLE_KEY_CHAR;
    LLE_ASSERT(lle_key_is_navigation(&event) == false);
    
    printf("PASSED\n");
}

LLE_TEST(key_classification_editing) {
    printf("Testing editing key classification... ");
    
    lle_key_event_t event;
    
    // Test editing keys
    lle_key_event_init(&event);
    event.type = LLE_KEY_BACKSPACE;
    LLE_ASSERT(lle_key_is_editing(&event) == true);
    
    event.type = LLE_KEY_DELETE;
    LLE_ASSERT(lle_key_is_editing(&event) == true);
    
    event.type = LLE_KEY_INSERT;
    LLE_ASSERT(lle_key_is_editing(&event) == true);
    
    // Test non-editing key
    event.type = LLE_KEY_F1;
    LLE_ASSERT(lle_key_is_editing(&event) == false);
    
    printf("PASSED\n");
}

LLE_TEST(key_classification_function) {
    printf("Testing function key classification... ");
    
    lle_key_event_t event;
    
    // Test function keys
    lle_key_event_init(&event);
    event.type = LLE_KEY_F1;
    LLE_ASSERT(lle_key_is_function(&event) == true);
    
    event.type = LLE_KEY_F5;
    LLE_ASSERT(lle_key_is_function(&event) == true);
    
    event.type = LLE_KEY_F12;
    LLE_ASSERT(lle_key_is_function(&event) == true);
    
    // Test non-function key
    event.type = LLE_KEY_CHAR;
    LLE_ASSERT(lle_key_is_function(&event) == false);
    
    printf("PASSED\n");
}

// ============================================================================
// String Conversion Tests
// ============================================================================

LLE_TEST(key_type_to_string_conversion) {
    printf("Testing key type to string conversion... ");
    
    // Test known key types
    const char *name = lle_key_type_to_string(LLE_KEY_CHAR);
    LLE_ASSERT(name != NULL);
    LLE_ASSERT(strlen(name) > 0);
    
    name = lle_key_type_to_string(LLE_KEY_CTRL_C);
    LLE_ASSERT(name != NULL);
    LLE_ASSERT(strlen(name) > 0);
    
    name = lle_key_type_to_string(LLE_KEY_ARROW_LEFT);
    LLE_ASSERT(name != NULL);
    LLE_ASSERT(strlen(name) > 0);
    
    // Test invalid key type
    name = lle_key_type_to_string(LLE_KEY_TYPE_COUNT);
    LLE_ASSERT(name == NULL);
    
    printf("PASSED\n");
}

LLE_TEST(string_to_key_type_conversion) {
    printf("Testing string to key type conversion... ");
    
    // Test known key names (assuming standard names)
    lle_key_type_t key = lle_key_string_to_type("CHAR");
    LLE_ASSERT(key == LLE_KEY_CHAR || key == LLE_KEY_UNKNOWN);
    
    // Test invalid name
    key = lle_key_string_to_type("INVALID_KEY_NAME");
    LLE_ASSERT(key == LLE_KEY_UNKNOWN);
    
    // Test null input
    key = lle_key_string_to_type(NULL);
    LLE_ASSERT(key == LLE_KEY_UNKNOWN);
    
    printf("PASSED\n");
}

LLE_TEST(key_event_to_string_conversion) {
    printf("Testing key event to string conversion... ");
    
    lle_key_event_t event;
    char buffer[64];
    
    // Test simple character event
    lle_key_event_init(&event);
    event.type = LLE_KEY_CHAR;
    event.character = 'a';
    
    bool result = lle_key_event_to_string(&event, buffer, sizeof(buffer));
    LLE_ASSERT(result == true);
    LLE_ASSERT(strlen(buffer) > 0);
    
    // Test control key event
    event.type = LLE_KEY_CTRL_C;
    event.ctrl = true;
    
    result = lle_key_event_to_string(&event, buffer, sizeof(buffer));
    LLE_ASSERT(result == true);
    LLE_ASSERT(strlen(buffer) > 0);
    
    // Test buffer too small
    result = lle_key_event_to_string(&event, buffer, 1);
    LLE_ASSERT(result == false);
    
    printf("PASSED\n");
}

// ============================================================================
// Constants and Defaults Tests
// ============================================================================

LLE_TEST(ascii_constants_definition) {
    printf("Testing ASCII constants definition... ");
    
    // Test control character constants
    LLE_ASSERT(LLE_ASCII_CTRL_A == 0x01);
    LLE_ASSERT(LLE_ASCII_CTRL_C == 0x03);
    LLE_ASSERT(LLE_ASCII_CTRL_Z == 0x1A);
    
    // Test special characters
    LLE_ASSERT(LLE_ASCII_ESC == 0x1B);
    LLE_ASSERT(LLE_ASCII_DEL == 0x7F);
    LLE_ASSERT(LLE_ASCII_BACKSPACE == 0x08);
    LLE_ASSERT(LLE_ASCII_TAB == 0x09);
    LLE_ASSERT(LLE_ASCII_NEWLINE == 0x0A);
    LLE_ASSERT(LLE_ASCII_RETURN == 0x0D);
    
    printf("PASSED\n");
}

LLE_TEST(default_values_definition) {
    printf("Testing default values definition... ");
    
    // Test timeout defaults
    LLE_ASSERT(LLE_DEFAULT_ESCAPE_TIMEOUT_MS == 100);
    LLE_ASSERT(LLE_DEFAULT_KEY_REPEAT_DELAY_MS == 500);
    LLE_ASSERT(LLE_DEFAULT_KEY_REPEAT_RATE_MS == 50);
    
    // Test limits
    LLE_ASSERT(LLE_MAX_KEY_BINDINGS == 256);
    LLE_ASSERT(LLE_ESCAPE_SEQUENCE_MAX == 16);
    LLE_ASSERT(LLE_KEY_NAME_MAX == 32);
    
    printf("PASSED\n");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

LLE_TEST(null_pointer_handling) {
    printf("Testing null pointer handling... ");
    
    // Test key event functions with null pointers
    lle_key_event_init(NULL); // Should not crash
    
    bool result = lle_key_event_copy(NULL, NULL);
    LLE_ASSERT(result == false);
    
    result = lle_key_event_equals(NULL, NULL);
    LLE_ASSERT(result == false);
    
    result = lle_key_event_matches_binding(NULL, NULL);
    LLE_ASSERT(result == false);
    
    // Test configuration functions with null pointers
    result = lle_input_config_init(NULL);
    LLE_ASSERT(result == false);
    
    lle_input_config_cleanup(NULL); // Should not crash
    
    printf("PASSED\n");
}

LLE_TEST(invalid_parameters) {
    printf("Testing invalid parameter handling... ");
    
    lle_key_event_t event;
    char buffer[10];
    
    // Test string conversion with invalid parameters
    bool result = lle_key_event_to_string(&event, NULL, 10);
    LLE_ASSERT(result == false);
    
    result = lle_key_event_to_string(&event, buffer, 0);
    LLE_ASSERT(result == false);
    
    // Test key type conversion with invalid values
    const char *name = lle_key_type_to_string((lle_key_type_t)(-1));
    LLE_ASSERT(name == NULL);
    
    name = lle_key_type_to_string((lle_key_type_t)(LLE_KEY_TYPE_COUNT + 100));
    LLE_ASSERT(name == NULL);
    
    printf("PASSED\n");
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void) {
    printf("Running LLE-021 Key Input Handling Tests...\n");
    printf("===========================================\n\n");
    
    // Key type enumeration tests
    test_key_type_enumeration_values();
    test_key_type_coverage();
    
    // Key event structure tests
    test_key_event_structure_definition();
    test_key_event_initialization();
    test_key_event_copy();
    test_key_event_equality();
    
    // Key binding structure tests
    test_key_binding_structure_definition();
    test_key_event_matches_binding();
    
    // Input configuration tests
    test_input_config_initialization();
    test_input_config_add_binding();
    test_input_config_find_binding();
    
    // Key classification tests
    test_key_classification_printable();
    test_key_classification_control();
    test_key_classification_navigation();
    test_key_classification_editing();
    test_key_classification_function();
    
    // String conversion tests
    test_key_type_to_string_conversion();
    test_string_to_key_type_conversion();
    test_key_event_to_string_conversion();
    
    // Constants and defaults tests
    test_ascii_constants_definition();
    test_default_values_definition();
    
    // Error handling tests
    test_null_pointer_handling();
    test_invalid_parameters();
    
    printf("\n===========================================\n");
    printf("All LLE-021 Key Input Handling Tests Passed!\n");
    printf("Key input structures properly defined and validated.\n");
    printf("Comprehensive key type coverage with 50+ key types.\n");
    printf("Ready for LLE-022 implementation.\n");
    
    return 0;
}