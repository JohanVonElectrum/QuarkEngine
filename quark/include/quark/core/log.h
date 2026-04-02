#pragma once

#include <quark/api.h>

typedef enum
{
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
} LogLevel;

QUARK_API void quark_log(LogLevel level, const char* format, ...);

#define QUARK_LOG_FATAL(...) quark_log(LOG_LEVEL_FATAL, __VA_ARGS__)
#define QUARK_LOG_ERROR(...) quark_log(LOG_LEVEL_ERROR, __VA_ARGS__)
#define QUARK_LOG_WARN(...) quark_log(LOG_LEVEL_WARN, __VA_ARGS__)
#define QUARK_LOG_INFO(...) quark_log(LOG_LEVEL_INFO, __VA_ARGS__)
#define QUARK_LOG_DEBUG(...) quark_log(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define QUARK_LOG_TRACE(...) quark_log(LOG_LEVEL_TRACE, __VA_ARGS__)
