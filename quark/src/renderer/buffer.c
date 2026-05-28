#include "buffer.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <cstdlib/mem.h>
#include <vulkan/vulkan.h>

b8_t create_vulkan_memory_block(
    const VulkanContext* context,
    const usize_t size,
    const b8_t host_visible,
    VulkanMemoryBlock* out
) {
    QUARK_ASSERT_RETURN(false, size > 0, "Memory block size must be greater than zero");

    const VkMemoryPropertyFlags required = host_visible
        ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u32_t memory_type_index = vulkan_find_memory_type(context, UINT32_MAX, required);
    if (memory_type_index == QUARK_VK_INVALID_QUEUE_FAMILY_INDEX) {
        QUARK_LOG_ERROR("Failed to find %s memory type for block of size %zu",
                        host_visible ? "host-visible + coherent" : "device-local", size);
        return false;
    }

    VulkanMemoryBlock block = {
        .size = size,
        .mapped = nullptr,
        .current = nullptr,
    };

    const VkMemoryAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = size,
        .memoryTypeIndex = memory_type_index,
    };

    VK_CHECK_RETURN(
        vkAllocateMemory(context->device.logical_device, &allocate_info, context->allocator, &block.memory),
        false
    );

    if (host_visible) {
        VK_CHECK_X(
            vkMapMemory(context->device.logical_device, block.memory, 0, size, 0, &block.mapped),
            {
                vkFreeMemory(context->device.logical_device, block.memory, context->allocator);
                block.memory = VK_NULL_HANDLE;
                return false;
            }
        );
        block.current = block.mapped;
    }

    *out = block;
    return true;
}

b8_t destroy_vulkan_memory_block(
    const VulkanContext* context,
    VulkanMemoryBlock* block
) {
    const VkDevice device = context->device.logical_device;

    if (block->mapped != nullptr && device != VK_NULL_HANDLE) {
        vkUnmapMemory(device, block->memory);
        block->mapped = nullptr;
    }

    if (block->memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkFreeMemory(device, block->memory, context->allocator);
        block->memory = VK_NULL_HANDLE;
    }

    *block = (VulkanMemoryBlock){0};

    return true;
}

b8_t create_vulkan_buffer(
    const VulkanContext* context,
    VulkanMemoryBlock* block,
    const VkBufferUsageFlags usage,
    const VkDeviceSize size,
    const VkDeviceSize alignment,
    VulkanBuffer* out
) {
    QUARK_ASSERT_RETURN(false, block->memory != VK_NULL_HANDLE, "Memory block has no allocation");
    QUARK_ASSERT_RETURN(false, size > 0, "Buffer size must be greater than zero");

    VkBuffer temp_buffer = VK_NULL_HANDLE;
    const VkBufferCreateInfo temp_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VK_CHECK_RETURN(
        vkCreateBuffer(context->device.logical_device, &temp_create_info, context->allocator, &temp_buffer),
        false
    );

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(context->device.logical_device, temp_buffer, &mem_reqs);
    vkDestroyBuffer(context->device.logical_device, temp_buffer, context->allocator);

    VkDeviceSize current_offset = 0;
    if (block->mapped != nullptr) {
        current_offset = (VkDeviceSize)((uintptr_t)block->current - (uintptr_t)block->mapped);
    } else {
        current_offset = (VkDeviceSize)(uintptr_t)block->current;
    }

    const VkDeviceSize aligned_offset = MEM_ALIGN_UP(MEM_ALIGN_UP(current_offset, alignment), mem_reqs.alignment);

    const VkDeviceSize required_end = aligned_offset + mem_reqs.size;
    if (required_end > block->size) {
        QUARK_LOG_ERROR("VulkanMemoryBlock out of space for buffer (need %llu bytes at offset %llu, %llu total available)",
                        mem_reqs.size,
                        aligned_offset,
                        block->size);
        return false;
    }

    VkBuffer buffer = VK_NULL_HANDLE;
    VK_CHECK_RETURN(
        vkCreateBuffer(context->device.logical_device, &temp_create_info, context->allocator, &buffer),
        false
    );

    VK_CHECK_RETURN(
        vkBindBufferMemory(context->device.logical_device, buffer, block->memory, aligned_offset),
        false
    );

    VulkanBuffer result = {
        .buffer = buffer,
        .offset = aligned_offset,
        .size = size,
        .mapped = nullptr,
    };

    if (block->mapped != nullptr) {
        result.mapped = (char*)block->mapped + aligned_offset;
    }

    if (block->mapped != nullptr) {
        block->current = (char*)block->mapped + required_end;
    } else {
        block->current = (void*)(uintptr_t)required_end;
    }

    *out = result;
    return true;
}

b8_t destroy_vulkan_buffer(
    const VulkanContext* context,
    VulkanMemoryBlock* block,
    VulkanBuffer* buffer
) {
    vkDestroyBuffer(context->device.logical_device, buffer->buffer, context->allocator);
    *buffer = (VulkanBuffer){0};
    return true;
}

u32_t vulkan_find_memory_type(
    const VulkanContext* context,
    const u32_t type_filter,
    const VkMemoryPropertyFlags required_properties
) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context->device.physical_device, &memory_properties);

    for (u32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) != 0 &&
            (memory_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties) {
            return i;
        }
    }

    return QUARK_VK_INVALID_QUEUE_FAMILY_INDEX;
}
