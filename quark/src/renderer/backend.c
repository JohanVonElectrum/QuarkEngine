#include "backend.h"

#include "vulkan/backend.h"

#include <quark/core/log.h>

QUARK_B8 init_renderer_backend(
    const RendererBackendKind backend,
    const char* appName,
    const QUARK_U16 appMajor, const QUARK_U16 appMinor, const QUARK_U16 appPatch
) {
    switch (backend) {
        case QUARK_RENDERER_BACKEND_VK:
            return vk_init_renderer_backend(appName, appMajor, appMinor, appPatch);
        default:
            QUARK_LOG_ERROR("Unsupported renderer backend: %u", backend);
            return QUARK_FALSE;
    };
}
