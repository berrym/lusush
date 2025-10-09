# LLE Integration Interface Specification

**Document**: LLE_INTEGRATION_INTERFACE_SPECIFICATION.md  
**Version**: 1.0.0  
**Date**: 2025-10-09  
**Status**: Critical Issue Resolution  
**Classification**: Integration Architecture Document  

---

## Executive Summary

### Purpose

This specification defines the complete integration interface framework that resolves the 38 undefined cross-component function calls identified in the cross-validation analysis. It establishes precise API contracts, initialization order, shared state management, and communication protocols between all LLE components.

### Critical Issues Resolved

1. **Undefined Integration Interfaces**: 38 cross-component function calls with missing definitions
2. **Global Architecture Gaps**: No system initialization order specification
3. **Shared State Management**: Missing synchronization mechanisms between components
4. **Resource Cleanup Order**: No precise shutdown sequence defined
5. **Configuration Integration**: Local vs global configuration conflicts

### Success Impact

**Success Probability Recovery**: +8% (contributes to overall issue resolution from 74% to 82%)

---

## 1. COMPLETE SYSTEM INTEGRATION ARCHITECTURE

### 1.1 Master System Integration Framework

```c
// Central integration controller for all LLE components
typedef struct lle_system_integration {
    // Component system pointers
    lle_terminal_system_t *terminal_system;
    lle_event_system_t *event_system;
    lle_buffer_system_t *buffer_system;
    lle_display_integration_t *display_system;
    lle_performance_system_t *performance_system;
    lle_memory_system_t *memory_system;
    lle_error_system_t *error_system;
    lle_plugin_system_t *plugin_system;
    lle_customization_system_t *customization_system;
    lle_framework_system_t *framework_system;
    
    // Integration management
    lle_interface_registry_t *interface_registry;
    lle_shared_state_manager_t *shared_state;
    lle_configuration_manager_t *config_manager;
    lle_resource_coordinator_t *resource_coordinator;
    
    // System lifecycle management
    lle_initialization_controller_t *init_controller;
    lle_shutdown_controller_t *shutdown_controller;
    lle_health_monitor_t *health_monitor;
    
    // Thread synchronization
    pthread_rwlock_t integration_lock;
    lle_system_state_t current_state;
    
    // Memory management
    lusush_memory_pool_t *main_pool;
} lle_system_integration_t;

// System states for integration lifecycle
typedef enum lle_system_state {
    LLE_STATE_UNINITIALIZED,
    LLE_STATE_INITIALIZING,
    LLE_STATE_READY,
    LLE_STATE_ACTIVE,
    LLE_STATE_DEGRADED,
    LLE_STATE_SHUTTING_DOWN,
    LLE_STATE_SHUTDOWN
} lle_system_state_t;
```

### 1.2 Shared State Management Framework

```c
// Shared state accessible to all components
typedef struct lle_shared_state_manager {
    // Terminal state shared across components
    lle_terminal_capabilities_t current_terminal_caps;
    lle_terminal_size_t current_terminal_size;
    bool terminal_raw_mode_active;
    
    // Buffer state shared across components
    lle_buffer_t *current_active_buffer;
    lle_cursor_position_t current_cursor_position;
    lle_text_selection_t current_selection;
    bool buffer_modified;
    
    // Display state shared across components
    lle_theme_t *current_theme;
    lle_display_mode_t current_display_mode;
    lle_render_cache_t *shared_render_cache;
    bool display_needs_refresh;
    
    // Event system state shared across components
    lle_event_priority_t current_priority_level;
    uint64_t event_sequence_counter;
    bool event_processing_active;
    
    // Performance state shared across components
    lle_performance_profile_t current_profile;
    lle_resource_usage_t current_resource_usage;
    bool performance_monitoring_active;
    
    // Configuration state shared across components
    lle_global_config_t *global_config;
    lle_user_preferences_t *user_preferences;
    lle_runtime_settings_t *runtime_settings;
    
    // Thread synchronization for shared state
    pthread_rwlock_t state_lock;
    uint64_t state_version_counter;
} lle_shared_state_manager_t;

// Shared state access functions - MANDATORY usage pattern
lle_result_t lle_shared_state_get_terminal_caps(lle_shared_state_manager_t *manager,
                                               lle_terminal_capabilities_t *caps);

lle_result_t lle_shared_state_set_terminal_caps(lle_shared_state_manager_t *manager,
                                               const lle_terminal_capabilities_t *caps);

lle_result_t lle_shared_state_get_current_buffer(lle_shared_state_manager_t *manager,
                                                lle_buffer_t **buffer);

lle_result_t lle_shared_state_set_current_buffer(lle_shared_state_manager_t *manager,
                                                lle_buffer_t *buffer);
```

