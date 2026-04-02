#pragma once

#include <quark/api.h>
#include <quark/primitives.h>
#include <quark/core/window.h>

typedef struct
{
    const char* name;
    const char* version;
    WindowCreateInfo window;
} ApplicationCreateInfo;

typedef struct Application Application;

extern QUARK_B8 init_application(ApplicationCreateInfo* create_info);
QUARK_API Application* create_application(const ApplicationCreateInfo* create_info);
QUARK_API QUARK_B8 run_application(Application* application);
QUARK_API QUARK_B8 destroy_application(Application* application);
