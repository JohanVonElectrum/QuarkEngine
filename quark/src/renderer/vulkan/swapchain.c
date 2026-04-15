#include "swapchain.h"

#include "device.h"
#include "../../platform/memory.h"

#include <quark/core/log.h>

static VkSurfaceFormatKHR choose_swapchain_surface_format(const SwapchainSupportDetails* support);
static VkPresentModeKHR choose_swapchain_present_mode(const SwapchainSupportDetails* support);
static VkExtent2D choose_swapchain_extent(
    const SwapchainSupportDetails* support,
    QUARK_U32 framebuffer_width,
    QUARK_U32 framebuffer_height
);
static QUARK_U32 choose_swapchain_image_count(const SwapchainSupportDetails* support);
static VkFormat choose_depth_format(VulkanContext* context);
static QUARK_U32 find_memory_type(
    VulkanContext* context,
    QUARK_U32 type_filter,
    VkMemoryPropertyFlags required_properties
);
static QUARK_B8 has_stencil_component(VkFormat format);
static QUARK_B8 create_depth_resources(VulkanContext* context, VulkanSwapchain* swapchain, VkFormat depth_format);
static QUARK_B8 create_swapchain_render_pass(VulkanContext* context, VulkanSwapchain* swapchain, VkFormat depth_format);
static QUARK_B8 create_swapchain_framebuffers(VulkanContext* context, VulkanSwapchain* swapchain);
static QUARK_B8 create_swapchain_sync_objects(VulkanContext* context, VulkanSwapchain* swapchain);
static void destroy_vulkan_swapchain_resources(VulkanContext* context, VulkanSwapchain* swapchain);

