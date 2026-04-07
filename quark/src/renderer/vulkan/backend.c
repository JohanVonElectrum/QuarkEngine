#include "backend.h"

#include "vk.h"

#include <quark/core/log.h>

typedef struct
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
} BackendContext;

static BackendContext context;

QUARK_B8 vk_init_renderer_backend(
    const char* appName, const QUARK_U16 appMajor, const QUARK_U16 appMinor, const QUARK_U16 appPatch
) {
    // TODO: use a custom allocator
    context.allocator = nullptr;

    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_4,
        .pApplicationName = appName,
        .applicationVersion = VK_MAKE_VERSION(appMajor, appMinor, appPatch),
        .pEngineName = "Quark",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .pNext = nullptr,
    };

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .flags = 0,
        .pNext = nullptr,
    };

    const VkResult result = vkCreateInstance(&createInfo, context.allocator, &context.instance);
    if (result != VK_SUCCESS) {
        QUARK_LOG_ERROR("Failed to create Vulkan instance: %u", result);
        return QUARK_FALSE;
    }

    QUARK_LOG_DEBUG("Created Vulkan instance");

    QUARK_LOG_INFO("Initialized renderer backend");

    return QUARK_TRUE;
}
