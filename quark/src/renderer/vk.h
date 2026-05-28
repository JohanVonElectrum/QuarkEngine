#pragma once

#include <quark/macro.h>
#include <quark/core/assert.h>

#include <cstdlib/primitives.h>
#include <vulkan/vulkan.h>

#define VK_CHECK_X(expr, extra) { \
    const VkResult result = (expr); \
    QUARK_INTERNAL_ASSERT_IMPL(extra, result == VK_SUCCESS, "Failed to execute `%s` with result `%u`", QUARK_STRINGIFY_MACRO(expr), result); \
}
#define VK_CHECK(expr) VK_CHECK_X(expr, {})
#define VK_CHECK_RETURN(expr, ret) VK_CHECK_X(expr, { return ret; })

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    u32_t format_count;
    VkSurfaceFormatKHR* formats;
    u32_t present_mode_count;
    VkPresentModeKHR* present_modes;
} SwapchainSupportDetails;

#define QUARK_VK_INVALID_QUEUE_FAMILY_INDEX ((u32_t) -1)

typedef struct QueueFamilyInfo
{
    u32_t graphics;
    u32_t graphics_count;
    u32_t present;
    u32_t present_count;
    u32_t compute;
    u32_t compute_count;
    u32_t transfer;
    u32_t transfer_count;
    b8_t dedicated_transfer;
} QueueFamilyInfo;

typedef struct DeviceFeatureSupport
{
    VkPhysicalDeviceFeatures core;
    b8_t acceleration_structure;
    b8_t ray_query;
    b8_t ray_tracing_pipeline;
    b8_t hardware_ray_tracing;
} DeviceFeatureSupport;

typedef struct VulkanSwapchain
{
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    u32_t image_count;
    VkImage* images;
    VkImageView* image_views;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;
    VkRenderPass render_pass;
    VkFramebuffer* framebuffers;
    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* in_flight_fences;
    u32_t current_frame;
    b8_t framebuffer_resized;
} VulkanSwapchain;

typedef struct VulkanDevice
{
    SwapchainSupportDetails swapchain_support;
    QueueFamilyInfo queue_families;
    DeviceFeatureSupport feature_support;
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;
} VulkanDevice;

/**
 * Central Vulkan state container.
 *
 * Holds all long-lived Vulkan objects used by the renderer backend.
 * There is currently one global instance of this structure.
 */
typedef struct
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
#ifdef QUARK_DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif // QUARK_DEBUG
    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;
    VulkanDevice device;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
} VulkanContext;
