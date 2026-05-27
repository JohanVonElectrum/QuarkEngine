#pragma once

#include <cstdlib/common.h>
#include <quark/api.h>

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

/**
 * Low-level logging entry point. Prefer the QUARK_LOG_* macros in practice.
 *
 * @param is_engine  true if this is an internal engine log, false for application logs.
 * @param level      Severity level.
 * @param format     printf-style format string.
 */
QUARK_EXPORT void quark_log(b8_t is_engine, LogLevel level, const char* format, ...);

// Set to `LOG_LEVEL_NONE` to disable console logging.
#define MIN_LOG_LEVEL LOG_LEVEL_TRACE

#ifdef QUARK_ENGINE
#define QUARK_LOG_INTERNAL_ENGINE true
#else
#define QUARK_LOG_INTERNAL_ENGINE false
#endif // QUARK_ENGINE

#if LOG_LEVEL_FATAL <= MIN_LOG_LEVEL
#define QUARK_LOG_FATAL(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_FATAL, __VA_ARGS__)
#if LOG_LEVEL_ERROR <= MIN_LOG_LEVEL
#define QUARK_LOG_ERROR(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_ERROR, __VA_ARGS__)
#if LOG_LEVEL_WARN <= MIN_LOG_LEVEL
#define QUARK_LOG_WARN(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_WARN, __VA_ARGS__)
#if LOG_LEVEL_INFO <= MIN_LOG_LEVEL
#define QUARK_LOG_INFO(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_INFO, __VA_ARGS__)
#if LOG_LEVEL_DEBUG <= MIN_LOG_LEVEL
#define QUARK_LOG_DEBUG(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_DEBUG, __VA_ARGS__)
#if LOG_LEVEL_TRACE <= MIN_LOG_LEVEL
#define QUARK_LOG_TRACE(...) quark_log(QUARK_LOG_INTERNAL_ENGINE, LOG_LEVEL_TRACE, __VA_ARGS__)
#endif // LOG_LEVEL_TRACE
#endif // LOG_LEVEL_DEBUG
#endif // LOG_LEVEL_INFO
#endif // LOG_LEVEL_WARN
#endif // LOG_LEVEL_ERROR
#elif MIN_LOG_LEVEL == LOG_LEVEL_NONE
#warning "Logging disabled. Set MIN_LOG_LEVEL to a lower value to enable logging."
#endif // LOG_LEVEL_FATAL
