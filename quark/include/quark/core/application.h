#pragma once

#include <quark/api.h>
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
} ApplicationGraphicsInfo;

typedef struct
{
    const char* name;
    const char* version;
    ApplicationGraphicsInfo graphics;
} ApplicationCreateInfo;

typedef struct Application Application;

extern QUARK_B8 init_application(ApplicationCreateInfo* create_info);
QUARK_API Application* create_application(const ApplicationCreateInfo* create_info);
QUARK_API QUARK_B8 run_application(Application* application);
QUARK_API QUARK_B8 destroy_application(Application* application);
