#pragma once

#include "vk.h"

#include <cstdlib/nullability.h>

typedef struct
{
    VkDeviceMemory memory;
    VkDeviceSize size;
    void* mapped;
    void* current;
} VulkanMemoryBlock;

b8_t create_vulkan_memory_block(
    IN_NONNULL const VulkanContext* context,
    usize_t size,
    b8_t host_visible,
    OUT_NONNULL VulkanMemoryBlock* out
) NONNULL_ARGS(1, 4);

b8_t destroy_vulkan_memory_block(
    IN_NONNULL const VulkanContext* context,
    INOUT_NONNULL VulkanMemoryBlock* block
) NONNULL_ARGS();

typedef struct
{
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
    void* mapped;
} VulkanBuffer;

b8_t create_vulkan_buffer(
    IN_NONNULL const VulkanContext* context,
    INOUT_NONNULL VulkanMemoryBlock* block,
    VkBufferUsageFlags usage,
    VkDeviceSize size,
    VkDeviceSize alignment,
    OUT_NONNULL VulkanBuffer* out
) NONNULL_ARGS(1, 2, 6);

b8_t destroy_vulkan_buffer(
    IN_NONNULL const VulkanContext* context,
    INOUT_NONNULL VulkanMemoryBlock* block,
    INOUT_NONNULL VulkanBuffer* buffer
) NONNULL_ARGS();

u32_t vulkan_find_memory_type(
    IN_NONNULL const VulkanContext* context,
    u32_t type_filter,
    VkMemoryPropertyFlags required_properties
) NONNULL_ARGS(1);
