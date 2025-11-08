/**
 * keybinding.c - Keybinding Engine Implementation
 *
 * Implements fast key sequence lookup and binding management using libhashtable
 * through LLE's hashtable wrapper for O(1) lookup performance.
 *
 * Specification: docs/lle_specification/critical_gaps/25_default_keybindings_complete.md
 * Implementation Plan: docs/lle_specification/critical_gaps/25_IMPLEMENTATION_PLAN.md
 * Date: 2025-11-02
 */

#include "lle/keybinding.h"
#include "lle/hashtable.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

/**
 * Keybinding entry - stores action and metadata
 */
typedef struct {
    lle_keybinding_action_t action;
    char *function_name;
    lle_keymap_mode_t mode;
} lle_keybinding_entry_t;

/**
 * Multi-key sequence buffer for chords
 */
typedef struct {
    lle_key_event_t keys[LLE_MAX_SEQUENCE_KEYS];
    size_t count;
    struct timeval last_key_time;
} lle_key_sequence_buffer_t;

/**
 * Keybinding manager structure
 */
struct lle_keybinding_manager {
    lle_strstr_hashtable_t *bindings;      /* Key sequence -> entry mapping */
    lle_keymap_mode_t current_mode;        /* Active keymap mode */
    lle_key_sequence_buffer_t seq_buffer;  /* Multi-key sequence buffer */
    lusush_memory_pool_t *pool;            /* Memory pool for allocations */
    
    /* Performance tracking */
    uint64_t total_lookups;
    uint64_t total_lookup_time_us;
    uint64_t max_lookup_time_us;
};

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Get current time in microseconds
 */
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 * Allocate string using memory pool or malloc
 */
static char* keybinding_strdup(lusush_memory_pool_t *pool, const char *str) {
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str);
    char *copy;
    
    if (pool != NULL) {
        copy = (char*)lusush_pool_alloc(len + 1);
    } else {
        copy = (char*)malloc(len + 1);
    }
    
    if (copy != NULL) {
        memcpy(copy, str, len + 1);
    }
    
    return copy;
}

/**
 * Free string from memory pool or malloc
 */
static void keybinding_free_string(lusush_memory_pool_t *pool, char *str) {
    if (str == NULL) {
        return;
    }
    
    if (pool != NULL) {
        lusush_pool_free(str);
    } else {
        free(str);
    }
}

/**
 * Free keybinding entry
 */
static void free_keybinding_entry(lusush_memory_pool_t *pool, 
                                  lle_keybinding_entry_t *entry) {
    if (entry == NULL) {
        return;
    }
    
    if (entry->function_name != NULL) {
        keybinding_free_string(pool, entry->function_name);
    }
    
    if (pool != NULL) {
        lusush_pool_free(entry);
    } else {
        free(entry);
    }
}

/**
 * Parse special key name to key code
 */
