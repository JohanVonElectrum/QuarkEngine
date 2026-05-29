#include "window.h"

#include "../renderer/backend.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <cstdlib/mem.h>
#include <GLFW/glfw3.h>

void error_callback(int error, const char* description) {
    QUARK_LOG_ERROR("GLFW Error: %s\n", description);
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    (void)width;
    (void)height;
    on_framebuffer_resized();
}

b8_t init_windowing() {
    const b8_t initialized = glfwInit();

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
    return false; \
    } \
    return true

b8_t shutdown_windowing() {
    EXECUTE_UNHANDLED(glfwTerminate());
}

b8_t windowing_poll_events() {
    EXECUTE_UNHANDLED(glfwPollEvents());
}

struct QuarkWindow
{
    GLFWwindow* handle;
};

QuarkWindow* create_window(const WindowCreateInfo* create_info) {
    if (create_info->mode == WINDOW_MODE_HEADLESS) {
        QUARK_LOG_WARN("Attempted to create a window in headless mode");
        return nullptr;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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

    if (!init_renderer_window(window)) {
        QUARK_LOG_ERROR("Failed to initialize renderer window");
        return nullptr;
    }

    QuarkWindow* quark_window = mem_heap_alloc(sizeof(QuarkWindow));
    quark_window->handle = window;

    glfwSetWindowUserPointer(window, quark_window);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    return quark_window;
}

b8_t destroy_window(QuarkWindow* window) {
    QUARK_ASSERT_RETURN(
        false,
        window,
        "Attempted to destroy NULL window"
    );

    GLFWwindow* window_handle = window->handle;
    shutdown_renderer_window();
    mem_heap_free(window);
    EXECUTE_UNHANDLED(glfwDestroyWindow(window_handle));
}

b8_t window_should_close(const QuarkWindow* window) {
    QUARK_ASSERT_RETURN(
        false,
        window,
        "Attempted to check if NULL window should close"
    );

    return glfwWindowShouldClose(window->handle);
}

GLFWwindow* window_get_glfw_handle(QuarkWindow* window) {
    QUARK_ASSERT_RETURN(
        nullptr,
        window,
        "Attempted to get GLFW handle from NULL window"
    );
    return window->handle;
}
