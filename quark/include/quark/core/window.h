#pragma once

#include <cstdlib/primitives.h>

typedef enum
{
    WINDOW_MODE_HEADLESS,
    WINDOW_MODE_WINDOWED,
    WINDOW_MODE_FULLSCREEN,
    WINDOW_MODE_BORDERLESS,
} WindowMode;

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
