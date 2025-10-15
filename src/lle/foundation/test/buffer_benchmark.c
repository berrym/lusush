// src/lle/foundation/test/buffer_benchmark.c
//
// Performance benchmarks for LLE buffer system
// Week 8: Testing & Documentation - Performance benchmarks

#include "../buffer/buffer.h"
#include "../buffer/undo.h"
#include "../buffer/buffer_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Print benchmark result
static void print_result(const char *name, size_t iterations, uint64_t total_ns) {
    double avg_us = (double)total_ns / (double)iterations / 1000.0;
    double ops_per_sec = 1000000000.0 / ((double)total_ns / (double)iterations);
    printf("%-40s: %8.3f μs/op (%10.0f ops/sec)\n", name, avg_us, ops_per_sec);
}

// Benchmark: Buffer initialization
static void benchmark_buffer_init(void) {
    const size_t iterations = 10000;
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_t buffer;
        lle_buffer_init(&buffer, 256);
        lle_buffer_cleanup(&buffer);
    }
    
    uint64_t elapsed = get_time_ns() - start;
    print_result("Buffer init/cleanup", iterations, elapsed);
}

// Benchmark: Single character insertion
static void benchmark_char_insert(void) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 1024);
    
    const size_t iterations = 10000;
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_insert_char(&buffer, 0, 'a');
    }
    
    uint64_t elapsed = get_time_ns() - start;
    print_result("Single char insert (at start)", iterations, elapsed);
    
    lle_buffer_cleanup(&buffer);
}

// Benchmark: String insertion
static void benchmark_string_insert(void) {
    lle_buffer_t buffer;
    lle_buffer_init(&buffer, 10240);
    
    const char *text = "hello world";
    const size_t text_len = strlen(text);
    const size_t iterations = 10000;
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_insert_string(&buffer, 0, text, text_len);
    }
    
    uint64_t elapsed = get_time_ns() - start;
    print_result("String insert (11 chars, at start)", iterations, elapsed);
    
    lle_buffer_cleanup(&buffer);
}

// Benchmark: Character deletion
static void benchmark_char_delete(void) {
    lle_buffer_t buffer;
    lle_buffer_init_from_string(&buffer, "aaaaaaaaaa", 10000);
    
    const size_t iterations = 5000;
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations && lle_buffer_size(&buffer) > 0; i++) {
        lle_buffer_delete_char(&buffer, 0);
    }
    
    uint64_t elapsed = get_time_ns() - start;
    print_result("Single char delete (from start)", iterations, elapsed);
    
    lle_buffer_cleanup(&buffer);
}

// Benchmark: Range deletion
static void benchmark_range_delete(void) {
    const size_t iterations = 1000;
    uint64_t total = 0;
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_t buffer;
        lle_buffer_init(&buffer, 1024);
        
        // Insert 100 chars
        for (int j = 0; j < 100; j++) {
            lle_buffer_insert_char(&buffer, j, 'a');
        }
        
        uint64_t start = get_time_ns();
        lle_buffer_delete_range(&buffer, 0, 50);
        total += get_time_ns() - start;
        
        lle_buffer_cleanup(&buffer);
    }
    
    print_result("Range delete (50 chars)", iterations, total);
}

// Benchmark: Buffer growth
static void benchmark_buffer_growth(void) {
    const size_t iterations = 100;
    uint64_t total = 0;
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_t buffer;
        lle_buffer_init(&buffer, 16); // Small initial capacity
        
        uint64_t start = get_time_ns();
        
        // Insert until growth happens multiple times
        for (int j = 0; j < 1000; j++) {
            lle_buffer_insert_char(&buffer, j, 'a');
        }
        
        total += get_time_ns() - start;
        lle_buffer_cleanup(&buffer);
    }
    
    print_result("Buffer growth (1000 inserts from 16)", iterations, total);
}

// Benchmark: Undo/redo
static void benchmark_undo_redo(void) {
    const size_t iterations = 1000;
    uint64_t undo_total = 0, redo_total = 0;
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_t buffer;
        lle_change_tracker_t tracker;
        
        lle_buffer_init(&buffer, 256);
        lle_change_tracker_init(&tracker, 100, 1024 * 1024);
        
        // Record some operations
        lle_change_tracker_begin_sequence(&tracker);
        for (int j = 0; j < 10; j++) {
            lle_buffer_insert_char(&buffer, j, 'a');
            lle_change_tracker_record_insert(&tracker, j, "a", 1, j, j+1);
        }
        lle_change_tracker_end_sequence(&tracker);
        
        // Benchmark undo
        uint64_t start = get_time_ns();
        lle_change_tracker_undo(&tracker, &buffer);
        undo_total += get_time_ns() - start;
        
        // Benchmark redo
        start = get_time_ns();
        lle_change_tracker_redo(&tracker, &buffer);
        redo_total += get_time_ns() - start;
        
        lle_buffer_cleanup(&buffer);
        lle_change_tracker_cleanup(&tracker);
    }
    
    print_result("Undo (10 char sequence)", iterations, undo_total);
    print_result("Redo (10 char sequence)", iterations, redo_total);
}

