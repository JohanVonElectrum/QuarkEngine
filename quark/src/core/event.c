#include "event.h"

#include "../platform/memory.h"

#include <quark/primitives.h>
#include <quark/core/assert.h>

#include <stdatomic.h>

typedef struct Event
{
    QuarkEventId id;
    QuarkEventData arg1;
    QuarkEventData arg2;
    atomic_size_t sequence;
} Event;

struct EventQueue
{
    Event* buffer;
    QUARK_USIZE mask;
    atomic_size_t enqueue;
    atomic_size_t dequeue;
};

static constexpr QUARK_USIZE OFFSET = QUARK_ALIGN_UP(sizeof(EventQueue), alignof(Event));

EventQueue* create_event_queue(const QUARK_USIZE capacity) {
    QUARK_ASSERT_RETURN(
        nullptr,
        capacity != 0,
        "Event queue capacity cannot be 0"
    );
    QUARK_ASSERT_RETURN(
        nullptr,
        (capacity & (capacity - 1)) == 0,
        "Event queue capacity must be a power of 2"
    );

    EventQueue* queue = quark_mem_calloc(1, OFFSET + capacity * sizeof(Event));
    QUARK_ASSERT_RETURN(
        nullptr,
        queue != nullptr,
        "Failed to allocate event queue"
    );

    queue->buffer = (Event*) (((QUARK_U8*) queue) + OFFSET);
    queue->mask = capacity - 1;

    for (size_t idx = 0; idx < capacity; ++idx) {
        atomic_init(&queue->buffer[idx].sequence, idx);
    }
    atomic_init(&queue->enqueue, 0);
    atomic_init(&queue->dequeue, 0);

    return queue;
}

QUARK_B8 destroy_event_queue(EventQueue* queue) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        queue != nullptr,
        "Event queue is null"
    );

    const size_t enqueue = atomic_load_explicit(&queue->enqueue, memory_order_relaxed);
    const size_t dequeue = atomic_load_explicit(&queue->dequeue, memory_order_relaxed);
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        enqueue == dequeue,
        "Event queue is not empty"
    );

    return quark_mem_free(queue);
}

EventQueueResult emit_event(EventQueue* queue, const QuarkEventId id, const QuarkEventData arg1, const QuarkEventData arg2) {
    QUARK_ASSERT_RETURN(
        EVENT_QUEUE_FAILURE,
        queue != nullptr,
        "Event queue is null"
    );
    QUARK_ASSERT_RETURN(
        EVENT_QUEUE_FAILURE,
        id != 0,
        "Event ID cannot be 0"
    );

    Event* event;
    QUARK_USIZE pos = atomic_load_explicit(&queue->enqueue, memory_order_relaxed);

    while (QUARK_TRUE) {
        event = &queue->buffer[pos & queue->mask];
        const QUARK_USIZE seq = atomic_load_explicit(&event->sequence, memory_order_acquire);
        const QUARK_IPTR diff = (QUARK_IPTR) seq - (QUARK_IPTR) pos;

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(
                &queue->enqueue, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed
            )) {
                break;
            }
        } else if (diff < 0) {
            return EVENT_QUEUE_RETRY;
        } else {
            pos = atomic_load_explicit(&queue->enqueue, memory_order_relaxed);
        }
    }

    event->id = id;
    event->arg1 = arg1;
    event->arg2 = arg2;

    atomic_store_explicit(&event->sequence, pos + 1, memory_order_release);

    return EVENT_QUEUE_SUCCESS;
}

EventQueueResult poll_event(EventQueue* queue, QuarkEventId* out_id, QuarkEventData* out_arg1, QuarkEventData* out_arg2) {
    QUARK_ASSERT_RETURN(
        EVENT_QUEUE_FAILURE,
        queue != nullptr,
        "Event queue is null"
    );
    QUARK_ASSERT_RETURN(
        EVENT_QUEUE_FAILURE,
        out_id != nullptr,
        "Event id pointer is null"
    );

    Event* event;
    QUARK_USIZE pos = atomic_load_explicit(&queue->dequeue, memory_order_relaxed);

    while (QUARK_TRUE) {
        event = &queue->buffer[pos & queue->mask];
        const QUARK_USIZE seq = atomic_load_explicit(&event->sequence, memory_order_acquire);
        const QUARK_IPTR diff = (QUARK_IPTR) seq - (QUARK_IPTR) (pos + 1);

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(
                &queue->dequeue, &pos, pos + 1, memory_order_relaxed, memory_order_relaxed
            )) {
                break;
            }
        } else if (diff < 0) {
            return EVENT_QUEUE_RETRY;
        } else {
            pos = atomic_load_explicit(&queue->dequeue, memory_order_relaxed);
        }
    }

    *out_id = event->id;
    if (out_arg1) *out_arg1 = event->arg1;
    if (out_arg2) *out_arg2 = event->arg2;

    atomic_store_explicit(&event->sequence, pos + queue->mask + 1, memory_order_release);

    return EVENT_QUEUE_SUCCESS;
}
