/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Performance Benchmarking Implementation - Week 3 Day 11
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#include "benchmark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// Maximum number of timing samples we'll store
#define MAX_SAMPLES 1000000

// Global timing storage
static uint64_t *timing_samples = NULL;
static size_t sample_count = 0;
static size_t sample_capacity = 0;
static size_t initial_rss = 0;

// Helper: Get current RSS (Resident Set Size) in bytes
static size_t get_rss_bytes(void) {
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f) {
        return 0;
    }
    
    unsigned long size, resident;
    if (fscanf(f, "%lu %lu", &size, &resident) != 2) {
        fclose(f);
        return 0;
    }
    fclose(f);
    
    // resident is in pages, convert to bytes
    long page_size = sysconf(_SC_PAGESIZE);
    return (size_t)(resident * page_size);
}

// Comparison function for qsort
static int compare_uint64(const void *a, const void *b) {
    uint64_t val_a = *(const uint64_t *)a;
    uint64_t val_b = *(const uint64_t *)b;
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

int lle_perf_init(void) {
    // Allocate initial sample buffer
    sample_capacity = 100000;  // Start with 100K samples
    timing_samples = malloc(sample_capacity * sizeof(uint64_t));
    if (!timing_samples) {
        return -1;
    }
    
    sample_count = 0;
    
    // Record initial RSS
    initial_rss = get_rss_bytes();
    
    return 0;
}

int lle_perf_record_timing(uint64_t time_ns) {
    if (!timing_samples) {
        return -1;
    }
    
    // Expand buffer if needed
    if (sample_count >= sample_capacity) {
        size_t new_capacity = sample_capacity * 2;
        if (new_capacity > MAX_SAMPLES) {
            new_capacity = MAX_SAMPLES;
        }
        
        if (sample_count >= new_capacity) {
            return -1;  // Can't store more samples
        }
        
        uint64_t *new_buffer = realloc(timing_samples, new_capacity * sizeof(uint64_t));
        if (!new_buffer) {
            return -1;
        }
        
        timing_samples = new_buffer;
        sample_capacity = new_capacity;
    }
    
    timing_samples[sample_count++] = time_ns;
    return 0;
}

int lle_perf_calculate_metrics(lle_perf_metrics_t *metrics) {
    if (!metrics || !timing_samples || sample_count == 0) {
        return -1;
    }
    
    memset(metrics, 0, sizeof(lle_perf_metrics_t));
    
    // Sort samples for percentile calculation
    qsort(timing_samples, sample_count, sizeof(uint64_t), compare_uint64);
    
    // Calculate basic stats
    metrics->operation_count = sample_count;
    metrics->min_time_ns = timing_samples[0];
    metrics->max_time_ns = timing_samples[sample_count - 1];
    
    // Calculate sum for average
    uint64_t sum = 0;
    for (size_t i = 0; i < sample_count; i++) {
        sum += timing_samples[i];
    }
    metrics->total_time_ns = sum;
    metrics->avg_time_ns = (double)sum / sample_count;
    
    // Calculate percentiles
    size_t p50_idx = sample_count / 2;
    size_t p95_idx = (sample_count * 95) / 100;
    size_t p99_idx = (sample_count * 99) / 100;
    
    metrics->p50_time_ns = timing_samples[p50_idx];
    metrics->p95_time_ns = timing_samples[p95_idx];
    metrics->p99_time_ns = timing_samples[p99_idx];
    
    // Calculate standard deviation
    double variance_sum = 0.0;
    for (size_t i = 0; i < sample_count; i++) {
        double diff = timing_samples[i] - metrics->avg_time_ns;
        variance_sum += diff * diff;
    }
    metrics->std_dev_ns = sqrt(variance_sum / sample_count);
    
    return 0;
}

int lle_perf_get_memory_metrics(lle_memory_metrics_t *metrics) {
    if (!metrics) {
        return -1;
    }
    
    memset(metrics, 0, sizeof(lle_memory_metrics_t));
    
    // Get current RSS
    size_t current_rss = get_rss_bytes();
    
    metrics->initial_rss_bytes = initial_rss;
    metrics->current_rss_bytes = current_rss;
    
    // Calculate incremental memory
    if (current_rss > initial_rss) {
        metrics->incremental_bytes = current_rss - initial_rss;
    } else {
        metrics->incremental_bytes = 0;
    }
    
    // For Phase 0, we track our known allocations
    // In Phase 1, this would integrate with memory pool tracking
    metrics->allocated_bytes = sample_capacity * sizeof(uint64_t);
    metrics->freed_bytes = 0;  // Will be updated on cleanup
    metrics->peak_bytes = metrics->current_rss_bytes;
    metrics->allocation_count = 1;  // timing_samples allocation
    metrics->deallocation_count = 0;
    
    // Simple leak detection: if incremental > 2MB, likely a leak
    metrics->has_leaks = (metrics->incremental_bytes > (2 * 1024 * 1024));
    metrics->leaked_bytes = metrics->has_leaks ? metrics->incremental_bytes : 0;
    
    return 0;
}

void lle_perf_reset(void) {
    sample_count = 0;
    initial_rss = get_rss_bytes();
}

void lle_perf_cleanup(void) {
    if (timing_samples) {
        free(timing_samples);
        timing_samples = NULL;
    }
    sample_count = 0;
    sample_capacity = 0;
}

void lle_perf_print_report(const lle_perf_metrics_t *metrics, bool detailed) {
    if (!metrics) {
        return;
    }
    
    printf("\n=== Performance Metrics ===\n\n");
    printf("Operations: %lu\n", metrics->operation_count);
    printf("Total time: %.3f ms\n", metrics->total_time_ns / 1000000.0);
    printf("\nLatency Statistics:\n");
    printf("  Minimum:  %lu ns (%.3f μs)\n", metrics->min_time_ns, metrics->min_time_ns / 1000.0);
    printf("  p50:      %lu ns (%.3f μs)\n", metrics->p50_time_ns, metrics->p50_time_ns / 1000.0);
    printf("  Average:  %.0f ns (%.3f μs)\n", metrics->avg_time_ns, metrics->avg_time_ns / 1000.0);
    printf("  p95:      %lu ns (%.3f μs)\n", metrics->p95_time_ns, metrics->p95_time_ns / 1000.0);
    printf("  p99:      %lu ns (%.3f μs)\n", metrics->p99_time_ns, metrics->p99_time_ns / 1000.0);
    printf("  Maximum:  %lu ns (%.3f μs)\n", metrics->max_time_ns, metrics->max_time_ns / 1000.0);
    
    if (detailed) {
        printf("  Std Dev:  %.0f ns (%.3f μs)\n", metrics->std_dev_ns, metrics->std_dev_ns / 1000.0);
    }
}

void lle_perf_print_memory_report(const lle_memory_metrics_t *metrics) {
    if (!metrics) {
        return;
    }
    
    printf("\n=== Memory Metrics ===\n\n");
    printf("Initial RSS:     %zu bytes (%.2f MB)\n", 
           metrics->initial_rss_bytes, metrics->initial_rss_bytes / (1024.0 * 1024.0));
    printf("Current RSS:     %zu bytes (%.2f MB)\n",
           metrics->current_rss_bytes, metrics->current_rss_bytes / (1024.0 * 1024.0));
    printf("Incremental:     %zu bytes (%.2f MB)\n",
           metrics->incremental_bytes, metrics->incremental_bytes / (1024.0 * 1024.0));
    printf("Peak usage:      %zu bytes (%.2f MB)\n",
           metrics->peak_bytes, metrics->peak_bytes / (1024.0 * 1024.0));
    printf("Allocations:     %lu\n", metrics->allocation_count);
    printf("Deallocations:   %lu\n", metrics->deallocation_count);
    printf("Memory leaks:    %s\n", metrics->has_leaks ? "DETECTED" : "None");
    if (metrics->has_leaks) {
        printf("  Leaked bytes:  %zu\n", metrics->leaked_bytes);
    }
}

bool lle_perf_validate_week3_criteria(const lle_perf_metrics_t *perf_metrics,
                                       const lle_memory_metrics_t *mem_metrics,
                                       int *passed_count,
                                       int *total_count) {
    if (!perf_metrics || !mem_metrics) {
        return false;
    }
    
    int passed = 0;
    int total = 5;
    
    printf("\n=== Week 3 Success Criteria Validation ===\n\n");
    
    // Criterion 1: Input latency p50 < 50μs
    bool p50_pass = (perf_metrics->p50_time_ns < 50000);
    printf("1. Input latency (p50):     %.3f μs < 50 μs  [%s]\n",
           perf_metrics->p50_time_ns / 1000.0,
           p50_pass ? "PASS" : "FAIL");
    if (p50_pass) passed++;
    
    // Criterion 2: Input latency p99 < 100μs
    bool p99_pass = (perf_metrics->p99_time_ns < 100000);
    printf("2. Input latency (p99):     %.3f μs < 100 μs [%s]\n",
           perf_metrics->p99_time_ns / 1000.0,
           p99_pass ? "PASS" : "FAIL");
    if (p99_pass) passed++;
    
    // Criterion 3: Memory footprint < 1MB
    bool mem_pass = (mem_metrics->incremental_bytes < (1024 * 1024));
    printf("3. Memory footprint:        %.2f MB < 1 MB    [%s]\n",
           mem_metrics->incremental_bytes / (1024.0 * 1024.0),
           mem_pass ? "PASS" : "FAIL");
    if (mem_pass) passed++;
    
    // Criterion 4: Allocation latency < 100μs
    // For Phase 0, we check average operation time as proxy
    bool alloc_pass = (perf_metrics->avg_time_ns < 100000);
    printf("4. Operation latency:       %.3f μs < 100 μs [%s]\n",
           perf_metrics->avg_time_ns / 1000.0,
           alloc_pass ? "PASS" : "FAIL");
    if (alloc_pass) passed++;
    
    // Criterion 5: Zero memory leaks
    bool leak_pass = !mem_metrics->has_leaks;
    printf("5. Memory leaks:            %s             [%s]\n",
           mem_metrics->has_leaks ? "DETECTED" : "None",
           leak_pass ? "PASS" : "FAIL");
    if (leak_pass) passed++;
    
    printf("\nResult: %d/%d criteria passed\n", passed, total);
    
    if (passed_count) *passed_count = passed;
    if (total_count) *total_count = total;
    
    return (passed == total);
}
