#pragma once

#include <quark/core/application.h>

#include <cstdlib/common.h>

/**
 * Internal lifecycle flags for the Application singleton.
 */
typedef enum ApplicationFlags {
    APPLICATION_FLAG_INIT = BIT(0),
    APPLICATION_FLAG_RUNNING = BIT(1),
    APPLICATION_FLAG_SHOULD_CLOSE = BIT(2),
    APPLICATION_FLAG_HEADLESS = BIT(3),
} ApplicationFlags;
