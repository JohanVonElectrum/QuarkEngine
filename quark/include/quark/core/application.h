#pragma once

#include <quark/api.h>
#include <quark/core/camera.h>
#include <quark/core/window.h>

#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>

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
 *
 * Filled by the application in `init_application`.
 */
typedef struct
{
    const char* name;
    Version version;
    Camera camera;
    WindowCreateInfo window;
} ApplicationCreateInfo;

/** Opaque handle to the running application. */
typedef struct Application Application;

/**
 * Application-provided initialization hook.
 *
 * Called very early by the engine (before `create_application`).
 * The application must fill the provided structure.
 *
 * @param create_info [out, non-null] Structure to be populated by the application.
 * @retval true on success.
 * @retval false on failure (engine will abort startup).
 */
extern b8_t init_application(OUT_NONNULL ApplicationCreateInfo* create_info) NONNULL_ARGS();

/**
 * Create the main application instance.
 *
 * Called after `init_application`. This initializes the singleton and
 * (if not headless) brings up the window and renderer.
 *
 * @param create_info [in, non-null] Information filled during initialization.
 * @return Pointer to the new Application (never NULL on success).
 */
QUARK_EXPORT Application* create_application(IN_NONNULL const ApplicationCreateInfo* create_info) NONNULL_ARGS();

/**
 * Run the application's main loop.
 *
 * This function is called once by the engine (from `main`). It contains the
 * application's main loop and blocks until the application clears the
 * `APPLICATION_FLAG_RUNNING` flag (typically via `APPLICATION_FLAG_SHOULD_CLOSE`).
 *
 * @param application [in, non-null] The application whose main loop should run.
 * @retval true on clean exit.
 * @retval false if the application exited due to an error.
 */
QUARK_EXPORT b8_t run_application(IN_NONNULL Application* application) NONNULL_ARGS();

/**
 * Destroy the application and release all associated resources.
 *
 * @param application [in, non-null] The application to destroy.
 * @retval true on success.
 * @retval false if destruction encountered errors.
 */
QUARK_EXPORT b8_t destroy_application(IN_NONNULL Application* application) NONNULL_ARGS();
