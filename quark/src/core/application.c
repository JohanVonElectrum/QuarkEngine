#include "application.h"

struct Application
{
    const char* name;
    const char* version;
    QUARK_U8 flags;
};

static Application s_application;
static QUARK_B8 s_initialized = QUARK_FALSE;

Application* create_application(const ApplicationCreateInfo* create_info) {
    s_application.name = create_info->name;
    s_application.version = create_info->version;
    s_application.flags = APPLICATION_FLAG_RUNNING;

    s_initialized = QUARK_TRUE;
    return &s_application;
}

QUARK_B8 run_application(Application* application) {
    if (!s_initialized) {
        // TODO: Log error
        return QUARK_FALSE;
    }

    while (application->flags & APPLICATION_FLAG_RUNNING) {
    }

    return QUARK_TRUE;
}

QUARK_B8 destroy_application(Application* application) {
    if (!s_initialized) {
        // TODO: Log error
        return QUARK_FALSE;
    }

    return QUARK_TRUE;
}
