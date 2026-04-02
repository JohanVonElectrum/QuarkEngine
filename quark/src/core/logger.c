#include <quark/core/log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char* const LOG_LEVEL_NAMES[6] = {
    [LOG_LEVEL_FATAL] = "FATAL",
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_WARN] = "WARN",
    [LOG_LEVEL_INFO] = "INFO",
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_TRACE] = "TRACE",
};

void quark_log(const QUARK_B8 is_engine, const LogLevel level, const char* format, ...) {
    /* TODO: This implementation is bad.
     * When allocators are implemented start using them.
     * Create an async ring buffer and make console output optional.
     * The target output must be a binary file with tracing events.
     */

    char buffer[32000] = {0};

    __builtin_va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("[%s | %s] %s\n", is_engine ? "Quark" : "App", LOG_LEVEL_NAMES[level], buffer);
}
