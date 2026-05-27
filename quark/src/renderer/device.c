#include "device.h"

#include <cstdlib/mem.h>

#include <string.h>

static QueueFamilyInfo make_invalid_queue_family_info();
static void free_swapchain_support_details(SwapchainSupportDetails* swapchain_support_details);
static b8_t extension_supported(
    const VkExtensionProperties* available_extensions,
    u32_t available_extension_count,
    const char* extension_name
);

b8_t select_physical_device(VulkanContext* context);
b8_t create_vulkan_logical_device(VulkanContext* context);
b8_t destroy_vulkan_logical_device(VulkanContext* context);

b8_t create_vulkan_device(VulkanContext* context) {
    if (!select_physical_device(context)) {
        return false;
    }

    if (!create_vulkan_logical_device(context)) {
        return false;
    }

    return true;
}

b8_t destroy_vulkan_device(VulkanContext* context) {
    destroy_vulkan_logical_device(context);
    free_swapchain_support_details(&context->device.swapchain_support);
    context->device.queue_families = make_invalid_queue_family_info();
    context->device.feature_support = (DeviceFeatureSupport) {0};
    context->device.physical_device = VK_NULL_HANDLE;
    context->device.logical_device = VK_NULL_HANDLE;
    context->device.graphics_queue = VK_NULL_HANDLE;
    context->device.present_queue = VK_NULL_HANDLE;
    context->device.compute_queue = VK_NULL_HANDLE;
    context->device.transfer_queue = VK_NULL_HANDLE;
    return true;
}

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
        .dedicated_transfer = false,
    };
}

static void free_swapchain_support_details(SwapchainSupportDetails* swapchain_support_details) {
    if (swapchain_support_details->formats != nullptr) {
        if (mem_heap_free(swapchain_support_details->formats) == false) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain surface formats");
        }
        swapchain_support_details->formats = nullptr;
    }

    if (swapchain_support_details->present_modes != nullptr) {
        if (mem_heap_free(swapchain_support_details->present_modes) == false) {
            QUARK_LOG_ERROR("Failed to free memory for swapchain surface present modes");
        }
        swapchain_support_details->present_modes = nullptr;
    }

    swapchain_support_details->format_count = 0;
    swapchain_support_details->present_mode_count = 0;
}

static b8_t extension_supported(
    const VkExtensionProperties* available_extensions,
    const u32_t available_extension_count,
    const char* extension_name
) {
    for (u32_t i = 0; i < available_extension_count; ++i) {
        if (strcmp(extension_name, available_extensions[i].extensionName) == 0) {
            return true;
        }
    }

    return false;
}

