#include "backend.h"

#include "vulkan/backend.h"

#include <quark/core/log.h>

static RendererBackendKind backend_kind;

QUARK_B8 init_renderer_backend(
    const RendererBackendKind backend,
    const char* app_name,
    const QUARK_U16 app_major, const QUARK_U16 app_minor, const QUARK_U16 app_patch
) {
    backend_kind = backend;
    switch (backend) {
        case QUARK_RENDERER_BACKEND_VK:
            return vk_init_renderer_backend(app_name, app_major, app_minor, app_patch);
        default:
            QUARK_LOG_ERROR("Unsupported renderer backend: %u", backend);
            return QUARK_FALSE;
    };
}

QUARK_B8 shutdown_renderer_backend() {
    switch (backend_kind) {
        case QUARK_RENDERER_BACKEND_VK:
            return vk_shutdown_renderer_backend();
        default:
            QUARK_LOG_ERROR("Unsupported renderer backend: %u", backend_kind);
            return QUARK_FALSE;
    };
}
