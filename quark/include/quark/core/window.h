#pragma once

#include <quark/primitives.h>

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
            QUARK_U16 width;
            QUARK_U16 height;
        } graphics;
    } data;
} WindowCreateInfo;
