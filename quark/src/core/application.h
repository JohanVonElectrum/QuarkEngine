#pragma once

#include <quark/core/application.h>

typedef enum ApplicationFlags {
    APPLICATION_FLAG_RUNNING = BIT(0),
    APPLICATION_FLAG_SHOULD_CLOSE = BIT(1),
    APPLICATION_FLAG_HEADLESS = BIT(2),
} ApplicationFlags;
