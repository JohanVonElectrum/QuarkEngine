#pragma once

#include <cstdlib/common.h>
#include <quark/api.h>
#include <quark/core/camera.h>
#include <quark/core/window.h>

/**
 * Simple semantic version structure.
 */
typedef struct
{
    u16_t major;
    u16_t minor;
    u16_t patch;
} Version;

/**
 * Information required to create an Application instance.
 */
typedef struct
{
    const char* name;
    Version version;
    Camera camera;
    WindowCreateInfo window;
} ApplicationCreateInfo;

typedef struct Application Application;

/**
 * Application-provided initialization hook.
 *
 * This function is called by the engine early in startup. The application
 * must fill in the provided `create_info` structure.
 *
 * @param create_info [out] Structure to be populated by the application.
 * @retval true on success.
 */
extern b8_t init_application(ApplicationCreateInfo* create_info);

/**
 * Create the application instance.
 *
 * Called after `init_application`. The application should perform any
 * heavy initialization here.
 */
QUARK_EXPORT Application* create_application(const ApplicationCreateInfo* create_info);

/**
 * Run the main application loop.
 *
 * The engine will call this after creation. The application returns when it
 * wants to exit.
 */
QUARK_EXPORT b8_t run_application(Application* application);

/**
 * Destroy the application and release its resources.
 */
QUARK_EXPORT b8_t destroy_application(Application* application);
