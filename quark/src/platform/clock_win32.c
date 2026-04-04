#include "clock.h"

#include <windows.h>

static_assert(sizeof(LARGE_INTEGER) <= sizeof(QUARK_USIZE), "LARGE_INTEGER is too large to fit in QUARK_USIZE");

QUARK_USIZE get_monotonic_ticks() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (QUARK_USIZE) counter.QuadPart;
}

QUARK_USIZE get_monotonic_frequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (QUARK_USIZE) frequency.QuadPart;
}
