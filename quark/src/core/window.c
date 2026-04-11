#include "window.h"

#include "../platform/memory.h"
#include "../renderer/backend.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

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
    QuarkWindowRendering* rendering;
};

QuarkWindow* create_window(const WindowCreateInfo* create_info) {
    if (create_info->mode == GRAPHICS_MODE_NONE) {
        QUARK_LOG_WARN("Attempted to create a window in headless mode");
        return nullptr;
    }

    if (create_info->mode == GRAPHICS_MODE_VULKAN) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    GLFWwindow* window = glfwCreateWindow(
        create_info->data.graphics.width,
        create_info->data.graphics.height,
        create_info->data.graphics.title,
        nullptr,
        nullptr
    );

    if (!window) {
        QUARK_LOG_ERROR("Failed to create GLFW window");
        return nullptr;
    }

    QuarkWindowRendering* rendering;
    if (!create_window_rendering(window, &rendering)) {
        QUARK_LOG_ERROR("Failed to create rendering");
        return nullptr;
    }

    QuarkWindow* quark_window = quark_mem_alloc(sizeof(QuarkWindow));
    quark_window->handle = window;
    quark_window->rendering = rendering;

    glfwSetWindowUserPointer(window, quark_window);

    return quark_window;
}

QUARK_B8 destroy_window(QuarkWindow* window) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        window,
        "Attempted to destroy NULL window"
    );

    GLFWwindow* window_handle = window->handle;
    QuarkWindowRendering* rendering = window->rendering;
    quark_mem_free(window);
    destroy_window_rendering(rendering);
    EXECUTE_UNHANDLED(glfwDestroyWindow(window_handle));
}

QUARK_B8 window_should_close(const QuarkWindow* window) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        window,
        "Attempted to check if NULL window should close"
    );

    return glfwWindowShouldClose(window->handle);
}
