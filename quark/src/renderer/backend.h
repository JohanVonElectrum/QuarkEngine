#ifndef QUARK_RENDERER_BACKEND_TYPES
#define QUARK_RENDERER_BACKEND_TYPES

typedef enum
{
    QUARK_RENDERER_BACKEND_VK,
} RendererBackendKind;

#endif // QUARK_RENDERER_BACKEND_TYPES

#ifndef QUARK_RENDERER_BACKEND_FUNCTIONS
#define QUARK_RENDERER_BACKEND_FUNCTIONS

#include <quark/macro.h>
#include <quark/primitives.h>

#ifndef BACKEND_PREFIX
#define BACKEND_COMMON
#define BACKEND_PREFIX
#endif // BACKEND_PREFIX

#define BACKEND_PREFIXED(ident) QUARK_MERGE(QUARK_EXPAND_MACRO(BACKEND_PREFIX), ident)

QUARK_B8 BACKEND_PREFIXED(init_renderer_backend)(
#ifdef BACKEND_COMMON
    const RendererBackendKind backend,
#endif // BACKEND_COMMON
    const char* appName, QUARK_U16 appMajor, QUARK_U16 appMinor, QUARK_U16 appPatch
);

#undef BACKEND_COMMON
#undef BACKEND_PREFIX

#endif // QUARK_RENDERER_BACKEND_FUNCTIONS