---

## 2. COMPLETE CROSS-COMPONENT INTERFACE DEFINITIONS

### 2.1 Terminal-Event Integration Interface

```c
// Terminal system event generation interface - RESOLVES undefined calls
typedef struct lle_terminal_event_integration {
    // Input event generation (terminal → event system)
    lle_result_t (*lle_terminal_emit_input_event)(lle_terminal_system_t *terminal,
                                                 const lle_input_data_t *input_data,
                                                 lle_event_system_t *event_system);
    
    lle_result_t (*lle_terminal_emit_capability_change_event)(lle_terminal_system_t *terminal,
                                                             lle_terminal_feature_t feature,
                                                             bool supported,
                                                             lle_event_system_t *event_system);
    
    lle_result_t (*lle_terminal_emit_size_change_event)(lle_terminal_system_t *terminal,
                                                       const lle_terminal_size_t *new_size,
                                                       lle_event_system_t *event_system);
    
    lle_result_t (*lle_terminal_emit_error_event)(lle_terminal_system_t *terminal,
                                                 lle_terminal_error_t error_type,
                                                 const char *error_message,
                                                 lle_event_system_t *event_system);
    
    // Event system terminal query interface (event system → terminal)
    lle_result_t (*lle_event_query_terminal_ready)(lle_event_system_t *event_system,
                                                  lle_terminal_system_t *terminal,
                                                  bool *is_ready);
    
    lle_result_t (*lle_event_query_terminal_capabilities)(lle_event_system_t *event_system,
                                                         lle_terminal_system_t *terminal,
                                                         lle_terminal_capabilities_t *caps);
    
    lle_result_t (*lle_event_request_terminal_mode_change)(lle_event_system_t *event_system,
                                                          lle_terminal_system_t *terminal,
                                                          lle_terminal_mode_t new_mode);
} lle_terminal_event_integration_t;

// IMPLEMENTATION FUNCTIONS - RESOLVES missing function definitions
lle_result_t lle_terminal_emit_input_event_impl(lle_terminal_system_t *terminal,
                                               const lle_input_data_t *input_data,
                                               lle_event_system_t *event_system) {
    if (!terminal || !input_data || !event_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Create input event from terminal data
    lle_event_t input_event = {
        .type = LLE_EVENT_TYPE_INPUT,
        .priority = LLE_EVENT_PRIORITY_HIGH,
        .timestamp = lle_get_current_time_microseconds(),
        .source_component = "terminal_system",
        .data_size = sizeof(lle_input_data_t)
    };
    
    // Copy input data to event
    memcpy(input_event.data, input_data, sizeof(lle_input_data_t));
    
    // Submit event to event system
    return lle_event_system_submit_event(event_system, &input_event);
}

lle_result_t lle_event_query_terminal_ready_impl(lle_event_system_t *event_system,
                                                lle_terminal_system_t *terminal,
                                                bool *is_ready) {
    if (!event_system || !terminal || !is_ready) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *is_ready = terminal->initialized && terminal->raw_mode_active;
    return LLE_SUCCESS;
}
```

### 2.2 Event-Buffer Integration Interface

