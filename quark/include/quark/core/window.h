#pragma once

#include <quark/primitives.h>

typedef struct
{
    enum
    {
        GRAPHICS_MODE_NONE,
        GRAPHICS_MODE_VULKAN,
    } mode;
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
