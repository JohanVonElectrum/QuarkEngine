#include "backend.h"

#include "device.h"
#include "swapchain.h"
#include "vk.h"
#include "../../platform/memory.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <GLFW/glfw3.h>

#include <string.h>

static VulkanContext context;
static GLFWwindow* s_current_window = nullptr;

static QUARK_B8 vk_recreate_swapchain() {
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(s_current_window, &framebuffer_width, &framebuffer_height);

    while (framebuffer_width == 0 || framebuffer_height == 0) {
        glfwGetFramebufferSize(s_current_window, &framebuffer_width, &framebuffer_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(context.device.logical_device);

    destroy_vulkan_swapchain(&context);

    if (!create_vulkan_swapchain(&context, (QUARK_U32) framebuffer_width, (QUARK_U32) framebuffer_height)) {
        QUARK_LOG_ERROR("Failed to recreate swapchain");
        return QUARK_FALSE;
    }

    context.swapchain.framebuffer_resized = QUARK_FALSE;

    QUARK_LOG_DEBUG("Swapchain recreated with dimensions %ux%u", framebuffer_width, framebuffer_height);
    return QUARK_TRUE;
}

void vk_on_framebuffer_resized() {
    context.swapchain.framebuffer_resized = QUARK_TRUE;
}

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
    const char* extensions[instance_create_info.enabledExtensionCount + 1];
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
    VkLayerProperties available_layers[available_layer_count];
    VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers), QUARK_FALSE);
    for (QUARK_U32 i = 0; i < required_layer_count; ++i) {
        for (QUARK_U32 j = 0; j < available_layer_count; ++j) {
            if (strcmp(required_layers[i], available_layers[j].layerName) == 0) {
                goto found;
            }
        }
        QUARK_LOG_ERROR("Required Vulkan layer not found: %s", required_layers[i]);
        return QUARK_FALSE;
    found:
        QUARK_LOG_DEBUG("Found required Vulkan layer: %s", required_layers[i]);
    }
    instance_create_info.enabledLayerCount = required_layer_count;
    instance_create_info.ppEnabledLayerNames = required_layers;
#endif // QUARK_DEBUG

    VK_CHECK_RETURN(vkCreateInstance(&instance_create_info, context.allocator, &context.instance), QUARK_FALSE);

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

QUARK_B8 vk_shutdown_renderer_backend() {
#ifdef QUARK_DEBUG
    const auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        context.instance,
        "vkDestroyDebugUtilsMessengerEXT"
    );
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        vkDestroyDebugUtilsMessengerEXT != nullptr,
        "Failed to get vkDestroyDebugUtilsMessengerEXT function pointer"
    );
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, context.allocator);
    QUARK_LOG_DEBUG("Destroyed Vulkan debug messenger");
#endif // QUARK_DEBUG

    vkDestroyInstance(context.instance, context.allocator);
    QUARK_LOG_DEBUG("Destroyed Vulkan instance");

    QUARK_LOG_INFO("Shutdown renderer backend");

    return QUARK_TRUE;
}

QUARK_B8 vk_init_renderer_window(GLFWwindow* window) {
    s_current_window = window;

    VK_CHECK_RETURN(
        glfwCreateWindowSurface(context.instance, window, context.allocator, &context.surface),
        QUARK_FALSE
    );

    if (!create_vulkan_device(&context)) {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = VK_NULL_HANDLE;
        return QUARK_FALSE;
    }

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    if (!create_vulkan_swapchain(
            &context,
            framebuffer_width > 0 ? (QUARK_U32) framebuffer_width : 1,
            framebuffer_height > 0 ? (QUARK_U32) framebuffer_height : 1
        )) {
        destroy_vulkan_device(&context);
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = VK_NULL_HANDLE;
        return QUARK_FALSE;
    }

    return QUARK_TRUE;
}

QUARK_B8 vk_shutdown_renderer_window() {
    QUARK_B8 success = QUARK_TRUE;
    success = success && destroy_vulkan_swapchain(&context);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    context.surface = VK_NULL_HANDLE;
    success = success && destroy_vulkan_device(&context);
    s_current_window = nullptr;
    return success;
}

QUARK_B8 vk_render_renderer_frame() {
    const QUARK_U32 frame_index = context.swapchain.current_frame;
    VkFence frame_fence = context.swapchain.in_flight_fences[frame_index];

    VK_CHECK_RETURN(vkWaitForFences(context.device.logical_device, 1, &frame_fence, VK_TRUE, UINT64_MAX), QUARK_FALSE);

    QUARK_U32 image_index = 0;
    const VkResult acquire_result = vkAcquireNextImageKHR(
        context.device.logical_device,
        context.swapchain.swapchain,
        UINT64_MAX,
        context.swapchain.image_available_semaphores[frame_index],
        VK_NULL_HANDLE,
        &image_index
    );
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        vk_recreate_swapchain();
        return QUARK_TRUE;
    }
    if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
        QUARK_LOG_ERROR("Failed to acquire swapchain image: %u", acquire_result);
        return QUARK_FALSE;
    }
    VK_CHECK_RETURN(vkResetFences(context.device.logical_device, 1, &frame_fence), QUARK_FALSE);

    VkCommandBuffer command_buffer = context.swapchain.command_buffers[frame_index];
    VK_CHECK_RETURN(vkResetCommandBuffer(command_buffer, 0), QUARK_FALSE);

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK_RETURN(vkBeginCommandBuffer(command_buffer, &begin_info), QUARK_FALSE);

    const VkClearValue clear_values[] = {
        {
            .color = {
                .float32 = {0.08f, 0.09f, 0.12f, 1.0f},
            },
        },
        {
            .depthStencil = {
                .depth = 1.0f,
                .stencil = 0,
            },
        },
    };
    const VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = context.swapchain.render_pass,
        .framebuffer = context.swapchain.framebuffers[image_index],
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = context.swapchain.extent,
        },
        .clearValueCount = (QUARK_U32) (sizeof(clear_values) / sizeof(clear_values[0])),
        .pClearValues = clear_values,
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffer);

    VK_CHECK_RETURN(vkEndCommandBuffer(command_buffer), QUARK_FALSE);

    const VkSemaphore wait_semaphores[] = {
        context.swapchain.image_available_semaphores[frame_index],
    };
    const VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    const VkSemaphore signal_semaphores[] = {
        context.swapchain.render_finished_semaphores[image_index],
    };
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = (QUARK_U32) (sizeof(wait_semaphores) / sizeof(wait_semaphores[0])),
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = (QUARK_U32) (sizeof(signal_semaphores) / sizeof(signal_semaphores[0])),
        .pSignalSemaphores = signal_semaphores,
    };
    VK_CHECK_RETURN(vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, frame_fence), QUARK_FALSE);

    const VkSwapchainKHR swapchains[] = {
        context.swapchain.swapchain,
    };
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = (QUARK_U32) (sizeof(signal_semaphores) / sizeof(signal_semaphores[0])),
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = (QUARK_U32) (sizeof(swapchains) / sizeof(swapchains[0])),
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
        .pResults = nullptr,
    };

    const VkResult present_result = vkQueuePresentKHR(context.device.present_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR || context.swapchain.framebuffer_resized) {
        if (!vk_recreate_swapchain()) {
            return QUARK_FALSE;
        }
    } else if (present_result != VK_SUCCESS) {
        QUARK_LOG_ERROR("Failed to present swapchain image: %u", present_result);
        return QUARK_FALSE;
    }

    context.swapchain.current_frame = (context.swapchain.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

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