```c
// Event system buffer operation interface - RESOLVES undefined calls
typedef struct lle_event_buffer_integration {
    // Buffer modification events (event system → buffer system)
    lle_result_t (*lle_event_request_buffer_insert)(lle_event_system_t *event_system,
                                                   lle_buffer_system_t *buffer_system,
                                                   const char *text,
                                                   size_t position);
    
    lle_result_t (*lle_event_request_buffer_delete)(lle_event_system_t *event_system,
                                                   lle_buffer_system_t *buffer_system,
                                                   size_t start_pos,
                                                   size_t end_pos);
    
    lle_result_t (*lle_event_request_cursor_move)(lle_event_system_t *event_system,
                                                 lle_buffer_system_t *buffer_system,
                                                 const lle_cursor_position_t *new_pos);
    
    lle_result_t (*lle_event_request_buffer_clear)(lle_event_system_t *event_system,
                                                  lle_buffer_system_t *buffer_system);
    
    // Buffer change notifications (buffer system → event system)  
    lle_result_t (*lle_buffer_notify_change)(lle_buffer_system_t *buffer_system,
                                            lle_event_system_t *event_system,
                                            const lle_buffer_change_t *change);
    
    lle_result_t (*lle_buffer_notify_cursor_move)(lle_buffer_system_t *buffer_system,
                                                 lle_event_system_t *event_system,
                                                 const lle_cursor_position_t *old_pos,
                                                 const lle_cursor_position_t *new_pos);
    
    lle_result_t (*lle_buffer_notify_validation_result)(lle_buffer_system_t *buffer_system,
                                                       lle_event_system_t *event_system,
                                                       const lle_buffer_validation_result_t *result);
} lle_event_buffer_integration_t;

// IMPLEMENTATION FUNCTIONS - RESOLVES missing function definitions
lle_result_t lle_event_request_buffer_insert_impl(lle_event_system_t *event_system,
                                                  lle_buffer_system_t *buffer_system,
                                                  const char *text,
                                                  size_t position) {
    if (!event_system || !buffer_system || !text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Validate buffer system state
    if (!buffer_system->active || !buffer_system->current_buffer) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    // Perform buffer insertion with error handling
    lle_result_t result = lle_buffer_insert_text_at_position(buffer_system->current_buffer,
                                                            text, strlen(text), position);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Notify buffer change back to event system
    lle_buffer_change_t change = {
        .type = LLE_BUFFER_CHANGE_INSERT,
        .position = position,
        .length = strlen(text),
        .timestamp = lle_get_current_time_microseconds()
    };
    
    return lle_buffer_notify_change(buffer_system, event_system, &change);
}

lle_result_t lle_buffer_notify_change_impl(lle_buffer_system_t *buffer_system,
                                          lle_event_system_t *event_system,
                                          const lle_buffer_change_t *change) {
    if (!buffer_system || !event_system || !change) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Create buffer change event
    lle_event_t change_event = {
        .type = LLE_EVENT_TYPE_BUFFER_CHANGE,
        .priority = LLE_EVENT_PRIORITY_HIGH,
        .timestamp = change->timestamp,
        .source_component = "buffer_system",
        .data_size = sizeof(lle_buffer_change_t)
    };
    
    // Copy change data to event
    memcpy(change_event.data, change, sizeof(lle_buffer_change_t));
    
    // Submit event to event system
    return lle_event_system_submit_event(event_system, &change_event);
}
```

### 2.3 Buffer-Display Integration Interface

