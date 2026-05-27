#pragma once

#ifdef QUARK_DEBUG
#ifdef QUARK_PLATFORM_WINDOWS
#include <intrin.h>
#define QUARK_DEBUGBREAK() __debugbreak()
#elifdef QUARK_PLATFORM_LINUX
#include <signal.h>
#define QUARK_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform does not support debugbreak yet"
#endif // QUARK_PLATFORM_WINDOWS
#else
#define QUARK_DEBUGBREAK()
#endif // QUARK_DEBUG

#define QUARK_ASSERTIONS_ENABLED

#ifdef QUARK_ASSERTIONS_ENABLED
#include <quark/macro.h>
#include <quark/core/log.h>
#define QUARK_INTERNAL_ASSERT_IMPL(extra, check, msg, ...) { if (!(check)) { QUARK_LOG_ERROR(msg, __VA_ARGS__); QUARK_DEBUGBREAK(); extra } }
#define QUARK_INTERNAL_ASSERT_WITH_MSG(extra, check, ...) QUARK_INTERNAL_ASSERT_IMPL(extra, check, "Assertion failed: %s", __VA_ARGS__)
#define QUARK_INTERNAL_ASSERT_NO_MSG(extra, check) QUARK_INTERNAL_ASSERT_IMPL(extra, check, "Assertion '%s' failed at %s:%d", QUARK_STRINGIFY_MACRO(check), __FILE__, __LINE__)
#define QUARK_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define QUARK_INTERNAL_ASSERT_GET_MACRO(...) QUARK_EXPAND_MACRO(QUARK_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, QUARK_INTERNAL_ASSERT_WITH_MSG, QUARK_INTERNAL_ASSERT_NO_MSG))
#define QUARK_ASSERT(...) QUARK_EXPAND_MACRO(QUARK_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)({}, __VA_ARGS__))
#define QUARK_ASSERT_X(extra, ...) QUARK_EXPAND_MACRO(QUARK_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(extra, __VA_ARGS__))
#define QUARK_ASSERT_RETURN(ret, ...) QUARK_ASSERT_X({ return ret; }, __VA_ARGS__)
#else
#define QUARK_ASSERT(...)
#define QUARK_ASSERT_X(extra, ...)
#define QUARK_ASSERT_RETURN(ret, ...)
#endif // QUARK_ASSERTIONS_ENABLED
