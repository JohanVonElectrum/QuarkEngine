#include "memory.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <windows.h>

void* quark_mem_alloc(const QUARK_USIZE size) {
    HANDLE hHeap = GetProcessHeap();
    QUARK_ASSERT_RETURN(
        nullptr,
        hHeap != nullptr,
        "Failed to get process heap"
    );

    void* ptr = HeapAlloc(hHeap, 0, size);
    QUARK_ASSERT_RETURN(
        nullptr,
        ptr != nullptr,
        "Failed to allocate memory"
    );

    return ptr;
}

void* quark_mem_calloc(const QUARK_USIZE count, const QUARK_USIZE stride) {
    HANDLE hHeap = GetProcessHeap();
    QUARK_ASSERT_RETURN(
        nullptr,
        hHeap != nullptr,
        "Failed to get process heap"
    );

    void* ptr = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, count * stride);
    QUARK_ASSERT_RETURN(
        nullptr,
        ptr != nullptr,
        "Failed to allocate memory"
    );

    return ptr;
}

void* quark_mem_realloc(void* ptr, const QUARK_USIZE size) {
    HANDLE hHeap = GetProcessHeap();
    QUARK_ASSERT_RETURN(
        nullptr,
        hHeap != nullptr,
        "Failed to get process heap"
    );

    ptr = HeapReAlloc(hHeap, 0, ptr, size);
    QUARK_ASSERT_RETURN(
        nullptr,
        ptr != nullptr,
        "Failed to reallocate memory"
    );

    return ptr;
}

QUARK_B8 quark_mem_free(void* ptr) {
    HANDLE hHeap = GetProcessHeap();
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        hHeap != nullptr,
        "Failed to get process heap"
    );

    if (!HeapFree(hHeap, 0, ptr)) {
        QUARK_LOG_ERROR("Failed to free memory");
        return QUARK_FALSE;
    }

    return QUARK_TRUE;
}

void* quark_mem_reserve(const QUARK_USIZE size) {
    void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    QUARK_ASSERT_RETURN(
        nullptr,
        ptr != nullptr,
        "Failed to reserve virtual memory"
    );

    return ptr;
}

void* quark_mem_commit(void* ptr, const QUARK_USIZE size) {
    void* result = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    QUARK_ASSERT_RETURN(
        nullptr,
        result != nullptr,
        "Failed to commit virtual memory"
    );

    return result;
}

QUARK_B8 quark_mem_decommit(void* ptr, const QUARK_USIZE size) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        VirtualFree(ptr, size, MEM_DECOMMIT),
        "Failed to decommit virtual memory"
    );

    return QUARK_TRUE;
}

QUARK_B8 quark_mem_release(void* ptr, const QUARK_USIZE size) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        VirtualFree(ptr, 0, MEM_RELEASE),
        "Failed to release virtual memory"
    );

    return QUARK_TRUE;
}

QUARK_USIZE quark_mem_alignment() {
    return MEMORY_ALLOCATION_ALIGNMENT;
}

QUARK_USIZE quark_mem_page_size() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize;
}
