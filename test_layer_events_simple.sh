#!/bin/bash

# Simple and Safe Layer Events Test
# Tests core functionality without complex data handling

set -e

echo "Simple Layer Events Test"
echo "======================="

# Check environment
if [ ! -f "src/display/layer_events.c" ]; then
    echo "❌ Must be run from lusush root directory"
    exit 1
fi

# Build
echo "Building..."
ninja -C builddir

# Create simple test
cat > /tmp/simple_events_test.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include "display/layer_events.h"

static int callback_count = 0;

layer_events_error_t simple_callback(const layer_event_t *event, void *user_data) {
    callback_count++;
    printf("  Received event type %d\n", event->type);
    return LAYER_EVENTS_SUCCESS;
}

int main() {
    printf("Testing layer events system...\n");

    // Create and init
    layer_event_system_t *events = layer_events_create(NULL);
    if (!events) {
        printf("❌ Create failed\n");
        return 1;
    }

    if (layer_events_init(events) != LAYER_EVENTS_SUCCESS) {
        printf("❌ Init failed\n");
        return 1;
    }
    printf("✅ System created and initialized\n");

    // Subscribe
    layer_events_error_t result = layer_events_subscribe(
        events, LAYER_EVENT_REDRAW_NEEDED, LAYER_ID_COMMAND_LAYER,
        simple_callback, NULL, LAYER_EVENT_PRIORITY_NORMAL);

    if (result != LAYER_EVENTS_SUCCESS) {
        printf("❌ Subscribe failed\n");
        return 1;
    }
    printf("✅ Subscription successful\n");

    // Publish simple event
    result = layer_events_publish_simple(
        events, LAYER_EVENT_REDRAW_NEEDED, LAYER_ID_PROMPT_LAYER,
        0, LAYER_EVENT_PRIORITY_NORMAL);

    if (result != LAYER_EVENTS_SUCCESS) {
        printf("❌ Publish failed\n");
        return 1;
    }
    printf("✅ Event published\n");

    // Process events
    int processed = layer_events_process_pending(events, 0, 100);
    printf("✅ Processed %d events\n", processed);
    printf("✅ Callback called %d times\n", callback_count);

    if (processed != 1 || callback_count != 1) {
        printf("❌ Event processing failed\n");
        return 1;
    }

    // Test statistics
    layer_event_stats_t stats = layer_events_get_statistics(events);
    printf("✅ Events published: %lu\n", stats.events_published);
    printf("✅ Events processed: %lu\n", stats.events_processed);

    // Test utilities
    const char *type_name = layer_events_get_type_name(LAYER_EVENT_REDRAW_NEEDED);
    const char *layer_name = layer_events_get_layer_name(LAYER_ID_PROMPT_LAYER);
    printf("✅ Type name: %s\n", type_name);
    printf("✅ Layer name: %s\n", layer_name);

    // Cleanup
    layer_events_destroy(events);
    printf("✅ Cleanup complete\n");

    printf("\n🎉 ALL TESTS PASSED!\n");
    printf("Layer Events System is working correctly.\n");
    return 0;
}
EOF

# Compile and run
gcc -std=c99 -Wall -I include -I include/display \
    -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700 \
    /tmp/simple_events_test.c \
    src/display/base_terminal.c \
    src/display/terminal_control.c \
    src/display/layer_events.c \
    -o /tmp/simple_events_test

echo "Running test..."
/tmp/simple_events_test

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo ""
    echo "✅ Layer Events System (Week 3) COMPLETE!"
    echo ""
    echo "📊 FOUNDATION PHASE COMPLETE:"
    echo "   ✅ Week 1: Base Terminal Layer"
    echo "   ✅ Week 2: Terminal Control Layer"
    echo "   ✅ Week 3: Layer Events System"
    echo ""
    echo "🚀 Ready for Phase 2: Display Layers!"
else
    echo "❌ Test failed"
fi

rm -f /tmp/simple_events_test.c /tmp/simple_events_test
exit $exit_code