// Benchmark: Buffer manager operations
static void benchmark_buffer_manager(void) {
    const size_t iterations = 1000;
    uint64_t create_total = 0, switch_total = 0, delete_total = 0;
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_manager_t manager;
        lle_buffer_manager_init(&manager, 50, 256);
        
        uint32_t id1, id2, id3;
        
        // Benchmark create
        uint64_t start = get_time_ns();
        lle_buffer_manager_create_buffer(&manager, "test1", &id1);
        create_total += get_time_ns() - start;
        
        lle_buffer_manager_create_buffer(&manager, "test2", &id2);
        lle_buffer_manager_create_buffer(&manager, "test3", &id3);
        
        // Benchmark switch
        start = get_time_ns();
        lle_buffer_manager_switch_to_buffer(&manager, id2);
        switch_total += get_time_ns() - start;
        
        // Benchmark delete
        start = get_time_ns();
        lle_buffer_manager_delete_buffer(&manager, id1);
        delete_total += get_time_ns() - start;
        
        lle_buffer_manager_cleanup(&manager);
    }
    
    print_result("Buffer manager: create buffer", iterations, create_total);
    print_result("Buffer manager: switch buffer", iterations, switch_total);
    print_result("Buffer manager: delete buffer", iterations, delete_total);
}

// Benchmark: Search operations
static void benchmark_search(void) {
    lle_buffer_t buffer;
    const char *text = "The quick brown fox jumps over the lazy dog. ";
    size_t text_len = strlen(text);
    
    // Build large buffer
    lle_buffer_init(&buffer, 100000);
    for (int i = 0; i < 1000; i++) {
        lle_buffer_insert_string(&buffer, lle_buffer_size(&buffer), text, text_len);
    }
    
    const size_t iterations = 10000;
    uint64_t start = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_find_char(&buffer, 0, 'z');
    }
    
    uint64_t elapsed = get_time_ns() - start;
    print_result("Find char in 45KB buffer", iterations, elapsed);
    
    start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_find_string(&buffer, 0, "lazy", 4);
    }
    
    elapsed = get_time_ns() - start;
    print_result("Find string in 45KB buffer", iterations, elapsed);
    
    lle_buffer_cleanup(&buffer);
}

// Benchmark: Line operations
static void benchmark_line_operations(void) {
    lle_buffer_t buffer;
    
    // Build buffer with many lines
    lle_buffer_init(&buffer, 10000);
    for (int i = 0; i < 100; i++) {
        lle_buffer_insert_string(&buffer, lle_buffer_size(&buffer), "line of text\n", 13);
    }
    
    const size_t iterations = 10000;
    
    uint64_t start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_line_start(&buffer, 650); // Middle of buffer
    }
    uint64_t elapsed = get_time_ns() - start;
    print_result("Find line start (100 lines)", iterations, elapsed);
    
    start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        lle_buffer_line_count(&buffer);
    }
    elapsed = get_time_ns() - start;
    print_result("Count lines (100 lines)", iterations, elapsed);
    
    lle_buffer_cleanup(&buffer);
}

// Memory usage report
static void report_memory_usage(void) {
    printf("\n=== Memory Usage ===\n");
    
    // Buffer overhead
    printf("lle_buffer_t size:              %zu bytes\n", sizeof(lle_buffer_t));
    printf("lle_change_tracker_t size:      %zu bytes\n", sizeof(lle_change_tracker_t));
    printf("lle_buffer_manager_t size:      %zu bytes\n", sizeof(lle_buffer_manager_t));
    printf("lle_managed_buffer_t size:      %zu bytes\n", sizeof(lle_managed_buffer_t));
    
    // Memory for typical buffer
    size_t buffer_overhead = sizeof(lle_buffer_t);
    size_t buffer_data_256 = 256; // initial capacity
    size_t total_256 = buffer_overhead + buffer_data_256;
    printf("\nTypical buffer (256 cap):       %zu bytes total\n", total_256);
    printf("  Overhead:                     %zu bytes (%.1f%%)\n", 
           buffer_overhead, 100.0 * buffer_overhead / total_256);
    printf("  Data:                         %zu bytes\n", buffer_data_256);
    
    // Memory for managed buffer with undo
    size_t managed_total = sizeof(lle_managed_buffer_t) + 256 + 1024; // buf + undo estimate
    printf("\nManaged buffer with undo:       ~%zu bytes\n", managed_total);
}

int main(void) {
    printf("\n");
    printf("================================================================================\n");
    printf("LLE Buffer System Performance Benchmarks\n");
    printf("================================================================================\n");
    printf("\n");
    
    printf("=== Buffer Operations ===\n");
    benchmark_buffer_init();
    benchmark_char_insert();
    benchmark_string_insert();
    benchmark_char_delete();
    benchmark_range_delete();
    benchmark_buffer_growth();
    
    printf("\n=== Undo/Redo Operations ===\n");
    benchmark_undo_redo();
    
    printf("\n=== Buffer Manager Operations ===\n");
    benchmark_buffer_manager();
    
    printf("\n=== Search Operations ===\n");
    benchmark_search();
    
    printf("\n=== Line Operations ===\n");
    benchmark_line_operations();
    
    report_memory_usage();
    
    printf("\n");
    printf("================================================================================\n");
    printf("Performance targets from Phase 1 goals:\n");
    printf("  - Buffer operations:  < 10 μs per operation    (Week 5 target)\n");
    printf("  - Undo/redo:          < 50 μs per operation    (Week 6 target)\n");
    printf("  - Input latency:      < 100 μs (p99)           (Phase 1 target)\n");
    printf("  - Memory efficiency:  < 2x text size           (Phase 1 target)\n");
    printf("================================================================================\n");
    printf("\n");
    
    return 0;
}