QUARK_B8 create_vulkan_swapchain(
    VulkanContext* context,
    const QUARK_U32 framebuffer_width,
    const QUARK_U32 framebuffer_height
) {
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        context->device.logical_device != VK_NULL_HANDLE,
        "Cannot create a Vulkan swapchain without a logical device"
    );

    const SwapchainSupportDetails* support = &context->device.swapchain_support;
    QUARK_ASSERT_RETURN(
        QUARK_FALSE,
        support->format_count != 0 && support->present_mode_count != 0,
        "Swapchain support details are incomplete"
    );

    VulkanSwapchain swapchain = {0};
    const VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(support);
    const VkPresentModeKHR present_mode = choose_swapchain_present_mode(support);
    const VkExtent2D extent = choose_swapchain_extent(support, framebuffer_width, framebuffer_height);
    const QUARK_U32 image_count = choose_swapchain_image_count(support);

    const QUARK_U32 queue_family_indices[] = {
        context->device.queue_families.graphics,
        context->device.queue_families.present,
    };
    const QUARK_B8 use_concurrent_sharing =
        context->device.queue_families.graphics != context->device.queue_families.present;

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = context->surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = use_concurrent_sharing ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = use_concurrent_sharing ? (QUARK_U32) (sizeof(queue_family_indices) / sizeof(queue_family_indices[0])) : 0,
        .pQueueFamilyIndices = use_concurrent_sharing ? queue_family_indices : nullptr,
        .preTransform = support->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    VK_CHECK_RETURN(
        vkCreateSwapchainKHR(context->device.logical_device, &swapchain_create_info, context->allocator, &swapchain.swapchain),
        QUARK_FALSE
    );

    swapchain.format = surface_format.format;
    swapchain.extent = extent;

    VK_CHECK_X(
        vkGetSwapchainImagesKHR(context->device.logical_device, swapchain.swapchain, &swapchain.image_count, nullptr),
        {
            destroy_vulkan_swapchain_resources(context, &swapchain);
            return QUARK_FALSE;
        }
    );

    swapchain.images = quark_mem_alloc(sizeof(VkImage) * swapchain.image_count);
    if (swapchain.images == nullptr) {
        QUARK_LOG_ERROR("Failed to allocate memory for swapchain images");
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    VK_CHECK_X(
        vkGetSwapchainImagesKHR(context->device.logical_device, swapchain.swapchain, &swapchain.image_count, swapchain.images),
        {
            destroy_vulkan_swapchain_resources(context, &swapchain);
            return QUARK_FALSE;
        }
    );

    swapchain.image_views = quark_mem_calloc(swapchain.image_count, sizeof(VkImageView));
    if (swapchain.image_views == nullptr) {
        QUARK_LOG_ERROR("Failed to allocate memory for swapchain image views");
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    for (QUARK_U32 i = 0; i < swapchain.image_count; ++i) {
        const VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = swapchain.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        VK_CHECK_X(
            vkCreateImageView(context->device.logical_device, &image_view_create_info, context->allocator, &swapchain.image_views[i]),
            {
                destroy_vulkan_swapchain_resources(context, &swapchain);
                return QUARK_FALSE;
            }
        );
    }

    const VkFormat depth_format = choose_depth_format(context);
    if (depth_format == VK_FORMAT_UNDEFINED) {
        QUARK_LOG_ERROR("Failed to find a supported depth format");
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    if (!create_depth_resources(context, &swapchain, depth_format)) {
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    if (!create_swapchain_render_pass(context, &swapchain, depth_format)) {
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    if (!create_swapchain_framebuffers(context, &swapchain)) {
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    const VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = context->device.queue_families.graphics,
    };
    VK_CHECK_X(
        vkCreateCommandPool(context->device.logical_device, &command_pool_create_info, context->allocator, &swapchain.command_pool),
        {
            destroy_vulkan_swapchain_resources(context, &swapchain);
            return QUARK_FALSE;
        }
    );

    swapchain.command_buffers = quark_mem_alloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
    if (swapchain.command_buffers == nullptr) {
        QUARK_LOG_ERROR("Failed to allocate memory for swapchain command buffers");
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    const VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = swapchain.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    VK_CHECK_X(
        vkAllocateCommandBuffers(context->device.logical_device, &command_buffer_allocate_info, swapchain.command_buffers),
        {
            destroy_vulkan_swapchain_resources(context, &swapchain);
            return QUARK_FALSE;
        }
    );

    if (!create_swapchain_sync_objects(context, &swapchain)) {
        destroy_vulkan_swapchain_resources(context, &swapchain);
        return QUARK_FALSE;
    }

    swapchain.current_frame = 0;
    context->swapchain = swapchain;

    QUARK_LOG_DEBUG(
        "Created Vulkan swapchain: %ux%u, format=%u, images=%u, frames-in-flight=%u",
        swapchain.extent.width,
        swapchain.extent.height,
        swapchain.format,
        swapchain.image_count,
        MAX_FRAMES_IN_FLIGHT
    );

    return QUARK_TRUE;
}

QUARK_B8 destroy_vulkan_swapchain(VulkanContext* context) {
    destroy_vulkan_swapchain_resources(context, &context->swapchain);
    context->swapchain = (VulkanSwapchain) {0};
    return QUARK_TRUE;
}

static VkSurfaceFormatKHR choose_swapchain_surface_format(const SwapchainSupportDetails* support) {
    for (QUARK_U32 i = 0; i < support->format_count; ++i) {
        if (
            support->formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            support->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            return support->formats[i];
        }
    }

    return support->formats[0];
}

static VkPresentModeKHR choose_swapchain_present_mode(const SwapchainSupportDetails* support) {
    for (QUARK_U32 i = 0; i < support->present_mode_count; ++i) {
        if (support->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swapchain_extent(
    const SwapchainSupportDetails* support,
    const QUARK_U32 framebuffer_width,
    const QUARK_U32 framebuffer_height
) {
    if (support->capabilities.currentExtent.width != UINT32_MAX) {
        return support->capabilities.currentExtent;
    }

    VkExtent2D extent = {
        .width = framebuffer_width == 0 ? 1 : framebuffer_width,
        .height = framebuffer_height == 0 ? 1 : framebuffer_height,
    };

    if (extent.width < support->capabilities.minImageExtent.width) {
        extent.width = support->capabilities.minImageExtent.width;
    }
    if (extent.height < support->capabilities.minImageExtent.height) {
        extent.height = support->capabilities.minImageExtent.height;
    }
    if (support->capabilities.maxImageExtent.width != 0 && extent.width > support->capabilities.maxImageExtent.width) {
        extent.width = support->capabilities.maxImageExtent.width;
    }
    if (support->capabilities.maxImageExtent.height != 0 && extent.height > support->capabilities.maxImageExtent.height) {
        extent.height = support->capabilities.maxImageExtent.height;
    }

    return extent;
}

static QUARK_U32 choose_swapchain_image_count(const SwapchainSupportDetails* support) {
    QUARK_U32 image_count = support->capabilities.minImageCount + 1;
    if (image_count < 3 && (support->capabilities.maxImageCount == 0 || support->capabilities.maxImageCount >= 3)) {
        image_count = 3;
    }
    if (support->capabilities.maxImageCount != 0 && image_count > support->capabilities.maxImageCount) {
        image_count = support->capabilities.maxImageCount;
    }
    return image_count;
}

static VkFormat choose_depth_format(VulkanContext* context) {
    const VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    for (QUARK_U32 i = 0; i < (QUARK_U32) (sizeof(candidates) / sizeof(candidates[0])); ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(context->device.physical_device, candidates[i], &properties);
        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return candidates[i];
        }
    }

    return VK_FORMAT_UNDEFINED;
}

static QUARK_U32 find_memory_type(
    VulkanContext* context,
    const QUARK_U32 type_filter,
    const VkMemoryPropertyFlags required_properties
) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context->device.physical_device, &memory_properties);

    for (QUARK_U32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) != 0 &&
            (memory_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties) {
            return i;
        }
    }

    return QUARK_VK_INVALID_QUEUE_FAMILY_INDEX;
}

static QUARK_B8 has_stencil_component(const VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

static QUARK_B8 create_depth_resources(VulkanContext* context, VulkanSwapchain* swapchain, const VkFormat depth_format) {
    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = {
            .width = swapchain->extent.width,
            .height = swapchain->extent.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK_RETURN(
        vkCreateImage(context->device.logical_device, &image_create_info, context->allocator, &swapchain->depth_image),
        QUARK_FALSE
    );

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(context->device.logical_device, swapchain->depth_image, &memory_requirements);

    const QUARK_U32 memory_type_index = find_memory_type(
        context,
        memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    if (memory_type_index == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX) {
        QUARK_LOG_ERROR("Failed to find a suitable memory type for the depth image");
        return QUARK_FALSE;
    }

    const VkMemoryAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index,
    };
    VK_CHECK_RETURN(
        vkAllocateMemory(context->device.logical_device, &allocate_info, context->allocator, &swapchain->depth_image_memory),
        QUARK_FALSE
    );

    VK_CHECK_X(
        vkBindImageMemory(context->device.logical_device, swapchain->depth_image, swapchain->depth_image_memory, 0),
        {
            return QUARK_FALSE;
        }
    );

    const VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = swapchain->depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask = has_stencil_component(depth_format) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    VK_CHECK_RETURN(
        vkCreateImageView(context->device.logical_device, &image_view_create_info, context->allocator, &swapchain->depth_image_view),
        QUARK_FALSE
    );

    return QUARK_TRUE;
}

static QUARK_B8 create_swapchain_render_pass(VulkanContext* context, VulkanSwapchain* swapchain, const VkFormat depth_format) {
    const VkAttachmentDescription attachments[] = {
        {
            .flags = 0,
            .format = swapchain->format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            .flags = 0,
            .format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };

    const VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_attachment_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass_description = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = &depth_attachment_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };
    const VkSubpassDependency dependencies[] = {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
        },
    };
    const VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = (QUARK_U32) (sizeof(attachments) / sizeof(attachments[0])),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = (QUARK_U32) (sizeof(dependencies) / sizeof(dependencies[0])),
        .pDependencies = dependencies,
    };

    VK_CHECK_RETURN(
        vkCreateRenderPass(context->device.logical_device, &render_pass_create_info, context->allocator, &swapchain->render_pass),
        QUARK_FALSE
    );

    return QUARK_TRUE;
}

static QUARK_B8 create_swapchain_framebuffers(VulkanContext* context, VulkanSwapchain* swapchain) {
    swapchain->framebuffers = quark_mem_calloc(swapchain->image_count, sizeof(VkFramebuffer));
    if (swapchain->framebuffers == nullptr) {
        QUARK_LOG_ERROR("Failed to allocate memory for swapchain framebuffers");
        return QUARK_FALSE;
    }
    for (QUARK_U32 i = 0; i < swapchain->image_count; ++i) {
        const VkImageView attachments[] = {
            swapchain->image_views[i],
            swapchain->depth_image_view,
        };
        const VkFramebufferCreateInfo framebuffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = swapchain->render_pass,
            .attachmentCount = (QUARK_U32) (sizeof(attachments) / sizeof(attachments[0])),
            .pAttachments = attachments,
            .width = swapchain->extent.width,
            .height = swapchain->extent.height,
            .layers = 1,
        };

        VK_CHECK_X(
            vkCreateFramebuffer(context->device.logical_device, &framebuffer_create_info, context->allocator, &swapchain->framebuffers[i]),
            {
                return QUARK_FALSE;
            }
        );
    }

    return QUARK_TRUE;
}

static QUARK_B8 create_swapchain_sync_objects(VulkanContext* context, VulkanSwapchain* swapchain) {
    swapchain->image_available_semaphores = quark_mem_calloc(MAX_FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
    swapchain->render_finished_semaphores = quark_mem_calloc(MAX_FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
    swapchain->in_flight_fences = quark_mem_calloc(MAX_FRAMES_IN_FLIGHT, sizeof(VkFence));

    if (
        swapchain->image_available_semaphores == nullptr ||
        swapchain->render_finished_semaphores == nullptr ||
        swapchain->in_flight_fences == nullptr
    ) {
        QUARK_LOG_ERROR("Failed to allocate memory for swapchain synchronization objects");
        return QUARK_FALSE;
    }

    const VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    const VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (QUARK_U32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VK_CHECK_X(
            vkCreateSemaphore(context->device.logical_device, &semaphore_create_info, context->allocator, &swapchain->image_available_semaphores[i]),
            {
                return QUARK_FALSE;
            }
        );
        VK_CHECK_X(
            vkCreateSemaphore(context->device.logical_device, &semaphore_create_info, context->allocator, &swapchain->render_finished_semaphores[i]),
            {
                return QUARK_FALSE;
            }
        );
        VK_CHECK_X(
            vkCreateFence(context->device.logical_device, &fence_create_info, context->allocator, &swapchain->in_flight_fences[i]),
            {
                return QUARK_FALSE;
            }
        );
    }

    return QUARK_TRUE;
}

static void destroy_vulkan_swapchain_resources(VulkanContext* context, VulkanSwapchain* swapchain) {
    const VkDevice device = context->device.logical_device;

    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    if (device != VK_NULL_HANDLE) {
        if (swapchain->command_buffers != nullptr && swapchain->command_pool != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, swapchain->command_pool, MAX_FRAMES_IN_FLIGHT, swapchain->command_buffers);
        }
    }

    if (swapchain->in_flight_fences != nullptr) {
        if (device != VK_NULL_HANDLE) {
            for (QUARK_U32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                if (swapchain->in_flight_fences[i] != VK_NULL_HANDLE) {
                    vkDestroyFence(device, swapchain->in_flight_fences[i], context->allocator);
                }
            }
        }
        if (quark_mem_free(swapchain->in_flight_fences) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain fences");
        }
        swapchain->in_flight_fences = nullptr;
    }

    if (swapchain->render_finished_semaphores != nullptr) {
        if (device != VK_NULL_HANDLE) {
            for (QUARK_U32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                if (swapchain->render_finished_semaphores[i] != VK_NULL_HANDLE) {
                    vkDestroySemaphore(device, swapchain->render_finished_semaphores[i], context->allocator);
                }
            }
        }
        if (quark_mem_free(swapchain->render_finished_semaphores) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain render-finished semaphores");
        }
        swapchain->render_finished_semaphores = nullptr;
    }

    if (swapchain->image_available_semaphores != nullptr) {
        if (device != VK_NULL_HANDLE) {
            for (QUARK_U32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                if (swapchain->image_available_semaphores[i] != VK_NULL_HANDLE) {
                    vkDestroySemaphore(device, swapchain->image_available_semaphores[i], context->allocator);
                }
            }
        }
        if (quark_mem_free(swapchain->image_available_semaphores) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain image-available semaphores");
        }
        swapchain->image_available_semaphores = nullptr;
    }

    if (swapchain->command_buffers != nullptr) {
        if (quark_mem_free(swapchain->command_buffers) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain command buffers");
        }
        swapchain->command_buffers = nullptr;
    }

    if (swapchain->command_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, swapchain->command_pool, context->allocator);
        swapchain->command_pool = VK_NULL_HANDLE;
    }

    if (swapchain->framebuffers != nullptr) {
        if (device != VK_NULL_HANDLE) {
            for (QUARK_U32 i = 0; i < swapchain->image_count; ++i) {
                if (swapchain->framebuffers[i] != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(device, swapchain->framebuffers[i], context->allocator);
                }
            }
        }
        if (quark_mem_free(swapchain->framebuffers) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain framebuffers");
        }
        swapchain->framebuffers = nullptr;
    }

    if (swapchain->render_pass != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, swapchain->render_pass, context->allocator);
        swapchain->render_pass = VK_NULL_HANDLE;
    }

    if (swapchain->depth_image_view != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImageView(device, swapchain->depth_image_view, context->allocator);
        swapchain->depth_image_view = VK_NULL_HANDLE;
    }

    if (swapchain->depth_image != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImage(device, swapchain->depth_image, context->allocator);
        swapchain->depth_image = VK_NULL_HANDLE;
    }

    if (swapchain->depth_image_memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkFreeMemory(device, swapchain->depth_image_memory, context->allocator);
        swapchain->depth_image_memory = VK_NULL_HANDLE;
    }

    if (swapchain->image_views != nullptr) {
        if (device != VK_NULL_HANDLE) {
            for (QUARK_U32 i = 0; i < swapchain->image_count; ++i) {
                if (swapchain->image_views[i] != VK_NULL_HANDLE) {
                    vkDestroyImageView(device, swapchain->image_views[i], context->allocator);
                }
            }
        }
        if (quark_mem_free(swapchain->image_views) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain image views");
        }
        swapchain->image_views = nullptr;
    }

    if (swapchain->images != nullptr) {
        if (quark_mem_free(swapchain->images) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain images");
        }
        swapchain->images = nullptr;
    }

    if (swapchain->swapchain != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain->swapchain, context->allocator);
        swapchain->swapchain = VK_NULL_HANDLE;
    }

    swapchain->image_count = 0;
    swapchain->format = VK_FORMAT_UNDEFINED;
    swapchain->extent = (VkExtent2D) {0};
    swapchain->current_frame = 0;
}
