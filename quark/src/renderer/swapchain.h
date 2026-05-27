#pragma once

#include "vk.h"

#include <cstdlib/primitives.h>

b8_t create_vulkan_swapchain(VulkanContext* context, u32_t framebuffer_width, u32_t framebuffer_height);
b8_t destroy_vulkan_swapchain(VulkanContext* context);
