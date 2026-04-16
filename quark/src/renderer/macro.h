#pragma once

#include <quark/macro.h>

#ifndef BACKEND_PREFIX
#define BACKEND_COMMON
#define BACKEND_PREFIX
#endif // BACKEND_PREFIX

#define BACKEND_PREFIXED(ident) QUARK_MERGE(QUARK_EXPAND_MACRO(BACKEND_PREFIX), ident)

#ifdef BACKEND_COMMON_IMPL
#include <quark/core/log.h>

#define GET_ARGS_DECL(ty, name, ...) ty name __VA_OPT__(, GET_ARGS_DECL(__VA_ARGS__))
#define GET_ARGS_NAME(ty, name, ...) name __VA_OPT__(, GET_ARGS_NAME(__VA_ARGS__))

#ifdef BACKEND_KIND_OWNER
#define BACKEND_KIND_DECL
#else
#include "backend.h"
#define BACKEND_KIND_DECL RendererBackendKind backend_kind = get_renderer_backend_kind();
#endif // BACKEND_KIND_OWNER

#define BACKEND_PREFIX_DISPATCHER(default_return, ret_ty, ident, ...) \
ret_ty ident(__VA_OPT__(GET_ARGS_DECL(__VA_ARGS__))) { \
    BACKEND_KIND_DECL \
    switch (backend_kind) { \
        case QUARK_RENDERER_BACKEND_VK: \
            return QUARK_MERGE(vk_, ident)(__VA_OPT__(GET_ARGS_NAME(__VA_ARGS__))); \
        default: \
            QUARK_LOG_ERROR("Unsupported renderer backend: %u", backend_kind); \
            return default_return; \
    } \
}

#endif // BACKEND_COMMON_IMPL
