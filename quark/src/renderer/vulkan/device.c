#include "device.h"

#include <string.h>

#include "../../platform/memory.h"

typedef struct QueueFamilyIndices
{
    QUARK_U32 graphics;
    QUARK_U32 graphics_count;
    QUARK_U32 present;
    QUARK_U32 present_count;
    QUARK_U32 compute;
    QUARK_U32 compute_count;
    QUARK_U32 transfer;
    QUARK_U32 transfer_count;
} QueueFamilyIndices;

QUARK_B8 select_physical_device(VulkanContext* context);

QUARK_B8 create_vulkan_device(VulkanContext* context) {
    return select_physical_device(context);
}

QUARK_B8 destroy_vulkan_device(VulkanContext* context) {
    quark_mem_free(context->device.swapchain_support.formats);
    quark_mem_free(context->device.swapchain_support.present_modes);
    return QUARK_TRUE;
}

QUARK_B8 select_physical_device(VulkanContext* context) {
    context->device.physical_device = VK_NULL_HANDLE;

    QUARK_U32 physical_device_count = 0;
    VK_CHECK_RETURN(
        vkEnumeratePhysicalDevices(context->instance, &physical_device_count, nullptr),
        QUARK_FALSE
    );
    if (physical_device_count == 0) {
        QUARK_LOG_ERROR("No devices with Vulkan support found");
        return QUARK_FALSE;
    }
    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK_RETURN(
        vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices),
        QUARK_FALSE
    );

    char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = {0};
    QUARK_U32 best_score = 0;
    for (QUARK_U32 i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory_properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        QUARK_U32 score = 0;

        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 10000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 5000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 3000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 1000;
                break;
            default:
                break;
        }

        QUARK_U64 device_local_memory = 0;
        for (QUARK_U32 j = 0; j < memory_properties.memoryHeapCount; ++j) {
            if (memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                device_local_memory += memory_properties.memoryHeaps[j].size;
            }
        }
        score += (QUARK_U32) (1000 * device_local_memory / (1024 * 1024 * 1024));

        QueueFamilyIndices queue_family_indices = {
            .graphics = -1,
            .graphics_count = 0,
            .present = -1,
            .present_count = 0,
            .compute = -1,
            .compute_count = 0,
            .transfer = -1,
            .transfer_count = 0,
        };

        QUARK_U32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, nullptr);
        VkQueueFamilyProperties queue_families[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_families);

        QUARK_U8 min_transfer_score = 255;
        QUARK_B8 dedicated_transfer_queue = QUARK_FALSE;
        for (QUARK_U32 j = 0; j < queue_family_count; ++j) {
            QUARK_U8 transfer_score = 0;

            if (queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                if (queue_families[j].queueCount > queue_family_indices.graphics_count) {
                    queue_family_indices.graphics = j;
                    queue_family_indices.graphics_count = queue_families[j].queueCount;
                }
                ++transfer_score;
            }

            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if (queue_families[j].queueCount > queue_family_indices.compute_count) {
                    queue_family_indices.compute = j;
                    queue_family_indices.compute_count = queue_families[j].queueCount;
                }
                ++transfer_score;
            }

            if (queue_families[j].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                if (transfer_score <= min_transfer_score) {
                    if (queue_families[j].queueCount > queue_family_indices.transfer_count || transfer_score <
                        min_transfer_score) {
                        queue_family_indices.transfer = j;
                        queue_family_indices.transfer_count = queue_families[j].queueCount;
                        min_transfer_score = transfer_score;
                        dedicated_transfer_queue =
                                (queue_families[j].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0;
                    }
                }
            }

            VkBool32 present_support = VK_FALSE;
            VK_CHECK_RETURN(
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, context->surface, &present_support),
                QUARK_FALSE
            );
            if (present_support) {
                queue_family_indices.present = j;
                queue_family_indices.present_count = 1;
                if (queue_family_indices.present == queue_family_indices.graphics) {
                    --queue_family_indices.graphics_count;
                } else if (queue_family_indices.present == queue_family_indices.compute) {
                    --queue_family_indices.compute_count;
                } else if (queue_family_indices.present == queue_family_indices.transfer) {
                    --queue_family_indices.transfer_count;
                }
            }
        }
        if (
            queue_family_indices.graphics == -1 ||
            queue_family_indices.present == -1 ||
            queue_family_indices.compute == -1 ||
            queue_family_indices.transfer == -1
        ) {
            continue;
        }

        score += queue_family_indices.graphics_count * 2000;
        score += queue_family_indices.compute_count * 1000;
        score += queue_family_indices.transfer_count * 500;
        if (dedicated_transfer_queue) {
            score += 3000;
        }

        SwapchainSupportDetails swapchain_support_details;
        VK_CHECK_RETURN(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_devices[i], context->surface, &swapchain_support_details.
                capabilities),
            QUARK_FALSE
        );
        VK_CHECK_RETURN(
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], context->surface, &swapchain_support_details.
                format_count, nullptr),
            QUARK_FALSE
        );
        if (swapchain_support_details.format_count != 0) {
            swapchain_support_details.formats = quark_mem_alloc(
                sizeof(VkSurfaceFormatKHR) * swapchain_support_details.format_count);

            if (swapchain_support_details.formats == nullptr) {
                QUARK_LOG_ERROR("Failed to allocate memory for swapchain surface formats");
                return QUARK_FALSE;
            }

            VK_CHECK_X(
                vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], context->surface, &swapchain_support_details.
                    format_count, swapchain_support_details.formats),
                {
                    if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                        QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                    }
                    return QUARK_FALSE;
                }
            );
        }
        VK_CHECK_X(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], context->surface, &swapchain_support_details.
                present_mode_count, nullptr),
            {
                if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                }
                return QUARK_FALSE;
            }
        );
        if (swapchain_support_details.present_mode_count != 0) {
            swapchain_support_details.present_modes = quark_mem_alloc(
                sizeof(VkPresentModeKHR) * swapchain_support_details.present_mode_count);

            if (swapchain_support_details.present_modes == nullptr) {
                QUARK_LOG_ERROR("Failed to allocate memory for swapchain surface present modes");
                if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                }
                return QUARK_FALSE;
            }

            VK_CHECK_X(
                vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], context->surface, &
                    swapchain_support_details.present_mode_count, swapchain_support_details.present_modes),
                {
                    if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                        QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                    }
                    if (quark_mem_free(swapchain_support_details.present_modes) == QUARK_FALSE) {
                        QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
                    }
                    return QUARK_FALSE;
                }
            );
        }
        if (swapchain_support_details.format_count == 0 || swapchain_support_details.present_mode_count == 0) {
            if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
            }
            if (quark_mem_free(swapchain_support_details.present_modes) == QUARK_FALSE) {
                QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
            }
            continue;
        }

        const char* required_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        QUARK_U32 required_extension_count = sizeof(required_extensions) / sizeof(required_extensions[0]);
        QUARK_U32 available_extension_count = 0;
        VK_CHECK_X(
            vkEnumerateDeviceExtensionProperties(physical_devices[i], nullptr, &available_extension_count, nullptr),
            {
                if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                }
                if (quark_mem_free(swapchain_support_details.present_modes) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
                }
                return QUARK_FALSE;
            }
        );
        VkExtensionProperties available_extensions[available_extension_count];
        VK_CHECK_X(
            vkEnumerateDeviceExtensionProperties(physical_devices[i], nullptr, &available_extension_count,
                available_extensions),
            {
                if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                }
                if (quark_mem_free(swapchain_support_details.present_modes) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
                }
                return QUARK_FALSE;
            }
        );
        for (QUARK_U32 j = 0; j < required_extension_count; ++j) {
            for (QUARK_U32 k = 0; k < available_extension_count; ++k) {
                if (strcmp(required_extensions[j], available_extensions[k].extensionName) == 0) {
                    goto found;
                }
            }
            QUARK_LOG_ERROR("Required Vulkan device extension not found: %s", required_extensions[j]);
            if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
            }
            if (quark_mem_free(swapchain_support_details.present_modes) == QUARK_FALSE) {
                QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
            }
            goto next_device;
        found:
            // found is just a label to continue the outer loop
        }

        if (!features.samplerAnisotropy) {
            QUARK_LOG_ERROR("Device does not support sampler anisotropy");
            if (quark_mem_free(swapchain_support_details.formats) == QUARK_FALSE) {
                QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
            }
            if (quark_mem_free(swapchain_support_details.present_modes) == QUARK_FALSE) {
                QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
            }
            continue;
        }

        score += properties.limits.maxImageDimension2D / 64;

        if (features.geometryShader) score += 500;
        if (features.tessellationShader) score += 400;
        if (features.multiViewport) score += 300;
        if (features.shaderStorageImageExtendedFormats) score += 200;
        if (features.shaderFloat64) score += 150;
        if (features.vertexPipelineStoresAndAtomics) score += 100;

        if (score >= best_score) {
            best_score = score;
            context->device.physical_device = physical_devices[i];
            if (context->device.swapchain_support.formats != nullptr) {
                if (quark_mem_free(context->device.swapchain_support.formats) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
                }
            }
            if (context->device.swapchain_support.present_modes != nullptr) {
                if (quark_mem_free(context->device.swapchain_support.present_modes) == QUARK_FALSE) {
                    QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
                }
            }
            context->device.swapchain_support = swapchain_support_details;
            strncpy(device_name, properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1);
            device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1] = '\0';
        }
    next_device:
        // next_device is just a label to continue the outer loop
    }

    if (context->device.physical_device == VK_NULL_HANDLE) {
        QUARK_LOG_ERROR("Failed to find a suitable physical device");
        return QUARK_FALSE;
    }

    QUARK_LOG_DEBUG("Selected physical device: %s", device_name);

    return QUARK_TRUE;
}
