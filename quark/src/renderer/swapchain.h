#pragma once

#include "vk.h"

#include <cstdlib/primitives.h>
#include <cstdlib/nullability.h>

b8_t create_vulkan_swapchain(IN_NONNULL VulkanContext* context, u32_t framebuffer_width, u32_t framebuffer_height) NONNULL_ARGS(1);
b8_t destroy_vulkan_swapchain(IN_NONNULL VulkanContext* context) NONNULL_ARGS();
