#ifndef QUARK_RENDERER_BACKEND_TYPES
#define QUARK_RENDERER_BACKEND_TYPES

typedef enum
{
    QUARK_RENDERER_BACKEND_VK,
} RendererBackendKind;

typedef struct GLFWwindow GLFWwindow;

#endif // QUARK_RENDERER_BACKEND_TYPES

#ifndef QUARK_RENDERER_BACKEND_FUNCTIONS
#define QUARK_RENDERER_BACKEND_FUNCTIONS

#include "macro.h"
#include "../core/camera.h"

#include <quark/primitives.h>

RendererBackendKind get_renderer_backend_kind();

QUARK_B8 BACKEND_PREFIXED(init_renderer_backend)(
#ifdef BACKEND_COMMON
    RendererBackendKind backend,
#endif // BACKEND_COMMON
    const char* app_name, QUARK_U16 app_major, QUARK_U16 app_minor, QUARK_U16 app_patch
);

QUARK_B8 BACKEND_PREFIXED(shutdown_renderer_backend)();

QUARK_B8 BACKEND_PREFIXED(init_renderer_window)(GLFWwindow* window);
QUARK_B8 BACKEND_PREFIXED(shutdown_renderer_window)();
QUARK_B8 BACKEND_PREFIXED(render_renderer_frame)(const Camera* camera);
void BACKEND_PREFIXED(on_framebuffer_resized)();

#undef BACKEND_COMMON
#undef BACKEND_PREFIX

#endif // QUARK_RENDERER_BACKEND_FUNCTIONS
