#pragma once

#include <cstdlib/common.h>

typedef struct EventQueue EventQueue;

EventQueue* create_event_queue(usize_t capacity);
b8_t destroy_event_queue(EventQueue* queue);

typedef u16_t QuarkEventId;
typedef void* QuarkEventData;

typedef enum EventQueueResult
{
    EVENT_QUEUE_SUCCESS,
    EVENT_QUEUE_FAILURE,
    EVENT_QUEUE_RETRY
} EventQueueResult;

EventQueueResult emit_event(EventQueue* queue, QuarkEventId id, QuarkEventData arg1, QuarkEventData arg2);
EventQueueResult poll_event(EventQueue* queue, QuarkEventId* out_id, QuarkEventData* out_arg1, QuarkEventData* out_arg2);
