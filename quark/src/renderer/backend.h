#pragma once

#include "../core/camera.h"

#include <cstdlib/nullability.h>
#include <cstdlib/primitives.h>

typedef struct GLFWwindow GLFWwindow;

/**
 * Initialize the Vulkan renderer backend (instance, debug layers, etc.).
 */
b8_t init_renderer_backend(IN_NONNULL const char* app_name, u16_t app_major, u16_t app_minor, u16_t app_patch) NONNULL_ARGS(1);

b8_t shutdown_renderer_backend();

/**
 * Initialize renderer resources tied to a window (surface, device, swapchain, etc.).
 */
b8_t init_renderer_window(IN_NONNULL GLFWwindow* window) NONNULL_ARGS();

b8_t shutdown_renderer_window();

b8_t render_renderer_frame(IN_NONNULL const Camera* camera) NONNULL_ARGS();

void on_framebuffer_resized();
