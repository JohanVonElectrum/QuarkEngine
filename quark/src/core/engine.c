#include <quark/core/engine.h>

#include "../core/tracing.h"

#include <quark/core/log.h>

#include <cstdlib/cstdlib.h>

QUARK_B8 init_quark(int argc, char** argv) {
    if (!cstdlib_init(nullptr)) {
        QUARK_LOG_FATAL("Failed to initialize cstdlib");
        return QUARK_FALSE;
    }

    if (!init_tracing()) {
        QUARK_LOG_FATAL("Failed to initialize tracing");
        return QUARK_FALSE;
    }

    return QUARK_TRUE;
}

QUARK_B8 shutdown_quark(void) {
    return shutdown_tracing();
}
