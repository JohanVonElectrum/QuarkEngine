#pragma once

#include "vk.h"
#include <cstdlib/primitives.h>

b8_t create_vulkan_device(VulkanContext* context);
b8_t destroy_vulkan_device(VulkanContext* context);