b8_t select_physical_device(VulkanContext* context) {
    free_swapchain_support_details(&context->device.swapchain_support);
    context->device.queue_families = make_invalid_queue_family_info();
    context->device.feature_support = (DeviceFeatureSupport) {0};
    context->device.physical_device = VK_NULL_HANDLE;
    context->device.graphics_queue = VK_NULL_HANDLE;
    context->device.present_queue = VK_NULL_HANDLE;
    context->device.compute_queue = VK_NULL_HANDLE;
    context->device.transfer_queue = VK_NULL_HANDLE;

    u32_t physical_device_count = 0;
    VK_CHECK_RETURN(
        vkEnumeratePhysicalDevices(context->instance, &physical_device_count, nullptr),
        false
    );
    if (physical_device_count == 0) {
        QUARK_LOG_ERROR("No devices with Vulkan support found");
        return false;
    }
    VkPhysicalDevice physical_devices[physical_device_count];
    VK_CHECK_RETURN(
        vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices),
        false
    );

    char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = {0};
    u32_t best_score = 0;
    for (u32_t i = 0; i < physical_device_count; ++i) {
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
            .acceleration_structure = acceleration_structure_features.accelerationStructure ? true : false,
            .ray_query = ray_query_features.rayQuery ? true : false,
            .ray_tracing_pipeline = ray_tracing_pipeline_features.rayTracingPipeline ? true : false,
            .hardware_ray_tracing = false,
        };

        u32_t score = 0;

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

        u64_t device_local_memory = 0;
        for (u32_t j = 0; j < memory_properties.memoryHeapCount; ++j) {
            if (memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                device_local_memory += memory_properties.memoryHeaps[j].size;
            }
        }
        score += (u32_t) (1000 * device_local_memory / (1024 * 1024 * 1024));

        QueueFamilyInfo queue_family_info = make_invalid_queue_family_info();

        u32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, nullptr);
        VkQueueFamilyProperties queue_families[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_families);

        u8_t min_transfer_score = 255;
        for (u32_t j = 0; j < queue_family_count; ++j) {
            u8_t transfer_score = 0;

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
                false
            );
            if (present_support && (queue_family_info.present == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX || queue_families[j].queueCount < queue_family_info.present_count)) {
                queue_family_info.present = j;
                queue_family_info.present_count = queue_families[j].queueCount;
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
            false
        );
        VK_CHECK_RETURN(
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], context->surface, &swapchain_support_details.
                format_count, nullptr),
            false
        );
        if (swapchain_support_details.format_count != 0) {
            swapchain_support_details.formats = mem_heap_alloc(
                sizeof(VkSurfaceFormatKHR) * swapchain_support_details.format_count);

            if (swapchain_support_details.formats == nullptr) {
                QUARK_LOG_ERROR("Failed to allocate memory for swapchain surface formats");
                return false;
            }

            VK_CHECK_X(
                vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], context->surface, &swapchain_support_details.
                    format_count, swapchain_support_details.formats),
                {
                    free_swapchain_support_details(&swapchain_support_details);
                    return false;
                }
            );
        }
        VK_CHECK_X(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], context->surface, &swapchain_support_details.
                present_mode_count, nullptr),
            {
                free_swapchain_support_details(&swapchain_support_details);
                return false;
            }
        );
        if (swapchain_support_details.present_mode_count != 0) {
            swapchain_support_details.present_modes = mem_heap_alloc(
                sizeof(VkPresentModeKHR) * swapchain_support_details.present_mode_count);

            if (swapchain_support_details.present_modes == nullptr) {
                QUARK_LOG_ERROR("Failed to allocate memory for swapchain surface present modes");
                free_swapchain_support_details(&swapchain_support_details);
                return false;
            }

            VK_CHECK_X(
                vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], context->surface, &
                    swapchain_support_details.present_mode_count, swapchain_support_details.present_modes),
                {
                    free_swapchain_support_details(&swapchain_support_details);
                    return false;
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
        u32_t required_extension_count = sizeof(required_extensions) / sizeof(required_extensions[0]);
        u32_t available_extension_count = 0;
        VK_CHECK_X(
            vkEnumerateDeviceExtensionProperties(physical_devices[i], nullptr, &available_extension_count, nullptr),
            {
                free_swapchain_support_details(&swapchain_support_details);
                return false;
            }
        );
        VkExtensionProperties available_extensions[available_extension_count];
        VK_CHECK_X(
            vkEnumerateDeviceExtensionProperties(physical_devices[i], nullptr, &available_extension_count,
                available_extensions),
            {
                free_swapchain_support_details(&swapchain_support_details);
                return false;
            }
        );
        for (u32_t j = 0; j < required_extension_count; ++j) {
            for (u32_t k = 0; k < available_extension_count; ++k) {
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

        const b8_t supports_acceleration_structure_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME
        );
        const b8_t supports_ray_query_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_RAY_QUERY_EXTENSION_NAME
        );
        const b8_t supports_ray_tracing_pipeline_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        );
        const b8_t supports_deferred_host_operations_extension = extension_supported(
            available_extensions, available_extension_count, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        );

        const b8_t pipeline_ray_tracing_supported =
            feature_support.acceleration_structure &&
            feature_support.ray_tracing_pipeline &&
            supports_acceleration_structure_extension &&
            supports_ray_tracing_pipeline_extension &&
            supports_deferred_host_operations_extension;

        const b8_t ray_query_supported =
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
        return false;
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

    return true;
}

