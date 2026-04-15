#include "device.h"

#include <string.h>

#include "../../platform/memory.h"

static QueueFamilyInfo make_invalid_queue_family_info();
static void free_swapchain_support_details(SwapchainSupportDetails* swapchain_support_details);
static QUARK_B8 extension_supported(
    const VkExtensionProperties* available_extensions,
    QUARK_U32 available_extension_count,
    const char* extension_name
);

QUARK_B8 select_physical_device(VulkanContext* context);

static QueueFamilyInfo make_invalid_queue_family_info() {
    return (QueueFamilyInfo) {
        .graphics = QUARK_VK_INVALID_QUEUE_FAMILY_INDEX,
        .graphics_count = 0,
        .present = QUARK_VK_INVALID_QUEUE_FAMILY_INDEX,
        .present_count = 0,
        .compute = QUARK_VK_INVALID_QUEUE_FAMILY_INDEX,
        .compute_count = 0,
        .transfer = QUARK_VK_INVALID_QUEUE_FAMILY_INDEX,
        .transfer_count = 0,
        .dedicated_transfer = QUARK_FALSE,
    };
}

static void free_swapchain_support_details(SwapchainSupportDetails* swapchain_support_details) {
    if (swapchain_support_details->formats != nullptr) {
        if (quark_mem_free(swapchain_support_details->formats) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
        }
        swapchain_support_details->formats = nullptr;
    }

    if (swapchain_support_details->present_modes != nullptr) {
        if (quark_mem_free(swapchain_support_details->present_modes) == QUARK_FALSE) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
        }
        swapchain_support_details->present_modes = nullptr;
    }

    swapchain_support_details->format_count = 0;
    swapchain_support_details->present_mode_count = 0;
}

static QUARK_B8 extension_supported(
    const VkExtensionProperties* available_extensions,
    const QUARK_U32 available_extension_count,
    const char* extension_name
) {
    for (QUARK_U32 i = 0; i < available_extension_count; ++i) {
        if (strcmp(extension_name, available_extensions[i].extensionName) == 0) {
            return QUARK_TRUE;
        }
    }

    return QUARK_FALSE;
}

QUARK_B8 create_vulkan_device(VulkanContext* context) {
    return select_physical_device(context);
}

QUARK_B8 destroy_vulkan_device(VulkanContext* context) {
    free_swapchain_support_details(&context->device.swapchain_support);
    context->device.queue_families = make_invalid_queue_family_info();
    context->device.feature_support = (DeviceFeatureSupport) {0};
    context->device.physical_device = VK_NULL_HANDLE;
    context->device.logical_device = VK_NULL_HANDLE;
    return QUARK_TRUE;
}

