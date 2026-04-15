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

#define QUARK_VK_INVALID_QUEUE_FAMILY_INDEX ((QUARK_U32) -1)

typedef struct QueueFamilyInfo
{
    QUARK_U32 graphics;
    QUARK_U32 graphics_count;
    QUARK_U32 present;
    QUARK_U32 present_count;
    QUARK_U32 compute;
    QUARK_U32 compute_count;
    QUARK_U32 transfer;
    QUARK_U32 transfer_count;
    QUARK_B8 dedicated_transfer;
} QueueFamilyInfo;

typedef struct DeviceFeatureSupport
{
    VkPhysicalDeviceFeatures core;
    QUARK_B8 acceleration_structure;
    QUARK_B8 ray_query;
    QUARK_B8 ray_tracing_pipeline;
    QUARK_B8 hardware_ray_tracing;
} DeviceFeatureSupport;

typedef struct VulkanDevice
{
    SwapchainSupportDetails swapchain_support;
    QueueFamilyInfo queue_families;
    DeviceFeatureSupport feature_support;
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
