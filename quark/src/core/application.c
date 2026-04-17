#include "application.h"

#include "window.h"
#include "../renderer/backend.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

struct Application
{
    const char* name;
    Version version;
    Camera camera;
    QUARK_U8 flags;
    QuarkWindow* window;
};

static Application s_application;

static const char* window_mode_name(const WindowMode mode) {
    switch (mode) {
        case WINDOW_MODE_HEADLESS:
            return "headless";
        case WINDOW_MODE_WINDOWED:
            return "windowed";
        case WINDOW_MODE_FULLSCREEN:
            return "fullscreen";
        case WINDOW_MODE_BORDERLESS:
            return "borderless";
        default:
            return "unknown";
    }
}

Application* create_application(const ApplicationCreateInfo* create_info) {
    QUARK_ASSERT_RETURN(
        nullptr,
        create_info,
        "Attempted to create application with NULL create info"
    );
    QUARK_ASSERT_RETURN(
        nullptr,
        s_application.flags == 0,
        "Attempted to create application when one already exists"
    );

    const QUARK_B8 headless = create_info->window.mode == WINDOW_MODE_HEADLESS;
    if (!headless && !init_windowing()) {
        QUARK_LOG_FATAL("Failed to initialize windowing system");
        return nullptr;
    }

    s_application.name = create_info->name;
    s_application.version = create_info->version;
    s_application.camera = create_info->camera;
    s_application.flags = APPLICATION_FLAG_RUNNING;
    QUARK_LOG_INFO("Application window mode: %s", window_mode_name(create_info->window.mode));
    if (headless) {
        s_application.flags |= APPLICATION_FLAG_HEADLESS;
    }

    if (!headless) {
        if (!init_renderer_backend(
            create_info->name,
            create_info->version.major,
            create_info->version.minor,
            create_info->version.patch
        )) {
            QUARK_LOG_ERROR("Failed to initialize renderer backend");
            if (!shutdown_windowing()) {
                QUARK_LOG_ERROR("Failed to shutdown windowing system");
            }
            return nullptr;
        }

        s_application.window = create_window(&create_info->window);
        if (!s_application.window) {
            QUARK_LOG_ERROR("Failed to create window");
            if (!shutdown_windowing()) {
                QUARK_LOG_ERROR("Failed to shutdown windowing system");
            }
            return nullptr;
        }
    }

    s_application.flags |= APPLICATION_FLAG_INIT;
    return &s_application;
}

QUARK_B8 run_application(Application* application) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        application,
        "Attempted to run NULL application"
    );
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        application->flags & APPLICATION_FLAG_INIT,
        "Attempted to run application that was not initialized"
    );

    while (application->flags & APPLICATION_FLAG_RUNNING) {
        if (application->flags & APPLICATION_FLAG_SHOULD_CLOSE) {
            application->flags &= ~APPLICATION_FLAG_RUNNING;
            continue;
        }

        if (!(application->flags & APPLICATION_FLAG_HEADLESS)) {
            if (!windowing_poll_events()) {
                QUARK_LOG_ERROR("Failed to poll windowing events");
                return QUARK_FALSE;
            }

            if (window_should_close(application->window)) {
                application->flags |= APPLICATION_FLAG_SHOULD_CLOSE;
                continue;
            }

            if (!render_renderer_frame(&application->camera)) {
                QUARK_LOG_ERROR("Failed to render frame");
                return QUARK_FALSE;
            }
        }
    }

    return QUARK_TRUE;
}

QUARK_B8 destroy_application(Application* application) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        application,
        "Attempted to destroy NULL application"
    );
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        application->flags & APPLICATION_FLAG_INIT,
        "Attempted to destroy application that was not initialized"
    );

    QUARK_B8 result = QUARK_TRUE;

    if (!(application->flags & APPLICATION_FLAG_HEADLESS)) {
         if (application->window) {
            if (!destroy_window(application->window)) {
                QUARK_LOG_ERROR("Failed to destroy window");
                result = QUARK_FALSE;
            }
        }

        if (!shutdown_windowing()) {
            QUARK_LOG_ERROR("Failed to shutdown windowing system");
            result = QUARK_FALSE;
        }

        if (!shutdown_renderer_backend()) {
            QUARK_LOG_ERROR("Failed to shutdown renderer backend");
            result = QUARK_FALSE;
        }
    }

    application->flags = 0;

    return result;
}
