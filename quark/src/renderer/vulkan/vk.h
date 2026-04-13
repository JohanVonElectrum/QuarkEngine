#pragma once

#include <quark/macro.h>
#include <quark/core/assert.h>

#include <vulkan/vulkan.h>

#define VK_CHECK_X(expr, extra) { \
    const VkResult result = (expr); \
    QUARK_INTERNAL_ASSERT_IMPL(extra, result == VK_SUCCESS, "Failed to execute `%s` with result `%u`", QUARK_STRINGIFY_MACRO(expr), result); \
}
#define VK_CHECK(expr) VK_CHECK_X(expr, {})
#define VK_CHECK_RETURN(expr, ret) VK_CHECK_X(expr, { return ret; })

typedef struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    QUARK_U32 format_count;
    VkSurfaceFormatKHR* formats;
    QUARK_U32 present_mode_count;
    VkPresentModeKHR* present_modes;
} SwapchainSupportDetails;

typedef struct VulkanDevice
{
    SwapchainSupportDetails swapchain_support;
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
} VulkanDevice;

typedef struct
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
#ifdef QUARK_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif // QUARK_DEBUG
    VkSurfaceKHR surface;
    VulkanDevice device;
} VulkanContext;
