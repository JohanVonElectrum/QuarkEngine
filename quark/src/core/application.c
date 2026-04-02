#include "application.h"

#include "window.h"

struct Application
{
    const char* name;
    const char* version;
    QUARK_U8 flags;
    // TODO: Support multiple windows
    QuarkWindow* window;
};

static Application s_application;
static QUARK_B8 s_initialized = QUARK_FALSE;

Application* create_application(const ApplicationCreateInfo* create_info) {
    if (s_initialized) {
        // TODO: Log error
        return NULL;
    }

    const QUARK_B8 headless = create_info->window.mode == GRAPHICS_MODE_NONE;
    if (!headless && !init_windowing()) {
        // TODO: Log error
        return NULL;
    }

    s_application.name = create_info->name;
    s_application.version = create_info->version;
    s_application.flags = APPLICATION_FLAG_RUNNING;
    if (headless) {
        s_application.flags |= APPLICATION_FLAG_HEADLESS;
    }

    if (!headless) {
        s_application.window = create_window(&create_info->window);
        if (!s_application.window) {
            if (!shutdown_windowing()) {
                // TODO: Log error
            }
            return NULL;
        }
    }

    s_initialized = QUARK_TRUE;
    return &s_application;
}

QUARK_B8 run_application(Application* application) {
    if (!s_initialized) {
        // TODO: Log error
        return QUARK_FALSE;
    }

    while (application->flags & APPLICATION_FLAG_RUNNING) {
        if (application->flags & APPLICATION_FLAG_SHOULD_CLOSE) {
            application->flags &= ~APPLICATION_FLAG_RUNNING;
            continue;
        }

        if (!(application->flags & APPLICATION_FLAG_HEADLESS)) {
            if (!windowing_poll_events()) {
                // TODO: Log error
                return QUARK_FALSE;
            }

            if (window_should_close(application->window)) {
                application->flags |= APPLICATION_FLAG_SHOULD_CLOSE;
            }
        }
    }

    return QUARK_TRUE;
}

QUARK_B8 destroy_application(Application* application) {
    if (!s_initialized) {
        // TODO: Log error
        return QUARK_FALSE;
    }

    if (!(application->flags & APPLICATION_FLAG_HEADLESS)) {
        QUARK_B8 destroy_window_success = QUARK_FALSE;
        if (application->window) {
            if (!((destroy_window_success = destroy_window(application->window)))) {
                // TODO: Log error
            }
        }

        const QUARK_B8 shutdown_windowing_success = shutdown_windowing();
        if (!shutdown_windowing_success) {
            // TODO: Log error
        }

        return destroy_window_success && shutdown_windowing_success;
    }

    s_initialized = QUARK_FALSE;

    return QUARK_TRUE;
}
