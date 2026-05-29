#pragma once

#include "camera.h"
#include "window.h"

#include <cstdlib/nullability.h>
#include <cstdlib/primitives.h>

/**
 * Initialize the input system.
 *
 * Captures the mouse cursor and prepares to drive the main camera.
 * Must be called after the primary window has been created.
 *
 * @param window The Quark window to attach input to.
 * @retval true on success.
 */
b8_t init_input(IN_NONNULL QuarkWindow* window) NONNULL_ARGS();

/**
 * Shut down the input system and release any resources.
 */
void shutdown_input(void);

/**
 * Process pending input and apply movement/look to the provided camera.
 *
 * Must be called once per frame with a valid delta time (in seconds).
 * Modifies the camera's position and orientation in place.
 *
 * @param dt      Delta time since last frame, in seconds (> 0).
 * @param camera  The camera to update (typically the application's main camera).
 */
void input_process(f32_t dt, IN_NONNULL Camera* camera) NONNULL_ARGS(2);