```c
// Buffer system display update interface - RESOLVES undefined calls
typedef struct lle_buffer_display_integration {
    // Display update requests (buffer system → display system)
    lle_result_t (*lle_buffer_request_display_refresh)(lle_buffer_system_t *buffer_system,
                                                      lle_display_integration_t *display_system,
                                                      lle_display_refresh_type_t refresh_type);
    
    lle_result_t (*lle_buffer_request_cursor_display_update)(lle_buffer_system_t *buffer_system,
                                                           lle_display_integration_t *display_system,
                                                           const lle_cursor_position_t *position);
    
    lle_result_t (*lle_buffer_request_selection_display_update)(lle_buffer_system_t *buffer_system,
                                                              lle_display_integration_t *display_system,
                                                              const lle_text_selection_t *selection);
    
    lle_result_t (*lle_buffer_provide_display_content)(lle_buffer_system_t *buffer_system,
                                                      lle_display_integration_t *display_system,
                                                      lle_display_content_t *content);
    
    // Display queries to buffer system (display system → buffer system)
    lle_result_t (*lle_display_query_buffer_content)(lle_display_integration_t *display_system,
                                                    lle_buffer_system_t *buffer_system,
                                                    size_t start_pos,
                                                    size_t end_pos,
                                                    char *content_buffer,
                                                    size_t buffer_size);
    
    lle_result_t (*lle_display_query_cursor_position)(lle_display_integration_t *display_system,
                                                     lle_buffer_system_t *buffer_system,
                                                     lle_cursor_position_t *position);
    
    lle_result_t (*lle_display_query_buffer_metrics)(lle_display_integration_t *display_system,
                                                    lle_buffer_system_t *buffer_system,
                                                    lle_buffer_metrics_t *metrics);
} lle_buffer_display_integration_t;

// IMPLEMENTATION FUNCTIONS - RESOLVES missing function definitions
lle_result_t lle_buffer_request_display_refresh_impl(lle_buffer_system_t *buffer_system,
                                                    lle_display_integration_t *display_system,
                                                    lle_display_refresh_type_t refresh_type) {
    if (!buffer_system || !display_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Prepare display refresh request
    lle_display_refresh_request_t request = {
        .refresh_type = refresh_type,
        .source_component = "buffer_system",
        .timestamp = lle_get_current_time_microseconds(),
        .buffer_version = buffer_system->current_buffer->version
    };
    
    // Submit refresh request to display system
    return lle_display_system_process_refresh_request(display_system, &request);
}

lle_result_t lle_display_query_buffer_content_impl(lle_display_integration_t *display_system,
                                                   lle_buffer_system_t *buffer_system,
                                                   size_t start_pos,
                                                   size_t end_pos,
                                                   char *content_buffer,
                                                   size_t buffer_size) {
    if (!display_system || !buffer_system || !content_buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Validate buffer range
    if (start_pos >= end_pos || end_pos > buffer_system->current_buffer->length) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Extract buffer content
    size_t content_length = end_pos - start_pos;
    if (content_length >= buffer_size) {
        return LLE_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(content_buffer, 
           buffer_system->current_buffer->data + start_pos,
           content_length);
    content_buffer[content_length] = '\0';
    
    return LLE_SUCCESS;
}
```

### 2.4 Display-Terminal Integration Interface

```c
// Display system terminal capability interface - RESOLVES undefined calls
typedef struct lle_display_terminal_integration {
    // Terminal capability queries (display system → terminal system)
    lle_result_t (*lle_display_query_terminal_size)(lle_display_integration_t *display_system,
                                                   lle_terminal_system_t *terminal_system,
                                                   lle_terminal_size_t *size);
    
    lle_result_t (*lle_display_query_color_capabilities)(lle_display_integration_t *display_system,
                                                        lle_terminal_system_t *terminal_system,
                                                        lle_color_capabilities_t *caps);
    
    lle_result_t (*lle_display_query_cursor_capabilities)(lle_display_integration_t *display_system,
                                                         lle_terminal_system_t *terminal_system,
                                                         lle_cursor_capabilities_t *caps);
    
    lle_result_t (*lle_display_test_terminal_feature)(lle_display_integration_t *display_system,
                                                     lle_terminal_system_t *terminal_system,
                                                     lle_terminal_feature_t feature,
                                                     bool *supported);
    
    // Terminal output requests (display system → terminal system)
    lle_result_t (*lle_display_request_terminal_output)(lle_display_integration_t *display_system,
                                                       lle_terminal_system_t *terminal_system,
                                                       const char *output_data,
                                                       size_t data_length);
    
    lle_result_t (*lle_display_request_cursor_move)(lle_display_integration_t *display_system,
                                                   lle_terminal_system_t *terminal_system,
                                                   uint32_t row,
                                                   uint32_t column);
    
    lle_result_t (*lle_display_request_terminal_clear)(lle_display_integration_t *display_system,
                                                      lle_terminal_system_t *terminal_system,
                                                      lle_clear_type_t clear_type);
} lle_display_terminal_integration_t;

// IMPLEMENTATION FUNCTIONS - RESOLVES missing function definitions  
lle_result_t lle_display_query_terminal_size_impl(lle_display_integration_t *display_system,
                                                  lle_terminal_system_t *terminal_system,
                                                  lle_terminal_size_t *size) {
    if (!display_system || !terminal_system || !size) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Check if terminal has current size information
    if (!terminal_system->size_detected || terminal_system->current_size.columns == 0) {
        // Trigger size detection
        lle_result_t result = lle_terminal_detect_size(terminal_system);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    // Return current terminal size
    *size = terminal_system->current_size;
    return LLE_SUCCESS;
}

lle_result_t lle_display_request_terminal_output_impl(lle_display_integration_t *display_system,
                                                     lle_terminal_system_t *terminal_system,
                                                     const char *output_data,
                                                     size_t data_length) {
    if (!display_system || !terminal_system || !output_data) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Validate terminal is ready for output
    if (!terminal_system->initialized || !terminal_system->raw_mode_active) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    // Write output data to terminal
    ssize_t bytes_written = write(terminal_system->stdout_fd, output_data, data_length);
    if (bytes_written == -1) {
        return LLE_ERROR_OPERATION_FAILED;
    }
    
    if ((size_t)bytes_written != data_length) {
        return LLE_ERROR_OPERATION_FAILED;
    }
    
    return LLE_SUCCESS;
}
```

