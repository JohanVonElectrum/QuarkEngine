#pragma once

#include "../core/camera.h"
#include <cstdlib/primitives.h>

typedef struct GLFWwindow GLFWwindow;

/**
 * Initialize the Vulkan renderer backend (instance, debug layers, etc.).
 */
b8_t init_renderer_backend(const char* app_name, u16_t app_major, u16_t app_minor, u16_t app_patch);

b8_t shutdown_renderer_backend();

/**
 * Initialize renderer resources tied to a window (surface, device, swapchain, etc.).
 */
b8_t init_renderer_window(GLFWwindow* window);

b8_t shutdown_renderer_window();

b8_t render_renderer_frame(const Camera* camera);

void on_framebuffer_resized();
