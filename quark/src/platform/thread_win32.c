#include "thread.h"

#include "memory.h"

#include <quark/core/assert.h>

#include <process.h>
#include <windows.h>

struct QuarkThread
{
    HANDLE handle;
    unsigned tid;
    PFN_QuarkThreadFunc func;
    void* arg;
    void* ret;
    QUARK_B8 is_main_thread;
};

thread_local QuarkThread* current_thread;

QuarkThread* init_main_thread(void) {
    QUARK_ASSERT_RETURN(
        current_thread->is_main_thread ? current_thread : nullptr,
        current_thread == nullptr,
        "Main thread already initialized"
    );
    current_thread = (QuarkThread*) quark_mem_calloc(1, sizeof(QuarkThread));
    if (!current_thread) return nullptr;

    current_thread->handle = GetCurrentThread();
    current_thread->tid = GetCurrentThreadId();
    current_thread->is_main_thread = QUARK_TRUE;

    return current_thread;
}

static unsigned __stdcall thread_wrapper(void* arg) {
    QuarkThread* thread = (QuarkThread*) arg;
    current_thread = thread;
    thread->ret = thread->func(thread->arg);
    return 0;
}

QuarkThread* spawn_thread(const PFN_QuarkThreadFunc func, void* arg) {
    QuarkThread* thread = quark_mem_calloc(1, sizeof(QuarkThread));
    if (!thread) return nullptr;

    thread->func = func;
    thread->arg = arg;
    thread->is_main_thread = QUARK_FALSE;

    thread->handle = (HANDLE) _beginthreadex(nullptr, 0, thread_wrapper, thread, 0, &thread->tid);
    QUARK_ASSERT_X(
        {
        quark_mem_free(thread);
        return nullptr;
        },
        thread->handle != nullptr,
        "Failed to create thread"
    );
    return thread;
}

QUARK_B8 join_thread(QuarkThread* thread, void** result) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        thread != nullptr,
        "Thread is null"
    );
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        !thread->is_main_thread,
        "Cannot join main thread"
    )

    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    if (result) *result = thread->ret;
    return quark_mem_free(thread);
}

QUARK_U64 get_thread_id(const QuarkThread* thread) {
    return thread ? thread->tid : 0;
}
