#pragma once

#include <quark/core/window.h>
#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>

typedef struct GLFWwindow GLFWwindow;

b8_t init_windowing();
b8_t shutdown_windowing();
b8_t windowing_poll_events();

typedef struct QuarkWindow QuarkWindow;

QuarkWindow* create_window(IN_NONNULL const WindowCreateInfo* create_info) NONNULL_ARGS();
b8_t destroy_window(IN_NONNULL QuarkWindow* window) NONNULL_ARGS();
b8_t window_should_close(IN_NONNULL const QuarkWindow* window) NONNULL_ARGS();

/**
 * Returns the underlying GLFWwindow handle.
 * Intended for internal engine systems (e.g. input) only. Not exported publicly.
 */
GLFWwindow* window_get_glfw_handle(IN_NONNULL QuarkWindow* window) NONNULL_ARGS();
