#include "thread.h"

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
    current_thread = (QuarkThread*)calloc(1, sizeof(QuarkThread));
    if (!current_thread) return nullptr;

    current_thread->handle = GetCurrentThread();
    current_thread->tid = GetCurrentThreadId();
    current_thread->is_main_thread = QUARK_TRUE;

    return current_thread;
}

static unsigned __stdcall thread_wrapper(void* arg) {
    QuarkThread* thread = (QuarkThread*)arg;
    current_thread = thread;
    thread->ret = thread->func(thread->arg);
    return 0;
}

QuarkThread* spawn_thread(const PFN_QuarkThreadFunc func, void* arg) {
    QuarkThread* thread = calloc(1, sizeof(QuarkThread));
    if (!thread) return nullptr;

    thread->func = func;
    thread->arg = arg;
    thread->is_main_thread = QUARK_FALSE;

    thread->handle = (HANDLE)_beginthreadex(nullptr, 0, thread_wrapper, thread, 0, &thread->tid);
    if (thread->handle == nullptr) {
        free(thread);
        return nullptr;
    }
    return thread;
}

void* join_thread(QuarkThread* thread) {
    if (!thread) return nullptr;
    if (thread->is_main_thread) return nullptr;

    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    void* ret = thread->ret;
    free(thread);
    return ret;
}

QUARK_U64 get_thread_id(const QuarkThread* thread) {
    return thread ? thread->tid : 0;
}
