# Lusush Hash Table Implementation Guide

**Project:** Lusush Shell Enhancement Project  
**Module:** All Performance Optimizations and Caching Systems  
**Library:** `src/libhashtable` (Lusush's Built-in Hash Table Library)  
**Status:** **MANDATORY REQUIREMENT** for all hash table implementations

## 🚨 CRITICAL REQUIREMENT

**ALL hash table implementations in Lusush MUST use the existing `src/libhashtable` library.**

❌ **DO NOT:**
- Implement custom hash tables
- Use external hash table libraries (glib, uthash, etc.)
- Create your own hash map implementations
- Use std::unordered_map or other external containers

✅ **DO:**
- Use `src/libhashtable` for all hash table needs
- Follow existing Lusush patterns for hash table usage
- Maintain consistency with the rest of the codebase

## 📚 Available Hash Table Types

Lusush's `libhashtable` provides the following specialized hash tables:

### String-to-String Hash Table (`ht_strstr_t`)
**Use Cases:** Command descriptions, configuration mappings, name-value pairs
```c
#include "../include/libhashtable/ht.h"

ht_strstr_t *table = ht_strstr_create(128);
ht_strstr_insert(table, "key", "value");
const char *result = ht_strstr_get(table, "key");
ht_strstr_destroy(table);
```

### String-to-Integer Hash Table (`ht_strint_t`)
**Use Cases:** Counters, statistics, configuration values
```c
ht_strint_t *counters = ht_strint_create(64);
int count = 42;
ht_strint_insert(counters, "usage_count", &count);
int *result = ht_strint_get(counters, "usage_count");
ht_strint_destroy(counters);
```

### String-to-Double Hash Table (`ht_strdouble_t`)
**Use Cases:** Performance metrics, timing statistics
```c
ht_strdouble_t *metrics = ht_strdouble_create(32);
double timing = 1.25;
ht_strdouble_insert(metrics, "avg_response_ms", &timing);
double *result = ht_strdouble_get(metrics, "avg_response_ms");
ht_strdouble_destroy(metrics);
```

### String-to-Float Hash Table (`ht_strfloat_t`)
**Use Cases:** Lightweight metrics, ratios
```c
ht_strfloat_t *ratios = ht_strfloat_create(16);
float ratio = 0.85f;
ht_strfloat_insert(ratios, "cache_hit_ratio", &ratio);
float *result = ht_strfloat_get(ratios, "cache_hit_ratio");
ht_strfloat_destroy(ratios);
```

## 🔧 Implementation Patterns

### Pattern 1: Simple Caching (Menu Completion Example)
```c
#include "../include/libhashtable/ht.h"

// Global cache variable
static ht_strstr_t *description_cache = NULL;

// Initialize cache
static void init_description_cache(void) {
    if (description_cache) return;
    
    description_cache = ht_strstr_create(128); // Initial capacity
    if (!description_cache) {
        fprintf(stderr, "lusush: failed to create description cache\n");
        return;
    }
    
    // Pre-populate with known values
    ht_strstr_insert(description_cache, "ls", "list directory contents");
    ht_strstr_insert(description_cache, "cd", "change directory");
    ht_strstr_insert(description_cache, "pwd", "print working directory");
    // ... more entries
}

// Fast O(1) lookup
static const char* get_cached_description(const char *command) {
    if (!command || !description_cache) return NULL;
    return ht_strstr_get(description_cache, command);
}

// Proper cleanup
static void cleanup_description_cache(void) {
    if (description_cache) {
        ht_strstr_destroy(description_cache);
        description_cache = NULL;
    }
}
```

### Pattern 2: Dynamic Insertion and Removal
```c
// Add entries dynamically
void cache_completion_result(const char *prefix, const char *result) {
    if (!prefix || !result || !cache_table) return;
    
    // Insert or update entry
    ht_strstr_insert(cache_table, prefix, result);
}

// Remove entries when needed
void invalidate_cache_entry(const char *prefix) {
    if (!prefix || !cache_table) return;
    
    ht_strstr_remove(cache_table, prefix);
}
```

### Pattern 3: Enumeration (Iteration)
```c
// Iterate through all entries
void print_all_cached_entries(void) {
    if (!cache_table) return;
    
    ht_enum_t *enumerator = ht_strstr_enum_create(cache_table);
    if (!enumerator) return;
    
    const char *key, *value;
    while (ht_strstr_enum_next(enumerator, &key, &value)) {
        printf("Key: %s, Value: %s\n", key, value);
    }
    
    ht_strstr_enum_destroy(enumerator);
}
```

## 🎯 Specific Use Cases in Lusush

### Menu Completion Caching
**File:** `src/menu_completion.c`  
**Purpose:** Cache command descriptions for instant lookup
```c
// Global cache for command descriptions
static ht_strstr_t *description_cache = NULL;

// Initialize with common commands
static void init_description_cache(void) {
    description_cache = ht_strstr_create(128);
    ht_strstr_insert(description_cache, "git", "version control system");
    ht_strstr_insert(description_cache, "ssh", "secure shell connection");
    // ... 80+ pre-built descriptions
}

// O(1) lookup instead of linear search
static char *get_fast_description(const char *command) {
    const char *desc = ht_strstr_get(description_cache, command);
    return desc ? strdup(desc) : NULL;
}
```

### Completion History Tracking
**File:** `src/completion.c` (Future Enhancement)  
**Purpose:** Track completion usage frequency
```c
static ht_strint_t *completion_usage = NULL;

void record_completion_usage(const char *completion) {
    if (!completion_usage) {
        completion_usage = ht_strint_create(256);
    }
    
    int *current_count = ht_strint_get(completion_usage, completion);
    int new_count = current_count ? (*current_count + 1) : 1;
    ht_strint_insert(completion_usage, completion, &new_count);
}
```

### Performance Metrics Collection
**File:** `src/themes.c` (Future Enhancement)  
**Purpose:** Track theme switching performance
```c
static ht_strdouble_t *theme_metrics = NULL;

void record_theme_switch_time(const char *theme_name, double elapsed_ms) {
    if (!theme_metrics) {
        theme_metrics = ht_strdouble_create(32);
    }
    
    ht_strdouble_insert(theme_metrics, theme_name, &elapsed_ms);
}
```

## 📋 Best Practices

### 1. Initialization and Cleanup
```c
// Always check for existing instance
static void init_hash_table(void) {
    if (my_table) return; // Already initialized
    
    my_table = ht_strstr_create(initial_size);
    if (!my_table) {
        fprintf(stderr, "lusush: module: failed to create hash table\n");
        return;
    }
}

// Always provide cleanup function
static void cleanup_hash_table(void) {
    if (my_table) {
        ht_strstr_destroy(my_table);
        my_table = NULL;
    }
}
```

### 2. Capacity Planning
```c
// Choose appropriate initial sizes:
// Small cache (< 50 items): 32 or 64
// Medium cache (50-200 items): 128 or 256  
// Large cache (200+ items): 512 or 1024

ht_strstr_t *small_cache = ht_strstr_create(64);    // Command descriptions
ht_strstr_t *medium_cache = ht_strstr_create(256);  // File completions
ht_strstr_t *large_cache = ht_strstr_create(1024);  // Full command history
```

### 3. Error Handling
```c
static const char* safe_get_cached_value(const char *key) {
    if (!key || !cache_table) {
        return NULL; // Safe fallback
    }
    
    const char *result = ht_strstr_get(cache_table, key);
    return result; // ht_strstr_get handles NULL cases internally
}
```

### 4. Memory Management
```c
// For string-to-string tables, the library handles string copying
// You don't need to manage key/value memory manually

void cache_string_value(const char *key, const char *value) {
    // Library automatically copies key and value strings
    ht_strstr_insert(cache_table, key, value);
    
    // Original key and value can go out of scope safely
}
```

## 🚫 Common Mistakes to Avoid

### ❌ Don't Create Multiple Tables for Same Purpose
```c
// WRONG: Multiple tables for related data
static ht_strstr_t *command_descriptions = NULL;
static ht_strstr_t *command_aliases = NULL;
static ht_strstr_t *command_categories = NULL;

// BETTER: Single table with prefixed keys
static ht_strstr_t *command_metadata = NULL;
// Keys: "desc:ls", "alias:ll", "category:file"
```

### ❌ Don't Forget Cleanup in Module Cleanup Functions
```c
// WRONG: Missing cleanup
void lusush_module_cleanup(void) {
    // cleanup other resources
    // forgot to cleanup hash tables!
}

// RIGHT: Comprehensive cleanup
void lusush_module_cleanup(void) {
    cleanup_command_cache();
    cleanup_performance_metrics();
    // other cleanups
}
```

### ❌ Don't Use External Hash Table Libraries
```c
// WRONG: External dependencies
#include <glib.h>
GHashTable *table = g_hash_table_new(...);

// WRONG: Custom implementation
struct custom_hash_map { ... };

// RIGHT: Use lusush's built-in library
#include "../include/libhashtable/ht.h"
ht_strstr_t *table = ht_strstr_create(128);
```

## 🔗 Integration with Existing Modules

### Module Initialization Pattern
```c
// In module initialization function
bool lusush_module_init(void) {
    // Other initialization...
    
    // Initialize hash tables
    init_module_caches();
    
    initialized = true;
    return true;
}
```

### Module Cleanup Pattern
```c
// In module cleanup function
void lusush_module_cleanup(void) {
    if (!initialized) return;
    
    // Cleanup hash tables first
    cleanup_module_caches();
    
    // Other cleanup...
    initialized = false;
}
```

## 📈 Performance Guidelines

### Hash Table Sizing
- **Start small**: Begin with conservative sizes (32-128)
- **Growth is automatic**: Library handles resizing internally
- **Monitor performance**: Use enumeration to check table fullness

### Access Patterns
- **Pre-populate**: Insert common entries during initialization
- **Batch operations**: Group multiple insertions when possible
- **Avoid frequent removal**: Removals can impact performance

### Memory Usage
- **String tables**: Library copies strings, plan for 2x memory overhead
- **Numeric tables**: Store pointers to values, not values directly
- **Cleanup promptly**: Destroy tables when modules shut down

## 🎯 Future Enhancement Guidelines

When adding new caching or lookup features to Lusush:

1. **Always use `src/libhashtable`** - No exceptions
2. **Follow existing patterns** - See menu completion implementation
3. **Plan for cleanup** - Every table needs a cleanup function
4. **Document usage** - Add entries to this guide for new use cases
5. **Test thoroughly** - Verify no memory leaks with valgrind

## 📚 API Reference Summary

### Creation and Destruction
```c
ht_strstr_t *ht_strstr_create(unsigned int initial_size);
void ht_strstr_destroy(ht_strstr_t *table);
```

### Data Operations
```c
void ht_strstr_insert(ht_strstr_t *table, const char *key, const char *value);
const char *ht_strstr_get(ht_strstr_t *table, const char *key);
void ht_strstr_remove(ht_strstr_t *table, const char *key);
```

### Enumeration
```c
ht_enum_t *ht_strstr_enum_create(ht_strstr_t *table);
bool ht_strstr_enum_next(ht_enum_t *enumerator, const char **key, const char **value);
void ht_strstr_enum_destroy(ht_enum_t *enumerator);
```

---

## 🏆 Conclusion

Using `src/libhashtable` ensures:
- **Consistency** with existing Lusush architecture
- **Performance** with optimized hash functions and automatic resizing
- **Reliability** with well-tested, production-ready code
- **Maintainability** with standard patterns across the codebase

**Remember: ALL hash table implementations in Lusush must use `src/libhashtable`. This is a non-negotiable architectural requirement.**