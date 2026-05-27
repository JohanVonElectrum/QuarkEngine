#pragma once

#include <cstdlib/primitives.h>

/**
 * Supported window presentation modes.
 */
typedef enum
{
    WINDOW_MODE_HEADLESS,
    WINDOW_MODE_WINDOWED,
    WINDOW_MODE_FULLSCREEN,
    WINDOW_MODE_BORDERLESS,
} WindowMode;

/**
 * Parameters describing how a window should be created.
 */
typedef struct
{
    WindowMode mode;
    union
    {
        struct {} headless;
        struct
        {
            const char* title;
            u16_t width;
            u16_t height;
        } graphics;
    } data;
} WindowCreateInfo;