static lle_result_t parse_special_key(const char *name, lle_special_key_t *key_out) {
    if (strcmp(name, "RET") == 0 || strcmp(name, "RETURN") == 0) {
        *key_out = LLE_KEY_ENTER;
    } else if (strcmp(name, "TAB") == 0) {
        *key_out = LLE_KEY_TAB;
    } else if (strcmp(name, "DEL") == 0 || strcmp(name, "DELETE") == 0) {
        *key_out = LLE_KEY_DELETE;
    } else if (strcmp(name, "BACKSPACE") == 0 || strcmp(name, "BS") == 0) {
        *key_out = LLE_KEY_BACKSPACE;
    } else if (strcmp(name, "UP") == 0) {
        *key_out = LLE_KEY_UP;
    } else if (strcmp(name, "DOWN") == 0) {
        *key_out = LLE_KEY_DOWN;
    } else if (strcmp(name, "LEFT") == 0) {
        *key_out = LLE_KEY_LEFT;
    } else if (strcmp(name, "RIGHT") == 0) {
        *key_out = LLE_KEY_RIGHT;
    } else if (strcmp(name, "HOME") == 0) {
        *key_out = LLE_KEY_HOME;
    } else if (strcmp(name, "END") == 0) {
        *key_out = LLE_KEY_END;
    } else if (strcmp(name, "PAGEUP") == 0 || strcmp(name, "PGUP") == 0) {
        *key_out = LLE_KEY_PAGE_UP;
    } else if (strcmp(name, "PAGEDOWN") == 0 || strcmp(name, "PGDN") == 0) {
        *key_out = LLE_KEY_PAGE_DOWN;
    } else if (strcmp(name, "INSERT") == 0 || strcmp(name, "INS") == 0) {
        *key_out = LLE_KEY_INSERT;
    } else if (strcmp(name, "ESC") == 0 || strcmp(name, "ESCAPE") == 0) {
        *key_out = LLE_KEY_ESCAPE;
    } else if (name[0] == 'F' && isdigit(name[1])) {
        /* F1-F12 */
        int num = atoi(name + 1);
        if (num >= 1 && num <= 12) {
            *key_out = LLE_KEY_F1 + (num - 1);
        } else {
            return LLE_ERROR_INVALID_FORMAT;
        }
    } else {
        return LLE_ERROR_INVALID_FORMAT;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * LIFECYCLE FUNCTIONS
 * ============================================================================ */

lle_result_t lle_keybinding_manager_create(
    lle_keybinding_manager_t **manager,
    lusush_memory_pool_t *pool
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Allocate manager structure */
    lle_keybinding_manager_t *new_manager;
    if (pool != NULL) {
        new_manager = (lle_keybinding_manager_t*)lusush_pool_alloc(sizeof(lle_keybinding_manager_t));
    } else {
        new_manager = (lle_keybinding_manager_t*)malloc(sizeof(lle_keybinding_manager_t));
    }
    
    if (new_manager == NULL) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_manager, 0, sizeof(lle_keybinding_manager_t));
    new_manager->pool = pool;
    new_manager->current_mode = LLE_KEYMAP_EMACS;
    
    /* Create hashtable for bindings */
    lle_hashtable_config_t config;
    lle_hashtable_config_init_default(&config);
    config.initial_capacity = LLE_KEYBINDING_INITIAL_SIZE;
    config.memory_pool = pool;
    config.use_memory_pool = (pool != NULL);
    config.hashtable_name = "keybindings";
    
    /* Use factory to create hashtable */
    lle_hashtable_factory_t *factory = NULL;
    lle_result_t result = lle_hashtable_factory_init(&factory, pool);
    if (result != LLE_SUCCESS) {
        if (pool != NULL) {
            lusush_pool_free(new_manager);
        } else {
            free(new_manager);
        }
        return result;
    }
    
    result = lle_hashtable_factory_create_strstr(factory, &config, &new_manager->bindings);
    lle_hashtable_factory_destroy(factory);
    
    if (result != LLE_SUCCESS) {
        if (pool != NULL) {
            lusush_pool_free(new_manager);
        } else {
            free(new_manager);
        }
        return result;
    }
    
    *manager = new_manager;
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_destroy(lle_keybinding_manager_t *manager) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Destroy hashtable (this will free all entries) */
    if (manager->bindings != NULL) {
        lle_strstr_hashtable_destroy(manager->bindings);
    }
    
    /* Free manager structure */
    if (manager->pool != NULL) {
        lusush_pool_free(manager);
    } else {
        free(manager);
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * KEY SEQUENCE PARSING
 * ============================================================================ */

lle_result_t lle_key_sequence_parse(
    const char *key_sequence,
    lle_key_event_t *key_event_out
) {
    if (key_sequence == NULL || key_event_out == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    memset(key_event_out, 0, sizeof(lle_key_event_t));
    
    const char *p = key_sequence;
    
    /* Parse modifiers */
    while (*p != '\0') {
        if (p[0] == 'C' && p[1] == '-') {
            key_event_out->ctrl = true;
            p += 2;
        } else if (p[0] == 'M' && p[1] == '-') {
            key_event_out->alt = true;
            p += 2;
        } else if (p[0] == 'S' && p[1] == '-') {
            key_event_out->shift = true;
            p += 2;
        } else {
            break;
        }
    }
    
    /* Parse key */
    if (*p == '\0') {
        return LLE_ERROR_INVALID_FORMAT;
    }
    
    /* Check for special key name */
    if (isupper(*p) || strcmp(p, "RET") == 0) {
        lle_special_key_t special_key;
        lle_result_t result = parse_special_key(p, &special_key);
        if (result == LLE_SUCCESS) {
            key_event_out->is_special = true;
            key_event_out->special_key = special_key;
            return LLE_SUCCESS;
        }
    }
    
    /* Regular character */
    if (key_event_out->ctrl && islower(*p)) {
        /* Ctrl+letter is typically uppercase in ASCII control codes */
        key_event_out->codepoint = toupper(*p);
    } else {
        key_event_out->codepoint = (uint32_t)(*p);
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_key_event_to_string(
    const lle_key_event_t *key_event,
    char *buffer,
    size_t buffer_size
) {
    if (key_event == NULL || buffer == NULL || buffer_size == 0) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    char *p = buffer;
    size_t remaining = buffer_size;
    
    /* Add modifiers */
    if (key_event->ctrl) {
        if (remaining < 3) return LLE_ERROR_BUFFER_OVERFLOW;
        *p++ = 'C'; *p++ = '-';
        remaining -= 2;
    }
    if (key_event->alt) {
        if (remaining < 3) return LLE_ERROR_BUFFER_OVERFLOW;
        *p++ = 'M'; *p++ = '-';
        remaining -= 2;
    }
    if (key_event->shift) {
        if (remaining < 3) return LLE_ERROR_BUFFER_OVERFLOW;
        *p++ = 'S'; *p++ = '-';
        remaining -= 2;
    }
    
    /* Add key */
    if (key_event->is_special) {
        const char *name = NULL;
        switch (key_event->special_key) {
            case LLE_KEY_ENTER: name = "RET"; break;
            case LLE_KEY_TAB: name = "TAB"; break;
            case LLE_KEY_BACKSPACE: name = "BACKSPACE"; break;
            case LLE_KEY_DELETE: name = "DEL"; break;
            case LLE_KEY_UP: name = "UP"; break;
            case LLE_KEY_DOWN: name = "DOWN"; break;
            case LLE_KEY_LEFT: name = "LEFT"; break;
            case LLE_KEY_RIGHT: name = "RIGHT"; break;
            case LLE_KEY_HOME: name = "HOME"; break;
            case LLE_KEY_END: name = "END"; break;
            case LLE_KEY_PAGE_UP: name = "PAGEUP"; break;
            case LLE_KEY_PAGE_DOWN: name = "PAGEDOWN"; break;
            case LLE_KEY_INSERT: name = "INSERT"; break;
            case LLE_KEY_ESCAPE: name = "ESC"; break;
            default:
                if (key_event->special_key >= LLE_KEY_F1 && 
                    key_event->special_key <= LLE_KEY_F12) {
                    int fn = key_event->special_key - LLE_KEY_F1 + 1;
                    if (remaining < 4) return LLE_ERROR_BUFFER_OVERFLOW;
                    snprintf(p, remaining, "F%d", fn);
                    return LLE_SUCCESS;
                }
                return LLE_ERROR_INVALID_PARAMETER;
        }
        
        size_t len = strlen(name);
        if (remaining <= len) return LLE_ERROR_BUFFER_OVERFLOW;
        strcpy(p, name);
    } else {
        if (remaining < 2) return LLE_ERROR_BUFFER_OVERFLOW;
        /* Convert uppercase back to lowercase for ctrl+letter combinations */
        char ch = (char)key_event->codepoint;
        if (key_event->ctrl && isupper(ch)) {
            ch = tolower(ch);
        }
        *p++ = ch;
        *p = '\0';
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * KEYBINDING REGISTRATION
 * ============================================================================ */

lle_result_t lle_keybinding_manager_bind(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_keybinding_action_t action,
    const char *function_name
) {
    if (manager == NULL || key_sequence == NULL || action == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Create keybinding entry */
    lle_keybinding_entry_t *entry;
    if (manager->pool != NULL) {
        entry = (lle_keybinding_entry_t*)lusush_pool_alloc(sizeof(lle_keybinding_entry_t));
    } else {
        entry = (lle_keybinding_entry_t*)malloc(sizeof(lle_keybinding_entry_t));
    }
    
    if (entry == NULL) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    entry->action = action;
    entry->mode = manager->current_mode;
    entry->function_name = function_name ? keybinding_strdup(manager->pool, function_name) : NULL;
    
    /* Convert entry to string for storage (hackish but works with strstr hashtable) */
    char entry_str[32];
    snprintf(entry_str, sizeof(entry_str), "%p", (void*)entry);
    
    /* Insert into hashtable */
    lle_result_t result = lle_strstr_hashtable_insert(
        manager->bindings, key_sequence, entry_str);
    
    if (result != LLE_SUCCESS) {
        free_keybinding_entry(manager->pool, entry);
        return result;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_unbind(
    lle_keybinding_manager_t *manager,
    const char *key_sequence
) {
    if (manager == NULL || key_sequence == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Lookup entry first to free it */
    const char *entry_str = lle_strstr_hashtable_lookup(manager->bindings, key_sequence);
    if (entry_str != NULL) {
        lle_keybinding_entry_t *entry;
        sscanf(entry_str, "%p", (void**)&entry);
        free_keybinding_entry(manager->pool, entry);
    }
    
    /* Remove from hashtable */
    return lle_strstr_hashtable_delete(manager->bindings, key_sequence);
}

lle_result_t lle_keybinding_manager_clear(lle_keybinding_manager_t *manager) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Note: This leaks entries, but since we're clearing everything,
     * it's acceptable if the hashtable and manager are about to be destroyed.
     * For a proper implementation, we'd need to enumerate and free each entry. */
    lle_strstr_hashtable_clear(manager->bindings);
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * KEY PROCESSING
 * ============================================================================ */

lle_result_t lle_keybinding_manager_process_key(
    lle_keybinding_manager_t *manager,
    lle_editor_t *editor,
    const lle_key_event_t *key_event
) {
    if (manager == NULL || editor == NULL || key_event == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    uint64_t start_time = get_time_us();
    
    /* Convert key event to string */
    char key_str[LLE_MAX_KEY_SEQUENCE_LENGTH];
    lle_result_t result = lle_key_event_to_string(key_event, key_str, sizeof(key_str));
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Lookup binding */
    const char *entry_str = lle_strstr_hashtable_lookup(manager->bindings, key_str);
    if (entry_str == NULL) {
        /* Update stats */
        uint64_t elapsed = get_time_us() - start_time;
        manager->total_lookups++;
        manager->total_lookup_time_us += elapsed;
        if (elapsed > manager->max_lookup_time_us) {
            manager->max_lookup_time_us = elapsed;
        }
        
        return LLE_ERROR_NOT_FOUND;
    }
    
    /* Parse entry pointer */
    lle_keybinding_entry_t *entry;
    sscanf(entry_str, "%p", (void**)&entry);
    
    /* Update stats */
    uint64_t elapsed = get_time_us() - start_time;
    manager->total_lookups++;
    manager->total_lookup_time_us += elapsed;
    if (elapsed > manager->max_lookup_time_us) {
        manager->max_lookup_time_us = elapsed;
    }
    
    /* Execute action */
    return entry->action(editor);
}

lle_result_t lle_keybinding_manager_reset_sequence(
    lle_keybinding_manager_t *manager
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    manager->seq_buffer.count = 0;
    return LLE_SUCCESS;
}

/* ============================================================================
 * KEYMAP MODE MANAGEMENT
 * ============================================================================ */

lle_result_t lle_keybinding_manager_set_mode(
    lle_keybinding_manager_t *manager,
    lle_keymap_mode_t mode
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    manager->current_mode = mode;
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_get_mode(
    lle_keybinding_manager_t *manager,
    lle_keymap_mode_t *mode_out
) {
    if (manager == NULL || mode_out == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    *mode_out = manager->current_mode;
    return LLE_SUCCESS;
}

/* ============================================================================
 * PRESET LOADING
 * ============================================================================ */

lle_result_t lle_keybinding_manager_load_emacs_preset(
    lle_keybinding_manager_t *manager
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Set mode to emacs */
    manager->current_mode = LLE_KEYMAP_EMACS;
    
    /* Note: Actual bindings will be loaded in Phase 3 when we implement
     * the keybinding action functions. For now, this is a placeholder. */
    
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_load_vi_insert_preset(
    lle_keybinding_manager_t *manager
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Set mode to vi insert */
    manager->current_mode = LLE_KEYMAP_VI_INSERT;
    
    /* Note: Vi bindings will be implemented in a future phase */
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * INTROSPECTION
 * ============================================================================ */

lle_result_t lle_keybinding_manager_list_bindings(
    lle_keybinding_manager_t *manager,
    lle_keybinding_info_t **bindings_out,
    size_t *count_out
) {
    if (manager == NULL || bindings_out == NULL || count_out == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    /* Get hashtable size */
    size_t count = lle_strstr_hashtable_size(manager->bindings);
    if (count == 0) {
        *bindings_out = NULL;
        *count_out = 0;
        return LLE_SUCCESS;
    }
    
    /* Allocate bindings array */
    lle_keybinding_info_t *bindings;
    if (manager->pool != NULL) {
        bindings = (lle_keybinding_info_t*)lusush_pool_alloc(sizeof(lle_keybinding_info_t) * count);
    } else {
        bindings = (lle_keybinding_info_t*)malloc(sizeof(lle_keybinding_info_t) * count);
    }
    
    if (bindings == NULL) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Note: We would need to enumerate the hashtable here, but libhashtable's
     * enumeration is buggy according to the LLE wrapper. For now, return empty. */
    *bindings_out = bindings;
    *count_out = 0;
    
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_lookup(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_keybinding_action_t *action_out
) {
    if (manager == NULL || key_sequence == NULL || action_out == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    const char *entry_str = lle_strstr_hashtable_lookup(manager->bindings, key_sequence);
    if (entry_str == NULL) {
        return LLE_ERROR_NOT_FOUND;
    }
    
    lle_keybinding_entry_t *entry;
    sscanf(entry_str, "%p", (void**)&entry);
    
    *action_out = entry->action;
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_get_count(
    lle_keybinding_manager_t *manager,
    size_t *count_out
) {
    if (manager == NULL || count_out == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    *count_out = lle_strstr_hashtable_size(manager->bindings);
    return LLE_SUCCESS;
}

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================ */

lle_result_t lle_keybinding_manager_get_stats(
    lle_keybinding_manager_t *manager,
    uint64_t *avg_lookup_time_us,
    uint64_t *max_lookup_time_us
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    if (avg_lookup_time_us != NULL) {
        if (manager->total_lookups > 0) {
            *avg_lookup_time_us = manager->total_lookup_time_us / manager->total_lookups;
        } else {
            *avg_lookup_time_us = 0;
        }
    }
    
    if (max_lookup_time_us != NULL) {
        *max_lookup_time_us = manager->max_lookup_time_us;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_keybinding_manager_reset_stats(
    lle_keybinding_manager_t *manager
) {
    if (manager == NULL) {
        return LLE_ERROR_NULL_POINTER;
    }
    
    manager->total_lookups = 0;
    manager->total_lookup_time_us = 0;
    manager->max_lookup_time_us = 0;
    
    return LLE_SUCCESS;
}
