#include <quark/core/engine.h>

#include "../core/tracing.h"

#include <quark/core/log.h>

#include <cstdlib/cstdlib.h>

b8_t init_quark(int argc, char** argv) {
    if (!cstdlib_init(nullptr)) {
        QUARK_LOG_FATAL("Failed to initialize cstdlib");
        return false;
    }

    if (!init_tracing()) {
        QUARK_LOG_FATAL("Failed to initialize tracing");
        return false;
    }

    return true;
}

b8_t shutdown_quark(void) {
    return shutdown_tracing();
}
