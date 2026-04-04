#include "tracing.h"

#include "event.h"
#include "../platform/clock.h"
#include "../platform/memory.h"
#include "../platform/thread.h"

#include <quark/core/assert.h>
#include <quark/core/engine.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Set to `LOG_LEVEL_NONE` to disable console logging.
#define CONSOLE_LOG_LEVEL LOG_LEVEL_TRACE

static thread_local QUARK_USIZE tracing_span_id_counter = 0;
static EventQueue* queue = nullptr;
static QuarkThread* thread = nullptr;
static QUARK_B8 initialized = QUARK_FALSE;

typedef enum MSG_TYPE
{
    MSG_TYPE_APP_LOG = 1,
    MSG_TYPE_ENGINE_LOG = 2,
    MSG_TYPE_TRACE = 3,
    MSG_TYPE_CLOSE = 4
} MSG_TYPE;

struct QuarkTracingSpan
{
    QUARK_USIZE id;
    QUARK_USIZE parent;
    QUARK_USIZE tid;
    QUARK_USIZE time;
};

void* worker(void* arg);
void emit_msg(MSG_TYPE type, QuarkEventData arg1, QuarkEventData arg2);
EventQueueResult poll_msg(MSG_TYPE* out_type, QuarkEventData* out_arg1, QuarkEventData* out_arg2);
void process_log(QUARK_B8 is_engine, LogLevel level, const char* msg);
void process_trace(const QuarkTracingSpan* span, QUARK_USIZE monotonic_ticks);

static const char* const LOG_LEVEL_NAMES[7] = {
    [LOG_LEVEL_NONE] = "NONE",
    [LOG_LEVEL_FATAL] = "FATAL",
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_WARN] = "WARN",
    [LOG_LEVEL_INFO] = "INFO",
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_TRACE] = "TRACE",
};

QUARK_B8 init_tracing() {
    queue = create_event_queue(256);
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        !initialized,
        "Tracing already initialized"
    );
    thread = spawn_thread(worker, nullptr);
    QUARK_ASSERT_X(
        QUARK_ASSERT_RETURN(
            QUARK_FALSE,
            destroy_event_queue(queue),
            "Failed to destroy tracing event queue"
        ),
        thread != nullptr,
        "Failed to spawn tracing thread"
    );
    initialized = QUARK_TRUE;
    return QUARK_TRUE;
}

// NOT THREAD SAFE! Call after every thread is closed before engine shutdown.
QUARK_B8 shutdown_tracing() {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        initialized,
        "Tracing not initialized"
    );

    emit_msg(MSG_TYPE_CLOSE, nullptr, nullptr);

    join_thread(thread, nullptr);
    thread = nullptr;

    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        destroy_event_queue(queue),
        "Failed to destroy tracing event queue"
    );
    queue = nullptr;

    initialized = QUARK_FALSE;
    return QUARK_TRUE;
}

void quark_log(const QUARK_B8 is_engine, const LogLevel level, const char* format, ...) {
    if (!initialized) {
        fprintf(
            stderr,
            "(TRACING_UNINIT) [%s | %s] %s\n",
            is_engine ? "Quark" : "App", LOG_LEVEL_NAMES[level], format
        );
        QUARK_DEBUGBREAK();
        exit(QUARK_EXIT_CODE_FAILED_TO_INITIALIZE_ENGINE);
        return;
    }

    __builtin_va_list args;
    va_start(args, format);
    const QUARK_USIZE len = vsnprintf(nullptr, 0, format, args);
    char* buffer = quark_mem_alloc(len + 1);
    vsnprintf(buffer, len + 1, format, args);
    va_end(args);

    emit_msg(is_engine ? MSG_TYPE_ENGINE_LOG : MSG_TYPE_APP_LOG, (QuarkEventData) level, (QuarkEventData) buffer);
}

const QuarkTracingSpan* quark_start_tracing_span() {
    QuarkTracingSpan* span = quark_mem_alloc(sizeof(QuarkTracingSpan));
    span->parent = tracing_span_id_counter == 0 ? 0 : tracing_span_id_counter - 1;
    span->id = tracing_span_id_counter++;
    span->tid = get_thread_id(current_thread);
    span->time = get_monotonic_ticks();
    return span;
}

void quark_end_tracing_span(const QuarkTracingSpan* span) {
    QUARK_ASSERT_X(
        { return; },
        span != nullptr,
        "Tracing span pointer is null"
    );
    QUARK_ASSERT_X(
        { return; },
        span->id == tracing_span_id_counter - 1,
        "Tracing span end called out of order"
    );
#ifdef QUARK_DEBUG
    QUARK_ASSERT_X(
        { return; },
        span->tid == get_thread_id(current_thread),
        "Tracing span end called from a different thread"
    );
#endif // QUARK_DEBUG
    emit_msg(MSG_TYPE_TRACE, (QuarkEventData) span, (QuarkEventData) (get_monotonic_ticks() - span->time));
}

void* worker(void* arg) {
    MSG_TYPE type;
    void* arg1;
    void* arg2;

    while (poll_msg(&type, &arg1, &arg2) != EVENT_QUEUE_FAILURE) {
        switch (type) {
            case MSG_TYPE_ENGINE_LOG:
                process_log(QUARK_TRUE, (LogLevel) (QUARK_USIZE) arg1, (const char*) arg2);
                break;
            case MSG_TYPE_APP_LOG:
                process_log(QUARK_FALSE, (LogLevel) (QUARK_USIZE) arg1, (const char*) arg2);
                break;
            case MSG_TYPE_TRACE:
                process_trace((QuarkTracingSpan*) arg1, (QUARK_USIZE) arg2);
                break;
            case MSG_TYPE_CLOSE:
                return nullptr;
            default:
                process_log(QUARK_TRUE, LOG_LEVEL_ERROR, "Received event with unknown type");
                QUARK_DEBUGBREAK();
                break;
        }
    }

    process_log(QUARK_TRUE, LOG_LEVEL_ERROR, "Failed to poll tracing event");
    QUARK_DEBUGBREAK();
    return nullptr;
}

void emit_msg(const MSG_TYPE type, const QuarkEventData arg1, const QuarkEventData arg2) {
    EventQueueResult result;
    do {
        result = emit_event(queue, (QuarkEventId) type, arg1, arg2);
    } while (result == EVENT_QUEUE_RETRY);
    QUARK_ASSERT(
        result == EVENT_QUEUE_SUCCESS,
        "Failed to emit tracing event"
    );
}

EventQueueResult poll_msg(MSG_TYPE* out_type, QuarkEventData* out_arg1, QuarkEventData* out_arg2) {
    EventQueueResult result;
    do {
        result = poll_event(queue, (QuarkEventId*) out_type, out_arg1, out_arg2);
    } while (result == EVENT_QUEUE_RETRY);
    return result;
}

void process_log(const QUARK_B8 is_engine, const LogLevel level, const char* msg) {
    if (level <= CONSOLE_LOG_LEVEL) {
        printf("[%s | %s] %s\n", is_engine ? "Quark" : "App", LOG_LEVEL_NAMES[level], msg);
    }

    // TODO: print binary event to file

    quark_mem_free((void*) msg);
}

void process_trace(const QuarkTracingSpan* span, QUARK_USIZE monotonic_ticks) {
    // TODO: print binary event to file

    quark_mem_free((void*) span);
}