QUARK_B8 select_physical_device(VulkanContext* context) {
    free_swapchain_support_details(&context->device.swapchain_support);
    context->device.queue_families = make_invalid_queue_family_info();
    context->device.feature_support = (DeviceFeatureSupport) {0};
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
        VkPhysicalDeviceFeatures2 features_2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = nullptr,
        };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .pNext = nullptr,
        };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .pNext = &acceleration_structure_features,
        };
        VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
            .pNext = &ray_tracing_pipeline_features,
        };
        features_2.pNext = &ray_query_features;
        vkGetPhysicalDeviceFeatures2(physical_devices[i], &features_2);

        DeviceFeatureSupport feature_support = {
            .core = features_2.features,
            .acceleration_structure = acceleration_structure_features.accelerationStructure ? QUARK_TRUE : QUARK_FALSE,
            .ray_query = ray_query_features.rayQuery ? QUARK_TRUE : QUARK_FALSE,
            .ray_tracing_pipeline = ray_tracing_pipeline_features.rayTracingPipeline ? QUARK_TRUE : QUARK_FALSE,
            .hardware_ray_tracing = QUARK_FALSE,
        };

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

        QueueFamilyInfo queue_family_info = make_invalid_queue_family_info();

        QUARK_U32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, nullptr);
        VkQueueFamilyProperties queue_families[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_families);

        QUARK_U8 min_transfer_score = 255;
        for (QUARK_U32 j = 0; j < queue_family_count; ++j) {
            QUARK_U8 transfer_score = 0;

            if (queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                if (queue_families[j].queueCount > queue_family_info.graphics_count) {
                    queue_family_info.graphics = j;
                    queue_family_info.graphics_count = queue_families[j].queueCount;
                }
                ++transfer_score;
            }

            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if (queue_families[j].queueCount > queue_family_info.compute_count) {
                    queue_family_info.compute = j;
                    queue_family_info.compute_count = queue_families[j].queueCount;
                }
                ++transfer_score;
            }

            if (queue_families[j].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                if (transfer_score <= min_transfer_score) {
                    if (queue_families[j].queueCount > queue_family_info.transfer_count || transfer_score <
                        min_transfer_score) {
                        queue_family_info.transfer = j;
                        queue_family_info.transfer_count = queue_families[j].queueCount;
                        min_transfer_score = transfer_score;
                        queue_family_info.dedicated_transfer =
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
                queue_family_info.present = j;
                queue_family_info.present_count = 1;
                if (queue_family_info.present == queue_family_info.graphics && queue_family_info.graphics_count > 0) {
                    --queue_family_info.graphics_count;
                } else if (queue_family_info.present == queue_family_info.compute && queue_family_info.compute_count >
                    0) {
                    --queue_family_info.compute_count;
                } else if (queue_family_info.present == queue_family_info.transfer && queue_family_info.transfer_count
                    > 0) {
                    --queue_family_info.transfer_count;
                }
            }
        }
        if (
            queue_family_info.graphics == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX ||
            queue_family_info.present == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX ||
            queue_family_info.compute == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX ||
            queue_family_info.transfer == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX
        ) {
            continue;
        }

        score += queue_family_info.graphics_count * 2000;
        score += queue_family_info.compute_count * 1000;
        score += queue_family_info.transfer_count * 500;
        if (queue_family_info.dedicated_transfer) {
            score += 3000;
        }

        SwapchainSupportDetails swapchain_support_details = {0};
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
                    free_swapchain_support_details(&swapchain_support_details);
                    return QUARK_FALSE;
                }
            );
        }
        VK_CHECK_X(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], context->surface, &swapchain_support_details.
                present_mode_count, nullptr),
            {
                free_swapchain_support_details(&swapchain_support_details);
                return QUARK_FALSE;
            }
        );
        if (swapchain_support_details.present_mode_count != 0) {
            swapchain_support_details.present_modes = quark_mem_alloc(
                sizeof(VkPresentModeKHR) * swapchain_support_details.present_mode_count);

            if (swapchain_support_details.present_modes == nullptr) {
                QUARK_LOG_ERROR("Failed to allocate memory for swapchain surface present modes");
                free_swapchain_support_details(&swapchain_support_details);
                return QUARK_FALSE;
            }

            VK_CHECK_X(
                vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], context->surface, &
                    swapchain_support_details.present_mode_count, swapchain_support_details.present_modes),
                {
                    free_swapchain_support_details(&swapchain_support_details);
                    return QUARK_FALSE;
                }
            );
        }
        if (swapchain_support_details.format_count == 0 || swapchain_support_details.present_mode_count == 0) {
            free_swapchain_support_details(&swapchain_support_details);
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
                free_swapchain_support_details(&swapchain_support_details);
                return QUARK_FALSE;
            }
        );
        VkExtensionProperties available_extensions[available_extension_count];
        VK_CHECK_X(
            vkEnumerateDeviceExtensionProperties(physical_devices[i], nullptr, &available_extension_count,
                available_extensions),
            {
                free_swapchain_support_details(&swapchain_support_details);
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
            free_swapchain_support_details(&swapchain_support_details);
            goto next_device;
        found:
            // found is just a label to continue the outer loop
        }

        const QUARK_B8 supports_acceleration_structure_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME
        );
        const QUARK_B8 supports_ray_query_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_RAY_QUERY_EXTENSION_NAME
        );
        const QUARK_B8 supports_ray_tracing_pipeline_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        );
        const QUARK_B8 supports_deferred_host_operations_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        );

        const QUARK_B8 pipeline_ray_tracing_supported =
            feature_support.acceleration_structure &&
            feature_support.ray_tracing_pipeline &&
            supports_acceleration_structure_extension &&
            supports_ray_tracing_pipeline_extension &&
            supports_deferred_host_operations_extension;

        const QUARK_B8 ray_query_supported =
            feature_support.acceleration_structure &&
            feature_support.ray_query &&
            supports_acceleration_structure_extension &&
            supports_ray_query_extension;

        feature_support.hardware_ray_tracing = pipeline_ray_tracing_supported || ray_query_supported;

        if (!feature_support.core.samplerAnisotropy) {
            QUARK_LOG_ERROR("Device does not support sampler anisotropy");
            free_swapchain_support_details(&swapchain_support_details);
            continue;
        }

        score += properties.limits.maxImageDimension2D / 64;
        if (feature_support.hardware_ray_tracing) score += 3500;
        if (pipeline_ray_tracing_supported) score += 2500;
        if (ray_query_supported) score += 1000;

        if (feature_support.core.geometryShader) score += 500;
        if (feature_support.core.tessellationShader) score += 400;
        if (feature_support.core.multiViewport) score += 300;
        if (feature_support.core.shaderStorageImageExtendedFormats) score += 200;
        if (feature_support.core.shaderFloat64) score += 150;
        if (feature_support.core.vertexPipelineStoresAndAtomics) score += 100;

        if (score >= best_score) {
            best_score = score;
            context->device.physical_device = physical_devices[i];
            free_swapchain_support_details(&context->device.swapchain_support);
            context->device.swapchain_support = swapchain_support_details;
            context->device.queue_families = queue_family_info;
            context->device.feature_support = feature_support;
            strncpy(device_name, properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1);
            device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1] = '\0';
        } else {
            free_swapchain_support_details(&swapchain_support_details);
        }
    next_device:
        // next_device is just a label to continue the outer loop
    }

    if (context->device.physical_device == VK_NULL_HANDLE) {
        QUARK_LOG_ERROR("Failed to find a suitable physical device");
        return QUARK_FALSE;
    }

    QUARK_LOG_DEBUG("Selected physical device: %s", device_name);
    QUARK_LOG_DEBUG(
        "Selected queue families: graphics=%u present=%u compute=%u transfer=%u",
        context->device.queue_families.graphics,
        context->device.queue_families.present,
        context->device.queue_families.compute,
        context->device.queue_families.transfer
    );
    QUARK_LOG_DEBUG(
        "Hardware ray tracing support: %s",
        context->device.feature_support.hardware_ray_tracing ? "yes" : "no"
    );

    return QUARK_TRUE;
}
