#include "backend.h"

#include "vk.h"
#include "../../platform/memory.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <GLFW/glfw3.h>

#include <string.h>

typedef struct
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
#ifdef QUARK_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif // QUARK_DEBUG
} BackendContext;

static BackendContext context;

#ifdef QUARK_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
);
#endif // QUARK_DEBUG

QUARK_B8 vk_init_renderer_backend(
    const char* app_name, const QUARK_U16 app_major, const QUARK_U16 app_minor, const QUARK_U16 app_patch
) {
    // TODO: use a custom allocator
    context.allocator = nullptr;

    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_4,
        .pApplicationName = app_name,
        .applicationVersion = VK_MAKE_VERSION(app_major, app_minor, app_patch),
        .pEngineName = "Quark",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .pNext = nullptr,
    };

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .flags = 0,
        .pNext = nullptr,
    };

    instance_create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(
        &instance_create_info.enabledExtensionCount);

    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        instance_create_info.enabledExtensionCount == 0 || instance_create_info.ppEnabledExtensionNames != NULL,
        "Failed to get required Vulkan instance extensions from GLFW"
    );

#ifdef QUARK_DEBUG
    const char** extensions = quark_mem_alloc(sizeof(const char*) * (instance_create_info.enabledExtensionCount + 1));
    quark_mem_copy(extensions, instance_create_info.ppEnabledExtensionNames,
                   instance_create_info.enabledExtensionCount * sizeof(const char*));
    extensions[instance_create_info.enabledExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    instance_create_info.ppEnabledExtensionNames = extensions;

    QUARK_LOG_DEBUG("Enabled Vulkan instance extensions:");
    for (QUARK_U32 i = 0; i < instance_create_info.enabledExtensionCount; ++i) {
        QUARK_LOG_DEBUG("  %s", instance_create_info.ppEnabledExtensionNames[i]);
    }

    const char* required_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    constexpr QUARK_USIZE required_layer_count = sizeof(required_layers) / sizeof(required_layers[0]);

    QUARK_U32 available_layer_count = 0;
    VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr), QUARK_FALSE);
    VkLayerProperties* available_layers = quark_mem_alloc(sizeof(VkLayerProperties) * available_layer_count);
    VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers), QUARK_FALSE);
    for (QUARK_U32 i = 0; i < required_layer_count; ++i) {
        for (QUARK_U32 j = 0; j < available_layer_count; ++j) {
            if (strcmp(required_layers[i], available_layers[j].layerName) == 0) {
                goto found;
            }
        }
        QUARK_LOG_ERROR("Required Vulkan layer not found: %s", required_layers[i]);
        quark_mem_free(extensions);
        quark_mem_free(available_layers);
        return QUARK_FALSE;
    found:
        QUARK_LOG_DEBUG("Found required Vulkan layer: %s", required_layers[i]);
    }
    instance_create_info.enabledLayerCount = required_layer_count;
    instance_create_info.ppEnabledLayerNames = required_layers;
#endif // QUARK_DEBUG

    VK_CHECK_X(vkCreateInstance(&instance_create_info, context.allocator, &context.instance), {
               quark_mem_free(extensions);
               quark_mem_free(available_layers);
               return QUARK_FALSE;
               });

    QUARK_LOG_DEBUG("Created Vulkan instance");

#ifdef QUARK_DEBUG
    constexpr QUARK_U32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            ;
    
    const VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = log_severity,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_debug_callback,
        .pUserData = nullptr,
        .pNext = nullptr,
    };

    const auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        context.instance,
        "vkCreateDebugUtilsMessengerEXT"
    );
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        vkCreateDebugUtilsMessengerEXT != nullptr,
        "Failed to get vkCreateDebugUtilsMessengerEXT function pointer"
    );

    VK_CHECK_RETURN(
        vkCreateDebugUtilsMessengerEXT(
            context.instance, &debug_create_info, context.allocator, &context.debug_messenger
        ),
        QUARK_FALSE
    );

    QUARK_LOG_DEBUG("Created Vulkan debug messenger");
#endif // QUARK_DEBUG

    QUARK_LOG_INFO("Initialized renderer backend");

    return QUARK_TRUE;
}

#ifdef QUARK_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            QUARK_LOG_ERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            QUARK_LOG_WARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            QUARK_LOG_INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            QUARK_LOG_TRACE(callback_data->pMessage);
            break;
        default:
            QUARK_DEBUGBREAK();
            QUARK_LOG_ERROR("(unknown severity) ", callback_data->pMessage);
    }
    return VK_FALSE;
}
#endif // QUARK_DEBUG
