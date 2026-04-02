#include "window.h"

#include <quark/core/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>

void error_callback(int error, const char* description) {
    QUARK_LOG_ERROR("GLFW Error: %s\n", description);
}

QUARK_B8 init_windowing() {
    const QUARK_B8 initialized = glfwInit();

    glfwSetErrorCallback(error_callback);

    return initialized;
}

#define EXECUTE_UNHANDLED(expr) \
    GLFWerrorfun previous_callback = glfwSetErrorCallback(NULL); \
    glfwGetError(NULL); \
    expr; \
    const char* description; \
    const int error = glfwGetError(&description); \
    glfwSetErrorCallback(previous_callback); \
    if (error != GLFW_NO_ERROR) { \
    error_callback(error, description); \
    return QUARK_FALSE; \
    } \
    return QUARK_TRUE

QUARK_B8 shutdown_windowing() {
    EXECUTE_UNHANDLED(glfwTerminate());
}

QUARK_B8 windowing_poll_events() {
    EXECUTE_UNHANDLED(glfwPollEvents());
}

struct QuarkWindow
{
    GLFWwindow* handle;
};

QuarkWindow* create_window(const WindowCreateInfo* create_info) {
    if (create_info->mode == GRAPHICS_MODE_NONE) {
        QUARK_LOG_WARN("Attempted to create a window in headless mode");
        return NULL;
    }

    if (create_info->mode == GRAPHICS_MODE_VULKAN) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    GLFWwindow* window = glfwCreateWindow(
        create_info->data.graphics.width,
        create_info->data.graphics.height,
        create_info->data.graphics.title,
        NULL,
        NULL
    );

    if (!window) {
        QUARK_LOG_ERROR("Failed to create GLFW window");
        return NULL;
    }

    QuarkWindow* quark_window = malloc(sizeof(QuarkWindow));
    quark_window->handle = window;
    return quark_window;
}

QUARK_B8 destroy_window(QuarkWindow* window) {
    if (!window) {
        // TODO: Log error (assert)
        return QUARK_FALSE;
    }

    GLFWwindow* window_handle = window->handle;
    free(window);
    EXECUTE_UNHANDLED(glfwDestroyWindow(window_handle));
}

QUARK_B8 window_should_close(const QuarkWindow* window) {
    if (!window) {
        // TODO: Log error (assert)
        return QUARK_FALSE;
    }

    return glfwWindowShouldClose(window->handle);
}
