#define BACKEND_COMMON_IMPL
#define BACKEND_KIND_OWNER
#include "backend.h"

#include "vulkan/backend.h"

#include <quark/core/log.h>

static RendererBackendKind backend_kind;

inline __attribute((always_inline)) RendererBackendKind get_renderer_backend_kind() {
    return backend_kind;
}

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

BACKEND_PREFIX_DISPATCHER(QUARK_FALSE, QUARK_B8, shutdown_renderer_backend);
BACKEND_PREFIX_DISPATCHER(QUARK_FALSE, QUARK_B8, init_renderer_window, GLFWwindow*, window);
BACKEND_PREFIX_DISPATCHER(QUARK_FALSE, QUARK_B8, shutdown_renderer_window);
BACKEND_PREFIX_DISPATCHER(QUARK_FALSE, QUARK_B8, render_renderer_frame, const Camera*, camera);
BACKEND_PREFIX_DISPATCHER(, void, on_framebuffer_resized);