b8_t create_vulkan_logical_device(VulkanContext* context) {
    const u32_t unique_queue_families[] = {
        context->device.queue_families.graphics,
        context->device.queue_families.present,
        context->device.queue_families.compute,
        context->device.queue_families.transfer,
    };
    u32_t unique_family_count = 0;
    u32_t deduped_families[4];

    for (u32_t i = 0; i < 4; ++i) {
        b8_t already_added = false;
        for (u32_t j = 0; j < unique_family_count; ++j) {
            if (deduped_families[j] == unique_queue_families[i]) {
                already_added = true;
                break;
            }
        }
        if (!already_added && unique_queue_families[i] != QUARK_VK_INVALID_QUEUE_FAMILY_INDEX) {
            deduped_families[unique_family_count++] = unique_queue_families[i];
        }
    }

    VkDeviceQueueCreateInfo queue_create_infos[unique_family_count];
    constexpr float queue_priorities[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    for (u32_t i = 0; i < unique_family_count; ++i) {
        u32_t queue_count = 0;

        if (deduped_families[i] == context->device.queue_families.graphics) {
            queue_count = context->device.queue_families.graphics_count;
        } else if (deduped_families[i] == context->device.queue_families.present) {
            queue_count = context->device.queue_families.present_count;
        } else if (deduped_families[i] == context->device.queue_families.compute) {
            queue_count = context->device.queue_families.compute_count;
        } else if (deduped_families[i] == context->device.queue_families.transfer) {
            queue_count = context->device.queue_families.transfer_count;
        }

        if (queue_count == 0) queue_count = 1;

        queue_create_infos[i] = (VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = deduped_families[i],
            .queueCount = queue_count,
            .pQueuePriorities = queue_priorities,
        };
    }

    const char* enabled_extensions[5];
    u32_t enabled_extension_count = 0;

    u32_t available_extension_count = 0;
    VK_CHECK_RETURN(
        vkEnumerateDeviceExtensionProperties(context->device.physical_device, nullptr, &available_extension_count,
            nullptr),
        false
    );

    VkExtensionProperties available_extensions[available_extension_count];
    VK_CHECK_RETURN(
        vkEnumerateDeviceExtensionProperties(context->device.physical_device, nullptr, &available_extension_count,
            available_extensions),
        false
    );

    enabled_extensions[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    if (context->device.feature_support.hardware_ray_tracing) {
        if (context->device.feature_support.acceleration_structure &&
            extension_supported(available_extensions, available_extension_count,
                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) {
            enabled_extensions[enabled_extension_count++] = VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME;
        }

        if (context->device.feature_support.ray_query &&
            extension_supported(available_extensions, available_extension_count,
                VK_KHR_RAY_QUERY_EXTENSION_NAME)) {
            enabled_extensions[enabled_extension_count++] = VK_KHR_RAY_QUERY_EXTENSION_NAME;
        }

        if (context->device.feature_support.ray_tracing_pipeline &&
            extension_supported(available_extensions, available_extension_count,
                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) {
            enabled_extensions[enabled_extension_count++] = VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
        }

        if (context->device.feature_support.ray_tracing_pipeline &&
            extension_supported(available_extensions, available_extension_count,
                VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)) {
            enabled_extensions[enabled_extension_count++] = VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
        }
    }

    QUARK_LOG_DEBUG("Enabled device extensions:");
    for (u32_t i = 0; i < enabled_extension_count; ++i) {
        QUARK_LOG_DEBUG("  %s", enabled_extensions[i]);
    }

    VkPhysicalDeviceFeatures2 enabled_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr,
        .features = context->device.feature_support.core,
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {0};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features = {0};
    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {0};

    void** last_pNext = &enabled_features.pNext;

    if (context->device.feature_support.acceleration_structure) {
        acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        acceleration_structure_features.pNext = nullptr;
        acceleration_structure_features.accelerationStructure = VK_TRUE;
        *last_pNext = &acceleration_structure_features;
        last_pNext = &acceleration_structure_features.pNext;
    }

    if (context->device.feature_support.ray_tracing_pipeline) {
        ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        ray_tracing_pipeline_features.pNext = nullptr;
        ray_tracing_pipeline_features.rayTracingPipeline = VK_TRUE;
        *last_pNext = &ray_tracing_pipeline_features;
        last_pNext = &ray_tracing_pipeline_features.pNext;
    }

    if (context->device.feature_support.ray_query) {
        ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        ray_query_features.pNext = nullptr;
        ray_query_features.rayQuery = VK_TRUE;
        *last_pNext = &ray_query_features;
    }

    const VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabled_features,
        .flags = 0,
        .queueCreateInfoCount = unique_family_count,
        .pQueueCreateInfos = queue_create_infos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = enabled_extensions,
        .pEnabledFeatures = nullptr, // pNext
    };

    VK_CHECK_RETURN(
        vkCreateDevice(context->device.physical_device, &device_create_info, context->allocator,
            &context->device.logical_device),
        false
    );

    vkGetDeviceQueue(context->device.logical_device, context->device.queue_families.graphics, 0,
        &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.queue_families.present, 0,
        &context->device.present_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.queue_families.compute, 0,
        &context->device.compute_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.queue_families.transfer, 0,
        &context->device.transfer_queue);

    QUARK_ASSERT_RETURN(
        false,
        context->device.graphics_queue != VK_NULL_HANDLE &&
        context->device.present_queue != VK_NULL_HANDLE &&
        context->device.compute_queue != VK_NULL_HANDLE &&
        context->device.transfer_queue != VK_NULL_HANDLE,
        "Failed to fetch one or more Vulkan device queues"
    );

    QUARK_LOG_DEBUG("Created Vulkan logical device with %u queue families", unique_family_count);

    return true;
}

b8_t destroy_vulkan_logical_device(VulkanContext* context) {
    context->device.graphics_queue = VK_NULL_HANDLE;
    context->device.present_queue = VK_NULL_HANDLE;
    context->device.compute_queue = VK_NULL_HANDLE;
    context->device.transfer_queue = VK_NULL_HANDLE;

    if (context->device.logical_device != VK_NULL_HANDLE) {
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = VK_NULL_HANDLE;
        QUARK_LOG_DEBUG("Destroyed Vulkan logical device");
    }
    return true;
}
