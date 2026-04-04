#pragma once

#include <quark/primitives.h>

typedef struct QuarkThread QuarkThread;
typedef void*(*PFN_QuarkThreadFunc)(void* arg);

QuarkThread* init_main_thread(void);

QuarkThread* spawn_thread(PFN_QuarkThreadFunc func, void* arg);
QUARK_B8 join_thread(QuarkThread* thread, void** result);
QUARK_U64 get_thread_id(const QuarkThread* thread);

extern thread_local QuarkThread* current_thread;