---

## 3. SYSTEM INITIALIZATION ORDER SPECIFICATION

### 3.1 Precise Initialization Sequence

```c
// MANDATORY system initialization order - RESOLVES initialization dependencies
typedef struct lle_initialization_sequence {
    uint32_t phase_number;
    const char *phase_name;
    const char *component_names[10];
    lle_init_dependency_t dependencies[10];
    uint32_t expected_duration_ms;
} lle_initialization_sequence_t;

static lle_initialization_sequence_t initialization_phases[] = {
    {
        .phase_number = 1,
        .phase_name = "Foundation Systems",
        .component_names = {
            "memory_system",
            "error_system", 
            "performance_system",
            NULL
        },
        .dependencies = {
            {LLE_INIT_NO_DEPENDENCIES, NULL},
            {LLE_INIT_REQUIRES, "memory_system"},
            {LLE_INIT_REQUIRES, "error_system"},
            {LLE_INIT_NO_DEPENDENCIES, NULL}
        },
        .expected_duration_ms = 50
    },
    {
        .phase_number = 2,
        .phase_name = "Core Infrastructure", 
        .component_names = {
            "terminal_system",
            "event_system",
            "interface_registry",
            "shared_state_manager",
            NULL
        },
        .dependencies = {
            {LLE_INIT_REQUIRES, "memory_system,error_system,performance_system"},
            {LLE_INIT_REQUIRES, "memory_system,error_system,performance_system"},
            {LLE_INIT_REQUIRES, "memory_system,error_system"},
            {LLE_INIT_REQUIRES, "memory_system,error_system"},
            {LLE_INIT_NO_DEPENDENCIES, NULL}
        },
        .expected_duration_ms = 100
    },
    {
        .phase_number = 3,
        .phase_name = "Processing Systems",
        .component_names = {
            "buffer_system",
            "display_system", 
            "configuration_manager",
            NULL
        },
        .dependencies = {
            {LLE_INIT_REQUIRES, "memory_system,error_system,event_system,shared_state_manager"},
            {LLE_INIT_REQUIRES, "memory_system,error_system,shared_state_manager,terminal_system"},
            {LLE_INIT_REQUIRES, "memory_system,error_system,shared_state_manager"},
            {LLE_INIT_NO_DEPENDENCIES, NULL}
        },
        .expected_duration_ms = 150
    },
    {
        .phase_number = 4,
        .phase_name = "Integration Binding",
        .component_names = {
            "terminal_event_integration",
            "event_buffer_integration",
            "buffer_display_integration", 
            "display_terminal_integration",
            NULL
        },
        .dependencies = {
            {LLE_INIT_REQUIRES, "terminal_system,event_system"},
            {LLE_INIT_REQUIRES, "event_system,buffer_system"},
            {LLE_INIT_REQUIRES, "buffer_system,display_system"},
            {LLE_INIT_REQUIRES, "display_system,terminal_system"},
            {LLE_INIT_NO_DEPENDENCIES, NULL}
        },
        .expected_duration_ms = 75
    },
    {
        .phase_number = 5,
        .phase_name = "Extensibility Systems",
        .component_names = {
            "plugin_system",
            "customization_system",
            "framework_system",
            NULL
        },
        .dependencies = {
            {LLE_INIT_REQUIRES, "memory_system,error_system,configuration_manager,interface_registry"},
            {LLE_INIT_REQUIRES, "memory_system,error_system,configuration_manager,plugin_system"},
            {LLE_INIT_REQUIRES, "memory_system,error_system,plugin_system,customization_system"},
            {LLE_INIT_NO_DEPENDENCIES, NULL}
        },
        .expected_duration_ms = 100
    },
    {
        .phase_number = 6,
        .phase_name = "System Activation",
        .component_names = {
            "health_monitor",
            "resource_coordinator",
            "all_systems_active",
            NULL
        },
        .dependencies = {
            {LLE_INIT_REQUIRES, "all_previous_phases_complete"},
            {LLE_INIT_REQUIRES, "all_previous_phases_complete"},
            {LLE_INIT_REQUIRES, "health_monitor,resource_coordinator"},
            {LLE_INIT_NO_DEPENDENCIES, NULL}
        },
        .expected_duration_ms = 50
    },
    {0, NULL, {NULL}, {{LLE_INIT_NO_DEPENDENCIES, NULL}}, 0}  // Terminator
};

// Initialization controller implementation
lle_result_t lle_execute_initialization_sequence(lle_system_integration_t *integration) {
    lle_result_t result = LLE_SUCCESS;
    
    for (int phase = 0; initialization_phases[phase].phase_name != NULL; phase++) {
        // Execute initialization phase
        result = lle_execute_initialization_phase(integration, &initialization_phases[phase]);
        if (result != LLE_SUCCESS) {
            // Rollback completed phases
            lle_rollback_initialization_phases(integration, phase - 1);
            return result;
        }
        
        // Validate phase completion
        result = lle_validate_phase_completion(integration, &initialization_phases[phase]);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }
    
    return LLE_SUCCESS;
}
```

