#pragma once

#include "vk.h"

#include <quark/primitives.h>

QUARK_B8 create_vulkan_swapchain(VulkanContext* context, QUARK_U32 framebuffer_width, QUARK_U32 framebuffer_height);
QUARK_B8 destroy_vulkan_swapchain(VulkanContext* context);

