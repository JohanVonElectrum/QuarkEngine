#pragma once

#include "../core/camera.h"

#include <quark/primitives.h>

typedef struct GLFWwindow GLFWwindow;

QUARK_B8 init_renderer_backend(const char* app_name, QUARK_U16 app_major, QUARK_U16 app_minor, QUARK_U16 app_patch);
QUARK_B8 shutdown_renderer_backend();

QUARK_B8 init_renderer_window(GLFWwindow* window);
QUARK_B8 shutdown_renderer_window();
QUARK_B8 render_renderer_frame(const Camera* camera);
void on_framebuffer_resized();
