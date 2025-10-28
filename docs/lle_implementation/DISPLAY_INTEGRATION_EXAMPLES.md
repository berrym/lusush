# LLE Display Integration - Practical Usage Examples

**Document**: DISPLAY_INTEGRATION_EXAMPLES.md  
**Version**: 1.0.0  
**Date**: 2025-10-28  
**Status**: Complete  
**Related**: DISPLAY_INTEGRATION_API_GUIDE.md

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Basic Rendering](#2-basic-rendering)
3. [Caching Strategies](#3-caching-strategies)
4. [Dirty Region Tracking](#4-dirty-region-tracking)
5. [Theme Integration](#5-theme-integration)
6. [Terminal Adaptation](#6-terminal-adaptation)
7. [Performance Optimization](#7-performance-optimization)
8. [Error Handling Patterns](#8-error-handling-patterns)
9. [Complete Applications](#9-complete-applications)

---

## 1. Getting Started

### 1.1 Minimal Setup

This example shows the absolute minimum code needed to initialize the display integration system.

```c
#include "lle/display_integration.h"
#include "lle/memory_management.h"
#include "lle/error_handling.h"

int main(void) {
    /* Initialize memory pool */
    lle_memory_pool_t *pool = NULL;
    lle_memory_pool_init(&pool, 1024 * 1024); /* 1MB pool */
    
    /* Initialize render pipeline */
    lle_render_pipeline_t *pipeline = NULL;
    lle_result_t result = lle_render_pipeline_init(&pipeline, pool);
    if (result != LLE_SUCCESS) {
        fprintf(stderr, "Failed to initialize pipeline\n");
        return 1;
    }
    
    printf("Display integration initialized successfully\n");
    
    /* Cleanup */
    lle_render_pipeline_cleanup(pipeline);
    lle_memory_pool_cleanup(pool);
    
    return 0;
}
```

### 1.2 Basic Component Initialization

This example initializes all core display components.

```c
#include "lle/display_integration.h"

typedef struct {
    lle_render_pipeline_t *pipeline;
    lle_display_cache_t *cache;
    lle_dirty_tracker_t *tracker;
    lle_memory_pool_t *pool;
} display_system_t;

lle_result_t init_display_system(display_system_t *sys) {
    lle_result_t result;
    
    /* Initialize memory pool */
    result = lle_memory_pool_init(&sys->pool, 2 * 1024 * 1024); /* 2MB */
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    /* Initialize render pipeline */
    result = lle_render_pipeline_init(&sys->pipeline, sys->pool);
    if (result != LLE_SUCCESS) {
        goto error_pool;
    }
    
    /* Initialize cache */
    result = lle_display_cache_init(&sys->cache, sys->pool);
    if (result != LLE_SUCCESS) {
        goto error_pipeline;
    }
    
    /* Initialize dirty tracker */
    result = lle_dirty_tracker_init(&sys->tracker, sys->pool);
    if (result != LLE_SUCCESS) {
        goto error_cache;
    }
    
    printf("Display system initialized successfully\n");
    return LLE_SUCCESS;
    
    /* Error cleanup chain */
error_cache:
    lle_display_cache_cleanup(sys->cache);
error_pipeline:
    lle_render_pipeline_cleanup(sys->pipeline);
error_pool:
    lle_memory_pool_cleanup(sys->pool);
    return result;
}

void cleanup_display_system(display_system_t *sys) {
    lle_dirty_tracker_cleanup(sys->tracker);
    lle_display_cache_cleanup(sys->cache);
    lle_render_pipeline_cleanup(sys->pipeline);
    lle_memory_pool_cleanup(sys->pool);
}
```

---

## 2. Basic Rendering

### 2.1 Simple Text Rendering

Render a simple text buffer to display output.

```c
#include "lle/display_integration.h"
#include "lle/buffer_management.h"

void render_simple_text(lle_render_pipeline_t *pipeline, 
                       const char *text) {
    /* Create buffer with text */
    lle_buffer_t *buffer = NULL;
    lle_memory_pool_t *pool = NULL;
    lle_memory_pool_init(&pool, 4096);
    lle_buffer_create(&buffer, pool, 0);
    lle_buffer_insert_text(buffer, 0, text, strlen(text));
    
    /* Create render context */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    /* Execute pipeline */
    lle_render_output_t *output = NULL;
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    if (result == LLE_SUCCESS) {
        /* Display the rendered output */
        printf("Rendered: %.*s\n", 
               (int)output->content_length, 
               output->content);
    } else {
        fprintf(stderr, "Render failed: %d\n", result);
    }
    
    /* Cleanup */
    lle_buffer_destroy(buffer);
    lle_memory_pool_cleanup(pool);
}

int main(void) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_memory_pool_t *pool = NULL;
    
    lle_memory_pool_init(&pool, 1024 * 1024);
    lle_render_pipeline_init(&pipeline, pool);
    
    render_simple_text(pipeline, "echo 'Hello, World!'");
    
    lle_render_pipeline_cleanup(pipeline);
    lle_memory_pool_cleanup(pool);
    
    return 0;
}
```

### 2.2 Rendering with Cursor Position

Render text with cursor highlighting.

```c
void render_with_cursor(lle_render_pipeline_t *pipeline,
                       lle_buffer_t *buffer,
                       size_t cursor_pos) {
    /* Create context with cursor info */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    /* Execute render */
    lle_render_output_t *output = NULL;
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    if (result == LLE_SUCCESS) {
        /* Output includes cursor position markers */
        write(STDOUT_FILENO, output->content, output->content_length);
    }
}
```

### 2.3 Full Screen Render

Render complete command line with prompt.

```c
void render_full_line(lle_render_pipeline_t *pipeline,
                     const char *prompt,
                     lle_buffer_t *buffer) {
    /* Render prompt first */
    printf("%s", prompt);
    
    /* Render buffer content */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    lle_render_output_t *output = NULL;
    if (lle_render_pipeline_execute(pipeline, &context, &output) == LLE_SUCCESS) {
        write(STDOUT_FILENO, output->content, output->content_length);
    }
    
    fflush(stdout);
}
```

---

## 3. Caching Strategies

### 3.1 Basic Cache Usage

Simple cache workflow with hits and misses.

```c
void render_with_cache(lle_render_pipeline_t *pipeline,
                      lle_display_cache_t *cache,
                      lle_buffer_t *buffer,
                      uint64_t cache_key) {
    /* Try cache first */
    void *cached_data = NULL;
    size_t cached_size = 0;
    
    lle_result_t result = lle_display_cache_lookup(cache, cache_key, 
                                                    &cached_data, &cached_size);
    
    if (result == LLE_SUCCESS) {
        /* Cache hit - use cached render */
        printf("[CACHE HIT] Using cached render\n");
        write(STDOUT_FILENO, cached_data, cached_size);
        return;
    }
    
    /* Cache miss - need to render */
    printf("[CACHE MISS] Rendering fresh\n");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    lle_render_output_t *output = NULL;
    result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    if (result == LLE_SUCCESS) {
        /* Store in cache for next time */
        lle_display_cache_store(cache, cache_key, 
                                output->content, 
                                output->content_length);
        
        /* Display */
        write(STDOUT_FILENO, output->content, output->content_length);
    }
}
```

### 3.2 Smart Cache Key Generation

Generate consistent cache keys from buffer state.

```c
/* FNV-1a hash algorithm */
uint64_t hash_fnv1a(const void *data, size_t len) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    const uint8_t *bytes = (const uint8_t *)data;
    
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }
    
    return hash;
}

uint64_t generate_cache_key(lle_buffer_t *buffer, 
                            size_t cursor_pos,
                            uint32_t theme_id) {
    uint64_t hash = 0;
    
    /* Hash buffer content */
    if (buffer->data && buffer->length > 0) {
        hash = hash_fnv1a(buffer->data, buffer->length);
    }
    
    /* Mix in cursor position */
    hash ^= (cursor_pos << 32);
    
    /* Mix in theme ID (affects colors) */
    hash ^= theme_id;
    
    return hash;
}

/* Usage */
void render_smart_cached(lle_render_pipeline_t *pipeline,
                        lle_display_cache_t *cache,
                        lle_buffer_t *buffer) {
    size_t cursor = get_cursor_position(buffer);
    uint32_t theme = get_current_theme_id();
    
    uint64_t key = generate_cache_key(buffer, cursor, theme);
    render_with_cache(pipeline, cache, buffer, key);
}
```

### 3.3 Cache Invalidation Strategies

Different invalidation patterns for different scenarios.

```c
/* Invalidate on buffer edit */
void on_buffer_edit(display_system_t *sys, 
                   uint64_t old_key,
                   size_t edit_offset,
                   size_t edit_length) {
    /* Invalidate old cache entry */
    lle_display_cache_invalidate(sys->cache, old_key);
    
    /* Mark dirty region */
    lle_dirty_tracker_mark_range(sys->tracker, edit_offset, edit_length);
}

/* Invalidate on theme change */
void on_theme_change(display_system_t *sys) {
    /* Invalidate ALL cache entries (theme affects all renders) */
    lle_display_cache_invalidate_all(sys->cache);
    
    /* Mark full redraw needed */
    lle_dirty_tracker_mark_full(sys->tracker);
}

/* Invalidate on cursor move (only if cursor affects render) */
void on_cursor_move(display_system_t *sys,
                   uint64_t old_key,
                   size_t new_cursor_pos) {
    /* Invalidate old position render */
    lle_display_cache_invalidate(sys->cache, old_key);
    
    /* Mark cursor region dirty */
    lle_dirty_tracker_mark_region(sys->tracker, new_cursor_pos);
}
```

---

## 4. Dirty Region Tracking

### 4.1 Basic Dirty Tracking

Track modified regions for efficient updates.

```c
void track_buffer_changes(lle_dirty_tracker_t *tracker,
                         size_t edit_offset,
                         size_t edit_length) {
    /* Check initial state */
    if (lle_dirty_tracker_needs_full_redraw(tracker)) {
        printf("Initial render - full redraw\n");
        return;
    }
    
    /* Mark edited range dirty */
    lle_result_t result = lle_dirty_tracker_mark_range(tracker, 
                                                        edit_offset, 
                                                        edit_length);
    if (result == LLE_SUCCESS) {
        printf("Marked range [%zu, %zu] as dirty\n", 
               edit_offset, edit_offset + edit_length);
    }
}

/* After rendering */
void complete_render(lle_dirty_tracker_t *tracker) {
    /* Clear dirty state */
    lle_dirty_tracker_clear(tracker);
    printf("Render complete - cleared dirty state\n");
}
```

### 4.2 Conditional Rendering

Only render if needed based on dirty state.

```c
bool should_render(lle_dirty_tracker_t *tracker, size_t cursor_pos) {
    /* Always render on full redraw */
    if (lle_dirty_tracker_needs_full_redraw(tracker)) {
        return true;
    }
    
    /* Check if cursor region is dirty */
    if (lle_dirty_tracker_is_region_dirty(tracker, cursor_pos)) {
        return true;
    }
    
    /* No render needed */
    return false;
}

void conditional_render(display_system_t *sys, lle_buffer_t *buffer) {
    size_t cursor = get_cursor_position(buffer);
    
    if (!should_render(sys->tracker, cursor)) {
        printf("Skipping render - no changes\n");
        return;
    }
    
    printf("Rendering - dirty regions detected\n");
    
    /* Execute render */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    lle_render_output_t *output = NULL;
    if (lle_render_pipeline_execute(sys->pipeline, &context, &output) == LLE_SUCCESS) {
        write(STDOUT_FILENO, output->content, output->content_length);
    }
    
    /* Clear dirty state */
    lle_dirty_tracker_clear(sys->tracker);
}
```

### 4.3 Multi-Region Tracking

Track multiple independent edit regions.

```c
typedef struct {
    size_t offset;
    size_t length;
} edit_region_t;

void track_multiple_edits(lle_dirty_tracker_t *tracker,
                         edit_region_t *edits,
                         size_t edit_count) {
    printf("Tracking %zu edit regions:\n", edit_count);
    
    for (size_t i = 0; i < edit_count; i++) {
        printf("  [%zu]: offset=%zu, length=%zu\n", 
               i, edits[i].offset, edits[i].length);
        
        /* Mark each region dirty */
        lle_dirty_tracker_mark_range(tracker, 
                                     edits[i].offset, 
                                     edits[i].length);
    }
}

/* Example: Multiple paste operations */
void handle_multiple_pastes(display_system_t *sys, lle_buffer_t *buffer) {
    edit_region_t edits[] = {
        { .offset = 10, .length = 5 },   /* Paste at offset 10 */
        { .offset = 50, .length = 8 },   /* Paste at offset 50 */
        { .offset = 100, .length = 12 }  /* Paste at offset 100 */
    };
    
    track_multiple_edits(sys->tracker, edits, 3);
    
    /* Single render handles all dirty regions */
    conditional_render(sys, buffer);
}
```

---

## 5. Theme Integration

### 5.1 Loading and Applying Theme

Load theme and extract colors.

```c
#include "themes.h"

typedef struct {
    lle_syntax_color_table_t *colors;
    uint32_t theme_id;
    char theme_name[64];
} theme_context_t;

lle_result_t load_theme(theme_context_t *ctx, 
                       const char *theme_name,
                       lle_memory_pool_t *pool) {
    /* Load theme definition */
    theme_definition_t *theme = theme_load(theme_name);
    if (!theme) {
        fprintf(stderr, "Failed to load theme: %s\n", theme_name);
        return LLE_ERROR_NOT_FOUND;
    }
    
    /* Extract syntax colors */
    lle_result_t result = lle_extract_syntax_colors_from_theme(theme, 
                                                                &ctx->colors, 
                                                                pool);
    if (result != LLE_SUCCESS) {
        fprintf(stderr, "Failed to extract colors: %d\n", result);
        return result;
    }
    
    /* Save theme info */
    ctx->theme_id = hash_fnv1a(theme_name, strlen(theme_name));
    strncpy(ctx->theme_name, theme_name, sizeof(ctx->theme_name) - 1);
    
    printf("Loaded theme: %s\n", theme_name);
    printf("  Keyword color: #%06x\n", ctx->colors->keyword_color);
    printf("  String color:  #%06x\n", ctx->colors->string_color);
    printf("  Comment color: #%06x\n", ctx->colors->comment_color);
    
    return LLE_SUCCESS;
}
```

### 5.2 Dynamic Theme Switching

Switch themes at runtime.

```c
void switch_theme(display_system_t *sys,
                 theme_context_t *theme_ctx,
                 const char *new_theme_name) {
    printf("Switching theme: %s -> %s\n", 
           theme_ctx->theme_name, new_theme_name);
    
    /* Load new theme */
    lle_result_t result = load_theme(theme_ctx, new_theme_name, sys->pool);
    if (result != LLE_SUCCESS) {
        fprintf(stderr, "Theme switch failed\n");
        return;
    }
    
    /* Invalidate cache (theme affects all renders) */
    lle_display_cache_invalidate_all(sys->cache);
    
    /* Mark full redraw needed */
    lle_dirty_tracker_mark_full(sys->tracker);
    
    printf("Theme switched successfully\n");
}

/* Example: Toggle between light/dark */
void toggle_theme(display_system_t *sys, theme_context_t *theme_ctx) {
    const char *current = theme_ctx->theme_name;
    const char *new_theme;
    
    if (strcmp(current, "solarized-light") == 0) {
        new_theme = "solarized-dark";
    } else {
        new_theme = "solarized-light";
    }
    
    switch_theme(sys, theme_ctx, new_theme);
}
```

### 5.3 Custom Color Override

Override specific theme colors.

```c
void customize_theme_colors(theme_context_t *theme_ctx) {
    /* Override specific colors */
    theme_ctx->colors->keyword_color = 0xFF5733;  /* Bright orange */
    theme_ctx->colors->string_color = 0x33FF57;   /* Bright green */
    theme_ctx->colors->error_color = 0xFF3333;    /* Bright red */
    
    printf("Applied custom color overrides\n");
}

/* Usage */
void setup_custom_theme(display_system_t *sys, theme_context_t *theme_ctx) {
    /* Load base theme */
    load_theme(theme_ctx, "monokai", sys->pool);
    
    /* Apply customizations */
    customize_theme_colors(theme_ctx);
    
    /* Invalidate to apply changes */
    lle_display_cache_invalidate_all(sys->cache);
    lle_dirty_tracker_mark_full(sys->tracker);
}
```

---

## 6. Terminal Adaptation

### 6.1 Capability Detection

Detect and adapt to terminal capabilities.

```c
void detect_terminal_features(lle_terminal_adapter_t *adapter) {
    lle_terminal_capabilities_t caps = {0};
    lle_result_t result = lle_terminal_adapter_get_capabilities(adapter, &caps);
    
    if (result == LLE_SUCCESS) {
        printf("Terminal Capabilities:\n");
        printf("  Width: %zu\n", caps.width);
        printf("  Height: %zu\n", caps.height);
        
        /* Adapt rendering based on capabilities */
        if (caps.supports_truecolor) {
            printf("  Colors: Truecolor (24-bit RGB)\n");
        } else if (caps.supports_256color) {
            printf("  Colors: 256-color palette\n");
        } else {
            printf("  Colors: Basic 16-color\n");
        }
        
        if (caps.supports_unicode) {
            printf("  Unicode: Supported\n");
        } else {
            printf("  Unicode: Not supported (ASCII only)\n");
        }
    }
}
```

### 6.2 Adaptive Rendering

Render differently based on terminal capabilities.

```c
void adaptive_render(lle_terminal_adapter_t *adapter,
                    lle_render_pipeline_t *pipeline,
                    lle_buffer_t *buffer) {
    lle_terminal_capabilities_t caps = {0};
    lle_terminal_adapter_get_capabilities(adapter, &caps);
    
    /* Create context with capability hints */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    /* Adjust rendering based on capabilities */
    if (!caps.supports_unicode) {
        printf("Rendering in ASCII mode\n");
        /* Pipeline will use ASCII-safe output */
    }
    
    if (!caps.supports_256color) {
        printf("Rendering with basic 16 colors\n");
        /* Pipeline will map colors to basic palette */
    }
    
    /* Execute adaptive render */
    lle_render_output_t *output = NULL;
    if (lle_render_pipeline_execute(pipeline, &context, &output) == LLE_SUCCESS) {
        write(STDOUT_FILENO, output->content, output->content_length);
    }
}
```

---

## 7. Performance Optimization

### 7.1 Render Batching

Batch multiple changes into single render.

```c
typedef struct {
    edit_region_t *pending_edits;
    size_t edit_count;
    size_t edit_capacity;
    bool render_scheduled;
} render_batcher_t;

void batch_edit(render_batcher_t *batcher, size_t offset, size_t length) {
    /* Add edit to batch */
    if (batcher->edit_count >= batcher->edit_capacity) {
        /* Expand capacity */
        batcher->edit_capacity *= 2;
        batcher->pending_edits = realloc(batcher->pending_edits, 
                                        batcher->edit_capacity * sizeof(edit_region_t));
    }
    
    batcher->pending_edits[batcher->edit_count].offset = offset;
    batcher->pending_edits[batcher->edit_count].length = length;
    batcher->edit_count++;
    
    /* Schedule render if not already scheduled */
    if (!batcher->render_scheduled) {
        schedule_render();
        batcher->render_scheduled = true;
    }
}

void flush_batch(render_batcher_t *batcher, display_system_t *sys) {
    if (batcher->edit_count == 0) {
        return; /* Nothing to flush */
    }
    
    /* Mark all regions dirty */
    track_multiple_edits(sys->tracker, 
                        batcher->pending_edits, 
                        batcher->edit_count);
    
    /* Execute single render for all changes */
    render_display(sys);
    
    /* Clear batch */
    batcher->edit_count = 0;
    batcher->render_scheduled = false;
}
```

### 7.2 Cache Warming

Pre-populate cache with common renders.

```c
void warm_cache(display_system_t *sys, lle_buffer_t *buffer) {
    printf("Warming cache with common commands...\n");
    
    const char *common_commands[] = {
        "ls -la",
        "cd ..",
        "git status",
        "echo ",
        "cat ",
        NULL
    };
    
    for (size_t i = 0; common_commands[i] != NULL; i++) {
        /* Create buffer with command */
        lle_buffer_t *temp_buffer = NULL;
        lle_buffer_create(&temp_buffer, sys->pool, 0);
        lle_buffer_insert_text(temp_buffer, 0, 
                              common_commands[i], 
                              strlen(common_commands[i]));
        
        /* Render and cache */
        lle_render_context_t context = {0};
        context.buffer = temp_buffer;
        
        lle_render_output_t *output = NULL;
        if (lle_render_pipeline_execute(sys->pipeline, &context, &output) == LLE_SUCCESS) {
            uint64_t key = generate_cache_key(temp_buffer, 0, 0);
            lle_display_cache_store(sys->cache, key, 
                                   output->content, 
                                   output->content_length);
        }
        
        lle_buffer_destroy(temp_buffer);
    }
    
    printf("Cache warmed with %zu entries\n", sizeof(common_commands)/sizeof(char*) - 1);
}
```

### 7.3 Performance Monitoring

Track render performance metrics.

```c
typedef struct {
    uint64_t total_renders;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t total_render_time_us;
} render_stats_t;

void render_with_stats(display_system_t *sys,
                      lle_buffer_t *buffer,
                      render_stats_t *stats,
                      uint64_t cache_key) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Try cache */
    void *cached_data = NULL;
    size_t cached_size = 0;
    
    if (lle_display_cache_lookup(sys->cache, cache_key, 
                                  &cached_data, &cached_size) == LLE_SUCCESS) {
        /* Cache hit */
        stats->cache_hits++;
        write(STDOUT_FILENO, cached_data, cached_size);
    } else {
        /* Cache miss - render */
        stats->cache_misses++;
        
        lle_render_context_t context = {0};
        context.buffer = buffer;
        
        lle_render_output_t *output = NULL;
        if (lle_render_pipeline_execute(sys->pipeline, &context, &output) == LLE_SUCCESS) {
            lle_display_cache_store(sys->cache, cache_key, 
                                   output->content, 
                                   output->content_length);
            write(STDOUT_FILENO, output->content, output->content_length);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + 
                          (end.tv_nsec - start.tv_nsec) / 1000;
    
    stats->total_renders++;
    stats->total_render_time_us += elapsed_us;
}

void print_render_stats(const render_stats_t *stats) {
    double hit_rate = (double)stats->cache_hits / stats->total_renders * 100.0;
    double avg_time_us = (double)stats->total_render_time_us / stats->total_renders;
    
    printf("\n=== Render Statistics ===\n");
    printf("Total renders: %lu\n", stats->total_renders);
    printf("Cache hits: %lu\n", stats->cache_hits);
    printf("Cache misses: %lu\n", stats->cache_misses);
    printf("Hit rate: %.1f%%\n", hit_rate);
    printf("Average render time: %.2f μs\n", avg_time_us);
}
```

---

## 8. Error Handling Patterns

### 8.1 Graceful Degradation

Handle errors without crashing.

```c
void robust_render(display_system_t *sys, lle_buffer_t *buffer) {
    lle_result_t result;
    
    /* Try cached render first */
    uint64_t key = generate_cache_key(buffer, 0, 0);
    void *cached_data = NULL;
    size_t cached_size = 0;
    
    result = lle_display_cache_lookup(sys->cache, key, &cached_data, &cached_size);
    if (result == LLE_SUCCESS) {
        write(STDOUT_FILENO, cached_data, cached_size);
        return; /* Success */
    }
    
    /* Cache miss - try fresh render */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    lle_render_output_t *output = NULL;
    result = lle_render_pipeline_execute(sys->pipeline, &context, &output);
    
    if (result == LLE_SUCCESS) {
        /* Render succeeded */
        write(STDOUT_FILENO, output->content, output->content_length);
        
        /* Try to cache (but don't fail if cache fails) */
        lle_display_cache_store(sys->cache, key, 
                               output->content, 
                               output->content_length);
        return;
    }
    
    /* Render failed - fall back to raw buffer */
    fprintf(stderr, "\n[Render failed, showing raw buffer]\n");
    if (buffer->data && buffer->length > 0) {
        write(STDOUT_FILENO, buffer->data, buffer->length);
    }
}
```

### 8.2 Error Recovery

Recover from component failures.

```c
bool recover_from_error(display_system_t *sys, lle_result_t error) {
    switch (error) {
        case LLE_ERROR_OUT_OF_MEMORY:
            fprintf(stderr, "Out of memory - clearing cache\n");
            lle_display_cache_invalidate_all(sys->cache);
            lle_dirty_tracker_clear(sys->tracker);
            return true; /* Recovered */
            
        case LLE_ERROR_INVALID_PARAMETER:
            fprintf(stderr, "Invalid parameter - resetting state\n");
            lle_dirty_tracker_mark_full(sys->tracker);
            return true; /* Recovered */
            
        default:
            fprintf(stderr, "Unrecoverable error: %d\n", error);
            return false; /* Cannot recover */
    }
}

void render_with_recovery(display_system_t *sys, lle_buffer_t *buffer) {
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    lle_render_output_t *output = NULL;
    lle_result_t result = lle_render_pipeline_execute(sys->pipeline, 
                                                       &context, 
                                                       &output);
    
    if (result != LLE_SUCCESS) {
        /* Try to recover */
        if (recover_from_error(sys, result)) {
            /* Retry after recovery */
            result = lle_render_pipeline_execute(sys->pipeline, 
                                                 &context, 
                                                 &output);
        }
    }
    
    if (result == LLE_SUCCESS) {
        write(STDOUT_FILENO, output->content, output->content_length);
    } else {
        fprintf(stderr, "Render failed after recovery attempt\n");
    }
}
```

---

## 9. Complete Applications

### 9.1 Simple Interactive Shell

Complete example of interactive command line editor.

```c
#include "lle/display_integration.h"
#include "lle/buffer_management.h"
#include <termios.h>
#include <unistd.h>

typedef struct {
    display_system_t display;
    lle_buffer_t *buffer;
    theme_context_t theme;
    render_stats_t stats;
    bool running;
} shell_context_t;

void init_shell(shell_context_t *ctx) {
    /* Initialize display system */
    init_display_system(&ctx->display);
    
    /* Create buffer */
    lle_buffer_create(&ctx->buffer, ctx->display.pool, 0);
    
    /* Load theme */
    load_theme(&ctx->theme, "default", ctx->display.pool);
    
    /* Initialize stats */
    memset(&ctx->stats, 0, sizeof(render_stats_t));
    
    ctx->running = true;
    
    printf("LLE Shell initialized\n");
    printf("Type 'exit' to quit, 'stats' for statistics\n\n");
}

void process_input(shell_context_t *ctx, char ch) {
    size_t cursor = get_cursor_position(ctx->buffer);
    
    if (ch == '\n') {
        /* Execute command */
        char cmd[256] = {0};
        if (ctx->buffer->data && ctx->buffer->length > 0) {
            memcpy(cmd, ctx->buffer->data, 
                   ctx->buffer->length < 255 ? ctx->buffer->length : 255);
        }
        
        printf("\n");
        
        if (strcmp(cmd, "exit") == 0) {
            ctx->running = false;
        } else if (strcmp(cmd, "stats") == 0) {
            print_render_stats(&ctx->stats);
        } else if (strncmp(cmd, "theme ", 6) == 0) {
            switch_theme(&ctx->display, &ctx->theme, cmd + 6);
        } else {
            printf("Executing: %s\n", cmd);
        }
        
        /* Clear buffer for next command */
        lle_buffer_clear(ctx->buffer);
        lle_dirty_tracker_mark_full(ctx->display.tracker);
        
    } else if (ch == 127 || ch == '\b') {
        /* Backspace */
        if (cursor > 0) {
            lle_buffer_delete_text(ctx->buffer, cursor - 1, 1);
            lle_dirty_tracker_mark_region(ctx->display.tracker, cursor - 1);
        }
        
    } else if (ch >= 32 && ch < 127) {
        /* Printable character */
        lle_buffer_insert_text(ctx->buffer, cursor, &ch, 1);
        lle_dirty_tracker_mark_region(ctx->display.tracker, cursor);
    }
}

void render_shell(shell_context_t *ctx) {
    /* Check if render needed */
    if (!should_render(ctx->display.tracker, 
                      get_cursor_position(ctx->buffer))) {
        return;
    }
    
    /* Clear line */
    printf("\r\033[K");
    
    /* Print prompt */
    printf("lle> ");
    
    /* Render buffer */
    uint64_t key = generate_cache_key(ctx->buffer, 
                                      get_cursor_position(ctx->buffer),
                                      ctx->theme.theme_id);
    render_with_stats(&ctx->display, ctx->buffer, &ctx->stats, key);
    
    fflush(stdout);
}

void run_shell(shell_context_t *ctx) {
    /* Set terminal to raw mode */
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    
    /* Main loop */
    while (ctx->running) {
        /* Render current state */
        render_shell(ctx);
        
        /* Read character */
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            process_input(ctx, ch);
        }
    }
    
    /* Restore terminal */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    
    printf("\nFinal statistics:\n");
    print_render_stats(&ctx->stats);
}

void cleanup_shell(shell_context_t *ctx) {
    lle_buffer_destroy(ctx->buffer);
    cleanup_display_system(&ctx->display);
    printf("Shell cleaned up\n");
}

int main(void) {
    shell_context_t ctx = {0};
    
    init_shell(&ctx);
    run_shell(&ctx);
    cleanup_shell(&ctx);
    
    return 0;
}
```

### 9.2 Performance Test Harness

Test performance with various workloads.

```c
void benchmark_render_performance(void) {
    display_system_t sys = {0};
    init_display_system(&sys);
    
    lle_buffer_t *buffer = NULL;
    lle_buffer_create(&buffer, sys.pool, 0);
    
    printf("Running performance benchmarks...\n\n");
    
    /* Benchmark 1: Small commands */
    printf("Benchmark 1: Small commands (1-20 chars)\n");
    const char *small_cmds[] = {"ls", "pwd", "cd ..", "echo hi", NULL};
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < 1000; i++) {
        const char *cmd = small_cmds[i % 4];
        lle_buffer_clear(buffer);
        lle_buffer_insert_text(buffer, 0, cmd, strlen(cmd));
        
        lle_render_context_t context = {0};
        context.buffer = buffer;
        lle_render_output_t *output = NULL;
        lle_render_pipeline_execute(sys.pipeline, &context, &output);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + 
                          (end.tv_nsec - start.tv_nsec) / 1000;
    
    printf("  1000 renders: %lu μs total, %.2f μs average\n", 
           elapsed_us, (double)elapsed_us / 1000.0);
    
    /* Benchmark 2: Large commands */
    printf("\nBenchmark 2: Large commands (100+ chars)\n");
    char large_cmd[256];
    memset(large_cmd, 'x', 200);
    large_cmd[200] = '\0';
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < 1000; i++) {
        lle_buffer_clear(buffer);
        lle_buffer_insert_text(buffer, 0, large_cmd, strlen(large_cmd));
        
        lle_render_context_t context = {0};
        context.buffer = buffer;
        lle_render_output_t *output = NULL;
        lle_render_pipeline_execute(sys.pipeline, &context, &output);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + 
                 (end.tv_nsec - start.tv_nsec) / 1000;
    
    printf("  1000 renders: %lu μs total, %.2f μs average\n", 
           elapsed_us, (double)elapsed_us / 1000.0);
    
    /* Benchmark 3: Cache performance */
    printf("\nBenchmark 3: Cache hit performance\n");
    lle_buffer_clear(buffer);
    lle_buffer_insert_text(buffer, 0, "test", 4);
    
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    lle_render_pipeline_execute(sys.pipeline, &context, &output);
    
    uint64_t key = generate_cache_key(buffer, 0, 0);
    lle_display_cache_store(sys.cache, key, 
                           output->content, 
                           output->content_length);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    void *cached_data = NULL;
    size_t cached_size = 0;
    for (int i = 0; i < 10000; i++) {
        lle_display_cache_lookup(sys.cache, key, &cached_data, &cached_size);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + 
                 (end.tv_nsec - start.tv_nsec) / 1000;
    
    printf("  10000 lookups: %lu μs total, %.2f μs average\n", 
           elapsed_us, (double)elapsed_us / 10000.0);
    
    lle_buffer_destroy(buffer);
    cleanup_display_system(&sys);
}
```

---

**End of Examples Document**

For API reference, see: DISPLAY_INTEGRATION_API_GUIDE.md  
For specification, see: docs/lle_specification/08_display_integration_complete.md
