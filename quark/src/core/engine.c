#include <quark/core/engine.h>

#include "../platform/thread.h"

#include <quark/core/log.h>

QUARK_B8 init_quark(int argc, char** argv) {
    if (!init_main_thread()) {
        QUARK_LOG_FATAL("Failed to initialize main thread");
        return QUARK_FALSE;
    }

    return QUARK_TRUE;
}

QUARK_B8 shutdown_quark(void) {
    return QUARK_TRUE;
}