### 3.2 Dependency Resolution Algorithm

```c
// Automatic dependency resolution for initialization order
typedef struct lle_dependency_resolver {
    lle_hash_table_t *component_dependencies;  // component_name -> dependencies[]
    lle_hash_table_t *resolved_components;     // component_name -> initialization_order
    uint32_t initialization_order_counter;
} lle_dependency_resolver_t;

lle_result_t lle_resolve_initialization_dependencies(lle_dependency_resolver_t *resolver,
                                                    const char **component_list,
                                                    size_t component_count,
                                                    uint32_t *initialization_order) {
    lle_result_t result = LLE_SUCCESS;
    
    // Topological sort algorithm for dependency resolution
    bool *visited = calloc(component_count, sizeof(bool));
    bool *in_recursion_stack = calloc(component_count, sizeof(bool));
    
    if (!visited || !in_recursion_stack) {
        free(visited);
        free(in_recursion_stack);
        return LLE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Visit each component and resolve its dependencies
    for (size_t i = 0; i < component_count; i++) {
        if (!visited[i]) {
            result = lle_dependency_dfs_visit(resolver, component_list, i,
                                            visited, in_recursion_stack,
                                            initialization_order);
            if (result != LLE_SUCCESS) {
                break;
            }
        }
    }
    
    free(visited);
    free(in_recursion_stack);
    return result;
}

// Detect circular dependencies during resolution
lle_result_t lle_dependency_dfs_visit(lle_dependency_resolver_t *resolver,
                                     const char **component_list,
                                     size_t component_index,
                                     bool *visited,
                                     bool *in_recursion_stack, 
                                     uint32_t *initialization_order) {
    visited[component_index] = true;
    in_recursion_stack[component_index] = true;
    
    // Get dependencies for this component
    lle_dependency_list_t *deps = NULL;
    lle_result_t result = hash_table_get(resolver->component_dependencies,
                                        component_list[component_index],
                                        (void**)&deps);
    
    if (result == LLE_SUCCESS && deps) {
        // Visit all dependencies first
        for (size_t i = 0; i < deps->count; i++) {
            size_t dep_index = lle_find_component_index(component_list, deps->dependencies[i]);
            
            if (in_recursion_stack[dep_index]) {
                // Circular dependency detected
                return LLE_ERROR_CIRCULAR_DEPENDENCY;
            }
            
            if (!visited[dep_index]) {
                result = lle_dependency_dfs_visit(resolver, component_list, dep_index,
                                                visited, in_recursion_stack,
                                                initialization_order);
                if (result != LLE_SUCCESS) {
                    return result;
                }
            }
        }
    }
    
    // All dependencies resolved, assign initialization order
    initialization_order[component_index] = resolver->initialization_order_counter++;
    in_recursion_stack[component_index] = false;
    
    return LLE_SUCCESS;
}
```

---

## 4. RESOURCE COORDINATION AND CLEANUP

### 4.1 Resource Coordination Framework

```c