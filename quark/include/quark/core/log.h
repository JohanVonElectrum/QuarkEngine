#pragma once

#include <quark/api.h>
#include <quark/primitives.h>

typedef enum
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
} LogLevel;

QUARK_API void quark_log(QUARK_B8 is_engine, LogLevel level, const char* format, ...);

#ifdef QUARK_ENGINE
#define QUARK_LOG_INTERNAL_ENGINE QUARK_TRUE
#else
#define QUARK_LOG_INTERNAL_ENGINE QUARK_FALSE
#endif // QUARK_ENGINE

#define QUARK_LOG_FATAL(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_FATAL, __VA_ARGS__)
#define QUARK_LOG_ERROR(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_ERROR, __VA_ARGS__)
#define QUARK_LOG_WARN(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_WARN, __VA_ARGS__)
#define QUARK_LOG_INFO(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_INFO, __VA_ARGS__)
#define QUARK_LOG_DEBUG(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define QUARK_LOG_TRACE(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_TRACE, __VA_ARGS__)
